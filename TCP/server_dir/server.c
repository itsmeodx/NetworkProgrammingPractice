
/**
 * @file server.c
 * @brief TCP server: listens for connections and sends a message to each client.
 *
 * Usage: server [MSG] [PORT]
 *   - If MSG is omitted, uses default message.
 *   - If PORT is omitted, uses default 4242.
 */

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
 * @brief SIGCHLD handler: reap zombie children.
 */
void sighandler(int signal)
{
	int m_errno = errno;

	if (signal == SIGCHLD)
		while (waitpid(-1, NULL, WNOHANG) > 0)
			;
	errno = m_errno;
}


/**
 * @brief Install SIGCHLD handler for child cleanup.
 */
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

/**
 * @brief Main entry point. Listens for TCP connections and sends a message to each client.
 */
int main(int argc, char const *argv[])
{
	struct addrinfo hints, *myAddr;
	const char *port, *msg;

	// Parse arguments: MSG and PORT optional
	if (argc < 1 or argc > 3)
	{
		fprintf(stderr, "Usage: server [MSG] [PORT]\n");
		return (EXIT_FAILURE);
	}
	if (argc > 1)
	{
		msg = argv[1];
		if (argc == 3)
			port = argv[2];
		else
			port = DEFAULT_PORT;
	}
	else
	{
		msg = DEFAULT_MSG;
		port = DEFAULT_PORT;
	}

	setbuf(stdout, NULL); // Disable buffering for stdout
	setbuf(stderr, NULL); // Disable buffering for stderr

	// Setup TCP socket hints
	hints = (struct addrinfo){0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// Resolve local address
	int rv;
	if ((rv = getaddrinfo(NULL, port, &hints, &myAddr)) != 0)
	{
		fprintf(stderr, "server: getaddrinfo(): %s\n", gai_strerror(rv));
		return (EXIT_FAILURE);
	}

	// Create and bind TCP socket
	int sockFd, yes = 1;
	struct addrinfo *p;
	for (p = myAddr; p != NULL; p = p->ai_next)
	{
		// Create socket
		if ((sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("server: socket()");
			continue;
		}

		// Set socket options to reuse address
		if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("server: setsocketopt()");
			close(sockFd);
			continue;
		}

		// Bind socket to address
		if (bind(sockFd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockFd);
			perror("server: bind()");
			continue;
		}

		break;
	}

	// No longer needed
	freeaddrinfo(myAddr);

	// Check if we successfully created and bound a socket
	if (p == NULL)
	{
		fprintf(stderr, "server: failed to start!\n");
		exit(EXIT_FAILURE);
	}

	// Start listening for incoming connections
	if (listen(sockFd, BACKLOG) == -1)
	{
		perror("server: listen()");
		close(sockFd);
		exit(EXIT_FAILURE);
	}

	// Install signal handler for child cleanup
	install_signals();

	printf("server: waiting for connections...\n");

	// Accept and handle client connections
	struct sockaddr_storage theirAddr;
	socklen_t addrLen = sizeof(theirAddr);
	int newSockFd;
	char theirIP[INET6_ADDRSTRLEN];
	while (true)
	{
		// Accept a new connection
		if ((newSockFd = accept(sockFd, (struct sockaddr *)&theirAddr, &addrLen)) == -1)
		{
			perror("server: accept()");
			continue;
		}

		// Get client IP address
		inet_ntop(theirAddr.ss_family, getinaddr((struct sockaddr *)&theirAddr), theirIP, sizeof(theirIP));
		printf("server: got connection from %s\n", theirIP);

		// Fork to handle client
		if (!fork())
		{
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
