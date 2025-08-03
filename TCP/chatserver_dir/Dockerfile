# Build stage using GCC
FROM gcc:latest AS builder

# Set working directory
WORKDIR /app

# Copy the chatserver source code
COPY chatserver_dir.c .

# Compile the chatserver statically
RUN gcc -Wall -Wextra -Werror -static -o chatserver chatserver_dir.c

# Runtime stage using Alpine
FROM alpine:latest

# Set working directory
WORKDIR /app

# Copy the compiled binary from build stage
COPY --from=builder /app/chatserver .

# Expose the default port
EXPOSE 4242

# Run the chatserver
CMD ["./chatserver"]
