#!/bin/bash

# SecureChat Qt Client Build Script
# Comprehensive build system for the Qt GUI client

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
QT_VERSION="6"
ENABLE_TESTS="OFF"
CLEAN_BUILD="OFF"
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
VERBOSE="OFF"

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
║                SecureChat Qt Client Builder                  ║
║              Modern C++ GUI Chat Application                 ║
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
  -q, --qt-version VER    Qt version (5|6, default: 6)
  -j, --jobs N            Number of parallel jobs (default: auto-detect)
  
Feature Options:
  --enable-tests          Enable unit tests
  --disable-tests         Disable unit tests (default)
  
Build Options:
  -c, --clean             Clean build directory before building
  -v, --verbose           Verbose build output
  --install               Install after building
  --package               Create distribution package
  
Utility Options:
  --deps                  Install build dependencies
  --check-qt              Check Qt installation
  -h, --help              Show this help message

Examples:
  $0                      # Standard release build
  $0 -t Debug --enable-tests
  $0 --clean --install
  $0 --check-qt           # Check Qt installation
EOF
}

# Check Qt installation
check_qt_installation() {
    log "Checking Qt installation..."
    
    local qt_found=false
    local qt_version_found=""
    
    # Check for Qt6 first
    if command -v qmake6 >/dev/null 2>&1; then
        qt_version_found="6"
        qt_found=true
        log_success "Found Qt6: $(qmake6 -version | grep 'Using Qt version')"
    elif command -v qmake >/dev/null 2>&1; then
        local version_output=$(qmake -version | grep 'Using Qt version')
        if [[ $version_output == *"6."* ]]; then
            qt_version_found="6"
            qt_found=true
            log_success "Found Qt6: $version_output"
        elif [[ $version_output == *"5."* ]]; then
            qt_version_found="5"
            qt_found=true
            log_success "Found Qt5: $version_output"
        fi
    fi
    
    # Check for cmake Qt support
    if $qt_found; then
        if cmake --help | grep -q "Qt${qt_version_found}"; then
            log_success "CMake has Qt${qt_version_found} support"
        else
            log_warn "CMake may not have Qt${qt_version_found} support"
        fi
    else
        log_error "Qt not found. Please install Qt development packages."
        return 1
    fi
    
    # Check for required Qt modules
    local required_modules=("Core" "Widgets" "Network")
    for module in "${required_modules[@]}"; do
        log_info "Checking for Qt${qt_version_found}${module}..."
    done
    
    return 0
}

# Install dependencies
install_dependencies() {
    log "Installing Qt client build dependencies..."
    
    if command -v apt-get >/dev/null 2>&1; then
        # Ubuntu/Debian
        sudo apt-get update
        if [ "$QT_VERSION" = "6" ]; then
            sudo apt-get install -y \
                build-essential \
                cmake \
                qt6-base-dev \
                qt6-tools-dev \
                qt6-tools-dev-tools \
                libqt6core6 \
                libqt6widgets6 \
                libqt6network6 \
                libssl-dev \
                pkg-config
        else
            sudo apt-get install -y \
                build-essential \
                cmake \
                qtbase5-dev \
                qttools5-dev \
                qttools5-dev-tools \
                libqt5core5a \
                libqt5widgets5 \
                libqt5network5 \
                libssl-dev \
                pkg-config
        fi
    elif command -v yum >/dev/null 2>&1; then
        # CentOS/RHEL
        sudo yum groupinstall -y "Development Tools"
        if [ "$QT_VERSION" = "6" ]; then
            sudo yum install -y \
                cmake \
                qt6-qtbase-devel \
                qt6-qttools-devel \
                openssl-devel
        else
            sudo yum install -y \
                cmake \
                qt5-qtbase-devel \
                qt5-qttools-devel \
                openssl-devel
        fi
    elif command -v brew >/dev/null 2>&1; then
        # macOS
        if [ "$QT_VERSION" = "6" ]; then
            brew install cmake qt@6 openssl
        else
            brew install cmake qt@5 openssl
        fi
    elif command -v pacman >/dev/null 2>&1; then
        # Arch Linux
        if [ "$QT_VERSION" = "6" ]; then
            sudo pacman -S --noconfirm \
                base-devel \
                cmake \
                qt6-base \
                qt6-tools \
                openssl
        else
            sudo pacman -S --noconfirm \
                base-devel \
                cmake \
                qt5-base \
                qt5-tools \
                openssl
        fi
    else
        log_error "Unsupported package manager. Please install dependencies manually."
        log_info "Required packages: cmake, qt${QT_VERSION}-base-dev, qt${QT_VERSION}-tools-dev, openssl-dev"
        exit 1
    fi
    
    log_success "Dependencies installed successfully"
}

# Check dependencies
check_dependencies() {
    log "Checking Qt client build dependencies..."
    
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
    
    # Check for Qt
    if ! check_qt_installation; then
        missing_deps+=("Qt${QT_VERSION} development packages")
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

# Configure build
configure_build() {
    log "Configuring Qt client build (Type: $BUILD_TYPE)..."
    
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
        "-DENABLE_TESTS=$ENABLE_TESTS"
    )
    
    # Qt version specific arguments
    if [ "$QT_VERSION" = "6" ]; then
        cmake_args+=("-DQT_VERSION_MAJOR=6")
    else
        cmake_args+=("-DQT_VERSION_MAJOR=5")
    fi
    
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
    log "Building Qt client with $PARALLEL_JOBS parallel jobs..."
    
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
        log "Running Qt client tests..."
        
        # Run unit tests
        ctest --output-on-failure --parallel "$PARALLEL_JOBS"
        
        log_success "All tests passed"
    fi
}

# Install project
install_project() {
    log "Installing Qt client..."
    
    make install
    
    log_success "Installation completed"
}

# Create package
create_package() {
    log "Creating Qt client package..."
    
    make package
    
    log_success "Package created successfully"
}

# Deploy Qt libraries (Windows/macOS)
deploy_qt() {
    log "Deploying Qt libraries..."
    
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
        # Windows
        if command -v windeployqt >/dev/null 2>&1; then
            windeployqt bin/securechat-client.exe
            log_success "Qt libraries deployed for Windows"
        else
            log_warn "windeployqt not found, manual Qt deployment may be required"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        if command -v macdeployqt >/dev/null 2>&1; then
            macdeployqt bin/securechat-client.app
            log_success "Qt libraries deployed for macOS"
        else
            log_warn "macdeployqt not found, manual Qt deployment may be required"
        fi
    else
        log_info "Qt deployment not needed on Linux (use system Qt libraries)"
    fi
}

# Main build function
main_build() {
    print_banner
    
    log_info "Qt Client build configuration:"
    log_info "  Build Type: $BUILD_TYPE"
    log_info "  Build Directory: $BUILD_DIR"
    log_info "  Qt Version: $QT_VERSION"
    log_info "  Parallel Jobs: $PARALLEL_JOBS"
    log_info "  Tests: $ENABLE_TESTS"
    
    check_dependencies
    configure_build
    build_project
    run_tests
    deploy_qt
    
    log_success "Qt client build process completed successfully!"
    
    # Print binary information
    if [ -f "bin/securechat-client" ] || [ -f "bin/securechat-client.exe" ]; then
        local binary_name="bin/securechat-client"
        if [ -f "bin/securechat-client.exe" ]; then
            binary_name="bin/securechat-client.exe"
        fi
        
        log_info "Binary size: $(du -h "$binary_name" | cut -f1)"
        log_info "Binary location: $(pwd)/$binary_name"
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
        -q|--qt-version)
            QT_VERSION="$2"
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
        --check-qt)
            check_qt_installation
            exit $?
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

# Validate Qt version
case $QT_VERSION in
    5|6)
        ;;
    *)
        log_error "Invalid Qt version: $QT_VERSION (must be 5 or 6)"
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