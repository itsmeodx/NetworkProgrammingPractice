#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iso646.h>

#define DEFAULT_PORT "4242"
#define DEFAULT_MSG "Hello from talker!"
#define MAXDSIZE 10

void *getinaddr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return (&(((struct sockaddr_in *)sa)->sin_addr));
	return (&(((struct sockaddr_in6 *)sa)->sin6_addr));
}

int main(int argc, char const *argv[])
{
	struct addrinfo hints, *theirAddr;
	const char *hostname, *port, *msg = NULL;

	if (argc < 2 || argc > 4)
	{
		fprintf(stderr, "Usage: talker hostname [PORT] [MSG]\n");
		return (EXIT_FAILURE);
	}
	hostname = argv[1];
	if (argc > 2)
	{
		port = argv[2];
		if (argc == 4)
			msg = argv[3];
	}
	else
		port = DEFAULT_PORT;

	hints = (struct addrinfo){0};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int rv;
	if ((rv = getaddrinfo(hostname, port, &hints, &theirAddr)) != 0)
	{
		fprintf(stderr, "talker: getaddrinfo(): %s\n", gai_strerror(rv));
		return (EXIT_FAILURE);
	}

	struct addrinfo *p;
	int sockFd;
	for (p = theirAddr; p != NULL; p = p->ai_next)
	{
		if ((sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("talker: socket()");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "talker: couldn't connect!");
		return (EXIT_FAILURE);
	}

	int rc;
	if (msg)
	{
		// Send provided message argument in chunks
		size_t sent = 0, msglen = strlen(msg), remain, chunk;
		while (sent < msglen)
		{
			remain = msglen - sent;
			chunk = (remain > MAXDSIZE) ? MAXDSIZE : remain;
			if ((rc = sendto(sockFd, msg + sent, chunk, 0,
							 p->ai_addr, p->ai_addrlen)) == -1)
			{
				perror("talker: sendto()");
				close(sockFd);
				freeaddrinfo(theirAddr);
				return (EXIT_FAILURE);
			}
			sent += chunk;
			usleep(1000);
		}
		if ((rc = sendto(sockFd, "\r", 1, 0,
						 p->ai_addr, p->ai_addrlen)) == -1)
		{
			perror("talker: sendto()");
			close(sockFd);
			freeaddrinfo(theirAddr);
			return (EXIT_FAILURE);
		}
	}
	else
	{
		char buf[MAXDSIZE];
		ssize_t nread;
		while ((nread = read(STDIN_FILENO, buf, MAXDSIZE)) > 0)
		{
			if ((rc = sendto(sockFd, buf, nread, 0,
							 p->ai_addr, p->ai_addrlen)) == -1)
			{
				perror("talker: sendto()");
				close(sockFd);
				freeaddrinfo(theirAddr);
				return (EXIT_FAILURE);
			}
			usleep(1000);
		}
		if ((rc = sendto(sockFd, "\r", 1, 0,
						 p->ai_addr, p->ai_addrlen)) == -1)
		{
			perror("talker: sendto()");
			close(sockFd);
			freeaddrinfo(theirAddr);
			return (EXIT_FAILURE);
		}
	}

	printf("talker: message was sent to %s!\n", hostname);
	freeaddrinfo(theirAddr);
	close(sockFd);

	return (EXIT_SUCCESS);
}
