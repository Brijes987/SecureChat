#pragma once

#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "network/async_io.hpp"
#include "utils/logger.hpp"

namespace securechat::core {

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    // Non-copyable, non-movable
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop(EventLoop&&) = delete;
    EventLoop& operator=(EventLoop&&) = delete;

    bool initialize();
    void start();
    void stop();

    // Event scheduling
    void scheduleTask(std::function<void()> task);
    void scheduleDelayedTask(std::function<void()> task, std::chrono::milliseconds delay);
    void schedulePeriodicTask(std::function<void()> task, std::chrono::milliseconds interval);

    // Statistics
    uint64_t getProcessedEvents() const { return processed_events_.load(); }
    bool isRunning() const { return running_.load(); }

private:
    void eventLoopThread();
    void processScheduledTasks();

    std::unique_ptr<network::AsyncIO> async_io_;
    
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::thread event_thread_;
    
    // Task scheduling
    struct ScheduledTask {
        std::function<void()> task;
        std::chrono::steady_clock::time_point execute_time;
        std::chrono::milliseconds interval{0}; // 0 for one-time tasks
        bool periodic{false};
    };
    
    std::vector<ScheduledTask> scheduled_tasks_;
    std::mutex tasks_mutex_;
    std::condition_variable tasks_cv_;
    
    // Statistics
    std::atomic<uint64_t> processed_events_{0};
    
    // Logging
    utils::Logger logger_;
};

} // namespace securechat::core