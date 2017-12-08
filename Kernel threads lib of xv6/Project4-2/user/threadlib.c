#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"
int lock_init(lock_t *lk)
{
	lk->flag = 0;
	return 0;
}

void lock_acquire(lock_t *lk)
{
	while(xchg(&lk->flag, 1) != 0);
}

void lock_release(lock_t *lk)
{
	xchg(&lk->flag, 0);
}

int thread_create(void (*start_routine)(void*), void *arg)
{
  lock_t lock;
  lock_init(&lock);
  lock_acquire(&lock);
  void* stack=malloc(4096);
  lock_release(&lock);

  int pid=clone(start_routine,arg,stack);
  if(pid<0) {
      printf(1,"error create new threads\n");
  }
  return pid;
}

int thread_join() {
    //void* stack=malloc(sizeof(void*));
    void *stack;
    int res = join(&stack);
    lock_t lock;
    lock_init(&lock);
    lock_acquire(&lock);
    if(res!=-1)
    free(stack);
    lock_release(&lock);
    return res;
}