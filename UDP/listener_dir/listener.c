
/**
 * @file listener.c
 * @brief UDP server: receives datagrams and prints message up to delimiter '\r'.
 *
 * Usage: listener [PORT]
 *   - If PORT is omitted, uses default 4242.
 */

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

/**
 * @brief Extracts pointer to IPv4 or IPv6 address from sockaddr.
 */
void *getinaddr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return (&(((struct sockaddr_in *)sa)->sin_addr));
	return (&(((struct sockaddr_in6 *)sa)->sin6_addr));
}

/**
 * @brief Main entry point. Receives UDP datagrams and prints message up to delimiter.
 */
int main(int argc, char const *argv[])
{
	struct addrinfo hints, *myAddr;
	const char *port;

	// Parse arguments: PORT optional
	if (argc < 1 or argc > 2)
	{
		fprintf(stderr, "Usage: listener [PORT]\n");
		return (EXIT_FAILURE);
	}
	if (argc == 2)
		port = argv[1];
	else
		port = DEFAULT_PORT;

	setbuf(stdout, NULL); // Disable buffering for stdout
	setbuf(stderr, NULL); // Disable buffering for stderr

	// Setup UDP socket hints
	hints = (struct addrinfo){0};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	// Resolve local address
	int rv;
	if ((rv = getaddrinfo(NULL, port, &hints, &myAddr)) != 0)
	{
		fprintf(stderr, "listener: getaddrinfo(): %s\n", gai_strerror(rv));
		return (EXIT_FAILURE);
	}

	// Create and bind UDP socket
	struct addrinfo *p;
	int sockFd;
	for (p = myAddr; p != NULL; p = p->ai_next)
	{
		// Create socket
		if ((sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("listener: socket()");
			continue;
		}

		// Bind socket to address
		if (bind(sockFd, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("listener: bind()");
			close(sockFd);
			continue;
		}

		break;
	}

	// No longer needed
	freeaddrinfo(myAddr);

	// Check if we successfully created and bound a socket
	if (p == NULL)
	{
		fprintf(stderr, "listner: failed to start!\n");
		exit(EXIT_FAILURE);
	}

	// Main receive loop: print message up to delimiter '\r'
	char buf[MAXDSIZE];
	struct sockaddr_storage theirAddr;
	socklen_t addrLen = sizeof(theirAddr);
	int rc;
	while (true)
	{
		printf("listener: waiting to recvfrom...\n");

		// Receive datagram
		if ((rc = recvfrom(sockFd, buf, MAXDSIZE, 0,
						   (struct sockaddr *)&theirAddr, &addrLen)) == -1)
		{
			perror("listner: recvfrom()");
			close(sockFd);
			return (EXIT_FAILURE);
		}

		// Get sender's IP address
		char theirIP[INET6_ADDRSTRLEN];
		inet_ntop(theirAddr.ss_family,
				  getinaddr((struct sockaddr *)&theirAddr), theirIP, sizeof(theirIP));

		printf("listener: got a message from %s:\n%.*s", theirIP, rc, buf);
		// Continue receiving until a datagram of size 1 and buf[0] == '\r' is found
		while (!(rc == 1 && buf[0] == '\r'))
		{
			if ((rc = recvfrom(sockFd, buf, MAXDSIZE, 0,
							   (struct sockaddr *)&theirAddr, &addrLen)) == -1)
			{
				printf("\n");
				perror("listner: recvfrom()");
				close(sockFd);
				return (EXIT_FAILURE);
			}
			// If delimiter '\r' is found as a single byte, break
			if (rc == 1 && buf[0] == '\r')
				break;
			printf("%.*s", rc, buf);
		}
		printf("\n");
	}

	close(sockFd);
	return (EXIT_SUCCESS);
}
