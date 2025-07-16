#include "main_window.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtGui/QCloseEvent>
#include <QtGui/QDesktopServices>

namespace securechat::client {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_chatWidget(nullptr)
    , m_userListWidget(nullptr)
    , m_loginDialog(nullptr)
    , m_settingsDialog(nullptr)
    , m_trayIcon(nullptr)
    , m_trayIconMenu(nullptr)
    , m_connection(nullptr)
    , m_isConnected(false)
    , m_reconnectTimer(new QTimer(this))
    , m_keepAliveTimer(new QTimer(this))
    , m_logger("MainWindow") {
    
    m_logger.info("Initializing SecureChat Client Main Window");
    
    setupUI();
    setupMenuBar();
    setupStatusBar();
    setupSystemTray();
    setupConnections();
    
    loadSettings();
    applyTheme();
    
    // Initialize connection
    m_connection = std::make_unique<network::ClientConnection>(this);
    
    // Setup timers
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(RECONNECT_INTERVAL_MS);
    
    m_keepAliveTimer->setInterval(KEEPALIVE_INTERVAL_MS);
    
    // Connect network signals
    connect(m_connection.get(), &network::ClientConnection::connected,
            this, &MainWindow::onLoginSuccessful);
    connect(m_connection.get(), &network::ClientConnection::disconnected,
            this, &MainWindow::onDisconnected);
    connect(m_connection.get(), &network::ClientConnection::connectionError,
            this, &MainWindow::onConnectionError);
    connect(m_connection.get(), &network::ClientConnection::messageReceived,
            this, &MainWindow::onMessageReceived);
    connect(m_connection.get(), &network::ClientConnection::userListReceived,
            this, &MainWindow::onUserListUpdated);
    connect(m_connection.get(), &network::ClientConnection::typingIndicatorReceived,
            this, &MainWindow::onTypingIndicator);
    
    // Show login dialog if not auto-connecting
    if (!m_autoConnect) {
        QTimer::singleShot(500, this, &MainWindow::showLoginDialog);
    } else {
        connectToServer();
    }
    
    m_logger.info("Main window initialized successfully");
}

MainWindow::~MainWindow() {
    m_logger.info("Destroying main window");
    saveSettings();
    
    if (m_connection && m_connection->isConnected()) {
        m_connection->disconnectFromServer();
    }
}

void MainWindow::setupUI() {
    // Create central widget
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    // Create main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    
    // Create chat widget
    m_chatWidget = new ChatWidget(m_mainSplitter);
    m_mainSplitter->addWidget(m_chatWidget);
    
    // Create user list widget
    m_userListWidget = new UserListWidget(m_mainSplitter);
    m_mainSplitter->addWidget(m_userListWidget);
    
    // Set splitter proportions (chat takes 75%, user list takes 25%)
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setSizes({600, 200});
    
    // Layout
    QHBoxLayout *layout = new QHBoxLayout(m_centralWidget);
    layout->addWidget(m_mainSplitter);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Set window properties
    setWindowTitle("SecureChat - Secure Real-Time Messaging");
    setMinimumSize(800, 600);
    resize(1200, 800);
    
    // Center window on screen
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
}

void MainWindow::setupMenuBar() {
    // File menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    
    m_connectAction = m_fileMenu->addAction(tr("&Connect"), this, &MainWindow::connectToServer);
    m_connectAction->setShortcut(QKeySequence::New);
    m_connectAction->setIcon(QIcon(":/icons/connect.png"));
    
    m_disconnectAction = m_fileMenu->addAction(tr("&Disconnect"), this, &MainWindow::disconnectFromServer);
    m_disconnectAction->setShortcut(QKeySequence("Ctrl+D"));
    m_disconnectAction->setIcon(QIcon(":/icons/disconnect.png"));
    m_disconnectAction->setEnabled(false);
    
    m_fileMenu->addSeparator();
    
    m_exportAction = m_fileMenu->addAction(tr("&Export Chat History..."), this, &MainWindow::exportChatHistory);
    m_exportAction->setShortcut(QKeySequence("Ctrl+E"));
    m_exportAction->setIcon(QIcon(":/icons/export.png"));
    
    m_clearHistoryAction = m_fileMenu->addAction(tr("&Clear Chat History"), this, &MainWindow::clearChatHistory);
    m_clearHistoryAction->setIcon(QIcon(":/icons/clear.png"));
    
    m_fileMenu->addSeparator();
    
    m_exitAction = m_fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setIcon(QIcon(":/icons/exit.png"));
    
    // Edit menu
    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    
    m_settingsAction = m_editMenu->addAction(tr("&Settings..."), this, &MainWindow::showSettings);
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    m_settingsAction->setIcon(QIcon(":/icons/settings.png"));
    
    // View menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    
    QAction *toggleUserListAction = m_viewMenu->addAction(tr("Toggle &User List"));
    toggleUserListAction->setShortcut(QKeySequence("Ctrl+U"));
    toggleUserListAction->setCheckable(true);
    toggleUserListAction->setChecked(true);
    connect(toggleUserListAction, &QAction::toggled, m_userListWidget, &QWidget::setVisible);
    
    // Help menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    
    m_aboutAction = m_helpMenu->addAction(tr("&About SecureChat"), this, &MainWindow::showAbout);
    m_aboutAction->setIcon(QIcon(":/icons/about.png"));
    
    QAction *aboutQtAction = m_helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
    aboutQtAction->setIcon(QIcon(":/icons/qt.png"));
}

void MainWindow::setupStatusBar() {
    // Connection status
    m_connectionStatusLabel = new QLabel(tr("Disconnected"));
    m_connectionStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    statusBar()->addWidget(m_connectionStatusLabel);
    
    statusBar()->addPermanentWidget(new QLabel("|"));
    
    // User count
    m_userCountLabel = new QLabel(tr("Users: 0"));
    statusBar()->addPermanentWidget(m_userCountLabel);
    
    statusBar()->addPermanentWidget(new QLabel("|"));
    
    // Encryption status
    m_encryptionStatusLabel = new QLabel(tr("🔒 Encrypted"));
    m_encryptionStatusLabel->setStyleSheet("QLabel { color: green; }");
    statusBar()->addPermanentWidget(m_encryptionStatusLabel);
}

void MainWindow::setupSystemTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        m_logger.warn("System tray is not available");
        return;
    }
    
    // Create tray icon
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/app-icon.png"));
    m_trayIcon->setToolTip("SecureChat - Secure Real-Time Messaging");
    
    // Create tray menu
    m_trayIconMenu = new QMenu(this);
    
    QAction *showAction = m_trayIconMenu->addAction(tr("Show"), this, &QWidget::showNormal);
    showAction->setIcon(QIcon(":/icons/show.png"));
    
    QAction *hideAction = m_trayIconMenu->addAction(tr("Hide"), this, &QWidget::hide);
    hideAction->setIcon(QIcon(":/icons/hide.png"));
    
    m_trayIconMenu->addSeparator();
    
    QAction *quitAction = m_trayIconMenu->addAction(tr("Quit"), this, &QWidget::close);
    quitAction->setIcon(QIcon(":/icons/exit.png"));
    
    m_trayIcon->setContextMenu(m_trayIconMenu);
    
    // Connect signals
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &MainWindow::messageClicked);
    
    m_trayIcon->show();
}

void MainWindow::setupConnections() {
    // Chat widget connections
    connect(m_chatWidget, &ChatWidget::messageToSend, this, [this](const QString &content) {
        if (m_connection && m_connection->isConnected()) {
            m_connection->sendMessage(content);
        }
    });
    
    connect(m_chatWidget, &ChatWidget::fileToSend, this, [this](const QString &filePath) {
        if (m_connection && m_connection->isConnected()) {
            m_connection->sendFile(filePath);
        }
    });
    
    connect(m_chatWidget, &ChatWidget::typingStarted, this, [this]() {
        if (m_connection && m_connection->isConnected()) {
            m_connection->sendTypingIndicator(true);
        }
    });
    
    connect(m_chatWidget, &ChatWidget::typingStopped, this, [this]() {
        if (m_connection && m_connection->isConnected()) {
            m_connection->sendTypingIndicator(false);
        }
    });
    
    // Timer connections
    connect(m_reconnectTimer, &QTimer::timeout, this, &MainWindow::connectToServer);
    connect(m_keepAliveTimer, &QTimer::timeout, this, [this]() {
        if (m_connection && m_connection->isConnected()) {
            // Send keep-alive message
            m_connection->requestUserList();
        }
    });
}

void MainWindow::showLoginDialog() {
    if (!m_loginDialog) {
        m_loginDialog = std::make_unique<LoginDialog>(this);
        
        // Set saved server settings
        m_loginDialog->setServerHost(m_serverHost);
        m_loginDialog->setServerPort(m_serverPort);
        
        connect(m_loginDialog.get(), &LoginDialog::loginRequested,
                this, [this](const QString &username, const QString &password, const QString &host, int port) {
                    m_serverHost = host;
                    m_serverPort = port;
                    m_currentUsername = username;
                    
                    if (m_connection) {
                        m_connection->connectToServer(host, port);
                        m_connection->authenticate(username, password);
                    }
                });
    }
    
    m_loginDialog->show();
    m_loginDialog->raise();
    m_loginDialog->activateWindow();
}

void MainWindow::onLoginSuccessful(const QString &username, const QString &token) {
    m_logger.info("Login successful for user: {}", username);
    
    m_currentUsername = username;
    m_authToken = token;
    m_isConnected = true;
    
    setConnectedState(true);
    
    if (m_loginDialog) {
        m_loginDialog->hide();
    }
    
    // Update UI
    m_chatWidget->setCurrentUser(username);
    m_chatWidget->setConnectionStatus(true);
    
    // Start keep-alive timer
    m_keepAliveTimer->start();
    
    // Request initial user list
    if (m_connection) {
        m_connection->requestUserList();
    }
    
    showNotification(tr("Connected"), tr("Successfully connected to SecureChat server"));
}

void MainWindow::onLoginFailed(const QString &error) {
    m_logger.error("Login failed: {}", error);
    
    if (m_loginDialog) {
        m_loginDialog->setLoginError(error);
        m_loginDialog->setLoginInProgress(false);
    }
    
    QMessageBox::warning(this, tr("Login Failed"), 
                        tr("Failed to login: %1").arg(error));
}

void MainWindow::onDisconnected() {
    m_logger.info("Disconnected from server");
    
    m_isConnected = false;
    setConnectedState(false);
    
    // Stop timers
    m_keepAliveTimer->stop();
    
    // Update UI
    m_chatWidget->setConnectionStatus(false);
    m_userListWidget->clearUsers();
    m_onlineUsers.clear();
    
    updateConnectionStatus();
    updateUserCount();
    
    showNotification(tr("Disconnected"), tr("Connection to server lost"));
    
    // Auto-reconnect if enabled
    if (m_autoConnect && !m_reconnectTimer->isActive()) {
        m_reconnectTimer->start();
    }
}

void MainWindow::onConnectionError(const QString &error) {
    m_logger.error("Connection error: {}", error);
    
    if (m_loginDialog) {
        m_loginDialog->setLoginError(error);
        m_loginDialog->setLoginInProgress(false);
    }
    
    QMessageBox::critical(this, tr("Connection Error"), 
                         tr("Connection error: %1").arg(error));
}

void MainWindow::onMessageReceived(const network::Message &message) {
    m_logger.debug("Message received from: {}", message.sender);
    
    // Convert network message to chat message
    ChatWidget::Message chatMessage;
    chatMessage.id = message.id;
    chatMessage.sender = message.sender;
    chatMessage.content = message.content;
    chatMessage.timestamp = message.timestamp;
    chatMessage.type = static_cast<ChatWidget::MessageType>(message.type);
    chatMessage.status = static_cast<ChatWidget::MessageStatus>(message.status);
    chatMessage.isEncrypted = message.isEncrypted;
    
    m_chatWidget->addMessage(chatMessage);
    
    // Show notification if window is not active
    if (!isActiveWindow()) {
        showNotification(tr("New Message from %1").arg(message.sender), 
                        message.content);
    }
}

void MainWindow::onUserListUpdated(const QStringList &users) {
    m_logger.debug("User list updated, {} users online", users.size());
    
    m_onlineUsers = users;
    m_userListWidget->updateUserList(users);
    updateUserCount();
}

void MainWindow::onTypingIndicator(const QString &username, bool typing) {
    m_chatWidget->setTypingIndicator(username, typing);
}

void MainWindow::connectToServer() {
    if (m_isConnected || !m_connection) {
        return;
    }
    
    m_logger.info("Connecting to server: {}:{}", m_serverHost, m_serverPort);
    
    m_connection->connectToServer(m_serverHost, m_serverPort);
}

void MainWindow::disconnectFromServer() {
    if (!m_isConnected || !m_connection) {
        return;
    }
    
    m_logger.info("Disconnecting from server");
    
    m_connection->disconnectFromServer();
}

void MainWindow::setConnectedState(bool connected) {
    m_isConnected = connected;
    
    // Update actions
    m_connectAction->setEnabled(!connected);
    m_disconnectAction->setEnabled(connected);
    
    updateConnectionStatus();
}

void MainWindow::updateConnectionStatus() {
    if (m_isConnected) {
        m_connectionStatusLabel->setText(tr("Connected to %1:%2").arg(m_serverHost).arg(m_serverPort));
        m_connectionStatusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
    } else {
        m_connectionStatusLabel->setText(tr("Disconnected"));
        m_connectionStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    }
}

void MainWindow::updateUserCount() {
    m_userCountLabel->setText(tr("Users: %1").arg(m_onlineUsers.size()));
}

void MainWindow::showNotification(const QString &title, const QString &message) {
    if (!m_showNotifications) {
        return;
    }
    
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 5000);
    }
}

void MainWindow::loadSettings() {
    QSettings settings;
    
    // Window geometry
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    // Connection settings
    m_serverHost = settings.value("connection/host", "localhost").toString();
    m_serverPort = settings.value("connection/port", 8080).toInt();
    m_autoConnect = settings.value("connection/autoConnect", false).toBool();
    
    // UI settings
    m_minimizeToTray = settings.value("ui/minimizeToTray", true).toBool();
    m_showNotifications = settings.value("ui/showNotifications", true).toBool();
    m_theme = settings.value("ui/theme", "auto").toString();
    
    // Splitter state
    m_mainSplitter->restoreState(settings.value("ui/splitterState").toByteArray());
}

void MainWindow::saveSettings() {
    QSettings settings;
    
    // Window geometry
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    
    // Connection settings
    settings.setValue("connection/host", m_serverHost);
    settings.setValue("connection/port", m_serverPort);
    settings.setValue("connection/autoConnect", m_autoConnect);
    
    // UI settings
    settings.setValue("ui/minimizeToTray", m_minimizeToTray);
    settings.setValue("ui/showNotifications", m_showNotifications);
    settings.setValue("ui/theme", m_theme);
    
    // Splitter state
    settings.setValue("ui/splitterState", m_mainSplitter->saveState());
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_minimizeToTray && m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
        
        if (m_showNotifications) {
            showNotification(tr("SecureChat"), tr("Application was minimized to tray"));
        }
    } else {
        saveSettings();
        event->accept();
    }
}

void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
    
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && m_minimizeToTray && m_trayIcon && m_trayIcon->isVisible()) {
            hide();
        }
    }
}

// Implement remaining slots...
void MainWindow::showSettings() {
    if (!m_settingsDialog) {
        m_settingsDialog = std::make_unique<SettingsDialog>(this);
    }
    
    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}

void MainWindow::showAbout() {
    QMessageBox::about(this, tr("About SecureChat"),
                      tr("<h2>SecureChat Client v1.0.0</h2>"
                         "<p>Secure Real-Time Messaging Application</p>"
                         "<p>Built with Qt %1 and OpenSSL</p>"
                         "<p>Copyright © 2024 SecureChat Team</p>").arg(QT_VERSION_STR));
}

void MainWindow::exportChatHistory() {
    QString fileName = QFileDialog::getSaveFileName(this,
                                                   tr("Export Chat History"),
                                                   QString("chat_history_%1.txt").arg(QDate::currentDate().toString("yyyy-MM-dd")),
                                                   tr("Text Files (*.txt);;HTML Files (*.html)"));
    
    if (!fileName.isEmpty()) {
        m_chatWidget->exportMessages(fileName);
        showNotification(tr("Export Complete"), tr("Chat history exported to %1").arg(fileName));
    }
}

void MainWindow::clearChatHistory() {
    int ret = QMessageBox::question(this, tr("Clear Chat History"),
                                   tr("Are you sure you want to clear all chat history? This action cannot be undone."),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_chatWidget->clearMessages();
        showNotification(tr("History Cleared"), tr("Chat history has been cleared"));
    }
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        if (isVisible()) {
            hide();
        } else {
            showNormal();
            raise();
            activateWindow();
        }
        break;
    default:
        break;
    }
}

void MainWindow::messageClicked() {
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::applyTheme() {
    // Theme application would be handled by ThemeManager
    // This is a placeholder for theme-specific styling
}

} // namespace securechat::client