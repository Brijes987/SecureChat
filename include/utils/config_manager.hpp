#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace securechat::utils {

struct ServerStats {
    size_t connected_clients{0};
    uint64_t total_messages{0};
    uint64_t uptime_seconds{0};
    double cpu_usage{0.0};
    size_t memory_usage{0};
};

class ConfigManager {
public:
    ConfigManager() = default;
    ~ConfigManager() = default;

    // Non-copyable, movable
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = default;
    ConfigManager& operator=(ConfigManager&&) = default;

    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
    
    // Server configuration
    int getPort() const { return getInt("server.port", 8080); }
    void setPort(int port) { setInt("server.port", port); }
    
    std::string getBindAddress() const { return getString("server.bind_address", "0.0.0.0"); }
    int getMaxConnections() const { return getInt("server.max_connections", 1000); }
    int getWorkerThreads() const { return getInt("server.worker_threads", 0); }
    int getBacklog() const { return getInt("server.backlog", 128); }
    int getKeepaliveTimeout() const { return getInt("server.keepalive_timeout", 300); }
    int getClientTimeout() const { return getInt("server.client_timeout", 60); }
    
    void setWorkerThreads(int threads) { setInt("server.worker_threads", threads); }
    
    // Security configuration
    bool isTLSEnabled() const { return getBool("security.enable_tls", true); }
    std::string getTLSCertFile() const { return getString("security.tls_cert_file", "certs/server.crt"); }
    std::string getTLSKeyFile() const { return getString("security.tls_key_file", "certs/server.key"); }
    std::string getTLSCAFile() const { return getString("security.tls_ca_file", "certs/ca.crt"); }
    bool requireClientCert() const { return getBool("security.require_client_cert", false); }
    std::string getMinTLSVersion() const { return getString("security.min_tls_version", "1.3"); }
    bool isPerfectForwardSecrecy() const { return getBool("security.perfect_forward_secrecy", true); }
    int getKeyRotationInterval() const { return getInt("security.key_rotation_interval", 1800); }
    int getSessionTimeout() const { return getInt("security.session_timeout", 3600); }
    
    // Encryption configuration
    std::string getEncryptionAlgorithm() const { return getString("encryption.algorithm", "AES-256-GCM"); }
    std::string getKeyDerivation() const { return getString("encryption.key_derivation", "PBKDF2"); }
    int getKDFIterations() const { return getInt("encryption.iterations", 100000); }
    int getSaltLength() const { return getInt("encryption.salt_length", 32); }
    bool isCompressionEnabled() const { return getBool("encryption.enable_compression", true); }
    int getCompressionLevel() const { return getInt("encryption.compression_level", 6); }
    
    // Authentication configuration
    bool isJWTEnabled() const { return getBool("authentication.enable_jwt", true); }
    std::string getJWTSecret() const { return getString("authentication.jwt_secret", ""); }
    int getJWTExpiry() const { return getInt("authentication.jwt_expiry", 3600); }
    bool isOAuth2Enabled() const { return getBool("authentication.enable_oauth2", false); }
    
    // Rate limiting
    int getLoginAttempts() const { return getInt("authentication.rate_limiting.login_attempts", 5); }
    int getLockoutDuration() const { return getInt("authentication.rate_limiting.lockout_duration", 300); }
    int getMessagesPerSecond() const { return getInt("rate_limiting.messages_per_second", 100); }
    int getBurstSize() const { return getInt("rate_limiting.burst_size", 200); }
    int getConnectionRate() const { return getInt("rate_limiting.connection_rate", 10); }
    int getBandwidthLimit() const { return getInt("rate_limiting.bandwidth_limit", 1048576); }
    
    // Performance configuration
    std::string getIOModel() const { return getString("performance.io_model", "epoll"); }
    int getBufferSize() const { return getInt("performance.buffer_size", 8192); }
    int getMaxMessageSize() const { return getInt("performance.max_message_size", 1048576); }
    int getMessageQueueSize() const { return getInt("performance.message_queue_size", 1000); }
    bool isZeroCopyEnabled() const { return getBool("performance.enable_zero_copy", true); }
    bool isTCPNoDelayEnabled() const { return getBool("performance.enable_tcp_nodelay", true); }
    bool isTCPFastOpenEnabled() const { return getBool("performance.enable_tcp_fastopen", true); }
    int getSocketRecvBuffer() const { return getInt("performance.socket_recv_buffer", 65536); }
    int getSocketSendBuffer() const { return getInt("performance.socket_send_buffer", 65536); }
    
    // Logging configuration
    std::string getLogLevel() const { return getString("logging.level", "info"); }
    std::string getLogFile() const { return getString("logging.file", "logs/securechat.log"); }
    int getMaxLogFileSize() const { return getInt("logging.max_file_size", 104857600); }
    int getMaxLogFiles() const { return getInt("logging.max_files", 10); }
    bool isConsoleLoggingEnabled() const { return getBool("logging.enable_console", true); }
    bool isAsyncLoggingEnabled() const { return getBool("logging.enable_async", true); }
    bool isStructuredLogging() const { return getBool("logging.structured", true); }
    
    // Monitoring configuration
    bool isMetricsEnabled() const { return getBool("monitoring.enable_metrics", true); }
    int getMetricsPort() const { return getInt("monitoring.metrics_port", 9090); }
    std::string getMetricsPath() const { return getString("monitoring.metrics_path", "/metrics"); }
    int getHealthCheckPort() const { return getInt("monitoring.health_check_port", 8081); }
    std::string getHealthCheckPath() const { return getString("monitoring.health_check_path", "/health"); }
    bool isProfilingEnabled() const { return getBool("monitoring.enable_profiling", false); }
    
    // Database configuration
    std::string getDatabaseType() const { return getString("database.type", "postgresql"); }
    std::string getDatabaseHost() const { return getString("database.host", "localhost"); }
    int getDatabasePort() const { return getInt("database.port", 5432); }
    std::string getDatabaseName() const { return getString("database.database", "securechat"); }
    std::string getDatabaseUsername() const { return getString("database.username", "securechat"); }
    std::string getDatabasePassword() const { return getString("database.password", ""); }
    int getDatabasePoolSize() const { return getInt("database.pool_size", 20); }
    
    // Redis configuration
    std::string getRedisHost() const { return getString("redis.host", "localhost"); }
    int getRedisPort() const { return getInt("redis.port", 6379); }
    std::string getRedisPassword() const { return getString("redis.password", ""); }
    int getRedisDatabase() const { return getInt("redis.database", 0); }
    int getRedisPoolSize() const { return getInt("redis.pool_size", 10); }
    
    // Feature flags
    bool isFileTransferEnabled() const { return getBool("features.enable_file_transfer", true); }
    int getMaxFileSize() const { return getInt("features.max_file_size", 104857600); }
    bool isMessageHistoryEnabled() const { return getBool("features.enable_message_history", true); }
    int getHistoryRetentionDays() const { return getInt("features.history_retention_days", 30); }
    bool isUserPresenceEnabled() const { return getBool("features.enable_user_presence", true); }
    bool areTypingIndicatorsEnabled() const { return getBool("features.enable_typing_indicators", true); }
    bool areReadReceiptsEnabled() const { return getBool("features.enable_read_receipts", true); }
    
    // Plugin configuration
    std::string getPluginDirectory() const { return getString("plugins.directory", "plugins"); }
    bool isAutoLoadEnabled() const { return getBool("plugins.auto_load", true); }
    std::vector<std::string> getEnabledPlugins() const;

private:
    // Generic getters/setters
    std::string getString(const std::string& key, const std::string& default_value = "") const;
    int getInt(const std::string& key, int default_value = 0) const;
    bool getBool(const std::string& key, bool default_value = false) const;
    double getDouble(const std::string& key, double default_value = 0.0) const;
    
    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setBool(const std::string& key, bool value);
    void setDouble(const std::string& key, double value);
    
    // Configuration storage
    mutable std::mutex config_mutex_;
    std::unordered_map<std::string, std::string> config_data_;
    
    // Helper methods
    std::vector<std::string> splitKey(const std::string& key) const;
    std::string joinKey(const std::vector<std::string>& parts) const;
};

} // namespace securechat::utils