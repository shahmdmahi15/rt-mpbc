# ==============================================================================
# RT-MPBC: Real-Time Multi-Physics Bond-Graph Compiler & Symplectic Solver
# Multi-stage Dockerfile for CLI execution and C/Rust runtime distribution
# ==============================================================================

# Stage 1: Build Rust Compiler CLI and verification suite
FROM rust:1.80-slim as rust-builder

WORKDIR /build
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

COPY Cargo.toml Cargo.lock README.md LICENSE ./
COPY compiler/ compiler/
COPY runtime-rust/ runtime-rust/

RUN cargo build --release --bin rt-mpbc
RUN cargo test --release

# Stage 2: Build C Runtime & Run Industrial Benchmarks
FROM debian:bookworm-slim as c-builder

WORKDIR /build
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    && rm -rf /var/lib/apt/lists/*

COPY runtime-c/ runtime-c/
COPY examples/ examples/
COPY Makefile CMakeLists.txt ./

RUN make all
RUN make test

# Stage 3: Minimal Production Image
FROM debian:bookworm-slim

LABEL org.opencontainers.image.title="RT-MPBC" \
      org.opencontainers.image.description="Real-Time Multi-Physics Bond-Graph Compiler & Symplectic Solver" \
      org.opencontainers.image.authors="shahmdmahi15" \
      org.opencontainers.image.source="https://github.com/shahmdmahi15/rt-mpbc" \
      org.opencontainers.image.version="1.1.0" \
      org.opencontainers.image.licenses="MIT"

WORKDIR /workspace

# Copy CLI binary
COPY --from=rust-builder /build/target/release/rt-mpbc /usr/local/bin/rt-mpbc

# Copy C runtime libraries and headers
COPY --from=c-builder /build/librtmpbc.a /usr/local/lib/librtmpbc.a
COPY --from=c-builder /build/librtmpbc.so /usr/local/lib/librtmpbc.so
COPY --from=c-builder /build/runtime-c/include/ /usr/local/include/rtmpbc/

RUN ldconfig

ENTRYPOINT ["rt-mpbc"]
CMD ["--help"]
