#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  printf(1, "Tentando escrever no codigo (main)...\n");

  //ponteiro para o início da função main na memória
  char *p = (char*)main;

  printf(1, "Endereco de main: %p\n", p);
  printf(1, "Lendo valor atual: %x\n", *p);

  *p = 0; // Para travar

  printf(1, "ERRO: Consegui escrever no codigo! A protecao falhou.\n");
  exit();
}
