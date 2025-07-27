
/**
 * @file talker.c
 * @brief UDP client: sends message or stdin to a UDP server in fixed-size chunks.
 *
 * Usage: talker hostname [MSG] [PORT]
 *   - If MSG is omitted, reads from stdin.
 *   - If PORT is omitted, uses default 4242.
 */

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
 * @brief Main entry point. Sends a message or stdin to a UDP server in MAXDSIZE chunks.
 */
int main(int argc, char const *argv[])
{
	struct addrinfo hints, *theirAddr;
	const char *hostname, *port, *msg;

	// Parse arguments: hostname required, MSG and PORT optional
	if (argc < 2 or argc > 4)
	{
		fprintf(stderr, "Usage: talker hostname [MSG] [PORT]\n");
		return (EXIT_FAILURE);
	}
	hostname = argv[1];
	if (argc > 2)
	{
		msg = argv[2];
		if (argc == 4)
			port = argv[3];
		else
			port = DEFAULT_PORT;
	}
	else
	{
		msg = NULL;
		port = DEFAULT_PORT;
	}

	// Setup UDP socket hints
	hints = (struct addrinfo){0};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	// Resolve server address
	int rv;
	if ((rv = getaddrinfo(hostname, port, &hints, &theirAddr)) != 0)
	{
		fprintf(stderr, "talker: getaddrinfo(): %s\n", gai_strerror(rv));
		return (EXIT_FAILURE);
	}

	// Create UDP socket
	struct addrinfo *p;
	int sockFd;
	for (p = theirAddr; p != NULL; p = p->ai_next)
	{
		// Create socket
		if ((sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("talker: socket()");
			continue;
		}

		break;
	}

	// Check if we successfully created a socket
	if (p == NULL)
	{
		fprintf(stderr, "talker: couldn't connect!");
		return (EXIT_FAILURE);
	}

	int rc;
	if (msg) // If a message is provided
	{
		// Send provided message argument in MAXDSIZE chunks
		size_t sent = 0, msglen = strlen(msg), remain, chunk;
		while (sent < msglen)
		{
			remain = msglen - sent;
			chunk = (remain > MAXDSIZE) ? MAXDSIZE : remain; // Limit chunk size to MAXDSIZE
			if ((rc = sendto(sockFd, msg + sent, chunk, 0,
							 p->ai_addr, p->ai_addrlen)) == -1)
			{
				perror("talker: sendto()");
				close(sockFd);
				freeaddrinfo(theirAddr);
				return (EXIT_FAILURE);
			}
			sent += chunk; // Update total bytes sent
			usleep(1000); // avoid flooding
		}
	}
	else // If no message is provided
	{
		// Read from stdin and send in MAXDSIZE chunks
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
	}
	
	// Send delimiter to mark end of message (always)
	if ((rc = sendto(sockFd, "\r", 1, 0,
					 p->ai_addr, p->ai_addrlen)) == -1)
	{
		perror("talker: sendto()");
		close(sockFd);
		freeaddrinfo(theirAddr);
		return (EXIT_FAILURE);
	}

	// Success
	printf("talker: message was sent to %s!\n", hostname);
	freeaddrinfo(theirAddr);
	close(sockFd);

	return (EXIT_SUCCESS);
}
