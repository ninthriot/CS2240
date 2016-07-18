/*C-Server Thread Implementation - Spencer Hunt CS2240 2016
This is a simple web server which can serve up images (jpg), audio (mp3), and text based files (and html).
Threads are used to allow multiple clients accessing the same server. Much credit to Dr. Trenary for many ideas and code examples.
This only uses threads, but does not implement a thread pool, that is currently being worked on. 
The second version will utilize a thread pool which will hopefully improve performance!
Not sure about fixing signedness warning in gcc... working on it.
Still commenting out code as well. 
Thread Pool will now be a separate implementation so I can run tests on both to see if there are any advantages, one over the other.
*/
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

#define BufferSize 1024
#define BIG_ENUF 4096

void * handleRequest(void* data) {

	int fileExists = 1;
	int FileSize;
	int HeaderCount = 0;
	int newsockfd = (int)data;
	char buffer[BufferSize];
	char Response[BIG_ENUF]; 
	char * BigBuffer;
	char * GetToken;
	char * SavePtr;
	char * TmpBuffer;
	char * type;
	FILE * F;
	struct stat fileSize;

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
	return 0;
}

int main(int argc, char *argv[])
{
	int sockfd;
	int newsockfd;
	int port;
	int clilen;
	struct sockaddr_in server, client;
	pthread_t p_thread;
	
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
		
		while (1)
		{	
			listen(sockfd, 5);
			clilen = sizeof(client);
			newsockfd = accept(sockfd, (struct sockaddr *) &client, &clilen);
			if (newsockfd < 0) err_sys( "ERROR on accept");
			pthread_create(&p_thread, NULL, handleRequest, (void*)(intptr_t)newsockfd);
		}
	return 0;
}
