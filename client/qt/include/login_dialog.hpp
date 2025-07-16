#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QGroupBox>
#include <QtCore/QTimer>
#include <memory>

#include "network/client_connection.hpp"
#include "utils/client_logger.hpp"

namespace securechat::client {

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog() override;

    // Getters
    QString getUsername() const;
    QString getPassword() const;
    QString getServerHost() const;
    int getServerPort() const;
    bool getRememberCredentials() const;
    bool getAutoConnect() const;

    // Setters
    void setServerHost(const QString &host);
    void setServerPort(int port);
    void setRememberCredentials(bool remember);
    void setAutoConnect(bool autoConnect);

public slots:
    void setLoginInProgress(bool inProgress);
    void setLoginError(const QString &error);
    void clearError();

signals:
    void loginRequested(const QString &username, const QString &password,
                       const QString &host, int port);
    void registerRequested(const QString &username, const QString &password,
                          const QString &email, const QString &host, int port);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onCancelClicked();
    void onShowRegisterToggled();
    void onShowPasswordToggled();
    void onAdvancedToggled();
    void onTestConnectionClicked();
    void onFormChanged();
    void clearErrorAfterDelay();

private:
    void setupUI();
    void setupConnections();
    void setupValidation();
    void loadSettings();
    void saveSettings();
    
    bool validateForm();
    void setFormEnabled(bool enabled);
    void resetForm();
    void focusFirstEmptyField();
    
    // UI Components
    QVBoxLayout *m_mainLayout;
    
    // Logo and title
    QLabel *m_logoLabel;
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;
    
    // Login form
    QGroupBox *m_loginGroupBox;
    QFormLayout *m_loginFormLayout;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_showPasswordButton;
    QCheckBox *m_rememberCheckBox;
    QCheckBox *m_autoConnectCheckBox;
    
    // Registration form (hidden by default)
    QGroupBox *m_registerGroupBox;
    QFormLayout *m_registerFormLayout;
    QLineEdit *m_regUsernameEdit;
    QLineEdit *m_regPasswordEdit;
    QLineEdit *m_regConfirmPasswordEdit;
    QLineEdit *m_regEmailEdit;
    
    // Server settings (collapsible)
    QGroupBox *m_serverGroupBox;
    QFormLayout *m_serverFormLayout;
    QLineEdit *m_serverHostEdit;
    QLineEdit *m_serverPortEdit;
    QPushButton *m_testConnectionButton;
    QLabel *m_connectionStatusLabel;
    
    // Error display
    QLabel *m_errorLabel;
    QTimer *m_errorTimer;
    
    // Progress indicator
    QProgressBar *m_progressBar;
    
    // Buttons
    QHBoxLayout *m_buttonLayout;
    QPushButton *m_loginButton;
    QPushButton *m_registerButton;
    QPushButton *m_cancelButton;
    QPushButton *m_showRegisterButton;
    QPushButton *m_advancedButton;
    
    // State
    bool m_isLoginMode;
    bool m_isAdvancedMode;
    bool m_isPasswordVisible;
    bool m_loginInProgress;
    
    // Network testing
    std::unique_ptr<network::ClientConnection> m_testConnection;
    
    // Logging
    utils::ClientLogger m_logger;
    
    // Styling
    void applyStyles();
    void updateButtonStates();
    
    // Constants
    static constexpr int ERROR_DISPLAY_DURATION_MS = 5000;
    static constexpr int MIN_PASSWORD_LENGTH = 6;
    static constexpr int MAX_USERNAME_LENGTH = 32;
};

} // namespace securechat::client