#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <chrono>
#include <sstream>

namespace securechat::utils {

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

struct LogEntry {
    LogLevel level;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id thread_id;
    std::string component;
};

class Logger {
public:
    explicit Logger(const std::string& component = "");
    ~Logger();

    // Non-copyable, movable
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = default;
    Logger& operator=(Logger&&) = default;

    // Configuration
    static void setLogLevel(LogLevel level);
    static void setOutputFile(const std::string& filename);
    static void enableConsoleOutput(bool enable);
    static void enableAsyncLogging(bool enable);
    static void setMaxFileSize(size_t max_size);
    static void setMaxFiles(int max_files);

    // Logging methods
    void trace(const std::string& message, const char* file = __builtin_FILE(), 
               int line = __builtin_LINE(), const char* function = __builtin_FUNCTION());
    void debug(const std::string& message, const char* file = __builtin_FILE(), 
               int line = __builtin_LINE(), const char* function = __builtin_FUNCTION());
    void info(const std::string& message, const char* file = __builtin_FILE(), 
              int line = __builtin_LINE(), const char* function = __builtin_FUNCTION());
    void warn(const std::string& message, const char* file = __builtin_FILE(), 
              int line = __builtin_LINE(), const char* function = __builtin_FUNCTION());
    void error(const std::string& message, const char* file = __builtin_FILE(), 
               int line = __builtin_LINE(), const char* function = __builtin_FUNCTION());
    void fatal(const std::string& message, const char* file = __builtin_FILE(), 
               int line = __builtin_LINE(), const char* function = __builtin_FUNCTION());

    // Template logging with formatting
    template<typename... Args>
    void trace(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::TRACE)) {
            log(LogLevel::TRACE, formatString(format, std::forward<Args>(args)...));
        }
    }

    template<typename... Args>
    void debug(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::DEBUG)) {
            log(LogLevel::DEBUG, formatString(format, std::forward<Args>(args)...));
        }
    }

    template<typename... Args>
    void info(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::INFO)) {
            log(LogLevel::INFO, formatString(format, std::forward<Args>(args)...));
        }
    }

    template<typename... Args>
    void warn(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::WARN)) {
            log(LogLevel::WARN, formatString(format, std::forward<Args>(args)...));
        }
    }

    template<typename... Args>
    void error(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::ERROR)) {
            log(LogLevel::ERROR, formatString(format, std::forward<Args>(args)...));
        }
    }

    template<typename... Args>
    void fatal(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::FATAL)) {
            log(LogLevel::FATAL, formatString(format, std::forward<Args>(args)...));
        }
    }

    // Structured logging
    class LogBuilder {
    public:
        LogBuilder(Logger& logger, LogLevel level);
        ~LogBuilder();

        LogBuilder& field(const std::string& key, const std::string& value);
        LogBuilder& field(const std::string& key, int value);
        LogBuilder& field(const std::string& key, double value);
        LogBuilder& field(const std::string& key, bool value);
        LogBuilder& message(const std::string& msg);

    private:
        Logger& logger_;
        LogLevel level_;
        std::ostringstream stream_;
        bool has_fields_{false};
    };

    LogBuilder structured(LogLevel level);

    // Performance metrics
    static uint64_t getTotalLogEntries();
    static uint64_t getDroppedEntries();
    static double getAverageProcessingTime();

private:
    void log(LogLevel level, const std::string& message, 
             const char* file = "", int line = 0, const char* function = "");
    
    static void processLogQueue();
    static void writeLogEntry(const LogEntry& entry);
    static std::string formatLogEntry(const LogEntry& entry);
    static std::string levelToString(LogLevel level);
    static bool shouldLog(LogLevel level);
    static void rotateLogFile();

    template<typename... Args>
    std::string formatString(const std::string& format, Args&&... args) {
        std::ostringstream oss;
        formatStringImpl(oss, format, std::forward<Args>(args)...);
        return oss.str();
    }

    template<typename T, typename... Args>
    void formatStringImpl(std::ostringstream& oss, const std::string& format, 
                         T&& value, Args&&... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << std::forward<T>(value);
            formatStringImpl(oss, format.substr(pos + 2), std::forward<Args>(args)...);
        } else {
            oss << format;
        }
    }

    void formatStringImpl(std::ostringstream& oss, const std::string& format) {
        oss << format;
    }

    std::string component_;

    // Static configuration
    static std::atomic<LogLevel> log_level_;
    static std::atomic<bool> console_output_;
    static std::atomic<bool> async_logging_;
    static std::atomic<size_t> max_file_size_;
    static std::atomic<int> max_files_;
    static std::string output_file_;

    // Async logging
    static std::queue<LogEntry> log_queue_;
    static std::mutex queue_mutex_;
    static std::condition_variable queue_cv_;
    static std::thread log_thread_;
    static std::atomic<bool> shutdown_;

    // File handling
    static std::ofstream log_file_;
    static std::mutex file_mutex_;
    static std::atomic<size_t> current_file_size_;

    // Statistics
    static std::atomic<uint64_t> total_entries_;
    static std::atomic<uint64_t> dropped_entries_;
    static std::atomic<uint64_t> total_processing_time_us_;
};

// Convenience macros
#define LOG_TRACE(logger, msg) logger.trace(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG(logger, msg) logger.debug(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(logger, msg) logger.info(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARN(logger, msg) logger.warn(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(logger, msg) logger.error(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL(logger, msg) logger.fatal(msg, __FILE__, __LINE__, __FUNCTION__)

// Global logger instance
extern Logger g_logger;

} // namespace securechat::utils