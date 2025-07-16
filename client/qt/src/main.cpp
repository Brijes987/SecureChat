#include <QtWidgets/QApplication>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QLoggingCategory>
#include <QtCore/QCommandLineParser>
#include <QtCore/QTranslator>
#include <QtCore/QLibraryInfo>
#include <QtGui/QIcon>
#include <QtNetwork/QSslSocket>

#include "main_window.hpp"
#include "utils/client_logger.hpp"
#include "utils/theme_manager.hpp"

using namespace securechat::client;

// Logging categories
Q_LOGGING_CATEGORY(lcMain, "securechat.client.main")

void setupLogging() {
    // Create logs directory
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QDir().mkpath(logDir);
    
    // Initialize client logger
    utils::ClientLogger::initialize(logDir + "/securechat-client.log");
    
    // Set Qt logging rules
    QLoggingCategory::setFilterRules("securechat.client.*=true");
}

void setupApplication(QApplication &app) {
    // Set application properties
    app.setApplicationName("SecureChat Client");
    app.setApplicationVersion("1.0.0");
    app.setApplicationDisplayName("SecureChat - Secure Real-Time Messaging");
    app.setOrganizationName("SecureChat");
    app.setOrganizationDomain("securechat.org");
    
    // Set application icon
    app.setWindowIcon(QIcon(":/icons/app-icon.png"));
    
    // Enable high DPI support
    app.setAttribute(Qt::AA_EnableHighDpiScaling);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
}

void setupTranslations(QApplication &app) {
    // Load Qt translations
    QTranslator *qtTranslator = new QTranslator(&app);
    if (qtTranslator->load("qt_" + QLocale::system().name(),
                          QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(qtTranslator);
    }
    
    // Load application translations
    QTranslator *appTranslator = new QTranslator(&app);
    QString translationsPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, 
                                                     "translations", 
                                                     QStandardPaths::LocateDirectory);
    if (appTranslator->load("securechat_" + QLocale::system().name(), translationsPath)) {
        app.installTranslator(appTranslator);
    }
}

void checkSSLSupport() {
    if (!QSslSocket::supportsSsl()) {
        qCCritical(lcMain) << "SSL support is not available. Secure connections will not work.";
        qCCritical(lcMain) << "SSL library version:" << QSslSocket::sslLibraryVersionString();
    } else {
        qCInfo(lcMain) << "SSL support available. Library version:" << QSslSocket::sslLibraryVersionString();
    }
}

void printBanner() {
    qCInfo(lcMain) << "╔══════════════════════════════════════════════════════════════╗";
    qCInfo(lcMain) << "║                    SecureChat Client                         ║";
    qCInfo(lcMain) << "║              Secure Real-Time Messaging                     ║";
    qCInfo(lcMain) << "║                     Version 1.0.0                           ║";
    qCInfo(lcMain) << "╚══════════════════════════════════════════════════════════════╝";
}

int main(int argc, char *argv[]) {
    // Create application
    QApplication app(argc, argv);
    
    // Setup application properties
    setupApplication(app);
    
    // Setup logging
    setupLogging();
    
    // Print banner
    printBanner();
    
    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("SecureChat Client - Secure Real-Time Messaging Application");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption debugOption(QStringList() << "d" << "debug",
                                  "Enable debug logging");
    parser.addOption(debugOption);
    
    QCommandLineOption serverOption(QStringList() << "s" << "server",
                                   "Server address to connect to",
                                   "address", "localhost");
    parser.addOption(serverOption);
    
    QCommandLineOption portOption(QStringList() << "p" << "port",
                                 "Server port to connect to",
                                 "port", "8080");
    parser.addOption(portOption);
    
    QCommandLineOption themeOption(QStringList() << "t" << "theme",
                                  "Application theme (light|dark|auto)",
                                  "theme", "auto");
    parser.addOption(themeOption);
    
    QCommandLineOption noTrayOption("no-tray",
                                   "Disable system tray integration");
    parser.addOption(noTrayOption);
    
    parser.process(app);
    
    // Configure logging level
    if (parser.isSet(debugOption)) {
        utils::ClientLogger::setLogLevel(utils::LogLevel::Debug);
        qCInfo(lcMain) << "Debug logging enabled";
    }
    
    // Check SSL support
    checkSSLSupport();
    
    // Setup translations
    setupTranslations(app);
    
    // Initialize theme manager
    utils::ThemeManager::initialize();
    
    // Apply theme
    QString theme = parser.value(themeOption);
    if (theme == "light") {
        utils::ThemeManager::setTheme(utils::Theme::Light);
    } else if (theme == "dark") {
        utils::ThemeManager::setTheme(utils::Theme::Dark);
    } else {
        utils::ThemeManager::setTheme(utils::Theme::Auto);
    }
    
    qCInfo(lcMain) << "Application theme set to:" << theme;
    
    try {
        // Create main window
        MainWindow window;
        
        // Apply command line options
        if (parser.isSet(serverOption)) {
            // Set default server from command line
            // This would be handled in the main window's settings
        }
        
        if (parser.isSet(portOption)) {
            // Set default port from command line
            bool ok;
            int port = parser.value(portOption).toInt(&ok);
            if (ok && port > 0 && port <= 65535) {
                // Set port in settings
            } else {
                qCWarning(lcMain) << "Invalid port number:" << parser.value(portOption);
            }
        }
        
        // Configure system tray
        if (parser.isSet(noTrayOption)) {
            // Disable system tray integration
            qCInfo(lcMain) << "System tray integration disabled";
        }
        
        // Show main window
        window.show();
        
        qCInfo(lcMain) << "SecureChat Client started successfully";
        
        // Run application event loop
        int result = app.exec();
        
        qCInfo(lcMain) << "SecureChat Client shutting down with exit code:" << result;
        return result;
        
    } catch (const std::exception &e) {
        qCCritical(lcMain) << "Unhandled exception:" << e.what();
        return 1;
    } catch (...) {
        qCCritical(lcMain) << "Unknown exception occurred";
        return 1;
    }
}