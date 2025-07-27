#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iso646.h>

#define DEFAULT_PORT "4242"
#define MAXDSIZE 10

void *getinaddr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return (&(((struct sockaddr_in *)sa)->sin_addr));
	return (&(((struct sockaddr_in6 *)sa)->sin6_addr));
}

int main(int argc, char const *argv[])
{
	struct addrinfo hints, *myAddr;
	const char *port;

	if (argc < 1 or argc > 2)
	{
		fprintf(stderr, "Usage: listener [PORT]\n");
		return (EXIT_FAILURE);
	}
	if (argc == 2)
		port = argv[1];
	else
		port = DEFAULT_PORT;

	hints = (struct addrinfo){0};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	int rv;
	if ((rv = getaddrinfo(NULL, port, &hints, &myAddr)) != 0)
	{
		fprintf(stderr, "listener: getaddrinfo(): %s\n", gai_strerror(rv));
		return (EXIT_FAILURE);
	}

	struct addrinfo *p;
	int sockFd;
	for (p = myAddr; p != NULL; p = p->ai_next)
	{
		if ((sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("listener: socket()");
			continue;
		}

		if (bind(sockFd, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("listener: bind()");
			close(sockFd);
			continue;
		}

		break;
	}

	freeaddrinfo(myAddr);

	if (p == NULL)
	{
		fprintf(stderr, "listner: failed to start!\n");
		exit(EXIT_FAILURE);
	}

	char buf[MAXDSIZE];
	struct sockaddr_storage theirAddr;
	socklen_t addrLen;
	int rc;
	while (true)
	{
		write(STDOUT_FILENO, "listener: waiting to recvfrom...\n", 33);

		if ((rc = recvfrom(sockFd, buf, MAXDSIZE, 0,
						   (struct sockaddr *)&theirAddr, &addrLen)) == -1)
		{
			perror("listner: recvfrom()");
			close(sockFd);
			return (EXIT_FAILURE);
		}

		char theirIP[INET6_ADDRSTRLEN];
		inet_ntop(theirAddr.ss_family,
				  getinaddr((struct sockaddr *)&theirAddr), theirIP, sizeof(theirIP));

		printf("listener: got a message from %s:\n\"%.*s", theirIP, rc, buf);
		fflush(stdout);
		while (!memchr(buf, '\r', rc) || rc != 1)
		{
			if ((rc = recvfrom(sockFd, buf, MAXDSIZE, 0,
							   (struct sockaddr *)&theirAddr, &addrLen)) == -1)
			{
				printf("\"\n");
				perror("listner: recvfrom()");
				close(sockFd);
				return (EXIT_FAILURE);
			}
			if (memchr(buf, '\r', rc) && rc == 1)
				break;
			printf("%.*s", rc, buf);
		}
		printf("\"\n");
	}

	close(sockFd);
	return (EXIT_SUCCESS);
}
