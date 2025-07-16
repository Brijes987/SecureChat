#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QMenu>
#include <QtCore/QTimer>
#include <QtCore/QStringList>
#include <QtGui/QPixmap>

#include "utils/client_logger.hpp"

namespace securechat::client {

enum class UserStatus {
    Online,
    Away,
    Busy,
    Offline
};

struct UserInfo {
    QString username;
    UserStatus status;
    QString statusMessage;
    QDateTime lastSeen;
    bool isTyping;
    QPixmap avatar;
    
    UserInfo() : status(UserStatus::Offline), isTyping(false) {}
};

class UserListWidget : public QWidget {
    Q_OBJECT

public:
    explicit UserListWidget(QWidget *parent = nullptr);
    ~UserListWidget() override;

    // User management
    void updateUserList(const QStringList &users);
    void addUser(const QString &username);
    void removeUser(const QString &username);
    void updateUserStatus(const QString &username, UserStatus status, const QString &statusMessage = QString());
    void setUserTyping(const QString &username, bool typing);
    void clearUsers();

    // UI settings
    void setShowAvatars(bool show);
    void setShowStatusMessages(bool show);
    void setCompactMode(bool compact);

    // Getters
    QStringList getOnlineUsers() const;
    int getUserCount() const;
    QString getSelectedUser() const;

signals:
    void userSelected(const QString &username);
    void userDoubleClicked(const QString &username);
    void privateMessageRequested(const QString &username);
    void userInfoRequested(const QString &username);
    void userBlocked(const QString &username);
    void userUnblocked(const QString &username);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onUserItemClicked(QListWidgetItem *item);
    void onUserItemDoubleClicked(QListWidgetItem *item);
    void onSearchTextChanged(const QString &text);
    void onRefreshClicked();
    void onUserContextMenu(const QPoint &pos);
    void onPrivateMessageAction();
    void onUserInfoAction();
    void onBlockUserAction();
    void onUnblockUserAction();
    void updateTypingIndicators();

private:
    void setupUI();
    void setupConnections();
    void createUserItem(const UserInfo &userInfo);
    void updateUserItem(const QString &username);
    QListWidgetItem* findUserItem(const QString &username);
    void filterUsers();
    void sortUsers();
    
    QString getUserStatusText(UserStatus status) const;
    QIcon getUserStatusIcon(UserStatus status) const;
    QString formatLastSeen(const QDateTime &lastSeen) const;
    
    // UI Components
    QVBoxLayout *m_mainLayout;
    
    // Header
    QHBoxLayout *m_headerLayout;
    QLabel *m_titleLabel;
    QLabel *m_countLabel;
    QPushButton *m_refreshButton;
    
    // Search
    QLineEdit *m_searchEdit;
    
    // User list
    QListWidget *m_userList;
    
    // Context menu
    QMenu *m_contextMenu;
    QAction *m_privateMessageAction;
    QAction *m_userInfoAction;
    QAction *m_blockUserAction;
    QAction *m_unblockUserAction;
    
    // Data
    QMap<QString, UserInfo> m_users;
    QStringList m_blockedUsers;
    QString m_currentFilter;
    QString m_selectedUser;
    
    // Settings
    bool m_showAvatars;
    bool m_showStatusMessages;
    bool m_compactMode;
    
    // Timers
    QTimer *m_typingTimer;
    
    // Logging
    utils::ClientLogger m_logger;
    
    // Constants
    static constexpr int TYPING_INDICATOR_TIMEOUT_MS = 3000;
    static constexpr int REFRESH_INTERVAL_MS = 30000;
};

} // namespace securechat::client