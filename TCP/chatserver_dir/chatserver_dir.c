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
#include <termios.h>

#define DEFAULT_PORT "4242"
#define DEFAULT_MSG "Hello from ChatServer!"
#define BACKLOG 10
#define BUFFER_SIZE 256

// Global variables for input line management
static char current_input[BUFFER_SIZE] = {0};
static int input_pos = 0;
static struct termios orig_termios;

/**
 * @brief Enable raw mode for terminal input
 */
void enable_raw_mode(void)
{
	tcgetattr(STDIN_FILENO, &orig_termios);
	struct termios raw = orig_termios;
	raw.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/**
 * @brief Restore terminal to original mode
 */
void disable_raw_mode(void)
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/**
 * @brief Redraw the current input line
 */
void redraw_input_line(void)
{
	printf("\rServer: %.*s", input_pos, current_input);
	fflush(stdout);
}

void addNewConnection(int serverFd, struct pollfd **fds, int *fds_count, int *fds_capacity)
{
	struct sockaddr_storage clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	int newFd = accept(serverFd, (struct sockaddr *)&clientAddr, &addrLen);
	if (newFd == -1)
	{
		perror("ChatServer: addNewConnection: accept()");
		return;
	}

	if (*fds_count >= *fds_capacity)
	{
		*fds_capacity *= 2;
		*fds = realloc(*fds, *fds_capacity * sizeof(struct pollfd));
		if (*fds == NULL)
		{
			perror("ChatServer: addNewConnection: realloc()");
			close(newFd);
			exit(EXIT_FAILURE);
		}
	}

	(*fds)[*fds_count].fd = newFd;
	(*fds)[*fds_count].events = POLLIN;
	(*fds_count)++;
	printf("ChatServer: new connection accepted\n");
}

/**
 * @brief Handle server input character by character
 */
void handleServerInput(int serverFd, struct pollfd **fds, int *fds_count)
{
	char c;
	ssize_t n = read(STDIN_FILENO, &c, 1);

	if (n <= 0)
	{
		printf("\nChatServer: shutting down...\n");
		disable_raw_mode();
		exit(EXIT_SUCCESS);
	}

	if (c == '\n' || c == '\r')
	{
		// Send the message
		if (input_pos > 0)
		{
			// Check for server commands
			if (strncmp(current_input, "exit", 4) == 0 || strncmp(current_input, "quit", 4) == 0)
			{
				printf("\nChatServer: shutting down...\n");
				disable_raw_mode();
				exit(EXIT_SUCCESS);
			}

			// Send message to all clients
			char message[BUFFER_SIZE + 15];
			current_input[input_pos] = '\n';
			snprintf(message, sizeof(message), "Server: %.*s", input_pos + 1, current_input);

			for (int i = 0; i < *fds_count; i++)
			{
				if ((*fds)[i].fd != serverFd && (*fds)[i].fd != STDIN_FILENO)
				{
					if (send((*fds)[i].fd, message, strlen(message), 0) == -1)
						perror("ChatServer: handleServerInput: send()");
				}
			}

			// Clear input buffer
			input_pos = 0;
			memset(current_input, 0, sizeof(current_input));
		}
		printf("\nServer: ");
		fflush(stdout);
	}
	else if (c == 127 || c == '\b')  // Backspace
	{
		if (input_pos > 0)
		{
			input_pos--;
			current_input[input_pos] = '\0';
			redraw_input_line();
			printf(" \b");  // Clear the deleted character
			fflush(stdout);
		}
	}
	else if (c >= 32 && c <= 126 && input_pos < BUFFER_SIZE - 2)  // Printable characters
	{
		current_input[input_pos++] = c;
		printf("%c", c);
		fflush(stdout);
	}
}

void handleClientMessage(int serverFd, struct pollfd **fds,
						 int *fds_count, int *fds_capacity, int *fd_i)
{
	char buffer[BUFFER_SIZE];
	int clientFd = (*fds)[*fd_i].fd;
	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytesRead <= 0)
	{
		if (bytesRead == 0)
			printf("\nChatServer: client with fd %d disconnected\n", clientFd);
		else
			perror("ChatServer: handleClientMessage: recv()");
		close(clientFd);
		// Remove the client from the fds array by replacing it with the last one
		(*fds)[*fd_i] = (*fds)[--(*fds_count)];
		if (*fds_count < *fds_capacity / 4)
		{
			*fds_capacity /= 2;
			*fds = realloc(*fds, *fds_capacity * sizeof(struct pollfd));
			if (*fds == NULL)
			{
				perror("ChatServer: handleClientMessage: realloc()");
				return;
			}
		}
		printf("Server: ");
		fflush(stdout);
		return;
	}

	char message[BUFFER_SIZE + 15];
	sprintf(message, "Client %d: %.*s", clientFd, (int)bytesRead, buffer);
	printf("\r\033[2K%s", message);
	redraw_input_line();
	// Send to all Connections except sender/server socket, and STDIN
	for (int i = 0; i < *fds_count; i++)
	{
		if ((*fds)[i].fd != clientFd && (*fds)[i].fd != serverFd && (*fds)[i].fd != STDIN_FILENO)
		{
			if (send((*fds)[i].fd, message, strlen(message), 0) == -1)
				perror("ChatServer: handleClientMessage: send()");
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
			else if ((*fds)[i].fd == STDIN_FILENO)
				handleServerInput(serverFd, fds, fds_count);
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

	setbuf(stdout, NULL); // Disable buffering for stdout
	setbuf(stderr, NULL); // Disable buffering for stderr

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
			perror("ChatServer: main: socket()");
			continue;
		}

		// Set socket options
		if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("ChatServer: main: setsockopt()");
			close(serverFd);
			continue;
		}

		// Bind socket
		if (bind(serverFd, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("ChatServer: main: bind()");
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
		perror("ChatServer: main: listen()");
		close(serverFd);
		return (EXIT_FAILURE);
	}

	printf("ChatServer: listening on port %s\n", port);

	// Enable raw mode for character-by-character input
	enable_raw_mode();

	int fds_capacity = 10, fds_count = 0;
	struct pollfd *fds = malloc(fds_capacity * sizeof(struct pollfd));
	if (fds == NULL)
	{
		perror("ChatServer: main: malloc()");
		close(serverFd);
		return (EXIT_FAILURE);
	}

	// Add server socket
	fds[0].fd = serverFd;
	fds[0].events = POLLIN;
	fds_count++;

	// Add standard input
	fds[1].fd = STDIN_FILENO;
	fds[1].events = POLLIN;
	fds_count++;

	printf("ChatServer: waiting for connections...\n");
	printf("ChatServer: type messages to broadcast, 'exit' or 'quit' to shutdown\n");
	printf("Server: ");
	fflush(stdout);

	int polls;
	while (true)
	{
		polls = poll(fds, fds_count, -1);

		if (polls == -1)
		{
			perror("ChatServer: main: poll()");
			disable_raw_mode();
			return (EXIT_FAILURE);
		}

		polling(serverFd, &fds, &fds_count, &fds_capacity, polls);
	}

	// This should never be reached, but just in case
	disable_raw_mode();
	return (EXIT_SUCCESS);
}
