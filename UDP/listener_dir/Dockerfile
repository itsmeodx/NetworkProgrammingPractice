# Build stage using GCC
FROM gcc:latest AS builder

WORKDIR /app

COPY listener.c .

RUN gcc -Wall -Wextra -Werror -static -o listener listener.c

# Runtime stage using Alpine
FROM alpine:latest

WORKDIR /app

COPY --from=builder /app/listener .

CMD ["./listener"]
