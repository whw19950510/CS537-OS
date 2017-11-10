#include "types.h"
#include "stat.h"
#include "user.h"
int thread_create(void (*start_routine)(void*), void *arg)
{
    void* stack=malloc(PGSIZE);
    int pid=clone(start_routine,arg,stack);
    return pid;
}