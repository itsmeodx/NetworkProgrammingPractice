# Build stage using GCC
FROM gcc:latest AS builder

# Set working directory
WORKDIR /app

# Copy the client source code
COPY client.c .

# Compile the client statically
RUN gcc -Wall -Wextra -Werror -static -o client client.c

# Runtime stage using Alpine
FROM alpine:latest

# Set working directory
WORKDIR /app

# Copy the compiled binary from build stage
COPY --from=builder /app/client .

# Default command - can be overridden to specify server hostname
CMD ["./client", "server"]
