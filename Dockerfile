# Multi-stage build for optimal image size
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libpq-dev \
    pkg-config \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Create build directory and compile
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. && \
    make -j$(nproc) && \
    make install

# Production stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    libpq5 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && groupadd -r securechat \
    && useradd -r -g securechat -s /bin/false securechat

# Copy binary and configuration
COPY --from=builder /usr/local/bin/securechat-server /usr/local/bin/
COPY --from=builder /usr/local/etc/securechat /etc/securechat
COPY --from=builder /app/scripts/docker-entrypoint.sh /usr/local/bin/

# Create necessary directories
RUN mkdir -p /var/log/securechat /var/lib/securechat/certs /var/lib/securechat/plugins \
    && chown -R securechat:securechat /var/log/securechat /var/lib/securechat \
    && chmod +x /usr/local/bin/docker-entrypoint.sh

# Copy default certificates (for development)
COPY certs/ /var/lib/securechat/certs/

# Expose ports
EXPOSE 8080 8081 9090

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8081/health || exit 1

# Switch to non-root user
USER securechat

# Set entrypoint
ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
CMD ["securechat-server", "--config", "/etc/securechat/server.json"]