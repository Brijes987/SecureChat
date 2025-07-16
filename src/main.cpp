#include <iostream>
#include <csignal>
#include <memory>
#include <thread>
#include <chrono>

#include "core/server.hpp"
#include "utils/config_manager.hpp"
#include "utils/logger.hpp"

using namespace securechat;

// Global server instance for signal handling
std::unique_ptr<core::Server> g_server;

void signalHandler(int signal) {
    utils::g_logger.info("Received signal {}, initiating graceful shutdown", signal);
    
    if (g_server) {
        g_server->shutdown();
    }
    
    // Give server time to cleanup
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::exit(0);
}

void printBanner() {
    std::cout << R"(
   ____                           ____ _           _   
  / ___|  ___  ___ _   _ _ __ ___ / ___| |__   __ _| |_ 
  \___ \ / _ \/ __| | | | '__/ _ \ |   | '_ \ / _` | __|
   ___) |  __/ (__| |_| | | |  __/ |___| | | | (_| | |_ 
  |____/ \___|\___|\__,_|_|  \___|\____|_| |_|\__,_|\__|
                                                        
  Production-Ready Real-Time Chat Server v1.0.0
  High Performance • Enterprise Security • Sub-50ms Latency
)" << std::endl;
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
              << "\nOptions:\n"
              << "  -c, --config FILE    Configuration file path (default: config/server.json)\n"
              << "  -p, --port PORT      Server port (default: 8080)\n"
              << "  -t, --threads NUM    Number of worker threads (default: auto)\n"
              << "  -l, --log-level LVL  Log level (trace|debug|info|warn|error|fatal)\n"
              << "  -d, --daemon         Run as daemon\n"
              << "  -h, --help           Show this help message\n"
              << "  -v, --version        Show version information\n"
              << std::endl;
}

void printVersion() {
    std::cout << "SecureChat Server v1.0.0\n"
              << "Built with OpenSSL " << OPENSSL_VERSION_TEXT << "\n"
              << "C++ Standard: " << __cplusplus << "\n"
              << "Compiler: " << 
#ifdef __GNUC__
                 "GCC " << __GNUC__ << "." << __GNUC_MINOR__
#elif defined(__clang__)
                 "Clang " << __clang_major__ << "." << __clang_minor__
#elif defined(_MSC_VER)
                 "MSVC " << _MSC_VER
#else
                 "Unknown"
#endif
              << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string config_file = "config/server.json";
    int port = 8080;
    int threads = 0; // Auto-detect
    std::string log_level = "info";
    bool daemon_mode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            printVersion();
            return 0;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                config_file = argv[++i];
            } else {
                std::cerr << "Error: --config requires a file path" << std::endl;
                return 1;
            }
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                port = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --port requires a port number" << std::endl;
                return 1;
            }
        } else if (arg == "-t" || arg == "--threads") {
            if (i + 1 < argc) {
                threads = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --threads requires a number" << std::endl;
                return 1;
            }
        } else if (arg == "-l" || arg == "--log-level") {
            if (i + 1 < argc) {
                log_level = argv[++i];
            } else {
                std::cerr << "Error: --log-level requires a level" << std::endl;
                return 1;
            }
        } else if (arg == "-d" || arg == "--daemon") {
            daemon_mode = true;
        } else {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // Print banner
    if (!daemon_mode) {
        printBanner();
    }

    try {
        // Initialize logging
        utils::LogLevel level = utils::LogLevel::INFO;
        if (log_level == "trace") level = utils::LogLevel::TRACE;
        else if (log_level == "debug") level = utils::LogLevel::DEBUG;
        else if (log_level == "info") level = utils::LogLevel::INFO;
        else if (log_level == "warn") level = utils::LogLevel::WARN;
        else if (log_level == "error") level = utils::LogLevel::ERROR;
        else if (log_level == "fatal") level = utils::LogLevel::FATAL;

        utils::Logger::setLogLevel(level);
        utils::Logger::enableConsoleOutput(!daemon_mode);
        utils::Logger::enableAsyncLogging(true);
        utils::Logger::setOutputFile("logs/securechat.log");

        utils::g_logger.info("Starting SecureChat Server v1.0.0");
        utils::g_logger.info("Configuration file: {}", config_file);
        utils::g_logger.info("Port: {}", port);
        utils::g_logger.info("Log level: {}", log_level);

        // Load configuration
        utils::ConfigManager config;
        if (!config.loadFromFile(config_file)) {
            utils::g_logger.error("Failed to load configuration from {}", config_file);
            return 1;
        }

        // Override config with command line arguments
        if (port != 8080) {
            config.setPort(port);
        }
        if (threads > 0) {
            config.setWorkerThreads(threads);
        }

        // Create and initialize server
        g_server = std::make_unique<core::Server>(config);
        
        if (!g_server->initialize()) {
            utils::g_logger.fatal("Failed to initialize server");
            return 1;
        }

        // Setup signal handlers
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
#ifndef _WIN32
        std::signal(SIGHUP, signalHandler);
        std::signal(SIGQUIT, signalHandler);
#endif

        utils::g_logger.info("Server initialized successfully");
        utils::g_logger.info("Listening on port {}", config.getPort());
        utils::g_logger.info("Worker threads: {}", config.getWorkerThreads());
        utils::g_logger.info("Max connections: {}", config.getMaxConnections());

        // Start server
        g_server->start();

        utils::g_logger.info("Server started successfully");
        
        if (!daemon_mode) {
            std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
        }

        // Keep main thread alive
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Print periodic statistics
            static auto last_stats = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            if (now - last_stats >= std::chrono::minutes(5)) {
                auto stats = g_server->getStats();
                utils::g_logger.info("Server stats - Clients: {}, Messages: {}, Uptime: {}s", 
                                   stats.connected_clients, stats.total_messages, stats.uptime_seconds);
                last_stats = now;
            }
        }

    } catch (const std::exception& e) {
        utils::g_logger.fatal("Unhandled exception: {}", e.what());
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        utils::g_logger.fatal("Unknown exception occurred");
        std::cerr << "Fatal error: Unknown exception" << std::endl;
        return 1;
    }

    return 0;
}