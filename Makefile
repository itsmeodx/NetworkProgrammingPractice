CC = cc
CFLAGS = -Wall -Wextra #-Werror
SOURCES = server.c

all: server client

server: $(SOURCES)
	$(CC) $(CFLAGS) -o server $(SOURCES)

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client

re: clean all

.PHONY: clean
