#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QMutex>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtCore/QQueue>
#include <memory>

namespace securechat::client::utils {

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Fatal = 5
};

struct LogEntry {
    LogLevel level;
    QString component;
    QString message;
    QDateTime timestamp;
    QString file;
    int line;
    QString function;
};

class ClientLogger : public QObject {
    Q_OBJECT

public:
    explicit ClientLogger(const QString &component = QString(), QObject *parent = nullptr);
    ~ClientLogger() override;

    // Static initialization
    static void initialize(const QString &logFilePath);
    static void shutdown();
    static void setLogLevel(LogLevel level);
    static void setMaxFileSize(qint64 maxSize);
    static void setMaxFiles(int maxFiles);
    static void enableConsoleOutput(bool enable);
    static void enableAsyncLogging(bool enable);

    // Logging methods
    void trace(const QString &message, const char *file = __builtin_FILE(), 
               int line = __builtin_LINE(), const char *function = __builtin_FUNCTION());
    void debug(const QString &message, const char *file = __builtin_FILE(), 
               int line = __builtin_LINE(), const char *function = __builtin_FUNCTION());
    void info(const QString &message, const char *file = __builtin_FILE(), 
              int line = __builtin_LINE(), const char *function = __builtin_FUNCTION());
    void warn(const QString &message, const char *file = __builtin_FILE(), 
              int line = __builtin_LINE(), const char *function = __builtin_FUNCTION());
    void error(const QString &message, const char *file = __builtin_FILE(), 
               int line = __builtin_LINE(), const char *function = __builtin_FUNCTION());
    void fatal(const QString &message, const char *file = __builtin_FILE(), 
               int line = __builtin_LINE(), const char *function = __builtin_FUNCTION());

    // Template logging with formatting
    template<typename... Args>
    void trace(const QString &format, Args&&... args) {
        if (shouldLog(LogLevel::Trace)) {
            log(LogLevel::Trace, QString::asprintf(format.toUtf8().constData(), args...));
        }
    }

    template<typename... Args>
    void debug(const QString &format, Args&&... args) {
        if (shouldLog(LogLevel::Debug)) {
            log(LogLevel::Debug, QString::asprintf(format.toUtf8().constData(), args...));
        }
    }

    template<typename... Args>
    void info(const QString &format, Args&&... args) {
        if (shouldLog(LogLevel::Info)) {
            log(LogLevel::Info, QString::asprintf(format.toUtf8().constData(), args...));
        }
    }

    template<typename... Args>
    void warn(const QString &format, Args&&... args) {
        if (shouldLog(LogLevel::Warning)) {
            log(LogLevel::Warning, QString::asprintf(format.toUtf8().constData(), args...));
        }
    }

    template<typename... Args>
    void error(const QString &format, Args&&... args) {
        if (shouldLog(LogLevel::Error)) {
            log(LogLevel::Error, QString::asprintf(format.toUtf8().constData(), args...));
        }
    }

    template<typename... Args>
    void fatal(const QString &format, Args&&... args) {
        if (shouldLog(LogLevel::Fatal)) {
            log(LogLevel::Fatal, QString::asprintf(format.toUtf8().constData(), args...));
        }
    }

    // Component management
    void setComponent(const QString &component) { m_component = component; }
    QString getComponent() const { return m_component; }

    // Statistics
    static qint64 getTotalLogEntries();
    static qint64 getDroppedEntries();
    static qint64 getCurrentFileSize();

private slots:
    void processLogQueue();

private:
    void log(LogLevel level, const QString &message, 
             const char *file = "", int line = 0, const char *function = "");
    
    static void writeLogEntry(const LogEntry &entry);
    static QString formatLogEntry(const LogEntry &entry);
    static QString levelToString(LogLevel level);
    static bool shouldLog(LogLevel level);
    static void rotateLogFile();
    static void ensureLogDirectory();

    QString m_component;

    // Static members
    static LogLevel s_logLevel;
    static QString s_logFilePath;
    static qint64 s_maxFileSize;
    static int s_maxFiles;
    static bool s_consoleOutput;
    static bool s_asyncLogging;
    static bool s_initialized;
    
    static std::unique_ptr<QFile> s_logFile;
    static std::unique_ptr<QTextStream> s_logStream;
    static QMutex s_logMutex;
    
    // Async logging
    static QQueue<LogEntry> s_logQueue;
    static QMutex s_queueMutex;
    static QTimer *s_flushTimer;
    
    // Statistics
    static qint64 s_totalEntries;
    static qint64 s_droppedEntries;
    static qint64 s_currentFileSize;
    
    // Constants
    static constexpr qint64 DEFAULT_MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB
    static constexpr int DEFAULT_MAX_FILES = 5;
    static constexpr int FLUSH_INTERVAL_MS = 1000;
    static constexpr int MAX_QUEUE_SIZE = 10000;
};

// Convenience macros
#define LOG_TRACE(logger, msg) logger.trace(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG(logger, msg) logger.debug(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(logger, msg) logger.info(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARN(logger, msg) logger.warn(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(logger, msg) logger.error(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL(logger, msg) logger.fatal(msg, __FILE__, __LINE__, __FUNCTION__)

} // namespace securechat::client::utils