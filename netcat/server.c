
#define _POSIX_C_SOURCE 1
#include <signal.h> /* sigaction */
#include <stdio.h> /* printf */
#include <sys/types.h>   /* socket */
#include <sys/socket.h> /* socket */
#include <stdlib.h> /* exit */
#include <arpa/inet.h>  /* INADDR_ANY */
#include <string.h> /* memset */
#include <sys/epoll.h> /* epoll_create1 */
#include <unistd.h> /* close */


#define MAX_EVENTS (10)
#define BUFFSIZE (100)
#define LISTEN_BACKLOG (50)


int cfd = 0;

void SigintHandler(int signum)
{
	(void)signum;
	printf("Received a sigint, terminating\n");
	
	close(cfd);
	exit(1);
}


void Read(int fd, char *buffer)
{
	int bytes_read = 0;
	
	bytes_read = read(fd, buffer, BUFFSIZE);
	if(-1 == bytes_read)
	{
		perror("read");
	}
	buffer[bytes_read] = 0;
}

void WriteToClient(char *buffer)
{
	while(*buffer)
	{
		write(cfd, buffer++, 1);
	}
}

int CreateSocket()
{
	int opt = 1; 
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("socket creation");
		exit(1);
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
												&opt, sizeof(opt)))
	{
		perror("setsockopt"); 
		exit(EXIT_FAILURE);
	}
	
	return sockfd;
}

void Bind(int sockfd, int is_specific_addr, char* addr, int port)
{
	struct sockaddr_in servaddr;
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	if(is_specific_addr)
	{
		servaddr.sin_addr.s_addr = inet_addr(addr);
	}
	else
	{
		servaddr.sin_addr.s_addr = INADDR_ANY;
	}
	servaddr.sin_port = htons(port);
	
	if(-1 == bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)))
	{
		perror("bind");
		exit(1);
	}
	printf("Bound to %s\n", inet_ntoa(servaddr.sin_addr));
}


void TalkToClient(int epoll_fd)
{
	struct epoll_event events[MAX_EVENTS];
	char buffer[BUFFSIZE] = { 0 };
	int nfds = 0, i = 0;
	
	while(1)
	{
		nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if(-1 == nfds)
		{
			perror("epoll_wait");
			exit(1);
		}
		
		for(i = 0; i < nfds; ++i)
		{
			if(0 == events[i].data.fd)
			{
				
				Read(0, buffer);
				WriteToClient(buffer);
			}
			if(cfd == events[i].data.fd && (events[i].events & (EPOLLRDHUP | EPOLLHUP)))
			{	
				printf("peer hung up\n");
				exit(1);
			}
			if(cfd == events[i].data.fd && (events[i].events & EPOLLIN))
			{
				printf("peer: %d\n", events[i].events);
				Read(cfd, buffer);
				printf("%s", buffer);
			}
			
		}
		
	}
}


void ReceiveConnection(int sockfd)
{
	struct sockaddr_in cliaddr;
	socklen_t client_addr_size;	
		
	memset(&cliaddr, 0, sizeof(cliaddr));
	
	if(-1 == listen(sockfd, LISTEN_BACKLOG))
	{
		perror("listen");
		exit(1);
	}
	
	printf("After listen\n");
	client_addr_size = sizeof(cliaddr);
	cfd = accept(sockfd, (struct sockaddr*)&cliaddr, &client_addr_size);
	if(-1 == cfd)
	{
		perror("accept");
		exit(1);
	}
	
	printf("Received connection from: %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
	printf("New socket: %d\n", cfd);
}

int ConfigureEpoll()
{
	struct epoll_event stdin_event = { 0 };
	struct epoll_event client_event = { 0 };
	
	int epoll_fd = 0;
	
	epoll_fd = epoll_create1(0); /* 0 is the flags parameter */
	if(-1 == epoll_fd)
	{
		perror("epoll_create1");
		exit(1);	
	}
	
	stdin_event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
	stdin_event.data.fd = 0;
	if(-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &stdin_event))
	{
		perror("epoll_ctl");
	}
	
	client_event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
	client_event.data.fd = cfd;
	if(-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &client_event))
	{
		perror("epoll_ctl");
	}
	
	return epoll_fd;
}


void ConfigureSighandler()
{
	struct sigaction sa = { 0 };
	sa.sa_handler = SigintHandler;
	if(-1 == sigaction(SIGINT, &sa, NULL))
	{
		perror("sigaction");
		exit(1);
	}
}

void Usage(const char *prog_name)
{
    printf("Usage:\n");
    printf("%s [addr] <port>\n", prog_name);
}

int main(int argc, char** argv)
{
	
	int sockfd = CreateSocket();
	int epoll_fd = 0;
	
	if(argc > 2)
	{
		Bind(sockfd, 1, argv[1], atoi(argv[2]));
	}
	else if(argc > 1)
	{
		Bind(sockfd, 0, NULL, atoi(argv[1]));
    }
    else
    {
        Usage(argv[0]);
        return 1;
    }
	
	ConfigureSighandler();
	ReceiveConnection(sockfd);
	epoll_fd = ConfigureEpoll();
	TalkToClient(epoll_fd);
	
	
	return 0;
}
