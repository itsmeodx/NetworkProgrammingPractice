#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iso646.h>

#define DEFAULT_PORT "4242"
#define DEFAULT_MSG "Hello from server!"
#define BACKLOG 5

void *getinaddr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return (&(((struct sockaddr_in *)sa)->sin_addr));
	return (&(((struct sockaddr_in6 *)sa)->sin6_addr));
}

void sighandler(int signal)
{
	int m_errno = errno;

	if (signal == SIGCHLD)
		while (waitpid(-1, NULL, WNOHANG) > 0)
			;
	errno = m_errno;
}

void install_signals()
{
	struct sigaction sa;

	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("server: sigaction()");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char const *argv[])
{
	struct addrinfo hints, *myAddr;
	const char *port, *msg;

	if (argc > 1 and argc < 4)
	{
		port = argv[1];
		if (argc == 3)
			msg = argv[2];
		else
			msg = DEFAULT_MSG;
	}
	else if (argc == 1)
	{
		port = DEFAULT_PORT;
		msg = DEFAULT_MSG;
	}
	else
	{
		fprintf(stderr, "Usage: server [PORT] [MSG]\n");
		return (EXIT_FAILURE);
	}

	hints = (struct addrinfo){0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int rv;
	if ((rv = getaddrinfo(NULL, port, &hints, &myAddr)) != 0)
	{
		fprintf(stderr, "server: getaddrinfo(): %s\n", gai_strerror(rv));
		return (EXIT_FAILURE);
	}

	int sockFd, yes = 1;
	struct addrinfo *p;
	for (p = myAddr; p != NULL; p = p->ai_next)
	{
		if ((sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("server: socket()");
			continue;
		}

		if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("server: setsocketopt()");
			close(sockFd);
			continue;
		}

		if (bind(sockFd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockFd);
			perror("server: bind()");
			continue;
		}

		break;
	}

	freeaddrinfo(myAddr);

	if (p == NULL)
	{
		fprintf(stderr, "server: failed to start!\n");
		exit(EXIT_FAILURE);
	}

	if (listen(sockFd, BACKLOG) == -1)
	{
		perror("server: listen()");
		close(sockFd);
		exit(EXIT_FAILURE);
	}

	install_signals();

	printf("server: waiting for connections...\n");

	struct sockaddr_storage theirAddr;
	socklen_t addrLen = sizeof(theirAddr);
	int newSockFd;
	char theirIP[INET6_ADDRSTRLEN];
	while (true)
	{
		if ((newSockFd = accept(sockFd, (struct sockaddr *)&theirAddr, &addrLen)) == -1)
		{
			perror("server: accept()");
			continue;
		}

		inet_ntop(theirAddr.ss_family, getinaddr((struct sockaddr *)&theirAddr), theirIP, sizeof(theirIP));
		printf("server: got connection from %s\n", theirIP);

		if (!fork())
		{
			// printf("server: new child spawned to handle connection from %s...\n", theirIP);
			close(sockFd);
			if (send(newSockFd, msg, strlen(msg), 0) == -1)
				perror("server: send()");
			close(newSockFd);
			exit(EXIT_SUCCESS);
		}
		close(newSockFd);
	}

	close(sockFd);
	return (EXIT_SUCCESS);
}
