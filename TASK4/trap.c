#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  case T_PGFLT: // Page Fault (Erro 14)
    {
      // Pega o endereço que deu erro (está no registrador CR2)
      uint va = rcr2();
      struct proc *curproc = myproc();
      pte_t *pte;
      uint pa;
      char *mem;

      // Se o endereço for inválido ou o processo não existir -> mata
      // CORREÇÃO 1: troquei cpu->id por cpuid()
      if(curproc == 0 || va >= KERNBASE) {
         cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            curproc->pid, curproc->name, tf->trapno, tf->err, cpuid(), tf->eip, rcr2());
         curproc->killed = 1;
         break;
      }

      // Vamos verificar se é um erro COW
      // 1. Acha a entrada na tabela de páginas
      // (Precisamos declarar walkpgdir no topo do trap.c ou usar extern)
      extern pte_t* walkpgdir(pde_t *pgdir, const void *va, int alloc);

      // Arredonda para baixo para pegar o inicio da página
      va = PGROUNDDOWN(va);
      pte = walkpgdir(curproc->pgdir, (void*)va, 0);

      // 2. Se a página existe E tem a flag COW marcada
      if(pte && (*pte & PTE_P) && (*pte & PTE_COW)) {

         pa = PTE_ADDR(*pte); // Endereço físico antigo (compartilhado)

         // Aloca uma página NOVA só para nós
         if((mem = kalloc()) == 0) {
            cprintf("COW: Out of memory\n");
            curproc->killed = 1;
            break;
         }

         // Copia o conteúdo da página velha (pa) para a nova (mem)
         memmove(mem, (char*)P2V(pa), PGSIZE);

         // Decrementa o contador da página velha (já que não usamos mais ela)
         dec_ref(pa);

         // Atualiza a tabela: Aponta para a NOVA página
         // Remove COW, Adiciona Write (PTE_W)
         uint flags = PTE_FLAGS(*pte);
         flags &= ~PTE_COW; // Tira COW
         flags |= PTE_W;    // Põe Write

         // CORREÇÃO 2: Removi PA2PTE(), basta usar o endereço OR flags
         *pte = V2P(mem) | flags;

         // Atualiza a TLB (cache de endereços) para o processador perceber a mudança
         lcr3(V2P(curproc->pgdir));

         break; // Sucesso! O programa vai tentar escrever de novo e agora vai conseguir.
      }

      // Se não for COW, é um erro normal (ex: acesso inválido). Deixa cair no default.
    }

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
