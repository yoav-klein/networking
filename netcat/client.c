#include <stdio.h> /* printf */
#include <sys/types.h>   /* socket */
#include <sys/socket.h> /* socket */
#include <stdlib.h> /* exit */
#include <arpa/inet.h>  /* INADDR_ANY */
#include <string.h> /* memset */

#define TIMES (20)
#define PORT (8080)
#define LISTEN_BACKLOG (50)

int CreateSocket()
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("socket creation");
		exit(1);
	}
	
	return sockfd;
}

void Connect(int sockfd, char* server_addr_string)
{
	struct sockaddr_in servaddr;
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(server_addr_string);
	servaddr.sin_port = htons(PORT);
	
	if(-1 == connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)))
	{
		perror("connect");
		exit(1);
	}
}

void PingPong(int sockfd)
{
	size_t i = 0;
	socklen_t serv_addr_size;
	struct sockaddr_in servaddr;
	
	memset(&servaddr, 0, sizeof(servaddr));
	
	serv_addr_size = sizeof(servaddr);
	for(i = 0; i < TIMES; ++i)
	{
		char buffer[50];
		ssize_t n = 0;
		char message[20];
		
		sprintf(message, "Hello from client no. %lu", i);
		if(-1 == send(sockfd, message, strlen(message), 0))
		{
			perror("send");
			exit(1);
		}

		n = recv(sockfd, buffer, 50, 0);
		if(-1 == n)
		{
			perror("recv");
			exit(1);
		}
		buffer[n] = '\0';
		
		printf("Client: %s\n", buffer);
		
	}
	
}

int main(int argc, char** argv)
{
	int sockfd = 0;
	if(argc < 2) 
	{
		printf("Please enter server IP address\n");
		exit(1);
	}
	
	sockfd = CreateSocket();
		
	Connect(sockfd, argv[1]);
	
	PingPong(sockfd);
	
	return 0;
}
