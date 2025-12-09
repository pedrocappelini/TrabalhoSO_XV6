#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int *p = 0; // Ponteiro Nulo

  printf(1, "Vou tentar ler o endereco 0...\n");
  printf(1, "Valor: %d\n", *p); // <-- Aqui deve dar TRAP e morrer

  printf(1, "ERRO: O programa nao morreu! A protecao falhou.\n");
  exit();
}
