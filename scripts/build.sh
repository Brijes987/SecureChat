#!/bin/bash

# SecureChat Build Script
# Comprehensive build system with multiple configurations and optimizations

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
BUILD_DIR="build"
INSTALL_PREFIX="/usr/local"
ENABLE_TESTS="ON"
ENABLE_BENCHMARKS="OFF"
ENABLE_COVERAGE="OFF"
ENABLE_SANITIZERS="OFF"
ENABLE_LTO="ON"
ENABLE_STATIC="OFF"
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
VERBOSE="OFF"
CLEAN_BUILD="OFF"

# Logging functions
log() {
    echo -e "${BLUE}[$(date +'%H:%M:%S')]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[$(date +'%H:%M:%S')] ✓${NC} $1"
}

log_error() {
    echo -e "${RED}[$(date +'%H:%M:%S')] ✗${NC} $1" >&2
}

log_warn() {
    echo -e "${YELLOW}[$(date +'%H:%M:%S')] ⚠${NC} $1"
}

log_info() {
    echo -e "${CYAN}[$(date +'%H:%M:%S')] ℹ${NC} $1"
}

# Print banner
print_banner() {
    cat << 'EOF'
╔══════════════════════════════════════════════════════════════╗
║                    SecureChat Build System                   ║
║              High-Performance C++ Chat Server               ║
╚══════════════════════════════════════════════════════════════╝
EOF
}

# Print usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build Options:
  -t, --type TYPE          Build type (Debug|Release|RelWithDebInfo|MinSizeRel)
  -d, --dir DIR           Build directory (default: build)
  -p, --prefix PREFIX     Install prefix (default: /usr/local)
  -j, --jobs N            Number of parallel jobs (default: auto-detect)
  
Feature Options:
  --enable-tests          Enable unit tests (default: ON)
  --disable-tests         Disable unit tests
  --enable-benchmarks     Enable performance benchmarks
  --enable-coverage       Enable code coverage (Debug builds only)
  --enable-sanitizers     Enable AddressSanitizer and UBSan (Debug builds only)
  --enable-static         Build static binary
  --disable-lto           Disable Link Time Optimization
  
Build Options:
  -c, --clean             Clean build directory before building
  -v, --verbose           Verbose build output
  --install               Install after building
  --package               Create distribution package
  
Utility Options:
  --deps                  Install build dependencies
  --format                Format code with clang-format
  --lint                  Run static analysis
  --docker                Build Docker image
  -h, --help              Show this help message

Examples:
  $0                      # Standard release build
  $0 -t Debug --enable-coverage --enable-sanitizers
  $0 --clean --enable-benchmarks --install
  $0 --docker             # Build Docker image
EOF
}

# Check dependencies
check_dependencies() {
    log "Checking build dependencies..."
    
    local missing_deps=()
    
    # Check for required tools
    local required_tools=("cmake" "make" "pkg-config")
    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" >/dev/null 2>&1; then
            missing_deps+=("$tool")
        fi
    done
    
    # Check for compiler
    if ! command -v g++ >/dev/null 2>&1 && ! command -v clang++ >/dev/null 2>&1; then
        missing_deps+=("g++ or clang++")
    fi
    
    # Check for OpenSSL
    if ! pkg-config --exists openssl; then
        missing_deps+=("libssl-dev")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        log_error "Missing dependencies: ${missing_deps[*]}"
        log_info "Run '$0 --deps' to install dependencies"
        exit 1
    fi
    
    log_success "All dependencies satisfied"
}

# Install dependencies
install_dependencies() {
    log "Installing build dependencies..."
    
    if command -v apt-get >/dev/null 2>&1; then
        # Ubuntu/Debian
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            libssl-dev \
            libpq-dev \
            pkg-config \
            git \
            clang-format \
            clang-tidy \
            valgrind \
            lcov
    elif command -v yum >/dev/null 2>&1; then
        # CentOS/RHEL
        sudo yum groupinstall -y "Development Tools"
        sudo yum install -y \
            cmake \
            openssl-devel \
            postgresql-devel \
            pkgconfig \
            git \
            clang \
            valgrind
    elif command -v brew >/dev/null 2>&1; then
        # macOS
        brew install \
            cmake \
            openssl \
            postgresql \
            pkg-config \
            clang-format \
            llvm
    else
        log_error "Unsupported package manager. Please install dependencies manually."
        exit 1
    fi
    
    log_success "Dependencies installed successfully"
}

# Format code
format_code() {
    log "Formatting code with clang-format..."
    
    if ! command -v clang-format >/dev/null 2>&1; then
        log_error "clang-format not found"
        exit 1
    fi
    
    find src include tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
    
    log_success "Code formatting completed"
}

# Run static analysis
run_lint() {
    log "Running static analysis..."
    
    # clang-tidy
    if command -v clang-tidy >/dev/null 2>&1; then
        log "Running clang-tidy..."
        find src -name "*.cpp" -exec clang-tidy {} -- -Iinclude \;
    fi
    
    # cppcheck
    if command -v cppcheck >/dev/null 2>&1; then
        log "Running cppcheck..."
        cppcheck --enable=all --error-exitcode=1 --suppress=missingIncludeSystem src/ include/
    fi
    
    log_success "Static analysis completed"
}

# Configure build
configure_build() {
    log "Configuring build (Type: $BUILD_TYPE)..."
    
    # Create build directory
    if [ "$CLEAN_BUILD" = "ON" ] && [ -d "$BUILD_DIR" ]; then
        log "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
    fi
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Prepare CMake arguments
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
        "-DENABLE_TESTS=$ENABLE_TESTS"
        "-DENABLE_BENCHMARKS=$ENABLE_BENCHMARKS"
        "-DENABLE_COVERAGE=$ENABLE_COVERAGE"
        "-DENABLE_SANITIZERS=$ENABLE_SANITIZERS"
        "-DENABLE_LTO=$ENABLE_LTO"
        "-DENABLE_STATIC=$ENABLE_STATIC"
    )
    
    # Add compiler-specific optimizations
    if [ "$BUILD_TYPE" = "Release" ]; then
        cmake_args+=("-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG -march=native")
    fi
    
    # Run CMake
    cmake "${cmake_args[@]}" ..
    
    log_success "Build configured successfully"
}

# Build project
build_project() {
    log "Building project with $PARALLEL_JOBS parallel jobs..."
    
    local make_args=("-j$PARALLEL_JOBS")
    
    if [ "$VERBOSE" = "ON" ]; then
        make_args+=("VERBOSE=1")
    fi
    
    # Build
    make "${make_args[@]}"
    
    log_success "Build completed successfully"
}

# Run tests
run_tests() {
    if [ "$ENABLE_TESTS" = "ON" ]; then
        log "Running tests..."
        
        # Run unit tests
        ctest --output-on-failure --parallel "$PARALLEL_JOBS"
        
        # Generate coverage report if enabled
        if [ "$ENABLE_COVERAGE" = "ON" ] && [ "$BUILD_TYPE" = "Debug" ]; then
            log "Generating coverage report..."
            lcov --capture --directory . --output-file coverage.info
            lcov --remove coverage.info '/usr/*' --output-file coverage.info
            lcov --list coverage.info
            
            # Generate HTML report
            if command -v genhtml >/dev/null 2>&1; then
                genhtml coverage.info --output-directory coverage_html
                log_success "Coverage report generated in coverage_html/"
            fi
        fi
        
        log_success "All tests passed"
    fi
}

# Install project
install_project() {
    log "Installing project to $INSTALL_PREFIX..."
    
    make install
    
    log_success "Installation completed"
}

# Create package
create_package() {
    log "Creating distribution package..."
    
    make package
    
    log_success "Package created successfully"
}

# Build Docker image
build_docker() {
    log "Building Docker image..."
    
    cd ..
    docker build -t securechat:latest .
    
    log_success "Docker image built successfully"
}

# Performance benchmark
run_benchmarks() {
    if [ "$ENABLE_BENCHMARKS" = "ON" ]; then
        log "Running performance benchmarks..."
        
        ./bin/benchmark_crypto
        ./bin/benchmark_networking
        
        log_success "Benchmarks completed"
    fi
}

# Main build function
main_build() {
    print_banner
    
    log_info "Build configuration:"
    log_info "  Build Type: $BUILD_TYPE"
    log_info "  Build Directory: $BUILD_DIR"
    log_info "  Install Prefix: $INSTALL_PREFIX"
    log_info "  Parallel Jobs: $PARALLEL_JOBS"
    log_info "  Tests: $ENABLE_TESTS"
    log_info "  Benchmarks: $ENABLE_BENCHMARKS"
    log_info "  Coverage: $ENABLE_COVERAGE"
    log_info "  Sanitizers: $ENABLE_SANITIZERS"
    log_info "  LTO: $ENABLE_LTO"
    log_info "  Static: $ENABLE_STATIC"
    
    check_dependencies
    configure_build
    build_project
    run_tests
    run_benchmarks
    
    log_success "Build process completed successfully!"
    
    # Print binary information
    if [ -f "bin/securechat-server" ]; then
        log_info "Binary size: $(du -h bin/securechat-server | cut -f1)"
        log_info "Binary location: $(pwd)/bin/securechat-server"
    fi
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -d|--dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -p|--prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        --enable-tests)
            ENABLE_TESTS="ON"
            shift
            ;;
        --disable-tests)
            ENABLE_TESTS="OFF"
            shift
            ;;
        --enable-benchmarks)
            ENABLE_BENCHMARKS="ON"
            shift
            ;;
        --enable-coverage)
            ENABLE_COVERAGE="ON"
            shift
            ;;
        --enable-sanitizers)
            ENABLE_SANITIZERS="ON"
            shift
            ;;
        --enable-static)
            ENABLE_STATIC="ON"
            shift
            ;;
        --disable-lto)
            ENABLE_LTO="OFF"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD="ON"
            shift
            ;;
        -v|--verbose)
            VERBOSE="ON"
            shift
            ;;
        --install)
            INSTALL_AFTER_BUILD="ON"
            shift
            ;;
        --package)
            PACKAGE_AFTER_BUILD="ON"
            shift
            ;;
        --deps)
            install_dependencies
            exit 0
            ;;
        --format)
            format_code
            exit 0
            ;;
        --lint)
            run_lint
            exit 0
            ;;
        --docker)
            build_docker
            exit 0
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Validate build type
case $BUILD_TYPE in
    Debug|Release|RelWithDebInfo|MinSizeRel)
        ;;
    *)
        log_error "Invalid build type: $BUILD_TYPE"
        exit 1
        ;;
esac

# Run main build
main_build

# Post-build actions
if [ "$INSTALL_AFTER_BUILD" = "ON" ]; then
    install_project
fi

if [ "$PACKAGE_AFTER_BUILD" = "ON" ]; then
    create_package
fi