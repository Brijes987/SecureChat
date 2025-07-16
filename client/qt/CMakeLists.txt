cmake_minimum_required(VERSION 3.20)
project(SecureChatClient VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# Enable Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/client/qt/include)

# Source files
set(CLIENT_SOURCES
    src/main.cpp
    src/main_window.cpp
    src/login_dialog.cpp
    src/chat_widget.cpp
    src/user_list_widget.cpp
    src/message_widget.cpp
    src/settings_dialog.cpp
    src/network/client_connection.cpp
    src/network/message_handler.cpp
    src/network/encryption_client.cpp
    src/utils/client_logger.cpp
    src/utils/theme_manager.cpp
)

# Header files (for MOC)
set(CLIENT_HEADERS
    include/main_window.hpp
    include/login_dialog.hpp
    include/chat_widget.hpp
    include/user_list_widget.hpp
    include/message_widget.hpp
    include/settings_dialog.hpp
    include/network/client_connection.hpp
    include/network/message_handler.hpp
    include/network/encryption_client.hpp
    include/utils/client_logger.hpp
    include/utils/theme_manager.hpp
)

# UI files
set(CLIENT_UI_FILES
    ui/main_window.ui
    ui/login_dialog.ui
    ui/chat_widget.ui
    ui/settings_dialog.ui
)

# Resource files
set(CLIENT_RESOURCES
    resources/resources.qrc
)

# Create executable
add_executable(securechat-client
    ${CLIENT_SOURCES}
    ${CLIENT_HEADERS}
    ${CLIENT_UI_FILES}
    ${CLIENT_RESOURCES}
)

# Link libraries
target_link_libraries(securechat-client
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(securechat-client ws2_32)
elseif(UNIX)
    target_link_libraries(securechat-client pthread)
endif()

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(securechat-client PRIVATE
        -Wall -Wextra -Wpedantic
        -O3 -march=native
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(securechat-client PRIVATE
        /W4 /O2
    )
endif()

# Set output directory
set_target_properties(securechat-client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS securechat-client
    RUNTIME DESTINATION bin
)

# Copy Qt libraries (Windows)
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET securechat-client POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:securechat-client>
            COMMENT "Deploying Qt libraries"
        )
    endif()
endif()