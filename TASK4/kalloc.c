// kalloc.c - Gerenciador de memória com Reference Counting (Task 4)
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

// Correção para PGSHIFT caso não esteja definido
#ifndef PGSHIFT
#define PGSHIFT 12
#endif

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
  // TASK 4: Array de contagem
  uchar ref_count[PHYSTOP >> PGSHIFT];
} kmem;

void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0; // Travas desligadas no início
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1; // Travas ligadas agora
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE) {
    kmem.ref_count[V2P(p) >> PGSHIFT] = 1;
    kfree(p);
  }
}

// --- TASK 4: Funções Auxiliares (CORRIGIDAS) ---

void
inc_ref(uint pa)
{
  if(pa >= PHYSTOP || pa < V2P(end))
    panic("inc_ref");

  // Só usa lock se estiver habilitado (evita travamento no boot)
  if(kmem.use_lock)
    acquire(&kmem.lock);

  kmem.ref_count[pa >> PGSHIFT]++;

  if(kmem.use_lock)
    release(&kmem.lock);
}

int
dec_ref(uint pa)
{
  if(pa >= PHYSTOP || pa < V2P(end))
    panic("dec_ref");

  int count;

  if(kmem.use_lock)
    acquire(&kmem.lock);

  if(kmem.ref_count[pa >> PGSHIFT] > 0)
    kmem.ref_count[pa >> PGSHIFT]--;

  count = kmem.ref_count[pa >> PGSHIFT];

  if(kmem.use_lock)
    release(&kmem.lock);

  return count;
}

int
get_ref(uint pa)
{
  if(pa >= PHYSTOP || pa < V2P(end))
    return -1;

  int count;
  if(kmem.use_lock)
    acquire(&kmem.lock);

  count = kmem.ref_count[pa >> PGSHIFT];

  if(kmem.use_lock)
    release(&kmem.lock);

  return count;
}
// ------------------------------------------------

void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Decrementa e vê se ainda tem gente usando
  if(dec_ref(V2P(v)) > 0)
    return;

  // Ninguém usa, pode liberar
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  if(kmem.use_lock)
    release(&kmem.lock);
}

char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    // Página nova nasce com contador 1
    kmem.ref_count[V2P((char*)r) >> PGSHIFT] = 1;
  }
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}
