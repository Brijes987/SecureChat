#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QProgressBar>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QMimeData>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <memory>
#include <deque>

#include "message_widget.hpp"
#include "network/message_handler.hpp"
#include "utils/client_logger.hpp"

namespace securechat::client {

struct Message {
    QString id;
    QString sender;
    QString content;
    QDateTime timestamp;
    MessageType type;
    MessageStatus status;
    bool isEncrypted;
    QString attachmentPath;
    qint64 attachmentSize;
};

class ChatWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget() override;

    // Message management
    void addMessage(const Message &message);
    void updateMessageStatus(const QString &messageId, MessageStatus status);
    void clearMessages();
    void loadMessageHistory();
    void exportMessages(const QString &filePath);

    // User interaction
    void setCurrentUser(const QString &username);
    void setTypingIndicator(const QString &username, bool typing);
    void setConnectionStatus(bool connected);

    // Settings
    void setMaxMessages(int maxMessages);
    void setAutoScroll(bool autoScroll);
    void setShowTimestamps(bool showTimestamps);
    void setMessageEncryption(bool enabled);

signals:
    void messageToSend(const QString &content, MessageType type = MessageType::Text);
    void fileToSend(const QString &filePath);
    void typingStarted();
    void typingStopped();
    void messageRead(const QString &messageId);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onSendClicked();
    void onAttachFileClicked();
    void onEmojiClicked();
    void onMessageInputChanged();
    void onTypingTimeout();
    void onScrollToBottom();
    void onMessageContextMenu(const QPoint &pos);
    void onCopyMessage();
    void onDeleteMessage();
    void onResendMessage();

private:
    void setupUI();
    void setupConnections();
    void setupMessageArea();
    void setupInputArea();
    void setupToolbar();
    
    void sendMessage();
    void sendFile(const QString &filePath);
    void addSystemMessage(const QString &content);
    void scrollToBottom();
    void updateScrollButton();
    
    // Message rendering
    MessageWidget* createMessageWidget(const Message &message);
    void updateMessageWidget(MessageWidget *widget, const Message &message);
    QString formatTimestamp(const QDateTime &timestamp) const;
    QString formatFileSize(qint64 size) const;
    
    // Input validation
    bool validateMessage(const QString &content) const;
    bool validateFile(const QString &filePath) const;
    
    // Encryption status
    void updateEncryptionStatus();
    void showEncryptionIndicator(bool encrypted);
    
    // UI Components
    QVBoxLayout *m_mainLayout;
    
    // Message display area
    QScrollArea *m_messageScrollArea;
    QWidget *m_messageContainer;
    QVBoxLayout *m_messageLayout;
    QPushButton *m_scrollToBottomButton;
    
    // Typing indicator
    QLabel *m_typingLabel;
    QTimer *m_typingTimer;
    
    // Input area
    QFrame *m_inputFrame;
    QHBoxLayout *m_inputLayout;
    QTextEdit *m_messageInput;
    QPushButton *m_sendButton;
    QPushButton *m_attachButton;
    QPushButton *m_emojiButton;
    
    // Status bar
    QFrame *m_statusFrame;
    QHBoxLayout *m_statusLayout;
    QLabel *m_encryptionLabel;
    QLabel *m_connectionLabel;
    QLabel *m_messageCountLabel;
    
    // File upload progress
    QProgressBar *m_uploadProgressBar;
    QLabel *m_uploadStatusLabel;
    
    // Message storage
    std::deque<Message> m_messages;
    std::unordered_map<QString, MessageWidget*> m_messageWidgets;
    
    // State
    QString m_currentUser;
    bool m_isConnected;
    bool m_encryptionEnabled;
    bool m_autoScroll;
    bool m_showTimestamps;
    int m_maxMessages;
    
    // Typing state
    QStringList m_typingUsers;
    QTimer *m_typingTimeoutTimer;
    bool m_isTyping;
    
    // Context menu
    QMenu *m_messageContextMenu;
    QAction *m_copyAction;
    QAction *m_deleteAction;
    QAction *m_resendAction;
    MessageWidget *m_contextMenuWidget;
    
    // File handling
    QStringList m_supportedImageFormats;
    QStringList m_supportedFileTypes;
    qint64 m_maxFileSize;
    
    // Logging
    utils::ClientLogger m_logger;
    
    // Constants
    static constexpr int MAX_MESSAGE_LENGTH = 4096;
    static constexpr int MAX_MESSAGES_DEFAULT = 1000;
    static constexpr int TYPING_TIMEOUT_MS = 3000;
    static constexpr int SCROLL_ANIMATION_DURATION_MS = 300;
    static constexpr qint64 MAX_FILE_SIZE_DEFAULT = 50 * 1024 * 1024; // 50MB
};

} // namespace securechat::client