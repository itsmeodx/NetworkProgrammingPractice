# Build stage using GCC
FROM gcc:latest AS builder

# Set working directory
WORKDIR /app

# Copy the server source code
COPY server.c .

# Compile the server statically
RUN gcc -Wall -Wextra -Werror -static -o server server.c

# Runtime stage using Alpine
FROM alpine:latest

# Set working directory
WORKDIR /app

# Copy the compiled binary from build stage
COPY --from=builder /app/server .

# Expose the default port
EXPOSE 4242

# Run the server
CMD ["./server"]
