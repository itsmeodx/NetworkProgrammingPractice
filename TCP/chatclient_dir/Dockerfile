# Build stage using GCC
FROM gcc:latest AS builder

# Set working directory
WORKDIR /app

# Copy the chatclient source code
COPY chatclient.c .

# Compile the chatclient statically
RUN gcc -Wall -Wextra -Werror -static -o chatclient chatclient.c

# Runtime stage using Alpine
FROM alpine:latest

# Set working directory
WORKDIR /app

# Copy the compiled binary from build stage
COPY --from=builder /app/chatclient .

# Default command - can be overridden to specify server hostname
CMD ["./chatclient", "chatserver"]
