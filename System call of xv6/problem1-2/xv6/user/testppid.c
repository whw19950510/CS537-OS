#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
int stdout = 1;
int main()
{
  int pid,ppid;
  pid = fork();
  if(pid==-1)
  {
    printf(stdout, "Error starting process\n");
    exit();
  }
  else if(pid==0)
  {
    ppid = getppid();
    //printf("%d\n%d",pid,ppid );
    if(ppid==pid)
      printf(stdout,"Success\n");
    else
      printf(stdout,"Fail\n");
  }
  else
  {
    wait();
    //printf("%d\n%d",getpid(),ppid );
  }
  return 0;
}
