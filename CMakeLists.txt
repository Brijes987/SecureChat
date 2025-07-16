cmake_minimum_required(VERSION 3.20)
project(SecureChat VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fsanitize=address,undefined")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /GL")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /fsanitize=address")
endif()

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig QUIET)

# Optional: Find system libraries
if(PkgConfig_FOUND)
    pkg_check_modules(JSONCPP jsoncpp)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Source files
set(CORE_SOURCES
    src/core/server.cpp
    src/core/client_connection.cpp
    src/core/message_handler.cpp
    src/core/thread_pool.cpp
    src/core/event_loop.cpp
)

set(CRYPTO_SOURCES
    src/crypto/encryption_manager.cpp
    src/crypto/key_manager.cpp
    src/crypto/hmac_validator.cpp
    src/crypto/tls_context.cpp
)

set(NETWORK_SOURCES
    src/network/socket_manager.cpp
    src/network/protocol_handler.cpp
    src/network/message_queue.cpp
    src/network/async_io.cpp
)

set(SECURITY_SOURCES
    src/security/auth_manager.cpp
    src/security/jwt_handler.cpp
    src/security/rate_limiter.cpp
    src/security/replay_detector.cpp
)

set(UTILS_SOURCES
    src/utils/logger.cpp
    src/utils/config_manager.cpp
    src/utils/metrics_collector.cpp
    src/utils/memory_pool.cpp
)

# Main server executable
add_executable(securechat-server
    src/main.cpp
    ${CORE_SOURCES}
    ${CRYPTO_SOURCES}
    ${NETWORK_SOURCES}
    ${SECURITY_SOURCES}
    ${UTILS_SOURCES}
)

# Link libraries
target_link_libraries(securechat-server
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(securechat-server ws2_32 mswsock)
elseif(UNIX)
    target_link_libraries(securechat-server pthread)
endif()

# Set output directory
set_target_properties(securechat-server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Enable testing
enable_testing()

# Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50f33f9142fd2d6828d5a41ede87a9.zip
)
FetchContent_MakeAvailable(googletest)

# Test executables
set(TEST_SOURCES
    tests/test_encryption.cpp
    tests/test_networking.cpp
    tests/test_performance.cpp
    tests/test_security.cpp
    tests/test_utils.cpp
)

foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name}
        ${test_file}
        ${CORE_SOURCES}
        ${CRYPTO_SOURCES}
        ${NETWORK_SOURCES}
        ${SECURITY_SOURCES}
        ${UTILS_SOURCES}
    )
    target_link_libraries(${test_name}
        gtest_main
        gmock_main
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
    )
    if(WIN32)
        target_link_libraries(${test_name} ws2_32 mswsock)
    elseif(UNIX)
        target_link_libraries(${test_name} pthread)
    endif()
    
    set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Coverage support
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Benchmark support
option(ENABLE_BENCHMARKS "Enable benchmark builds" OFF)

# Qt Client (optional)
option(BUILD_QT_CLIENT "Build Qt GUI client" ON)
if(BUILD_QT_CLIENT)
    add_subdirectory(client/qt)
endif()

# Install targets
install(TARGETS securechat-server
    RUNTIME DESTINATION bin
)

install(DIRECTORY config/
    DESTINATION etc/securechat
)

# CPack configuration
set(CPACK_PACKAGE_NAME "SecureChat")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Production-ready real-time chat server")
include(CPack)