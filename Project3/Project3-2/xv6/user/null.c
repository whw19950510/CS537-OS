/* syscall argument checks (null page, code/heap boundaries) */
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#undef NULL
#define NULL ((void*)0)

#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED\n"); \
   exit(); \
}

int
main(int argc, char *argv[])
{
  char *arg;
  int fd = open("tmp", O_WRONLY|O_CREATE);
  assert(fd==-1);
  arg = (char*) 0x0;
  if(-1==write(fd, arg, 10))
  exit();  
  
}