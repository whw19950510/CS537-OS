#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
// ADD NECESSARY HEADERS
#include "stats_t.h"
// Mutex variables
pthread_mutex_t* mutex;
pthread_mutexattr_t mutexAttribute;
void* mappointer;
long pagesize;
void exit_handler(int sig) 
{
    // ADD
	if(-1==munmap(mappointer,(size_t)pagesize))
	{
		fprintf( stderr, "error happens");
		exit(1);
	}
	if(-1==shm_unlink("huawei"))
	{
		fprintf( stderr, "error happens");
		exit(1);
	}
	exit(0);
}

int main(int argc, char *argv[]) 
{
	// ADD
	int i;
	char* SHM_NAME="huawei";
	int iteration=0;
	//override exit handler
	struct sigaction newact;
	newact.sa_handler=exit_handler;
    if(sigaction(SIGINT, &newact,NULL)<0)
    {
        fprintf(stderr, "error happens");
		exit(1);
    }
    if(sigaction(SIGTERM, &newact,NULL)<0)
    {
        fprintf(stderr, "error happens");
		exit(1);
    }
	// Creating a new shared memory segment
	int fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0660);	
	pagesize=sysconf (_SC_PAGESIZE); 
	if(pagesize==-1)
	{
		fprintf( stderr, "error happens");
		exit(1);
	}
	if(-1==ftruncate(fd_shm,(off_t)pagesize))
	{
		fprintf( stderr, "error happens");
		exit(1);
	}
	mappointer=mmap(NULL,(size_t)pagesize,PROT_READ|PROT_WRITE,MAP_SHARED,fd_shm,0);
	if(mappointer==NULL)
	{
		fprintf( stderr, "error happens");
		exit(1);
	}
	
	int maxClient=pagesize/64-1;//each segment with 64 byte,one segment reserved for mutex
	
	// Initializing mutex
	mutex=((pthread_mutex_t*)mappointer);
	pthread_mutexattr_init(&mutexAttribute);
	pthread_mutexattr_setpshared(&mutexAttribute, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(mutex, &mutexAttribute);
	for(i=1;i<=maxClient;i++) {
		((stats_t*)(mappointer+i*64))->pid=-1;
	}
    while (1) 
	{
		iteration++;
		// ADD
		for(i=1;i<=maxClient;i++)
		{	
			if(((stats_t*)(mappointer+i*64))->pid!=-1)
			{
				//gettimeofday(&end_time,NULL);
				//timersub(&end_time, &((stats_t*)(mappointer+i*64))->start_time, &time_result);
				//time_t curTime=time(NULL);    
				//((stats_t*)(mappointer+i*64))->elapsed_sec=difftime(curTime,((stats_t*)(mappointer+i*64))->init);
				//((stats_t*)(mappointer+i*64))->elapsed_msec=0;
				//((stats_t*)(mappointer+i*64))->elapsed_sec=time_result.tv_sec;
				//((stats_t*)(mappointer+i*64))->elapsed_msec=time_result.tv_usec/1000.0;
				printf("%d, pid : %d, birth : %s, elapsed : %d s %.4lf ms, %s\n",iteration,((stats_t*)(mappointer+i*64))->pid,((stats_t*)(mappointer+i*64))->birth,((stats_t*)(mappointer+i*64))->elapsed_sec,((stats_t*)(mappointer+i*64))->elapsed_msec,((stats_t*)(mappointer+i*64))->clientString);
			}
		}
		
		sleep(1);
	}
	
    return 0;
}
