#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define DATASIZE (9 * 4096)

int
main(int argc, char *argv[])
{
  if (argc != 1){
    printf("Usage: attack\n");
    exit(1);
  }

  char *data = sbrk(DATASIZE);

  exit(1);
}
