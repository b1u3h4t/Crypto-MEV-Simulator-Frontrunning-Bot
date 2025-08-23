# MEV Frontrunning Bot - Dockerfile
# Multi-stage build for optimized production image

# Build stage
FROM ubuntu:22.04 AS builder

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV CMAKE_BUILD_TYPE=Release
ENV CXX=g++-11

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    wget \
    pkg-config \
    libssl-dev \
    libcurl4-openssl-dev \
    libboost-all-dev \
    libhiredis-dev \
    g++-11 \
    && rm -rf /var/lib/apt/lists/*

# Install Rust for Foundry
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# Install Foundry
RUN curl -L https://foundry.paradigm.xyz | bash
RUN /root/.foundry/bin/foundryup

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the application
RUN mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc)

# Production stage
FROM ubuntu:22.04 AS production

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libcurl4 \
    libssl3 \
    libboost-system1.74.0 \
    libhiredis0.14 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install Foundry runtime
RUN curl -L https://foundry.paradigm.xyz | bash
ENV PATH="/root/.foundry/bin:${PATH}"
RUN /root/.foundry/bin/foundryup

# Create non-root user
RUN useradd -m -s /bin/bash mevuser

# Set working directory
WORKDIR /app

# Copy built application from builder stage
COPY --from=builder /app/build/mev_sim_bot /app/mev_sim_bot
COPY --from=builder /app/config /app/config
COPY --from=builder /app/scripts /app/scripts
COPY --from=builder /app/env.example /app/env.example

# Create necessary directories
RUN mkdir -p logs data/tx_data data/results \
    && chown -R mevuser:mevuser /app

# Switch to non-root user
USER mevuser

# Expose metrics port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/metrics || exit 1

# Default command
ENTRYPOINT ["/app/mev_sim_bot"]

# Default arguments
CMD ["--mode", "realtime", "--strategies", "arbitrage,sandwich"]
