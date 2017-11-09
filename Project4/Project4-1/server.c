#include "cs537.h"
#include "request.h"
pthread_mutex_t lock;
pthread_cond_t full;               //add condition variables to judge
pthread_cond_t empty;
int bufferlen;                     //buffer length to hold incoming requests
int count;
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
void* processRequest(void* workBuffer) {
    while(1) {
        Mutex_lock(&lock);
        while(count==0) {//no request in the buffer
            Cond_wait(&full, &lock);
        }
        for(int i=0;i<bufferlen;i++) {
            if(workBuffer[i]!=-1) {
                requestHandle(workBuffer[i]);
                workBuffer[i]=-1;
                count--;
            }
        }
        Cond_signal(&empty);
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
    if(*threadnum<=0||bufferlen<=0) {
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
    pthread_t *workers = malloc(sizeof(pthread_t)*threadnum);
    int *workBuffer = malloc(sizeof(int)*bufferlen);
    Mutex_init(&lock);
    Cond_init(&full);
    Cond_init(&empty);
    for(int i=0;i<threadnum;i++) {
        Thread_create(&workers[i],NULL,processRequest,(void*)workBuffer);
    }
    for(int i=0;i<bufferlen;i++) {
        workBuffer[i] = -1;//initilize the buffer as -1
    }
    // 
    // CS537: Create some threads...
    //
    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        for(int i=0;i<bufferlen;i++) {
            if(workBuffer[i]==-1) {
                workBuffer[i]=connfd;//write work into Buffer;
                count++;
            }
                
        }
	// 
	// CS537: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 

	    //Close(connfd);
    }
    free(workers);
    free(workBuffer);
}


    


 
