#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace securechat::client::network {

enum class MessageType {
    Text,
    Image,
    File,
    Audio,
    Video,
    System,
    Typing,
    ReadReceipt,
    Delivery,
    Error
};

enum class MessageStatus {
    Pending,
    Sent,
    Delivered,
    Read,
    Failed
};

struct Message {
    QString id;
    QString sender;
    QString recipient;
    QString content;
    MessageType type;
    MessageStatus status;
    QDateTime timestamp;
    bool isEncrypted;
    QJsonObject metadata;
    
    // File-specific fields
    QString fileName;
    QString filePath;
    qint64 fileSize;
    QString mimeType;
    QString checksum;
    
    // Encryption fields
    QString encryptionAlgorithm;
    QString keyId;
    QByteArray iv;
    QByteArray hmac;
    
    Message() : type(MessageType::Text), status(MessageStatus::Pending), 
                isEncrypted(false), fileSize(0) {}
};

class MessageHandler : public QObject {
    Q_OBJECT

public:
    explicit MessageHandler(QObject *parent = nullptr);
    ~MessageHandler() override;

    // Message creation
    static QJsonObject createTextMessage(const QString &content, const QString &recipient = QString());
    static QJsonObject createFileMessage(const QString &filePath, const QString &recipient = QString());
    static QJsonObject createTypingMessage(bool typing);
    static QJsonObject createReadReceiptMessage(const QString &messageId);
    static QJsonObject createAuthMessage(const QString &username, const QString &password);
    static QJsonObject createUserListRequest();
    static QJsonObject createUserStatusRequest(const QString &username);

    // Message parsing
    static Message parseMessage(const QJsonObject &json);
    static QStringList parseUserList(const QJsonObject &json);
    static QString parseError(const QJsonObject &json);
    static bool parseTypingIndicator(const QJsonObject &json, QString &username, bool &typing);
    static bool parseAuthResponse(const QJsonObject &json, QString &token, QString &error);

    // Message validation
    static bool validateMessage(const QJsonObject &json);
    static bool validateTextMessage(const QJsonObject &json);
    static bool validateFileMessage(const QJsonObject &json);
    static bool validateAuthMessage(const QJsonObject &json);

    // Message serialization
    static QByteArray serializeMessage(const QJsonObject &json);
    static QJsonObject deserializeMessage(const QByteArray &data, bool &success);

    // Message utilities
    static QString generateMessageId();
    static QString getMessageTypeString(MessageType type);
    static MessageType getMessageTypeFromString(const QString &typeStr);
    static QString getMessageStatusString(MessageStatus status);
    static MessageStatus getMessageStatusFromString(const QString &statusStr);
    static QDateTime parseTimestamp(const QString &timestampStr);
    static QString formatTimestamp(const QDateTime &timestamp);

    // File handling
    static QString calculateFileChecksum(const QString &filePath);
    static QString getMimeType(const QString &filePath);
    static bool isImageFile(const QString &filePath);
    static bool isAudioFile(const QString &filePath);
    static bool isVideoFile(const QString &filePath);
    static qint64 getFileSize(const QString &filePath);

    // Encryption helpers
    static QJsonObject addEncryptionMetadata(const QJsonObject &message, 
                                           const QString &algorithm,
                                           const QString &keyId,
                                           const QByteArray &iv,
                                           const QByteArray &hmac);
    static bool extractEncryptionMetadata(const QJsonObject &message,
                                        QString &algorithm,
                                        QString &keyId,
                                        QByteArray &iv,
                                        QByteArray &hmac);

    // Message compression
    static QByteArray compressMessage(const QByteArray &data);
    static QByteArray decompressMessage(const QByteArray &compressedData);

    // Protocol constants
    static const QString PROTOCOL_VERSION;
    static const QString MESSAGE_TYPE_TEXT;
    static const QString MESSAGE_TYPE_FILE;
    static const QString MESSAGE_TYPE_IMAGE;
    static const QString MESSAGE_TYPE_AUDIO;
    static const QString MESSAGE_TYPE_VIDEO;
    static const QString MESSAGE_TYPE_SYSTEM;
    static const QString MESSAGE_TYPE_TYPING;
    static const QString MESSAGE_TYPE_READ_RECEIPT;
    static const QString MESSAGE_TYPE_DELIVERY;
    static const QString MESSAGE_TYPE_AUTH;
    static const QString MESSAGE_TYPE_USER_LIST;
    static const QString MESSAGE_TYPE_USER_STATUS;
    static const QString MESSAGE_TYPE_ERROR;

signals:
    void messageProcessed(const Message &message);
    void messageValidationFailed(const QString &error);

private:
    static QJsonObject createBaseMessage(const QString &type);
    static bool validateBaseMessage(const QJsonObject &json);
    static QString sanitizeContent(const QString &content);
    static bool isValidUsername(const QString &username);
    static bool isValidMessageId(const QString &messageId);
    
    // File validation
    static bool isAllowedFileType(const QString &filePath);
    static bool isFileSizeValid(qint64 size);
    
    // Constants
    static constexpr qint64 MAX_FILE_SIZE = 50 * 1024 * 1024; // 50MB
    static constexpr int MAX_MESSAGE_LENGTH = 4096;
    static constexpr int MAX_USERNAME_LENGTH = 32;
    static constexpr int MESSAGE_ID_LENGTH = 32;
};

} // namespace securechat::client::network

Q_DECLARE_METATYPE(securechat::client::network::Message)
Q_DECLARE_METATYPE(securechat::client::network::MessageType)
Q_DECLARE_METATYPE(securechat::client::network::MessageStatus)