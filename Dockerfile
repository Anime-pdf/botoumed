FROM gcc:14.3 AS builder

WORKDIR /build

RUN apt-get update && apt-get install -y \
    cmake \
    libopus-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . .
RUN cmake -B build -S . \
    && cmake --build build --config Release

FROM alpine:latest

WORKDIR /app
RUN apk add --no-cache ca-certificates opus
COPY --from=builder /build/build/botoumed /app/botoumed

RUN adduser -D botuser
USER botuser

CMD ["./botoumed"]
