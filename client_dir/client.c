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
#include <iso646.h>

#define elif else if

#define PORT "4242"
#define MAXDSIZE 100

void *getinaddr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return (&(((struct sockaddr_in *)sa)->sin_addr));
	return(&(((struct sockaddr_in6 *)sa)->sin6_addr));
}

int main(int argc, char const *argv[])
{
	struct addrinfo hints, *res;
	const char *hostname, *port;

	if (argc < 2 or argc > 3)
	{
		fprintf(stderr, "Usage: client hostname [PORT]\n");
		return (EXIT_FAILURE);
	}
	hostname = argv[1];
	if (argc == 3)
		port = argv[2];
	else
		port = PORT;

	hints = (struct addrinfo){0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int rv;
	if ((rv = getaddrinfo(hostname, port, &hints, &res)) != 0)
	{
		fprintf(stderr, "clinet: getaddrinfo: %s\n", gai_strerror(rv));
		return (EXIT_FAILURE);
	}

	struct addrinfo *p;
	char s[INET6_ADDRSTRLEN];
	int socketfd;
	for (p = res; p != NULL; p = p->ai_next)
	{
		if ((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client: socket()");
			continue;
		}

		inet_ntop(p->ai_family, getinaddr((struct sockaddr *)p->ai_addr), s, sizeof(s));
		printf("client: attempting conncetion to %s...\n", s);

		if (connect(socketfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("client: accept()");
			close(socketfd);
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		return (EXIT_FAILURE);
	}

	inet_ntop(p->ai_family, getinaddr((struct sockaddr *)p->ai_addr), s, sizeof(s));
	printf("client: connceted to %s...\n", s);

	int rc;
	char buf[MAXDSIZE];
	if ((rc = recv(socketfd, buf, MAXDSIZE, 0)) == -1)
	{
		perror("client: recv()");
		return (EXIT_FAILURE);
	}

	buf[rc] = '\0';
	printf("client: message recieved: \"%s\"\n", buf);

	close(socketfd);

	return (EXIT_SUCCESS);
}
