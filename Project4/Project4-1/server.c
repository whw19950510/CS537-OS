#include "cs537.h"
#include "request.h"
pthread_mutex_t lock;
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
    int threadnum,bufferlen;

    getargs(&port, &threadnum, &bufferlen, argc, argv);
    pthread_t *workers = malloc(sizeof(pthread_t)*threadnum);
    int *workBuffer = malloc(sizeof(int)*bufferlen);
    for(int i=0;i<threadnum;i++) {
        pthread_create(&workers[i],NULL,processRequest,(void*)clientaddr);
    }
    // 
    // CS537: Create some threads...
    //
    pthread_mutex_init(&lock);
    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

	// 
	// CS537: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 
	requestHandle(connfd);

	Close(connfd);
    }
    free(workers);
    free(workBuffer);
}


    


 
