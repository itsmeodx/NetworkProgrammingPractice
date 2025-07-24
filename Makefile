CC = cc
CFLAGS = -Wall -Wextra -Werror

all: server client

server: server_dir/server.c
	$(CC) $(CFLAGS) -o server server_dir/server.c

client: client_dir/client.c
	$(CC) $(CFLAGS) -o client client_dir/client.c

clean:
	rm -f server client

re: clean all

.PHONY: clean
