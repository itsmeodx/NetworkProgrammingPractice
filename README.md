# NetworkProgrammingPractice (NPP)

A simple C network programming project demonstrating TCP client-server communication.

## 📋 Overview

- **Server**: Multi-threaded TCP server that listens on port 4242 and sends "Hello World!" to connecting clients
- **Client**: TCP client that connects to the server and displays received messages
- **Features**: Fork-based concurrency, signal handling, and Docker containerization

## 🚀 Quick Start

### Local Development
```bash
make all          # Build both server and client
make test         # Run automated test
make run-server   # Start server (Terminal 1)
make run-client   # Connect client (Terminal 2)
```

### Docker (Recommended)
```bash
make docker-up    # Run in containers
make docker-logs  # View communication logs
make docker-down  # Stop when done
```

## 🛠️ Build Commands

### Build Targets
```bash
make all          # Build both server and client
make server       # Build server only
make client       # Build client only
make debug        # Build with debug symbols and flags
make clean        # Remove built binaries
make re           # Clean and rebuild all
```

### Development
```bash
make run-server   # Build and run server (port 4242)
make run-client   # Build and run client (connects to localhost)
make test         # Run basic functionality test
make help         # Show all available commands
```

## 🐳 Docker Commands

### Essential
```bash
make docker-up    # Run containers in background
make docker-down  # Stop all containers
make docker-logs  # Follow all container logs
```

### Advanced
```bash
make docker-build    # Build Docker images from scratch
make docker-run      # Run containers in foreground
make docker-restart  # Restart all containers
make docker-clean    # Stop and remove containers, images, volumes
```

## 🏗️ Architecture

### Docker Setup
- **Multi-stage build**: GCC compilation → Alpine runtime
- **Image size**: ~9.5MB (vs ~1.45GB with full GCC)
- **Network**: Custom bridge network with internal DNS
- **Static linking**: Portable binaries for minimal containers

### Network Flow
```
Host Machine (port 4242)
    ↓
Server Container (Alpine ~9.5MB)
    ↓ (internal network)
Client Container (Alpine ~9.5MB)
```

### Build Process
```
Stage 1: GCC Container
├── Compile with -static flag
├── Include all dependencies
└── Generate portable binary

Stage 2: Alpine Container
├── Copy binary from build stage
├── No build tools or libraries
└── Minimal runtime environment
```

## 🔧 Technical Details

- **Language**: C with POSIX sockets
- **Concurrency**: Fork-based server (one process per client)
- **Signal handling**: SIGCHLD cleanup for zombie processes
- **Compiler flags**: `-Wall -Wextra -Werror -O2` for optimized builds
- **Network**: IPv4/IPv6 compatible with `getaddrinfo()`
