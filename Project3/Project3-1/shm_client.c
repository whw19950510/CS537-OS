#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
// ADD NECESSARY HEADERS
#include "stats_t.h"
// Mutex variables
pthread_mutex_t* mutex;
void* sharepointer;
long pagesize;
void exit_handler(int sig) {
    // ADD
    int maxClient=pagesize/64-1;
    // critical section begins
	pthread_mutex_lock(mutex);

    // Client leaving; needs to reset its segment   
    for(int i=1;i<=maxClient;i++) {
        //printf("pid:%d \t %d\n",i,((stats_t*)sharepointer)[i].pid);
        if(((stats_t*)(sharepointer+i*64))->pid==getpid())
        {
            ((stats_t*)(sharepointer+i*64))->pid=-1;
            ((stats_t*)(sharepointer+i*64))->elapsed_sec=0;
            ((stats_t*)(sharepointer+i*64))->elapsed_msec=0;
            memset(((stats_t*)(sharepointer+i*64))->birth, 0, sizeof(((stats_t*)(sharepointer+i*64))->birth));
            memset(((stats_t*)(sharepointer+i*64))->clientString, 0, sizeof(((stats_t*)(sharepointer+i*64))->clientString));
            break;
        }
    }
	pthread_mutex_unlock(mutex);
	// critical section ends
    
    exit(0);
}

int main(int argc, char *argv[]) {
    // ADD
    if(argc!=2)
    {
        fprintf(stderr, "error happens");
		exit(1);
    }
    const char *format="%a %b %d %H:%M:%S %Y";//format furing the client storing it
    const char* SHM_NAME="huawei";
    pagesize=sysconf(_SC_PAGESIZE);
    /////////////////////////////////////////////
    int fd_shm =shm_open(SHM_NAME,O_RDWR,0);
    if(-1==fd_shm)
    {
        fprintf(stderr, "error happens");
		exit(1);
    }
    sharepointer=mmap(NULL,(size_t)pagesize,PROT_READ|PROT_WRITE,MAP_SHARED,fd_shm,0);
    mutex=((pthread_mutex_t*)sharepointer);
    int maxClient=pagesize/64-1;
    int find=0;
    struct timeval startTime;
    struct timeval endTime;
    struct timeval resultTime;
    // critical section begins
    pthread_mutex_lock(mutex);   
    // client updates available segment
    for(int i=1;i<=maxClient;i++) {
        if(-1==((stats_t*)(sharepointer+i*64))->pid) {
            ((stats_t*)(sharepointer+i*64))->pid=getpid();
            gettimeofday(&startTime,NULL);//get birth time
            ((stats_t*)(sharepointer+i*64))->init=time(NULL); 
            strcpy(((stats_t*)(sharepointer+i*64))->clientString,argv[1]);
            time_t curt = time(NULL);    
            struct tm *timeptr = localtime(&curt);
            strftime(((stats_t*)(sharepointer+i*64))->birth,25,format,timeptr);//write into sharepointer	
            find=1;
            break;
        }
    }
    if(find==0) {//no place for the client to write
        fprintf(stderr, "error happens");
		exit(0);
    }
    pthread_mutex_unlock(mutex);
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
	while (1) {
        
		// ADD
        sleep(1);
        //update elapsed time of client process
        gettimeofday(&endTime,NULL);
        timersub(&endTime, &startTime, &resultTime);
        for(int i=1;i<=maxClient;i++)
        {
            if(((stats_t*)(sharepointer+i*64))->pid==getpid()) {
                ((stats_t*)(sharepointer+i*64))->elapsed_sec=resultTime.tv_sec;
                ((stats_t*)(sharepointer+i*64))->elapsed_msec=resultTime.tv_usec/1000.0;    
            }
        }
        // Print active clients
        printf("Active clients : ");
        for(int i=1;i<=maxClient;i++)
        {
            if(-1!=((stats_t*)(sharepointer+i*64))->pid)
            {
                printf("%d ",((stats_t*)(sharepointer+i*64))->pid);
            }
        }
        printf("\n");
        
    }
    return 0;
}
