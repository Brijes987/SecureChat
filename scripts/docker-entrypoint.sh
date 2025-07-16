#!/bin/bash
set -e

# SecureChat Docker Entrypoint Script
# Handles initialization, configuration, and graceful shutdown

# Default values
SECURECHAT_CONFIG_FILE="${SECURECHAT_CONFIG_FILE:-/etc/securechat/server.json}"
SECURECHAT_LOG_LEVEL="${SECURECHAT_LOG_LEVEL:-info}"
SECURECHAT_PORT="${SECURECHAT_PORT:-8080}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log() {
    echo -e "${BLUE}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} $1"
}

log_error() {
    echo -e "${RED}[$(date +'%Y-%m-%d %H:%M:%S')] ERROR:${NC} $1" >&2
}

log_warn() {
    echo -e "${YELLOW}[$(date +'%Y-%m-%d %H:%M:%S')] WARN:${NC} $1"
}

log_success() {
    echo -e "${GREEN}[$(date +'%Y-%m-%d %H:%M:%S')] SUCCESS:${NC} $1"
}

# Signal handlers for graceful shutdown
shutdown_handler() {
    log "Received shutdown signal, initiating graceful shutdown..."
    if [ ! -z "$SECURECHAT_PID" ]; then
        kill -TERM "$SECURECHAT_PID" 2>/dev/null || true
        wait "$SECURECHAT_PID" 2>/dev/null || true
    fi
    log_success "SecureChat server stopped gracefully"
    exit 0
}

# Trap signals
trap 'shutdown_handler' SIGTERM SIGINT SIGQUIT

# Print banner
print_banner() {
    cat << 'EOF'
   ____                           ____ _           _   
  / ___|  ___  ___ _   _ _ __ ___ / ___| |__   __ _| |_ 
  \___ \ / _ \/ __| | | | '__/ _ \ |   | '_ \ / _` | __|
   ___) |  __/ (__| |_| | | |  __/ |___| | | | (_| | |_ 
  |____/ \___|\___|\__,_|_|  \___|\____|_| |_|\__,_|\__|
                                                        
  Production-Ready Real-Time Chat Server
  Docker Container - Starting...
EOF
}

# Validate environment
validate_environment() {
    log "Validating environment..."
    
    # Check if running as non-root user
    if [ "$(id -u)" = "0" ]; then
        log_warn "Running as root user. Consider using a non-root user for security."
    fi
    
    # Check required directories
    local required_dirs=(
        "/var/log/securechat"
        "/var/lib/securechat"
        "/etc/securechat"
    )
    
    for dir in "${required_dirs[@]}"; do
        if [ ! -d "$dir" ]; then
            log_error "Required directory $dir does not exist"
            exit 1
        fi
    done
    
    # Check configuration file
    if [ ! -f "$SECURECHAT_CONFIG_FILE" ]; then
        log_error "Configuration file $SECURECHAT_CONFIG_FILE not found"
        exit 1
    fi
    
    log_success "Environment validation completed"
}

# Wait for dependencies
wait_for_dependencies() {
    log "Waiting for dependencies..."
    
    # Wait for PostgreSQL
    if [ ! -z "$SECURECHAT_DB_HOST" ]; then
        local db_host="${SECURECHAT_DB_HOST}"
        local db_port="${SECURECHAT_DB_PORT:-5432}"
        
        log "Waiting for PostgreSQL at $db_host:$db_port..."
        while ! nc -z "$db_host" "$db_port" 2>/dev/null; do
            sleep 1
        done
        log_success "PostgreSQL is ready"
    fi
    
    # Wait for Redis
    if [ ! -z "$SECURECHAT_REDIS_HOST" ]; then
        local redis_host="${SECURECHAT_REDIS_HOST}"
        local redis_port="${SECURECHAT_REDIS_PORT:-6379}"
        
        log "Waiting for Redis at $redis_host:$redis_port..."
        while ! nc -z "$redis_host" "$redis_port" 2>/dev/null; do
            sleep 1
        done
        log_success "Redis is ready"
    fi
}

# Initialize certificates
init_certificates() {
    log "Initializing TLS certificates..."
    
    local cert_dir="/var/lib/securechat/certs"
    local server_cert="$cert_dir/server.crt"
    local server_key="$cert_dir/server.key"
    
    # Check if certificates exist
    if [ ! -f "$server_cert" ] || [ ! -f "$server_key" ]; then
        log_warn "TLS certificates not found, generating self-signed certificates..."
        
        # Generate self-signed certificate for development
        openssl req -x509 -newkey rsa:2048 -keyout "$server_key" -out "$server_cert" \
            -days 365 -nodes -subj "/C=US/ST=State/L=City/O=SecureChat/CN=localhost" \
            2>/dev/null
        
        chmod 600 "$server_key"
        chmod 644 "$server_cert"
        
        log_warn "Self-signed certificates generated. Replace with proper certificates in production!"
    else
        log_success "TLS certificates found"
    fi
}

# Health check function
health_check() {
    local health_port="${SECURECHAT_HEALTH_PORT:-8081}"
    curl -f "http://localhost:$health_port/health" >/dev/null 2>&1
}

# Pre-flight checks
preflight_checks() {
    log "Running pre-flight checks..."
    
    # Check if securechat-server binary exists
    if [ ! -x "/usr/local/bin/securechat-server" ]; then
        log_error "SecureChat server binary not found or not executable"
        exit 1
    fi
    
    # Test configuration file syntax
    if ! /usr/local/bin/securechat-server --config "$SECURECHAT_CONFIG_FILE" --validate-config 2>/dev/null; then
        log_warn "Configuration validation failed, proceeding anyway..."
    fi
    
    log_success "Pre-flight checks completed"
}

# Main execution
main() {
    print_banner
    
    # Validate environment
    validate_environment
    
    # Wait for dependencies
    wait_for_dependencies
    
    # Initialize certificates
    init_certificates
    
    # Run pre-flight checks
    preflight_checks
    
    # Prepare command line arguments
    local args=(
        "--config" "$SECURECHAT_CONFIG_FILE"
        "--log-level" "$SECURECHAT_LOG_LEVEL"
    )
    
    # Add port override if specified
    if [ ! -z "$SECURECHAT_PORT" ] && [ "$SECURECHAT_PORT" != "8080" ]; then
        args+=("--port" "$SECURECHAT_PORT")
    fi
    
    # Add daemon mode for container
    args+=("--daemon")
    
    log "Starting SecureChat server with arguments: ${args[*]}"
    
    # Start the server
    /usr/local/bin/securechat-server "${args[@]}" &
    SECURECHAT_PID=$!
    
    # Wait a moment for startup
    sleep 2
    
    # Check if process is still running
    if ! kill -0 "$SECURECHAT_PID" 2>/dev/null; then
        log_error "SecureChat server failed to start"
        exit 1
    fi
    
    log_success "SecureChat server started successfully (PID: $SECURECHAT_PID)"
    
    # Wait for the process and handle signals
    wait "$SECURECHAT_PID"
}

# Handle different invocation modes
if [ "$1" = "securechat-server" ]; then
    # Standard server startup
    shift
    main "$@"
elif [ "$1" = "health-check" ]; then
    # Health check mode
    health_check
    exit $?
elif [ "$1" = "validate-config" ]; then
    # Configuration validation mode
    /usr/local/bin/securechat-server --config "$SECURECHAT_CONFIG_FILE" --validate-config
    exit $?
else
    # Pass through to original command
    exec "$@"
fi