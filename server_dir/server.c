#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

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
	struct addrinfo hints, *res;
	const char *port, *msg;

	if (argc > 1 && argc < 4)
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
	if ((rv = getaddrinfo(NULL, port, &hints, &res)) != 0)
	{
		fprintf(stderr, "server: getaddrinfo: %s\n", gai_strerror(rv));
		return (EXIT_FAILURE);
	}

	int sockfd, yes = 1;
	struct addrinfo *p;
	for (p = res; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("server: socket()");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("server: setsocketopt()");
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("server: bind()");
			continue;
		}

		break;
	}

	freeaddrinfo(res);

	if (p == NULL)
	{
		fprintf(stderr, "server failed!\n");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("server: listen()");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	install_signals();

	printf("server: waiting for connections...\n");

	struct sockaddr_storage their_addr;
	socklen_t sin_size = sizeof(their_addr);
	int new_socketfd;
	char client_ip[INET6_ADDRSTRLEN];
	while (true)
	{
		if ((new_socketfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
		{
			perror("server: accept()");
			continue;
		}

		inet_ntop(their_addr.ss_family, getinaddr((struct sockaddr *)&their_addr), client_ip, sizeof(client_ip));
		printf("server: got connection from %s\n", client_ip);

		if (!fork())
		{
			// printf("server: new child spawned to handle connection from %s...\n", client_ip);
			close(sockfd);
			if (send(new_socketfd, msg, strlen(msg), 0) == -1)
				perror("server: send()");
			close(new_socketfd);
			exit(EXIT_SUCCESS);
		}
		close(new_socketfd);
	}

	close(sockfd);
	return (EXIT_SUCCESS);
}
