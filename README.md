


# NetworkProgrammingPractice

TCP and UDP networking in C. Chunked transfer, delimiter handling, and simple build/test with Makefile and Docker.

## Overview

- **TCP Server**: `server [MSG] [PORT]`
- **TCP Client**: `client hostname [PORT]`
- **TCP Chat Server**: `chatserver [PORT]`
- **TCP Chat Client**: `chatclient hostname [PORT]`
- **UDP Listener**: `listener [PORT]`
- **UDP Talker**: `talker hostname [MSG] [PORT]`

All binaries are built in the project root. See `Makefile` and `docker-compose.yml` for details.


## 🚀 Quick Start

### Local Development
```bash
make all          # Build all binaries (TCP and UDP)
make server       # Build TCP server only
make client       # Build TCP client only
make chatserver   # Build TCP chat server only
make chatclient   # Build TCP chat client only
make listener     # Build UDP listener only
make talker       # Build UDP talker only
make test-tcp     # Run TCP test (server+client)
make test-udp     # Run UDP test (listener+talker)
make test-chat    # Run chat test (chatserver+chatclient)
make clean        # Remove built binaries
make re           # Clean and rebuild all
```


### Docker
```bash
make docker-up    # Build and run all services in containers
make docker-logs  # View all container logs
make docker-down  # Stop all containers
```


## 🛠️ Build System

- Uses a single Makefile with explicit rules for each binary.
- Source files are in `TCP/server_dir`, `TCP/client_dir`, `TCP/chatserver_dir`, `TCP/chatclient_dir`, `UDP/listener_dir`, `UDP/talker_dir`.
- Binaries are built in the project root: `server`, `client`, `chatserver`, `chatclient`, `listener`, `talker`.
- `make debug` adds debug flags.

- Multi-stage builds for minimal images (Alpine runtime).
- All four services (TCP and UDP) are included in `docker-compose.yml` by default.
- Each service has its own Dockerfile in its source directory.
- No ports are mapped to the host by default; containers communicate on an internal Docker network.


## 🏗️ Project Structure

```
NetworkProgrammingPractice/
├── Makefile
├── docker-compose.yml
├── TCP/
│   ├── chatclient_dir/
│   ├── chatserver_dir/
│   ├── client_dir/
│   └── server_dir/
├── UDP/
│   ├── listener_dir/
│   └── talker_dir/
├── server
├── client
├── chatserver
├── chatclient
├── listener
└── talker
```


## 🔧 Technical Details

- **Language**: C with POSIX sockets
- **TCP**: Fork-based server, SIGCHLD handling, IPv4/IPv6 support
- **UDP**: Chunked datagram transfer, delimiter-based message boundaries
- **Compiler flags**: `-Wall -Wextra -Werror -O2` (plus `-g -DDEBUG` for debug)
- **Docker**: Multi-stage, static binaries, minimal images

## 📚 Usage Examples

### TCP
- Start server: `./server [MSG] [PORT]` (e.g. `./server "Hey!" 4242`).\
If message omitted, uses default "Hello from server!"; if port omitted, uses 4242.
- Start client: `./client hostname [PORT]` (e.g. `./client localhost 4242`).\
If port omitted, uses 4242.

### TCP Chat
- Start chat server: `./chatserver [PORT]` (e.g. `./chatserver 4242`).\
If port omitted, uses default 4242.
- Start chat client: `./chatclient hostname [PORT]` (e.g. `./chatclient localhost 4242`).\
If port omitted, uses 4242.

### UDP
- Start listener: `./listener [PORT]` (e.g. `./listener 4343`).\
If port omitted, uses default 4242.
- Start talker: `./talker hostname [MSG] [PORT]` (e.g. `./talker localhost "Hello world" 4343` or `echo "Hello world" | ./talker localhost`).\
If message omitted, reads from stdin; if port omitted, uses 4242.
## 📡 Protocol Details

- **TCP**: Server sends a null-terminated message to each client. Client prints until null terminator or connection closes.
- **UDP**: Talker sends message in MAXDSIZE chunks, then a single datagram of size 1 and value `\r` as delimiter. Listener prints all received data until it receives a datagram of size 1 and value `\r` (not just any datagram containing `\r`).


## 🆘 Help

Run `make help` for a full list of available targets and commands.

---

For more details, see the source files in the TCP and UDP directories.

---
Project maintained by itsmeodx. For questions, see code comments or open an issue.

---

<sub>© 2025 itsmeodx. Licensed under the GNU General Public License v3.0.</sub>

