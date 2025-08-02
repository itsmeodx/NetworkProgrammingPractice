#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iso646.h>

#define DEFAULT_PORT "4242"
#define DEFAULT_MSG "Hello from ChatServer!"
#define BACKLOG 10

void addNewConnection(int serverFd, struct pollfd **fds, int *fds_count, int *fds_capacity)
{
	struct sockaddr_storage clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	int newFd = accept(serverFd, (struct sockaddr *)&clientAddr, &addrLen);
	if (newFd == -1)
	{
		perror("ChatServer: accept()");
		return;
	}

	if (*fds_count >= *fds_capacity)
	{
		*fds_capacity *= 2;
		*fds = realloc(*fds, *fds_capacity * sizeof(struct pollfd));
		if (*fds == NULL)
		{
			perror("ChatServer: realloc()");
			close(newFd);
			return;
		}
	}

	(*fds)[*fds_count].fd = newFd;
	(*fds)[*fds_count].events = POLLIN;
	(*fds_count)++;
	printf("ChatServer: new connection accepted\n");
}

void handleClientMessage(int serverFd, struct pollfd **fds,
						 int *fds_count, int *fds_capacity, int *fd_i)
{
	char buffer[256];
	int clientFd = (*fds)[*fd_i].fd;
	ssize_t bytesRead = read(clientFd, buffer, sizeof(buffer));
	if (bytesRead <= 0)
	{
		if (bytesRead == 0)
			printf("ChatServer: client with fd %d disconnected\n", clientFd);
		else
			perror("ChatServer: read()");
		close(clientFd);
		for (int i = 0; i < *fds_count; i++)
		{
			if ((*fds)[i].fd == clientFd)
			{
				// Remove the client from the fds array
				(*fds)[i] = (*fds)[--(*fds_count)];
				break;
			}
		}
		if (*fds_count < *fds_capacity / 4)
		{
			*fds_capacity /= 2;
			*fds = realloc(*fds, *fds_capacity * sizeof(struct pollfd));
			if (*fds == NULL)
			{
				perror("ChatServer: realloc()");
				return;
			}
		}
		return;
	}

	char msgPrefix[32];
	snprintf(msgPrefix, sizeof(msgPrefix), "Client %d: ", clientFd);
	printf("%s%s", msgPrefix, buffer);
	// Send to all Connections except the sender and the server socket
	for (int i = 0; i < *fds_count; i++)
	{
		if ((*fds)[i].fd != clientFd && (*fds)[i].fd != serverFd)
		{
			if (send((*fds)[i].fd, msgPrefix, strlen(msgPrefix), 0) == -1 ||
				send((*fds)[i].fd, buffer, bytesRead, 0) == -1)
				perror("ChatServer: send()");
		}
	}
}

void polling(int serverFd, struct pollfd **fds, int *fds_count, int *fds_capacity, int polls)
{
	int polled = 0;

	for (int i = 0; i < *fds_count && polled < polls; i++)
	{
		if ((*fds)[i].revents & (POLLIN | POLLHUP))
		{
			if ((*fds)[i].fd == serverFd)
				addNewConnection(serverFd, fds, fds_count, fds_capacity);
			else
				handleClientMessage(serverFd, fds, fds_count, fds_capacity, &i);
			polled++;
		}
	}
}

int main(void)
{
	struct addrinfo hints, *serverAddr;
	const char *port = DEFAULT_PORT;

	hints = (struct addrinfo){0};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// Resolve local address
	int rv;
	if ((rv = getaddrinfo(NULL, port, &hints, &serverAddr)) != 0)
	{
		fprintf(stderr, "ChatServer: getaddrinfo(): %s\n",
				gai_strerror(rv));
		return (EXIT_FAILURE);
	}

	// Create and bind TCP socket
	int serverFd, yes = 1;
	struct addrinfo *p;
	for (p = serverAddr; p != NULL; p = p->ai_next)
	{
		// Create socket
		if ((serverFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("ChatServer: socket()");
			continue;
		}

		// Set socket options
		if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("ChatServer: setsockopt()");
			close(serverFd);
			continue;
		}

		// Bind socket
		if (bind(serverFd, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("server: bind()");
			close(serverFd);
			continue;
		}

		break; // Successfully bound
	}

	freeaddrinfo(serverAddr);

	if (p == NULL)
	{
		fprintf(stderr, "ChatServer: failed to bind\n");
		return (EXIT_FAILURE);
	}

	// Listen for incoming connections
	if (listen(serverFd, BACKLOG) == -1)
	{
		perror("ChatServer: listen()");
		close(serverFd);
		return (EXIT_FAILURE);
	}

	printf("ChatServer: listening on port %s\n", port);

	int fds_capacity = 10, fds_count = 0;
	struct pollfd *fds = malloc(fds_capacity * sizeof(struct pollfd));
	if (fds == NULL)
	{
		perror("ChatServer: malloc()");
		close(serverFd);
		return (EXIT_FAILURE);
	}

	fds[0].fd = serverFd;
	fds[0].events = POLLIN;
	fds_count++;

	printf("ChatServer: waiting for connections...\n");

	int polls;
	while (true)
	{
		polls = poll(fds, fds_count, -1);

		if (polls == -1)
		{
			perror("ChatServer: poll()");
			return (EXIT_FAILURE);
		}

		polling(serverFd, &fds, &fds_count, &fds_capacity, polls);
	}

	return (EXIT_SUCCESS);
}
