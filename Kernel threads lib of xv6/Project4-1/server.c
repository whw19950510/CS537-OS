#include "cs537.h"
#include "request.h"
pthread_mutex_t lock;
pthread_cond_t fill;               //add condition variables to judge
pthread_cond_t empty;
int bufferlen;                     //buffer length to hold incoming requests
int count;
int front;
int rear;
// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// CS537: Parse the new arguments too

//wrapper function for mutex and conditional variables
int Mutex_init(pthread_mutex_t *lock, const pthread_mutexattr_t *attr) {
    int rc;
    if((rc = pthread_mutex_init(lock,attr) )!=0) {
        fprintf(stderr, "initialize mutex error\n");
        exit(1);
    }
    return rc;
}

int Cond_init(pthread_cond_t *condvar, pthread_condattr_t *cond_attr) {
    int rc;
    if((rc = pthread_cond_init(condvar,cond_attr))!=0) {
        fprintf(stderr, "initialize conditionvar error\n");
        exit(1);
    }
    return rc;
}

int Thread_create(pthread_t *cur,const pthread_attr_t *attr, void *start_routine, void *arg) {
    int rc;
    if((rc = pthread_create(cur,attr,start_routine,arg))!=0) {
        fprintf(stderr, "create new thread error\n");
        exit(1);
    }
    return rc;
}

int Mutex_lock(pthread_mutex_t *lock) {
    int rc;
    if((rc = pthread_mutex_lock(lock))!=0) {
        fprintf(stderr, "get lock error\n");
        exit(1);
    }
    return rc;
}

int Mutex_unlock(pthread_mutex_t *lock) {
    int rc;
    if((rc = pthread_mutex_unlock(lock))!=0) {
        fprintf(stderr, "release lock error\n");
        exit(1);
    }
    return rc;
}

int Cond_wait(pthread_cond_t *condvar,pthread_mutex_t *lock) {
    int rc;
    if((rc = pthread_cond_wait(condvar, lock))!=0) {
        fprintf(stderr, "wait conditionvar error\n");
        exit(1);
    }
    return rc;
}

int Cond_signal(pthread_cond_t *condvar) {
    int rc;
    if((rc = pthread_cond_signal(condvar))!=0) {
        fprintf(stderr, "signal conditionvar error\n");
        exit(1);
    }
    return rc;
}

void processRequest(void* workBuffer) {
    while(1) {
        Mutex_lock(&lock);
        while(count==0) {//no request in the buffer
            Cond_wait(&fill, &lock);
        }
        int connfd = ((int*)workBuffer)[front];
        front = (front + 1)%bufferlen;
        count--;
        Cond_signal(&empty);
        Mutex_unlock(&lock);
        requestHandle(connfd);
        Close(connfd);
    } 
}

void getargs(int *port, int *threadnum, int *bufferlen, int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <port> <threadnum> <bufferlen>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *threadnum = atoi(argv[2]);
    *bufferlen = atoi(argv[3]);
    if(*port<=0||*threadnum<=0||*bufferlen<=0) {
        fprintf(stderr, "threadnum & buffer length must be an integer");
        exit(1);
    }
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    int threadnum;

    getargs(&port, &threadnum, &bufferlen, argc, argv);
    pthread_t *workers = (pthread_t*)malloc(sizeof(pthread_t)*threadnum);
    int *workBuffer = (int*)malloc(sizeof(int)*bufferlen);
    rear = 0;
    front = 0;
    count = 0;

    Mutex_init(&lock,NULL);
    Cond_init(&fill,NULL);
    Cond_init(&empty,NULL);
    for(int i=0;i<threadnum;i++) {
        Thread_create(&workers[i],NULL,processRequest,(void*)workBuffer);
    }
    // 
    // CS537: Create some threads...
    //
    listenfd = Open_listenfd(port);
    clientlen = sizeof(clientaddr);
    while (1) {
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);    
        Mutex_lock(&lock);
        while(count==bufferlen) {
            Cond_wait(&empty,&lock);
        }

        workBuffer[rear] = connfd;
        rear = (rear + 1)%bufferlen;
        count++;
        //printf("%d enqueued to queue\n", connfd);
        Cond_signal(&fill);
        Mutex_unlock(&lock);
    }
	// 
	// CS537: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 

    
    free(workers);
    free(workBuffer);
    return 0;
}


    


 
