CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2
DEBUG_FLAGS = -g -DDEBUG

all: server client listener talker

server: server_dir/server.c
	$(CC) $(CFLAGS) -o server server_dir/server.c

client: client_dir/client.c
	$(CC) $(CFLAGS) -o client client_dir/client.c

listener: listener_dir/listener.c
	$(CC) $(CFLAGS) -o listener listener_dir/listener.c

talker: talker_dir/talker.c
	$(CC) $(CFLAGS) -o talker talker_dir/talker.c

debug: CFLAGS += $(DEBUG_FLAGS)
debug: server client

clean:
	rm -f server client listener talker

re: clean all

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

# Development helpers
run-server: server
	./server

run-client: client
	./client localhost

test: all
	@echo "Testing..."
	@./server & sleep 1; ./client localhost; kill $$!

help:
	@echo "Build targets:"
	@echo "  all                    - Build both server and client"
	@echo "  server                 - Build server only"
	@echo "  client                 - Build client only"
	@echo "  debug                  - Build with debug symbols and flags"
	@echo "  clean                  - Remove built binaries"
	@echo "  re                     - Clean and rebuild all"
	@echo ""
	@echo "Development:"
	@echo "  run-server             - Build and run server (port 4242)"
	@echo "  run-client             - Build and run client (connects to localhost)"
	@echo "  test                   - Run basic functionality test"
	@echo ""
	@echo "Docker:"
	@echo "  docker-build           - Build Docker images from scratch"
	@echo "  docker-up              - Run containers in background"
	@echo "  docker-run             - Run containers in foreground"
	@echo "  docker-down            - Stop all containers"
	@echo "  docker-logs            - Follow all container logs"
	@echo "  docker-restart         - Restart all containers"
	@echo "  docker-clean           - Stop and remove containers, images, volumes"

.PHONY: all clean re debug run-server run-client test help docker-build docker-up docker-run docker-down docker-restart docker-logs docker-logs-server docker-logs-client docker-clean docker-prune
