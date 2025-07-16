#pragma once

#include <QtCore/QObject>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QSslSocket>
#include <QtCore/QTimer>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QQueue>
#include <memory>

#include "message_handler.hpp"
#include "encryption_client.hpp"
#include "../utils/client_logger.hpp"

namespace securechat::client::network {

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Authenticating,
    Authenticated,
    Disconnecting,
    Error
};

enum class MessagePriority {
    Low,
    Normal,
    High,
    Critical
};

struct QueuedMessage {
    QJsonObject data;
    MessagePriority priority;
    QDateTime timestamp;
    int retryCount;
    QString messageId;
};

class ClientConnection : public QObject {
    Q_OBJECT

public:
    explicit ClientConnection(QObject *parent = nullptr);
    ~ClientConnection() override;

    // Connection management
    void connectToServer(const QString &host, int port, bool useTLS = true);
    void disconnectFromServer();
    void reconnect();

    // Authentication
    void authenticate(const QString &username, const QString &password);
    void logout();

    // Message sending
    void sendMessage(const QString &content, const QString &recipient = QString());
    void sendFile(const QString &filePath, const QString &recipient = QString());
    void sendTypingIndicator(bool typing);
    void sendReadReceipt(const QString &messageId);

    // User management
    void requestUserList();
    void requestUserStatus(const QString &username);

    // State queries
    ConnectionState getState() const { return m_state; }
    bool isConnected() const { return m_state == ConnectionState::Authenticated; }
    bool isEncrypted() const { return m_encryptionEnabled && m_encryptionClient->isInitialized(); }
    QString getCurrentUser() const { return m_currentUsername; }
    QString getAuthToken() const { return m_authToken; }

    // Statistics
    qint64 getBytesSent() const { return m_bytesSent; }
    qint64 getBytesReceived() const { return m_bytesReceived; }
    int getPendingMessages() const { return m_messageQueue.size(); }
    QDateTime getLastActivity() const { return m_lastActivity; }

    // Settings
    void setAutoReconnect(bool enabled) { m_autoReconnect = enabled; }
    void setReconnectInterval(int intervalMs) { m_reconnectInterval = intervalMs; }
    void setKeepAliveInterval(int intervalMs);
    void setMessageTimeout(int timeoutMs) { m_messageTimeout = timeoutMs; }
    void setEncryptionEnabled(bool enabled);

signals:
    // Connection events
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    void stateChanged(ConnectionState state);

    // Authentication events
    void authenticationSuccessful(const QString &username, const QString &token);
    void authenticationFailed(const QString &error);

    // Message events
    void messageReceived(const Message &message);
    void messageSent(const QString &messageId);
    void messageDelivered(const QString &messageId);
    void messageRead(const QString &messageId);
    void messageFailed(const QString &messageId, const QString &error);

    // User events
    void userListReceived(const QStringList &users);
    void userStatusChanged(const QString &username, const QString &status);
    void typingIndicatorReceived(const QString &username, bool typing);

    // File transfer events
    void fileTransferStarted(const QString &transferId, const QString &filename, qint64 size);
    void fileTransferProgress(const QString &transferId, qint64 bytesTransferred, qint64 totalBytes);
    void fileTransferCompleted(const QString &transferId);
    void fileTransferFailed(const QString &transferId, const QString &error);

    // Encryption events
    void encryptionStatusChanged(bool enabled);
    void keyExchangeCompleted();
    void keyExchangeFailed(const QString &error);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketReadyRead();
    void onSocketBytesWritten(qint64 bytes);
    void onSslErrors(const QList<QSslError> &errors);
    
    void onKeepAliveTimer();
    void onReconnectTimer();
    void onMessageTimeoutTimer();
    void processMessageQueue();

private:
    void setupSocket();
    void setupTimers();
    void setState(ConnectionState state);
    void resetConnection();
    
    // Message processing
    void processIncomingData();
    void handleMessage(const QJsonObject &message);
    void sendQueuedMessage(const QueuedMessage &queuedMsg);
    void queueMessage(const QJsonObject &message, MessagePriority priority = MessagePriority::Normal);
    
    // Protocol handlers
    void handleAuthResponse(const QJsonObject &data);
    void handleChatMessage(const QJsonObject &data);
    void handleUserList(const QJsonObject &data);
    void handleUserStatus(const QJsonObject &data);
    void handleTypingIndicator(const QJsonObject &data);
    void handleFileTransfer(const QJsonObject &data);
    void handleSystemMessage(const QJsonObject &data);
    void handleError(const QJsonObject &data);
    
    // Encryption
    void initializeEncryption();
    void performKeyExchange();
    QByteArray encryptMessage(const QByteArray &data);
    QByteArray decryptMessage(const QByteArray &data);
    
    // Utility
    QString generateMessageId() const;
    QJsonObject createMessage(const QString &type, const QJsonObject &data = QJsonObject()) const;
    void logMessage(const QString &direction, const QJsonObject &message);
    
    // Network components
    std::unique_ptr<QSslSocket> m_socket;
    std::unique_ptr<MessageHandler> m_messageHandler;
    std::unique_ptr<EncryptionClient> m_encryptionClient;
    
    // Connection state
    ConnectionState m_state;
    QString m_serverHost;
    int m_serverPort;
    bool m_useTLS;
    bool m_autoReconnect;
    int m_reconnectInterval;
    int m_reconnectAttempts;
    int m_maxReconnectAttempts;
    
    // Authentication
    QString m_currentUsername;
    QString m_authToken;
    
    // Message queue
    QQueue<QueuedMessage> m_messageQueue;
    QTimer *m_messageQueueTimer;
    int m_messageTimeout;
    
    // Timers
    QTimer *m_keepAliveTimer;
    QTimer *m_reconnectTimer;
    QTimer *m_messageTimeoutTimer;
    
    // Statistics
    qint64 m_bytesSent;
    qint64 m_bytesReceived;
    QDateTime m_lastActivity;
    QDateTime m_connectionTime;
    
    // Settings
    bool m_encryptionEnabled;
    int m_keepAliveInterval;
    
    // Buffer for incomplete messages
    QByteArray m_receiveBuffer;
    
    // Logging
    utils::ClientLogger m_logger;
    
    // Constants
    static constexpr int DEFAULT_RECONNECT_INTERVAL_MS = 5000;
    static constexpr int DEFAULT_KEEPALIVE_INTERVAL_MS = 30000;
    static constexpr int DEFAULT_MESSAGE_TIMEOUT_MS = 10000;
    static constexpr int MAX_RECONNECT_ATTEMPTS = 10;
    static constexpr int MAX_MESSAGE_QUEUE_SIZE = 1000;
    static constexpr int MESSAGE_QUEUE_PROCESS_INTERVAL_MS = 100;
};

} // namespace securechat::client::network