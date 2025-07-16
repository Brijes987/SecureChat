#pragma once

#include <memory>
#include <atomic>
#include <string>
#include <queue>
#include <mutex>
#include <chrono>
#include <thread>

#include "crypto/encryption_manager.hpp"
#include "network/message_queue.hpp"
#include "security/rate_limiter.hpp"
#include "utils/logger.hpp"

namespace securechat::core {

enum class ClientState {
    CONNECTING,
    AUTHENTICATING,
    AUTHENTICATED,
    DISCONNECTING,
    DISCONNECTED
};

class ClientConnection {
public:
    explicit ClientConnection(int socket_fd, uint64_t client_id);
    ~ClientConnection();

    // Non-copyable, non-movable
    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(const ClientConnection&) = delete;
    ClientConnection(ClientConnection&&) = delete;
    ClientConnection& operator=(ClientConnection&&) = delete;

    bool initialize();
    void start();
    void disconnect();

    // Message handling
    bool sendMessage(const std::string& message);
    bool sendEncryptedMessage(const std::string& message);
    void queueMessage(const std::string& message);

    // Authentication
    bool authenticate(const std::string& credentials);
    void setAuthenticated(bool authenticated);

    // Getters
    uint64_t getId() const { return client_id_; }
    ClientState getState() const { return state_.load(); }
    bool isAuthenticated() const { return state_.load() == ClientState::AUTHENTICATED; }
    bool isConnected() const { 
        auto state = state_.load();
        return state != ClientState::DISCONNECTED && state != ClientState::DISCONNECTING;
    }

    // Statistics
    uint64_t getMessagesSent() const { return messages_sent_.load(); }
    uint64_t getMessagesReceived() const { return messages_received_.load(); }
    std::chrono::steady_clock::time_point getConnectTime() const { return connect_time_; }
    std::chrono::steady_clock::time_point getLastActivity() const { return last_activity_.load(); }

    // Rate limiting
    bool checkRateLimit();

private:
    void receiveLoop();
    void sendLoop();
    bool processIncomingData();
    bool handleMessage(const std::string& message);
    void updateLastActivity();
    void cleanup();

    // Connection details
    const int socket_fd_;
    const uint64_t client_id_;
    std::atomic<ClientState> state_{ClientState::CONNECTING};

    // Encryption
    std::unique_ptr<crypto::EncryptionManager> encryption_;

    // Message queuing
    std::unique_ptr<network::MessageQueue> message_queue_;

    // Security
    std::unique_ptr<security::RateLimiter> rate_limiter_;

    // Threading
    std::thread receive_thread_;
    std::thread send_thread_;

    // Statistics
    std::atomic<uint64_t> messages_sent_{0};
    std::atomic<uint64_t> messages_received_{0};
    const std::chrono::steady_clock::time_point connect_time_;
    std::atomic<std::chrono::steady_clock::time_point> last_activity_;

    // Buffers
    static constexpr size_t BUFFER_SIZE = 8192;
    std::vector<char> receive_buffer_;
    std::string partial_message_;

    // Logging
    utils::Logger logger_;

    // Synchronization
    mutable std::mutex send_mutex_;
    std::atomic<bool> shutdown_requested_{false};
};

} // namespace securechat::core