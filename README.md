# XV6 Memory Management Improvements

Este repositório contém implementações avançadas de gerenciamento de memória no sistema operacional educacional **xv6**. As modificações incluem visualização de tabelas de páginas, proteção contra ponteiros nulos, segmentos somente leitura e otimização Copy-on-Write (COW).

---

## 🛠️ Configuração do Ambiente (Como Compilar)

O xv6 requer um emulador (QEMU) e um compilador cruzado para arquitetura x86. Siga as instruções abaixo de acordo com seu Sistema Operacional.

###  Linux (Nativo)
Esta é a plataforma recomendada. Abra o terminal e instale as dependências:

```bash
sudo apt-get update
sudo apt-get install git build-essential qemu-system-x86 gdb
```

###  Windows (via WSL 2)
A forma mais eficiente de rodar no Windows é utilizando o WSL 2 (Windows Subsystem for Linux) com Ubuntu.

1. Abra o **PowerShell como Administrador** e instale o WSL:
   ```powershell
   wsl --install
   ```
   *(Reinicie o computador se solicitado e configure seu usuário/senha do Linux)*.

2. Abra o terminal do **Ubuntu** e instale as dependências (mesmo comando do Linux):
   ```bash
   sudo apt-get update
   sudo apt-get install git build-essential qemu-system-x86 gdb
   ```

###  macOS
No macOS, é necessário instalar as ferramentas via Homebrew. Note que processadores Apple Silicon (M1/M2/M3) precisam de compiladores específicos (`i386-elf-gcc`).

1. Instale as dependências:
   ```bash
   brew install qemu i386-elf-gcc i386-elf-gdb
   ```

2. **Nota Importante:** Você pode precisar editar o arquivo `Makefile`. Procure pela linha `CC = gcc` e altere para:
   ```makefile
   CC = i386-elf-gcc
   ```

---

## 🚀 Como Executar

Para iniciar o sistema operacional xv6, navegue até a pasta da Task desejada e execute os comandos abaixo.

**Limpar compilações anteriores (recomendado):**
```bash
make clean
```

**Compilar e rodar o emulador:**
```bash
make qemu
```

## ✅ Validação das Tarefas

Cada pasta implementa uma funcionalidade específica. Abaixo estão os comandos para validar cada uma delas dentro do shell do xv6.

### 📂 Task 1: Visualização de Memória
**Objetivo:** Exibir a hierarquia de tabelas de páginas e mapeamentos virtuais/físicos.

1. Inicie o xv6 (`make qemu`).
2. No terminal do xv6, pressione o atalho: `CTRL + P`

**Resultado Esperado:**
> O sistema listará os processos. Para cada processo, serão exibidas seções "Page tables" (com endereços PPN em hexadecimal) e "Page mappings" (mapeamento virtual -> físico em decimal).

### 📂 Task 2: Proteção Contra Null Pointer
**Objetivo:** Impedir o acesso à página 0 (endereço nulo), causando encerramento do processo.

1. Inicie o xv6.
2. Execute o programa de teste:
   ```bash
   $ nulltest
   ```

**Resultado Esperado:**
> O processo deve ser encerrado com um erro de **Trap 14 (Page Fault)**.
> Exemplo: `pid 3 nulltest: trap 14 err 4 on cpu 0 ... addr 0x0 --kill proc`

### 📂 Task 3: Segmentos Somente Leitura (Read-Only)
**Objetivo:** Garantir que o segmento de código (`.text`) não possa ser sobrescrito.

1. Inicie o xv6.
2. Execute o programa de teste:
   ```bash
   $ rotest
   ```

**Resultado Esperado:**
> O programa conseguirá ler o endereço da função main, mas falhará ao tentar escrever nele.
> Exemplo: `pid 3 rotest: trap 14 err 7 on cpu 0 ... --kill proc`
> *(O código err 7 indica: Página Presente + Escrita + Usuário)*.

### 📂 Task 4: Copy-on-Write (COW)
**Objetivo:** Otimizar o `fork()` compartilhando páginas físicas até que uma escrita ocorra.

1. Inicie o xv6.
2. Execute a suíte de testes completa (isso pode demorar alguns segundos):
   ```bash
   $ usertests
   ```

**Resultado Esperado:**
> O sistema executará dezenas de testes intensivos de memória. Ao final, deve exibir:
> **ALL TESTS PASSED**

---

