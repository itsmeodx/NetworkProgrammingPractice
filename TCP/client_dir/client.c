
/**
 * @file client.c
 * @brief TCP client: connects to server and prints received message.
 *
 * Usage: client hostname [PORT]
 *   - If PORT is omitted, uses default 4242.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iso646.h>

#define PORT "4242"
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
 * @brief Main entry point. Connects to TCP server and prints received message.
 */
int main(int argc, char const *argv[])
{
	struct addrinfo hints, *theirAddr;
	const char *hostname, *port;

	// Parse arguments: hostname required, PORT optional
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

	setbuf(stdout, NULL); // Disable buffering for stdout
	setbuf(stderr, NULL); // Disable buffering for stderr

	// Setup TCP socket hints
	hints = (struct addrinfo){0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// Resolve server address
	int rv;
	if ((rv = getaddrinfo(hostname, port, &hints, &theirAddr)) != 0)
	{
		fprintf(stderr, "client: getaddrinfo(): %s\n", gai_strerror(rv));
		return (EXIT_FAILURE);
	}

	// Create and connect TCP socket
	struct addrinfo *p;
	char theirIP[INET6_ADDRSTRLEN];
	int sockFd;
	for (p = theirAddr; p != NULL; p = p->ai_next)
	{
		// Create socket
		if ((sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client: socket()");
			continue;
		}

		// Get server IP address for debugging
		// inet_ntop(p->ai_family, getinaddr((struct sockaddr *)p->ai_addr), theirIP, sizeof(theirIP));
		// printf("client: attempting connection to %s...\n", theirIP);

		// Connect to server
		if (connect(sockFd, p->ai_addr, p->ai_addrlen) == -1)
		{
			// perror("client: connect()");
			close(sockFd);
			continue;
		}

		break;
	}

	// Check if we successfully connected
	if (p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		return (EXIT_FAILURE);
	}

	// Get server IP address for confirmation
	inet_ntop(p->ai_family, getinaddr((struct sockaddr *)p->ai_addr), theirIP, sizeof(theirIP));
	printf("client: connected to %s...\n", theirIP);

	// No longer needed
	freeaddrinfo(theirAddr);

	// Receive and print message from server
	char buf[MAXDSIZE];
	printf("client: message received: \"");
	int rc, done = false;
	// Read until null terminator or error
	while (!done)
	{
		rc = recv(sockFd, buf, MAXDSIZE, 0);
		if (rc <= 0)
			break;
		for (int i = 0; i < rc; ++i)
		{
			if (buf[i] == '\0')
			{
				done = true;
				break;
			}
			putchar(buf[i]);
		}
	}
	printf("\"\n");

	if (rc == -1)
		perror("client: recv()");

	close(sockFd);
	return (EXIT_SUCCESS);
}
