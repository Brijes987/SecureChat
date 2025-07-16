#pragma once

#include <memory>
#include <atomic>
#include <string>
#include <vector>

#include "utils/config_manager.hpp"
#include "utils/logger.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace securechat::network {

class SocketManager {
public:
    explicit SocketManager(const utils::ConfigManager& config);
    ~SocketManager();

    // Non-copyable, non-movable
    SocketManager(const SocketManager&) = delete;
    SocketManager& operator=(const SocketManager&) = delete;
    SocketManager(SocketManager&&) = delete;
    SocketManager& operator=(SocketManager&&) = delete;

    bool initialize();
    bool start();
    void stop();

    int acceptConnection();
    bool closeSocket(int socket_fd);

    // Socket configuration
    bool configureSocket(int socket_fd);
    bool setNonBlocking(int socket_fd);
    bool setReuseAddr(int socket_fd);
    bool setKeepAlive(int socket_fd);
    bool setNoDelay(int socket_fd);

    // Statistics
    uint64_t getTotalConnections() const { return total_connections_.load(); }
    uint64_t getActiveConnections() const { return active_connections_.load(); }

private:
    bool createListenSocket();
    bool bindSocket();
    bool startListening();

    const utils::ConfigManager& config_;
    
    int listen_socket_{-1};
    std::atomic<bool> running_{false};
    
    // Statistics
    std::atomic<uint64_t> total_connections_{0};
    std::atomic<uint64_t> active_connections_{0};
    
    // Logging
    utils::Logger logger_;

#ifdef _WIN32
    WSADATA wsa_data_;
    bool wsa_initialized_{false};
#endif
};

} // namespace securechat::network