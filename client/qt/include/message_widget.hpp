#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QProgressBar>
#include <QtCore/QDateTime>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QPixmap>
#include <QtGui/QMovie>

#include "network/message_handler.hpp"
#include "utils/client_logger.hpp"

namespace securechat::client {

using MessageType = network::MessageType;
using MessageStatus = network::MessageStatus;

class MessageWidget : public QFrame {
    Q_OBJECT
    Q_PROPERTY(int highlightOpacity READ highlightOpacity WRITE setHighlightOpacity)

public:
    explicit MessageWidget(QWidget *parent = nullptr);
    ~MessageWidget() override;

    // Message data
    void setMessage(const network::Message &message);
    void updateStatus(MessageStatus status);
    void setHighlighted(bool highlighted);
    
    // Getters
    QString getMessageId() const { return m_messageId; }
    QString getSender() const { return m_sender; }
    MessageType getMessageType() const { return m_messageType; }
    QDateTime getTimestamp() const { return m_timestamp; }
    
    // Appearance
    void setOwnMessage(bool own);
    void setShowTimestamp(bool show);
    void setShowAvatar(bool show);
    void setCompactMode(bool compact);
    void setMaxWidth(int maxWidth);

signals:
    void messageClicked();
    void messageDoubleClicked();
    void linkClicked(const QString &url);
    void imageClicked(const QString &imagePath);
    void fileDownloadRequested(const QString &fileId);
    void messageResendRequested();
    void messageDeleteRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void onImageLoadFinished();
    void onFileDownloadClicked();
    void onResendClicked();
    void onDeleteClicked();
    void onCopyClicked();
    void onSaveImageClicked();

private:
    void setupUI();
    void setupContextMenu();
    void updateLayout();
    void updateContent();
    void updateStatusIndicator();
    void updateTimestamp();
    
    // Content rendering
    void renderTextMessage();
    void renderImageMessage();
    void renderFileMessage();
    void renderSystemMessage();
    
    // Utility methods
    QString formatMessageContent(const QString &content) const;
    QString formatFileSize(qint64 size) const;
    QString formatTimestamp(const QDateTime &timestamp) const;
    QPixmap loadAndScaleImage(const QString &imagePath, const QSize &maxSize) const;
    QIcon getFileTypeIcon(const QString &fileName) const;
    QIcon getStatusIcon(MessageStatus status) const;
    QString getStatusText(MessageStatus status) const;
    
    // Animation
    void animateHighlight();
    int highlightOpacity() const { return m_highlightOpacity; }
    void setHighlightOpacity(int opacity);
    
    // UI Components
    QHBoxLayout *m_mainLayout;
    QVBoxLayout *m_contentLayout;
    
    // Avatar
    QLabel *m_avatarLabel;
    QPixmap m_avatarPixmap;
    
    // Message content
    QFrame *m_messageFrame;
    QVBoxLayout *m_messageLayout;
    QLabel *m_senderLabel;
    QLabel *m_contentLabel;
    QLabel *m_timestampLabel;
    QLabel *m_statusLabel;
    
    // File-specific widgets
    QHBoxLayout *m_fileLayout;
    QLabel *m_fileIconLabel;
    QLabel *m_fileNameLabel;
    QLabel *m_fileSizeLabel;
    QPushButton *m_downloadButton;
    QProgressBar *m_downloadProgress;
    
    // Image-specific widgets
    QLabel *m_imageLabel;
    QMovie *m_imageMovie; // For animated GIFs
    
    // Context menu
    QMenu *m_contextMenu;
    QAction *m_copyAction;
    QAction *m_resendAction;
    QAction *m_deleteAction;
    QAction *m_saveImageAction;
    
    // Message data
    QString m_messageId;
    QString m_sender;
    QString m_content;
    MessageType m_messageType;
    MessageStatus m_messageStatus;
    QDateTime m_timestamp;
    bool m_isEncrypted;
    bool m_isOwnMessage;
    
    // File data
    QString m_fileName;
    QString m_filePath;
    qint64 m_fileSize;
    QString m_mimeType;
    
    // Appearance settings
    bool m_showTimestamp;
    bool m_showAvatar;
    bool m_compactMode;
    int m_maxWidth;
    bool m_isHighlighted;
    int m_highlightOpacity;
    
    // Animation
    QPropertyAnimation *m_highlightAnimation;
    
    // Logging
    utils::ClientLogger m_logger;
    
    // Constants
    static constexpr int DEFAULT_MAX_WIDTH = 400;
    static constexpr int AVATAR_SIZE = 32;
    static constexpr int COMPACT_AVATAR_SIZE = 24;
    static constexpr int MAX_IMAGE_WIDTH = 300;
    static constexpr int MAX_IMAGE_HEIGHT = 200;
    static constexpr int HIGHLIGHT_DURATION_MS = 1000;
};

} // namespace securechat::client