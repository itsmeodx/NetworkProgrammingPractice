# Build stage using GCC
FROM gcc:latest AS builder

WORKDIR /app

COPY talker.c .

RUN gcc -Wall -Wextra -Werror -static -o talker talker.c

# Runtime stage using Alpine
FROM alpine:latest

WORKDIR /app

COPY --from=builder /app/talker .

CMD ["./talker", "listener"]
