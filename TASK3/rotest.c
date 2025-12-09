#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  printf(1, "Tentando escrever no codigo (main)...\n");

  // 'main' é um ponteiro para o início da função main na memória
  char *p = (char*)main;

  printf(1, "Endereco de main: %p\n", p);
  printf(1, "Lendo valor atual: %x\n", *p); // Isso deve funcionar (Leitura)

  *p = 0; // Tenta escrever ZERO em cima do código <-- ISSO DEVE TRAVAR!

  printf(1, "ERRO: Consegui escrever no codigo! A protecao falhou.\n");
  exit();
}
