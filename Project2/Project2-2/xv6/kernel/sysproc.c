#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"
#include "pstat.h"
int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

//return 0 on success -1 on failure,
//Because your MLFQ implementations are all in the kernel level, 
//you need to extract useful information for each process by creating 
//this system call so as to better test whether your implementations 
//work as expected.

//To be more specific, this system call returns 0 on success 
//and -1 on failure. If success, some basic information about each 
//process: its process ID, how many timer ticks have elapsed while running 
//in each level, which queue it is currently placed on (3, 2, 1, or 0), and 
//its current procstate (e.g., SLEEPING, RUNNABLE, or RUNNING) will be filled 
//in the pstat structure as defined 
int 
sys_getpinfo(void)
{
  struct pstat* cur;
  if(argptr(0, (void*)(&cur),sizeof(cur)) < 0)
    return -1;
  helper(cur);
  return 0;
}