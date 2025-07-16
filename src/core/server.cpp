#include "core/server.hpp"
#include <algorithm>
#include <chrono>

namespace securechat::core {

Server::Server(const utils::ConfigManager& config)
    : config_(config)
    , logger_("Server") {
    start_time_ = std::chrono::steady_clock::now();
}

Server::~Server() {
    shutdown();
}

bool Server::initialize() {
    logger_.info("Initializing SecureChat Server");

    try {
        // Initialize socket manager
        socket_manager_ = std::make_unique<network::SocketManager>(config_);
        if (!socket_manager_->initialize()) {
            logger_.error("Failed to initialize socket manager");
            return false;
        }

        // Initialize thread pool
        int worker_threads = config_.getWorkerThreads();
        if (worker_threads <= 0) {
            worker_threads = std::thread::hardware_concurrency();
        }
        thread_pool_ = std::make_unique<ThreadPool>(worker_threads);
        logger_.info("Initialized thread pool with {} workers", worker_threads);

        // Initialize event loop
        event_loop_ = std::make_unique<EventLoop>();
        if (!event_loop_->initialize()) {
            logger_.error("Failed to initialize event loop");
            return false;
        }

        // Initialize authentication manager
        auth_manager_ = std::make_unique<security::AuthManager>(config_);
        if (!auth_manager_->initialize()) {
            logger_.error("Failed to initialize authentication manager");
            return false;
        }

        // Initialize metrics collector
        if (config_.isMetricsEnabled()) {
            metrics_ = std::make_unique<utils::MetricsCollector>(config_);
            if (!metrics_->initialize()) {
                logger_.warn("Failed to initialize metrics collector");
            }
        }

        logger_.info("Server initialization completed successfully");
        return true;

    } catch (const std::exception& e) {
        logger_.error("Exception during server initialization: {}", e.what());
        return false;
    }
}

void Server::start() {
    if (running_.load()) {
        logger_.warn("Server is already running");
        return;
    }

    logger_.info("Starting SecureChat Server");

    try {
        // Start socket manager
        if (!socket_manager_->start()) {
            throw std::runtime_error("Failed to start socket manager");
        }

        // Start event loop
        event_loop_->start();

        // Start background threads
        accept_thread_ = std::thread(&Server::acceptConnections, this);
        cleanup_thread_ = std::thread([this]() {
            while (running_.load()) {
                std::this_thread::sleep_for(std::chrono::seconds(30));
                cleanupDisconnectedClients();
            }
        });

        if (metrics_) {
            metrics_thread_ = std::thread([this]() {
                while (running_.load()) {
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                    updateMetrics();
                }
            });
        }

        running_.store(true);
        logger_.info("Server started successfully on port {}", config_.getPort());

    } catch (const std::exception& e) {
        logger_.error("Failed to start server: {}", e.what());
        stop();
        throw;
    }
}

void Server::stop() {
    if (!running_.load()) {
        return;
    }

    logger_.info("Stopping SecureChat Server");
    running_.store(false);

    // Stop accepting new connections
    if (socket_manager_) {
        socket_manager_->stop();
    }

    // Stop event loop
    if (event_loop_) {
        event_loop_->stop();
    }

    // Wait for background threads
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
    if (metrics_thread_.joinable()) {
        metrics_thread_.join();
    }

    // Disconnect all clients
    {
        std::unique_lock<std::shared_mutex> lock(clients_mutex_);
        for (auto& [id, client] : clients_) {
            client->disconnect();
        }
        clients_.clear();
    }

    logger_.info("Server stopped");
}

void Server::shutdown() {
    shutdown_requested_.store(true);
    stop();
}

void Server::addClient(std::shared_ptr<ClientConnection> client) {
    if (!client) {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(clients_mutex_);
    clients_[client->getId()] = client;
    
    logger_.info("Client {} connected. Total clients: {}", 
                client->getId(), clients_.size());

    if (metrics_) {
        metrics_->incrementCounter("clients_connected_total");
        metrics_->setGauge("clients_active", static_cast<double>(clients_.size()));
    }
}

void Server::removeClient(uint64_t client_id) {
    std::unique_lock<std::shared_mutex> lock(clients_mutex_);
    auto it = clients_.find(client_id);
    if (it != clients_.end()) {
        clients_.erase(it);
        logger_.info("Client {} disconnected. Total clients: {}", 
                    client_id, clients_.size());

        if (metrics_) {
            metrics_->incrementCounter("clients_disconnected_total");
            metrics_->setGauge("clients_active", static_cast<double>(clients_.size()));
        }
    }
}

std::shared_ptr<ClientConnection> Server::getClient(uint64_t client_id) {
    std::shared_lock<std::shared_mutex> lock(clients_mutex_);
    auto it = clients_.find(client_id);
    return (it != clients_.end()) ? it->second : nullptr;
}

void Server::broadcastMessage(const std::string& message, uint64_t sender_id) {
    std::shared_lock<std::shared_mutex> lock(clients_mutex_);
    
    for (const auto& [id, client] : clients_) {
        if (id != sender_id && client->isAuthenticated()) {
            thread_pool_->enqueue([client, message]() {
                client->sendEncryptedMessage(message);
            });
        }
    }

    total_messages_sent_.fetch_add(clients_.size() - (sender_id ? 1 : 0));
    
    if (metrics_) {
        metrics_->incrementCounter("messages_broadcast_total");
    }
}

void Server::sendToClient(uint64_t client_id, const std::string& message) {
    auto client = getClient(client_id);
    if (client && client->isAuthenticated()) {
        thread_pool_->enqueue([client, message]() {
            client->sendEncryptedMessage(message);
        });
        
        total_messages_sent_.fetch_add(1);
        
        if (metrics_) {
            metrics_->incrementCounter("messages_sent_total");
        }
    }
}

size_t Server::getConnectedClientsCount() const {
    std::shared_lock<std::shared_mutex> lock(clients_mutex_);
    return clients_.size();
}

utils::ServerStats Server::getStats() const {
    utils::ServerStats stats;
    stats.connected_clients = getConnectedClientsCount();
    stats.total_messages = total_messages_sent_.load() + total_messages_received_.load();
    
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    stats.uptime_seconds = uptime.count();
    
    return stats;
}

void Server::acceptConnections() {
    logger_.info("Accept thread started");
    
    while (running_.load() && !shutdown_requested_.load()) {
        try {
            int client_socket = socket_manager_->acceptConnection();
            if (client_socket >= 0) {
                thread_pool_->enqueue([this, client_socket]() {
                    handleClientConnection(client_socket);
                });
            }
        } catch (const std::exception& e) {
            if (running_.load()) {
                logger_.error("Error accepting connection: {}", e.what());
            }
        }
    }
    
    logger_.info("Accept thread stopped");
}

void Server::handleClientConnection(int client_socket) {
    try {
        uint64_t client_id = next_client_id_.fetch_add(1);
        auto client = std::make_shared<ClientConnection>(client_socket, client_id);
        
        if (client->initialize()) {
            addClient(client);
            client->start();
        } else {
            logger_.warn("Failed to initialize client connection {}", client_id);
        }
    } catch (const std::exception& e) {
        logger_.error("Error handling client connection: {}", e.what());
        close(client_socket);
    }
}

void Server::cleanupDisconnectedClients() {
    std::vector<uint64_t> disconnected_clients;
    
    {
        std::shared_lock<std::shared_mutex> lock(clients_mutex_);
        for (const auto& [id, client] : clients_) {
            if (!client->isConnected()) {
                disconnected_clients.push_back(id);
            }
        }
    }
    
    for (uint64_t client_id : disconnected_clients) {
        removeClient(client_id);
    }
    
    if (!disconnected_clients.empty()) {
        logger_.debug("Cleaned up {} disconnected clients", disconnected_clients.size());
    }
}

void Server::updateMetrics() {
    if (!metrics_) {
        return;
    }
    
    auto stats = getStats();
    metrics_->setGauge("server_uptime_seconds", static_cast<double>(stats.uptime_seconds));
    metrics_->setGauge("messages_total", static_cast<double>(stats.total_messages));
    
    // Memory usage
    // Note: This is a simplified implementation
    // In production, you'd want more detailed memory tracking
    metrics_->setGauge("memory_usage_bytes", 0.0); // Placeholder
}

} // namespace securechat::core