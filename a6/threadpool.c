/*C-Server ThreadPool Implementation - Spencer Hunt CS2240 2016 This is a simple web server which can serve up images (jpg), audio (mp3), and text based files (and html). Threads are used to allow multiple clients accessing the same server. A thread pool is used to handle multiple requests simultaneously, and a queue is used to store the requests if all threads are in use at the time. Much credit to Dr. Trenary for many ideas and code examples. Also credit to Arden Ruttan for a thread pool skeleton code. This was very simple to implement my server code into.  Not sure about fixing signedness warning in gcc... working on it. Also, weird warnings about unused variables (which are actually used) looking into it. Still commenting out code as well.
Number of threads in the pool is currently 5... testing advantages of more and less.
This just in: server crashes with multiple refreshes of mp3 files... interesting.
*/ 

#define BufferSize 1024
#define BIG_ENUF 4096
#define NUM_HANDLER_THREADS 5
#define _GNU_SOURCE

#include "apue.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


/* global mutex for our program. assignment initializes it. */
/* note that we use a RECURSIVE mutex, since a handler      */
/* thread might try to lock it twice consecutively.         */
pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/* global condition variable for our program. assignment initializes it. */
pthread_cond_t  got_request   = PTHREAD_COND_INITIALIZER;

int num_requests = 0;	/* number of pending requests, initially none */

/* format of a single request. */
struct request {
    int fd;		    /* number of the request                  */
    struct request* next;   /* pointer to next request, NULL if none. */
};

struct request* requests = NULL;     /* head of linked list of requests. */
struct request* last_request = NULL; /* pointer to last request.         */

/*
 * function add_request(): add a request to the requests list
 * algorithm: creates a request structure, adds to the list, and
 *            increases number of pending requests by one.
 * input:     request number, linked list mutex.
 * output:    none.
 */
void add_request(int newsockfd, pthread_mutex_t* p_mutex,pthread_cond_t*  p_cond_var)
{
    int rc;	                    /* return code of pthreads functions.  */
    struct request* a_request;      /* pointer to newly added request.     */

    /* create structure with new request */
    a_request = (struct request*)malloc(sizeof(struct request));
    if (!a_request) { /* malloc failed?? */
	fprintf(stderr, "add_request: out of memory\n");
	exit(1);
    }
    a_request->fd = newsockfd;
    a_request->next = NULL;

    /* lock the mutex, to assure exclusive access to the list */
    rc = pthread_mutex_lock(p_mutex);

    /* add new request to the end of the list, updating list */
    /* pointers as required */
    if (num_requests == 0) { /* special case - list is empty */
	requests = a_request;
	last_request = a_request;
    }
    else {
	last_request->next = a_request;
	last_request = a_request;
    }

    /* increase total number of pending requests by one. */
    num_requests++;

#ifdef DEBUG
    printf("add_request: added request with sockfd '%d'\n", a_request->fd);
    fflush(stdout);
#endif /* DEBUG */

    /* unlock mutex */
    rc = pthread_mutex_unlock(p_mutex);

    /* signal the condition variable - there's a new request to handle */
    rc = pthread_cond_signal(p_cond_var);
}

/*
 * function get_request(): gets the first pending request from the requests list
 *                         removing it from the list.
 * algorithm: creates a request structure, adds to the list, and
 *            increases number of pending requests by one.
 * input:     request number, linked list mutex.
 * output:    pointer to the removed request, or NULL if none.
 * memory:    the returned request need to be freed by the caller.
 */
struct request* get_request(pthread_mutex_t* p_mutex)
{
    int rc;	                    /* return code of pthreads functions.  */
    struct request* a_request;      /* pointer to request.                 */

    /* lock the mutex, to assure exclusive access to the list */
    rc = pthread_mutex_lock(p_mutex);

    if (num_requests > 0) {
	a_request = requests;
	requests = a_request->next;
	if (requests == NULL) { /* this was the last request on the list */
	    last_request = NULL;
	}
	/* decrease the total number of pending requests */
	num_requests--;
    }
    else { /* requests list is empty */
	a_request = NULL;
    }

    /* unlock mutex */
    rc = pthread_mutex_unlock(p_mutex);

    /* return the request to the caller. */
    return a_request;
}

/*
 * function handle_request(): handle a single given request.
 * algorithm: prints a message stating that the given thread handled
 *            the given request.
 * input:     request pointer, id of calling thread.
 * output:    none.
 */
void handle_request(struct request* a_request, int thread_id)
{
	int fileExists = 1;
	int FileSize;
	int HeaderCount = 0;
	int newsockfd = a_request->fd;
	char buffer[BufferSize];
	char Response[BIG_ENUF]; 
	char * BigBuffer;
	char * GetToken;
	char * SavePtr;
	char * TmpBuffer;
	char * type;
	FILE * F;
	struct stat fileSize;
if (a_request) {
	memset(buffer, 0, BufferSize);
	GetToken = strtok_r(buffer, " ", &SavePtr);

	if ((read(newsockfd, buffer, BufferSize - 1)) < 0) err_sys("ERROR reading from socket");

	printf("%s\n", (TmpBuffer = strtok_r(buffer, "\n", &SavePtr)));
	GetToken = strtok_r(TmpBuffer, " ", &SavePtr);
	printf("%s\n", GetToken);
	GetToken = strtok_r(NULL, " ", &SavePtr);
	GetToken++;

        if((strcmp(GetToken,"")) == 0) //If nothing after / then send them to index.html
	{
        strcat(GetToken,"index.html"); 
	F = fopen(GetToken, "r");
        }
	
	else if ((F =  fopen(GetToken, "r")) == NULL) //If we can't open the file they think we might have, redirect them to missing.html
	{ 
		F =  fopen("missing.html", "r");
		fileExists = 0;
	}

	if (fileExists == 1) 
	{
		strtok_r(GetToken, ".", &SavePtr);
		type = strtok_r(NULL, ".", &SavePtr);
	}

	if ((fstat(fileno(F), &fileSize) == -1)) err_sys("Error in fstat!\n"); //get the filesize
	FileSize = fileSize.st_size;

	HeaderCount = 0; //variable counting the index in the message we send back so we don't have to 

	if (fileExists == 1)
	{
		HeaderCount += sprintf( Response + HeaderCount, "HTTP/1.0 200 OK\r\n");
		HeaderCount += sprintf( Response + HeaderCount, "Server: RiotServer/1.3.37\r\n");
		if (strcmp(type, "jpg") == 0) HeaderCount += sprintf( Response + HeaderCount, "Content-Type:image/jpeg\r\n");
		else if (strcmp(type, "mp3") == 0) HeaderCount += sprintf( Response + HeaderCount, "Content-Type:audio/mp3\r\n");
		else if (strcmp(type, "ico") == 0) HeaderCount += sprintf( Response + HeaderCount, "Content-Type:image/x-icon\r\n");
		else HeaderCount += sprintf( Response + HeaderCount, "Content-Type:text/html\r\n");
	}

	else 
	{
		HeaderCount += sprintf( Response + HeaderCount, "HTTP/1.0 404 NOT FOUND\r\n");
		HeaderCount += sprintf( Response + HeaderCount, "Server: RiotServer/1.3.37\r\n");
		HeaderCount += sprintf( Response + HeaderCount, "Content-Type:text/html\r\n");
	}

	HeaderCount += sprintf( Response + HeaderCount, "Content-Length:%d\r\n", FileSize);
	HeaderCount += sprintf( Response + HeaderCount, "\r\n");

	write(STDERR_FILENO, Response, HeaderCount);
	write(newsockfd, Response, HeaderCount);
	BigBuffer = malloc(FileSize + 2);
	fread(BigBuffer, 1, FileSize, F);
	write(newsockfd, BigBuffer, FileSize);

	free(BigBuffer);
	close(newsockfd);
	fclose(F);
	//printf("Thread '%d' handled request with sockfd '%d'\n",thread_id, a_request->fd);
	//fflush(stdout);
    }
}

/*
 * function handle_requests_loop(): infinite loop of requests handling
 * algorithm: forever, if there are requests to handle, take the first
 *            and handle it. Then wait on the given condition variable,
 *            and when it is signaled, re-do the loop.
 *            increases number of pending requests by one.
 * input:     id of thread, for printing purposes.
 * output:    none.
 */
void*
handle_requests_loop(void* data)
{
    int rc;	                    /* return code of pthreads functions.  */
    struct request* a_request;      /* pointer to a request.               */
    int thread_id = *((int*)data);  /* thread identifying number           */

#ifdef DEBUG
    printf("Starting thread '%d'\n", thread_id);
    fflush(stdout);
#endif /* DEBUG */

    /* lock the mutex, to access the requests list exclusively. */
    rc = pthread_mutex_lock(&request_mutex);

#ifdef DEBUG
    printf("thread '%d' after pthread_mutex_lock\n", thread_id);
    fflush(stdout);
#endif /* DEBUG */

    /* do forever.... */
    while (1) {
#ifdef DEBUG
    	printf("thread '%d', num_requests =  %d\n", thread_id, num_requests);
    	fflush(stdout);
#endif /* DEBUG */
	if (num_requests > 0) { /* a request is pending */
	    a_request = get_request(&request_mutex);
	    if (a_request) { /* got a request - handle it and free it */
		handle_request(a_request, thread_id);
		free(a_request);
	    }
	}
	else {
	    /* wait for a request to arrive. note the mutex will be */
	    /* unlocked here, thus allowing other threads access to */
	    /* requests list.                                       */
#ifdef DEBUG
    	    printf("thread '%d' before pthread_cond_wait\n", thread_id);
    	    fflush(stdout);
#endif /* DEBUG */
	    rc = pthread_cond_wait(&got_request, &request_mutex);
	    /* and after we return from pthread_cond_wait, the mutex  */
	    /* is locked again, so we don't need to lock it ourselves */
#ifdef DEBUG
    	    printf("thread '%d' after pthread_cond_wait\n", thread_id);
    	    fflush(stdout);
#endif /* DEBUG */
	}
    }
}

/* like any C program, program's execution begins in main */
int main(int argc, char* argv[])
{
    int        i;                                /* loop counter          */
    int        thr_id[NUM_HANDLER_THREADS];      /* thread IDs            */
    pthread_t  p_threads[NUM_HANDLER_THREADS];   /* thread's structures   */
    int sockfd;
    int newsockfd;
    int port;
    int clilen;
    struct sockaddr_in server, client;

	if (argc < 2) 
	{
		fprintf(stderr, "Please provide a port as an argument!\n");
		exit(1);
	}
	system("clear");
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) err_sys("Error opening socket!");

	memset((char *) &server, 0, sizeof(server));
	port = atoi(argv[1]);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) err_sys("Error on binding!");
		
    /* create the request-handling threads */
    for (i=0; i<NUM_HANDLER_THREADS; i++) {
	thr_id[i] = i;
	pthread_create(&p_threads[i], NULL, handle_requests_loop, (void*)&thr_id[i]);
    }

    sleep(3);
    /* run a loop that generates requests */
    while(1){	
		listen(sockfd, 5);
		clilen = sizeof(client);
		newsockfd = accept(sockfd, (struct sockaddr *) &client, &clilen);
		if (newsockfd < 0) err_sys( "ERROR on accept");
		else{
		add_request(newsockfd, &request_mutex, &got_request);
		}
	/* pause execution for a little bit, to allow      */
	/* other threads to run and handle some requests.  */
	//if (rand() > 3*(RAND_MAX/4)) { /* this is done about 25% of the time */
	  //  delay.tv_sec = 0;
	    //delay.tv_nsec = 10;
	    //nanosleep(&delay, NULL);
	//}
    }
    /* now wait till there are no more requests to process */
    sleep(5);
    printf("Glory,  we are done.\n");
    return 0;
}
