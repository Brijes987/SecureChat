#pragma once

#include <memory>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "core/client_connection.hpp"
#include "core/thread_pool.hpp"
#include "core/event_loop.hpp"
#include "network/socket_manager.hpp"
#include "security/auth_manager.hpp"
#include "utils/config_manager.hpp"
#include "utils/logger.hpp"
#include "utils/metrics_collector.hpp"

namespace securechat::core {

class Server {
public:
    explicit Server(const utils::ConfigManager& config);
    ~Server();

    // Non-copyable, non-movable
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

    bool initialize();
    void start();
    void stop();
    void shutdown();

    // Client management
    void addClient(std::shared_ptr<ClientConnection> client);
    void removeClient(uint64_t client_id);
    std::shared_ptr<ClientConnection> getClient(uint64_t client_id);

    // Message broadcasting
    void broadcastMessage(const std::string& message, uint64_t sender_id = 0);
    void sendToClient(uint64_t client_id, const std::string& message);

    // Statistics
    size_t getConnectedClientsCount() const;
    utils::ServerStats getStats() const;

private:
    void acceptConnections();
    void handleClientConnection(int client_socket);
    void cleanupDisconnectedClients();
    void updateMetrics();

    // Configuration
    const utils::ConfigManager& config_;
    
    // Core components
    std::unique_ptr<network::SocketManager> socket_manager_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<EventLoop> event_loop_;
    std::unique_ptr<security::AuthManager> auth_manager_;
    std::unique_ptr<utils::MetricsCollector> metrics_;

    // Client management
    mutable std::shared_mutex clients_mutex_;
    std::unordered_map<uint64_t, std::shared_ptr<ClientConnection>> clients_;
    std::atomic<uint64_t> next_client_id_{1};

    // Server state
    std::atomic<bool> running_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    // Background threads
    std::thread accept_thread_;
    std::thread cleanup_thread_;
    std::thread metrics_thread_;

    // Logging
    utils::Logger logger_;

    // Performance monitoring
    std::atomic<uint64_t> total_messages_sent_{0};
    std::atomic<uint64_t> total_messages_received_{0};
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace securechat::core