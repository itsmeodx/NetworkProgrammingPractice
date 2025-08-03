/**
 * @file chatclient.c
 * @brief TCP chat client: connects to chat server and handles bidirectional communication.
 *
 * Usage: chatclient hostname [PORT]
 *   - If PORT is omitted, uses default 4242.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <poll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iso646.h>
#include <termios.h>

#define DEFAULT_PORT "4242"
#define BUFFER_SIZE 256

// Global variables for input line management
static char current_input[BUFFER_SIZE] = {0};
static int input_pos = 0;
static struct termios orig_termios;

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
	printf("\rYou: %.*s", input_pos, current_input);
	fflush(stdout);
}

/**
 * @brief Handle incoming messages from server
 */
void handleServerMessage(int sockFd)
{
	char buffer[BUFFER_SIZE];
	ssize_t bytesRead = recv(sockFd, buffer, sizeof(buffer), 0);

	if (bytesRead == 0)
	{
		printf("\nChatClient: server disconnected\n");
		disable_raw_mode();
		exit(EXIT_SUCCESS);
	}
	else if (bytesRead < 0)
	{
		perror("ChatClient: handleServerMessage: recv()");
		disable_raw_mode();
		exit(EXIT_FAILURE);
	}

	// Clear current input line, print message, then restore input
	printf("\r\033[2K%.*s", (int)bytesRead, buffer);
	redraw_input_line();
}

/**
 * @brief Handle user input character by character
 */
void handleUserInput(int sockFd)
{
	char c;
	ssize_t n = read(STDIN_FILENO, &c, 1);

	if (n <= 0)
	{
		printf("\nChatClient: disconnecting...\n");
		disable_raw_mode();
		exit(EXIT_SUCCESS);
	}

	if (c == '\n' || c == '\r')
	{
		// Send the message
		if (input_pos > 0)
		{
			current_input[input_pos] = '\n';
			if (send(sockFd, current_input, input_pos + 1, 0) == -1)
			{
				perror("ChatClient: handleUserInput: send()");
				disable_raw_mode();
				exit(EXIT_FAILURE);
			}
			// Clear input buffer
			input_pos = 0;
			memset(current_input, 0, sizeof(current_input));
		}
		printf("\nYou: ");
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

void polling(int sockFd)
{
	// Set up polling for both server socket and stdin
	struct pollfd fds[2];
	fds[0].fd = sockFd; // Server socket
	fds[0].events = POLLIN;
	fds[1].fd = STDIN_FILENO; // Standard input
	fds[1].events = POLLIN;

	// Main communication loop
	while (true)
	{
		int polls = poll(fds, 2, -1);

		if (polls == -1)
		{
			perror("ChatClient: polling: poll()");
			break;
		}

		// Check for incoming messages from server
		if (fds[0].revents & POLLIN)
		{
			handleServerMessage(sockFd);
		}

		// Check for user input
		if (fds[1].revents & POLLIN)
		{
			handleUserInput(sockFd);
		}

		// Check for connection errors
		if (fds[0].revents & (POLLHUP | POLLERR))
		{
			printf("\nChatClient: connection lost\n");
			break;
		}
	}
}

/**
 * @brief Main entry point. Connects to chat server and handles bidirectional communication.
 */
int main(int argc, char const *argv[])
{
	struct addrinfo hints, *serverAddr;
	const char *hostname, *port;

	// Parse arguments: hostname required, PORT optional
	if (argc < 2 or argc > 3)
	{
		fprintf(stderr, "Usage: chatclient hostname [PORT]\n");
		return (EXIT_FAILURE);
	}
	hostname = argv[1];
	if (argc == 3)
		port = argv[2];
	else
		port = DEFAULT_PORT;

	setbuf(stdout, NULL); // Disable buffering for stdout
	setbuf(stderr, NULL); // Disable buffering for stderr

	// Setup TCP socket hints
	hints = (struct addrinfo){0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// Resolve server address
	int rv;
	if ((rv = getaddrinfo(hostname, port, &hints, &serverAddr)) != 0)
	{
		fprintf(stderr, "ChatClient: getaddrinfo(): %s\n", gai_strerror(rv));
		return (EXIT_FAILURE);
	}

	// Create and connect TCP socket
	struct addrinfo *p;
	char serverIP[INET6_ADDRSTRLEN];
	int sockFd;
	for (p = serverAddr; p != NULL; p = p->ai_next)
	{
		// Create socket
		if ((sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("ChatClient: main: socket()");
			continue;
		}

		// Connect to server
		if (connect(sockFd, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("ChatClient: main: connect()");
			close(sockFd);
			continue;
		}

		break;
	}

	// Check if we successfully connected
	if (p == NULL)
	{
		fprintf(stderr, "ChatClient: failed to connect to %s:%s\n", hostname, port);
		freeaddrinfo(serverAddr);
		return (EXIT_FAILURE);
	}

	// Get server IP address for confirmation
	inet_ntop(p->ai_family, getinaddr((struct sockaddr *)p->ai_addr), serverIP, sizeof(serverIP));
	printf("ChatClient: connected to %s:%s\n", serverIP, port);
	printf("ChatClient: type your messages and press Enter to send\n");
	printf("ChatClient: press Ctrl+C or Ctrl+D to quit\n");
	printf("----------------------------------------\n");

	// Enable raw mode for character-by-character input
	enable_raw_mode();

	printf("You: ");
	fflush(stdout);

	freeaddrinfo(serverAddr);

	polling(sockFd);

	// Restore terminal mode before exit
	disable_raw_mode();
	close(sockFd);
	return (EXIT_SUCCESS);
}
