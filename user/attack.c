#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define PGSIZE 4096
#define DATASIZE (32 * PGSIZE)
#define MSG "Here it is: "
#define MSG_LEN (sizeof(MSG) -1)

static int is_alnum(char c) {
  return (c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9');
}

int
main(int argc, char *argv[])
{
  // Your code here.
  char *mem = sbrk(DATASIZE);

  if(mem == ((char *)-1))
    exit(1);

  for(int i = 0; i + MSG_LEN < DATASIZE; i++) {
    int match = 1;
    for(int j = 0; j < MSG_LEN; j++) {
      if(mem[i + j] != MSG[j]) {
        match = 0;
        break;
      }
    }

    if(!match) 
      continue;
    char *secret = mem + i +  MSG_LEN;
    int len = 0;
    
    while(i + MSG_LEN + len < DATASIZE && secret[len] != '\0' && is_alnum(secret[len])) {
      len++;
    }
    
    if(len > 0 && i + MSG_LEN + len < DATASIZE && secret[len] == '\0') {
    write(1, secret, len);
    write(1, "\n", 1);
    exit(0);
    }
  }
  exit(0);
}
