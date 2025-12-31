FROM gcc:14.3 AS builder

WORKDIR /build

RUN apt-get update && apt-get install -y \
    cmake \
    && rm -rf /var/lib/apt/lists/*

COPY . .
RUN cmake -B build -S . \
    && cmake --build build --config Release

FROM alpine:latest

WORKDIR /app
RUN apk add --no-cache ca-certificates opus
COPY --from=builder /build/build/discord_bot /app/discord_bot

RUN adduser -D botuser
USER botuser

CMD ["./discord_bot"]
