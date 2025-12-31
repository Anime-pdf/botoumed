FROM debian:trixie-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++-14 \
    cmake \
    ninja-build \
    libopus-dev \
    zlib1g-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

ENV CC=gcc-14
ENV CXX=g++-14
RUN cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -B build . \
    && cmake --build build --config Release

RUN mv build/botoumed /usr/bin/botoumed

FROM debian:trixie-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libopus0 \
    zlib1g \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/libs/dpp/linux/lib64/libdpp.so* /usr/lib/
RUN ldconfig

COPY --from=builder /usr/bin/botoumed /usr/bin
RUN chmod +x /usr/bin/botoumed

CMD ["botoumed"]
