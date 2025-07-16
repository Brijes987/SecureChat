#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <mutex>

#ifdef _WIN32
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#else
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

namespace securechat::network {

enum class IOOperation {
    READ,
    WRITE,
    ACCEPT,
    CONNECT
};

struct IOEvent {
    int fd;
    IOOperation operation;
    std::vector<char> buffer;
    size_t bytes_transferred;
    int error_code;
    void* user_data;
};

using IOCallback = std::function<void(const IOEvent&)>;

class AsyncIO {
public:
    AsyncIO();
    ~AsyncIO();

    // Non-copyable, non-movable
    AsyncIO(const AsyncIO&) = delete;
    AsyncIO& operator=(const AsyncIO&) = delete;
    AsyncIO(AsyncIO&&) = delete;
    AsyncIO& operator=(AsyncIO&&) = delete;

    bool initialize();
    void start();
    void stop();

    // Socket operations
    bool addSocket(int fd, IOCallback callback);
    bool removeSocket(int fd);
    
    // Async operations
    bool asyncRead(int fd, size_t buffer_size, void* user_data = nullptr);
    bool asyncWrite(int fd, const std::vector<char>& data, void* user_data = nullptr);
    bool asyncAccept(int listen_fd, void* user_data = nullptr);
    bool asyncConnect(int fd, const sockaddr* addr, socklen_t addrlen, void* user_data = nullptr);

    // Zero-copy operations (where supported)
    bool asyncSendFile(int out_fd, int in_fd, off_t offset, size_t count, void* user_data = nullptr);
    bool asyncSplice(int in_fd, int out_fd, size_t len, void* user_data = nullptr);

    // Statistics
    uint64_t getTotalOperations() const { return total_operations_.load(); }
    uint64_t getPendingOperations() const { return pending_operations_.load(); }
    double getAverageLatency() const;

private:
    void eventLoop();
    void processEvents();
    void handleEvent(const IOEvent& event);

#ifdef _WIN32
    // Windows IOCP implementation
    bool initializeIOCP();
    void processIOCPEvents();
    HANDLE iocp_handle_;
    
    struct IOCPContext {
        OVERLAPPED overlapped;
        IOOperation operation;
        std::vector<char> buffer;
        IOCallback callback;
        void* user_data;
        std::chrono::steady_clock::time_point start_time;
    };
    
    std::unordered_map<int, std::unique_ptr<IOCPContext>> iocp_contexts_;
#else
    // Linux epoll implementation
    bool initializeEpoll();
    void processEpollEvents();
    int epoll_fd_;
    
    struct EpollContext {
        IOCallback callback;
        std::vector<char> read_buffer;
        std::vector<char> write_buffer;
        size_t write_offset;
        void* user_data;
        std::chrono::steady_clock::time_point start_time;
    };
    
    std::unordered_map<int, std::unique_ptr<EpollContext>> epoll_contexts_;
    std::vector<epoll_event> events_;
#endif

    // Thread management
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> running_{false};
    
    // Socket management
    std::mutex sockets_mutex_;
    std::unordered_map<int, IOCallback> socket_callbacks_;
    
    // Statistics
    std::atomic<uint64_t> total_operations_{0};
    std::atomic<uint64_t> pending_operations_{0};
    std::atomic<uint64_t> total_latency_us_{0};
    
    // Configuration
    static constexpr int MAX_EVENTS = 1024;
    static constexpr int WORKER_THREADS = 4;
};

// Platform-specific socket utilities
class SocketUtils {
public:
    static bool setNonBlocking(int fd);
    static bool setReuseAddr(int fd);
    static bool setNoDelay(int fd);
    static bool setKeepAlive(int fd);
    static bool setReceiveBuffer(int fd, int size);
    static bool setSendBuffer(int fd, int size);
    
    // Zero-copy support detection
    static bool supportsZeroCopy();
    static bool supportsSendFile();
    static bool supportsSplice();
    
    // Performance optimizations
    static bool enableTCPFastOpen(int fd);
    static bool enableTCPNoDelay(int fd);
    static bool enableTCPCork(int fd);
};

} // namespace securechat::network