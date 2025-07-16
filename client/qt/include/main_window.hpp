#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QSystemTrayIcon>
#include <QtCore/QTimer>
#include <memory>

#include "chat_widget.hpp"
#include "user_list_widget.hpp"
#include "login_dialog.hpp"
#include "settings_dialog.hpp"
#include "network/client_connection.hpp"
#include "utils/client_logger.hpp"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QLabel;
QT_END_NAMESPACE

namespace securechat::client {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void showLoginDialog();
    void onLoginSuccessful(const QString &username, const QString &token);
    void onLoginFailed(const QString &error);
    void onDisconnected();
    void onConnectionError(const QString &error);
    void onMessageReceived(const Message &message);
    void onUserListUpdated(const QStringList &users);
    void onTypingIndicator(const QString &username, bool typing);
    
    // Menu actions
    void showSettings();
    void showAbout();
    void toggleConnection();
    void exportChatHistory();
    void clearChatHistory();
    
    // System tray
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void showMessage();
    void messageClicked();
    
    // Status updates
    void updateConnectionStatus();
    void updateUserCount();

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupSystemTray();
    void setupConnections();
    
    void connectToServer();
    void disconnectFromServer();
    void setConnectedState(bool connected);
    
    void loadSettings();
    void saveSettings();
    void applyTheme();
    
    void showNotification(const QString &title, const QString &message);
    
    // UI Components
    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    ChatWidget *m_chatWidget;
    UserListWidget *m_userListWidget;
    
    // Dialogs
    std::unique_ptr<LoginDialog> m_loginDialog;
    std::unique_ptr<SettingsDialog> m_settingsDialog;
    
    // Menu and toolbar
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_helpMenu;
    
    QAction *m_connectAction;
    QAction *m_disconnectAction;
    QAction *m_settingsAction;
    QAction *m_exportAction;
    QAction *m_clearHistoryAction;
    QAction *m_exitAction;
    QAction *m_aboutAction;
    
    // Status bar
    QLabel *m_connectionStatusLabel;
    QLabel *m_userCountLabel;
    QLabel *m_encryptionStatusLabel;
    
    // System tray
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayIconMenu;
    
    // Network
    std::unique_ptr<network::ClientConnection> m_connection;
    
    // State
    bool m_isConnected;
    QString m_currentUsername;
    QString m_authToken;
    QStringList m_onlineUsers;
    
    // Timers
    QTimer *m_reconnectTimer;
    QTimer *m_keepAliveTimer;
    
    // Settings
    QString m_serverHost;
    int m_serverPort;
    bool m_autoConnect;
    bool m_minimizeToTray;
    bool m_showNotifications;
    QString m_theme;
    
    // Logging
    utils::ClientLogger m_logger;
    
    // Constants
    static constexpr int RECONNECT_INTERVAL_MS = 5000;
    static constexpr int KEEPALIVE_INTERVAL_MS = 30000;
};

} // namespace securechat::client