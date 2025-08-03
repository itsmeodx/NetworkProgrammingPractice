
# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -Werror -O2
DEBUG_FLAGS := -g -DDEBUG

# Directories
TCP_DIR := TCP
UDP_DIR := UDP

# Source files
TCP_SRCS := $(TCP_DIR)/server_dir/server.c $(TCP_DIR)/client_dir/client.c $(TCP_DIR)/chatserver_dir/chatserver.c $(TCP_DIR)/chatclient_dir/chatclient.c
UDP_SRCS := $(UDP_DIR)/listener_dir/listener.c $(UDP_DIR)/talker_dir/talker.c

# Binaries
TCP_BINS := server client chatserver chatclient
UDP_BINS := listener talker
BINS := $(TCP_BINS) $(UDP_BINS)

all: $(BINS)

TCP: $(TCP_BINS)

UDP: $(UDP_BINS)

server: TCP/server_dir/server.c
	$(CC) $(CFLAGS) -o $@ $<

client: TCP/client_dir/client.c
	$(CC) $(CFLAGS) -o $@ $<

chatserver: TCP/chatserver_dir/chatserver.c
	$(CC) $(CFLAGS) -o $@ $<

chatclient: TCP/chatclient_dir/chatclient.c
	$(CC) $(CFLAGS) -o $@ $<

listener: UDP/listener_dir/listener.c
	$(CC) $(CFLAGS) -o $@ $<

talker: UDP/talker_dir/talker.c
	$(CC) $(CFLAGS) -o $@ $<

debug: CFLAGS += $(DEBUG_FLAGS)
debug: server client chatserver chatclient

clean:
	rm -f $(BINS)

re: clean all

test-tcp: TCP
	@echo "Testing TCP..."
	@./server "Hello TCP!" & sleep 1; ./client localhost; kill $$!

test-udp: UDP
	@echo "Testing UDP..."
	@./listener & sleep 1; echo "Hello UDP!" | ./talker localhost; kill $$!;

test-chat: chatserver chatclient
	@echo "Testing Chat..."
	@echo "Start the chat server with: ./chatserver"
	@echo "Then connect clients with: ./chatclient localhost"

# Docker commands
docker-build:
	docker-compose build --no-cache

docker-up:
	docker-compose up -d --build

docker-run:
	docker-compose up --build

docker-down:
	docker-compose down -t 0

docker-restart: docker-down docker-up

docker-logs:
	docker-compose logs -f

docker-clean:
	docker-compose down -t 0 --rmi all --volumes --remove-orphans

help:
	@echo "Build targets:"
	@echo "  all                    - Build all binaries (TCP and UDP)"
	@echo "  TCP                    - Build TCP programs (server, client, chatserver, chatclient)"
	@echo "  UDP                    - Build UDP programs (listener, talker)"
	@echo "  server                 - Build TCP server only"
	@echo "  client                 - Build TCP client only"
	@echo "  chatserver             - Build TCP chat server only"
	@echo "  chatclient             - Build TCP chat client only"
	@echo "  listener               - Build UDP listener only"
	@echo "  talker                 - Build UDP talker only"
	@echo "  debug                  - Build with debug symbols and flags"
	@echo "  clean                  - Remove built binaries"
	@echo "  re                     - Clean and rebuild all"
	@echo ""
	@echo "Testing:"
	@echo "  test-tcp               - Run basic TCP functionality test"
	@echo "  test-udp               - Run basic UDP functionality test"
	@echo "  test-chat              - Show instructions for chat testing"
	@echo ""
	@echo "Docker:"
	@echo "  docker-build           - Build Docker images from scratch"
	@echo "  docker-up              - Run containers in background"
	@echo "  docker-run             - Run containers in foreground"
	@echo "  docker-down            - Stop all containers"
	@echo "  docker-logs            - Follow all container logs"
	@echo "  docker-restart         - Restart all containers"
	@echo "  docker-clean           - Stop and remove containers, images, volumes"

.PHONY: all clean re debug TCP UDP run-server run-client test help docker-build docker-up docker-run docker-down docker-restart docker-logs docker-logs-server docker-logs-client docker-clean docker-prune
