#include <sys/types.h>
#include <unistd.h>
#include<stdio.h>
#include<stdlib.h>
int main()
{
  int pid,ppid;
  pid = fork();
  if(pid==-1)
  {
    fprintf(stderr, "Error starting process\n");
    exit(1);
  }
  else if(pid==0)
  {
    ppid = getppid();
    //printf("%d\n%d",pid,ppid );
    if(ppid==pid)
      printf("Success\n");
    else
      printf("Fail\n");
  }
  else
  {
    int wc = wait(NULL);
    //printf("%d\n%d",getpid(),ppid );
  }
  return 0;
}
