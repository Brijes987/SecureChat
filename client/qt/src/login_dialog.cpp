#include "login_dialog.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtCore/QSettings>
#include <QtCore/QRegularExpression>
#include <QtCore/QRegularExpressionValidator>
#include <QtGui/QKeyEvent>
#include <QtGui/QPixmap>

namespace securechat::client {

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_logoLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_subtitleLabel(nullptr)
    , m_loginGroupBox(nullptr)
    , m_registerGroupBox(nullptr)
    , m_serverGroupBox(nullptr)
    , m_errorLabel(nullptr)
    , m_errorTimer(new QTimer(this))
    , m_progressBar(nullptr)
    , m_isLoginMode(true)
    , m_isAdvancedMode(false)
    , m_isPasswordVisible(false)
    , m_loginInProgress(false)
    , m_testConnection(nullptr)
    , m_logger("LoginDialog") {
    
    m_logger.info("Initializing login dialog");
    
    setupUI();
    setupConnections();
    setupValidation();
    loadSettings();
    applyStyles();
    
    // Configure error timer
    m_errorTimer->setSingleShot(true);
    m_errorTimer->setInterval(ERROR_DISPLAY_DURATION_MS);
    connect(m_errorTimer, &QTimer::timeout, this, &LoginDialog::clearErrorAfterDelay);
    
    m_logger.info("Login dialog initialized successfully");
}

LoginDialog::~LoginDialog() {
    saveSettings();
}

void LoginDialog::setupUI() {
    setWindowTitle(tr("SecureChat - Login"));
    setModal(true);
    setFixedSize(400, 600);
    
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(20);
    m_mainLayout->setContentsMargins(30, 30, 30, 30);
    
    // Logo and title section
    m_logoLabel = new QLabel();
    m_logoLabel->setPixmap(QPixmap(":/icons/app-logo.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_logoLabel);
    
    m_titleLabel = new QLabel(tr("SecureChat"));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("QLabel { font-size: 24px; font-weight: bold; color: #2c3e50; }");
    m_mainLayout->addWidget(m_titleLabel);
    
    m_subtitleLabel = new QLabel(tr("Secure Real-Time Messaging"));
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet("QLabel { font-size: 12px; color: #7f8c8d; }");
    m_mainLayout->addWidget(m_subtitleLabel);
    
    // Login form
    m_loginGroupBox = new QGroupBox(tr("Login"));
    m_loginFormLayout = new QFormLayout(m_loginGroupBox);
    
    m_usernameEdit = new QLineEdit();
    m_usernameEdit->setPlaceholderText(tr("Enter your username"));
    m_usernameEdit->setMaxLength(MAX_USERNAME_LENGTH);
    m_loginFormLayout->addRow(tr("Username:"), m_usernameEdit);
    
    // Password field with show/hide button
    QHBoxLayout *passwordLayout = new QHBoxLayout();
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setPlaceholderText(tr("Enter your password"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    passwordLayout->addWidget(m_passwordEdit);
    
    m_showPasswordButton = new QPushButton();
    m_showPasswordButton->setIcon(QIcon(":/icons/eye-closed.png"));
    m_showPasswordButton->setFixedSize(24, 24);
    m_showPasswordButton->setCheckable(true);
    passwordLayout->addWidget(m_showPasswordButton);
    
    m_loginFormLayout->addRow(tr("Password:"), passwordLayout);
    
    // Options
    m_rememberCheckBox = new QCheckBox(tr("Remember credentials"));
    m_loginFormLayout->addRow(m_rememberCheckBox);
    
    m_autoConnectCheckBox = new QCheckBox(tr("Auto-connect on startup"));
    m_loginFormLayout->addRow(m_autoConnectCheckBox);
    
    m_mainLayout->addWidget(m_loginGroupBox);
    
    // Registration form (initially hidden)
    m_registerGroupBox = new QGroupBox(tr("Create Account"));
    m_registerGroupBox->setVisible(false);
    m_registerFormLayout = new QFormLayout(m_registerGroupBox);
    
    m_regUsernameEdit = new QLineEdit();
    m_regUsernameEdit->setPlaceholderText(tr("Choose a username"));
    m_regUsernameEdit->setMaxLength(MAX_USERNAME_LENGTH);
    m_registerFormLayout->addRow(tr("Username:"), m_regUsernameEdit);
    
    m_regPasswordEdit = new QLineEdit();
    m_regPasswordEdit->setPlaceholderText(tr("Choose a password"));
    m_regPasswordEdit->setEchoMode(QLineEdit::Password);
    m_registerFormLayout->addRow(tr("Password:"), m_regPasswordEdit);
    
    m_regConfirmPasswordEdit = new QLineEdit();
    m_regConfirmPasswordEdit->setPlaceholderText(tr("Confirm password"));
    m_regConfirmPasswordEdit->setEchoMode(QLineEdit::Password);
    m_registerFormLayout->addRow(tr("Confirm:"), m_regConfirmPasswordEdit);
    
    m_regEmailEdit = new QLineEdit();
    m_regEmailEdit->setPlaceholderText(tr("Enter your email (optional)"));
    m_registerFormLayout->addRow(tr("Email:"), m_regEmailEdit);
    
    m_mainLayout->addWidget(m_registerGroupBox);
    
    // Server settings (initially hidden)
    m_serverGroupBox = new QGroupBox(tr("Server Settings"));
    m_serverGroupBox->setVisible(false);
    m_serverFormLayout = new QFormLayout(m_serverGroupBox);
    
    m_serverHostEdit = new QLineEdit("localhost");
    m_serverHostEdit->setPlaceholderText(tr("Server address"));
    m_serverFormLayout->addRow(tr("Host:"), m_serverHostEdit);
    
    m_serverPortEdit = new QLineEdit("8080");
    m_serverPortEdit->setPlaceholderText(tr("Port number"));
    QIntValidator *portValidator = new QIntValidator(1, 65535, this);
    m_serverPortEdit->setValidator(portValidator);
    m_serverFormLayout->addRow(tr("Port:"), m_serverPortEdit);
    
    // Test connection button
    QHBoxLayout *testLayout = new QHBoxLayout();
    m_testConnectionButton = new QPushButton(tr("Test Connection"));
    m_testConnectionButton->setIcon(QIcon(":/icons/test.png"));
    testLayout->addWidget(m_testConnectionButton);
    
    m_connectionStatusLabel = new QLabel();
    testLayout->addWidget(m_connectionStatusLabel);
    testLayout->addStretch();
    
    m_serverFormLayout->addRow(testLayout);
    
    m_mainLayout->addWidget(m_serverGroupBox);
    
    // Error label
    m_errorLabel = new QLabel();
    m_errorLabel->setStyleSheet("QLabel { color: #e74c3c; font-weight: bold; }");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setVisible(false);
    m_mainLayout->addWidget(m_errorLabel);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_mainLayout->addWidget(m_progressBar);
    
    // Buttons
    m_buttonLayout = new QHBoxLayout();
    
    m_showRegisterButton = new QPushButton(tr("Create Account"));
    m_showRegisterButton->setIcon(QIcon(":/icons/register.png"));
    m_buttonLayout->addWidget(m_showRegisterButton);
    
    m_advancedButton = new QPushButton(tr("Advanced"));
    m_advancedButton->setIcon(QIcon(":/icons/settings.png"));
    m_advancedButton->setCheckable(true);
    m_buttonLayout->addWidget(m_advancedButton);
    
    m_buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton(tr("Cancel"));
    m_cancelButton->setIcon(QIcon(":/icons/cancel.png"));
    m_buttonLayout->addWidget(m_cancelButton);
    
    m_loginButton = new QPushButton(tr("Login"));
    m_loginButton->setIcon(QIcon(":/icons/login.png"));
    m_loginButton->setDefault(true);
    m_buttonLayout->addWidget(m_loginButton);
    
    m_registerButton = new QPushButton(tr("Register"));
    m_registerButton->setIcon(QIcon(":/icons/register.png"));
    m_registerButton->setVisible(false);
    m_buttonLayout->addWidget(m_registerButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Add stretch to push everything up
    m_mainLayout->addStretch();
}

void LoginDialog::setupConnections() {
    // Button connections
    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &LoginDialog::onCancelClicked);
    connect(m_showRegisterButton, &QPushButton::clicked, this, &LoginDialog::onShowRegisterToggled);
    connect(m_advancedButton, &QPushButton::toggled, this, &LoginDialog::onAdvancedToggled);
    connect(m_showPasswordButton, &QPushButton::toggled, this, &LoginDialog::onShowPasswordToggled);
    connect(m_testConnectionButton, &QPushButton::clicked, this, &LoginDialog::onTestConnectionClicked);
    
    // Form change connections
    connect(m_usernameEdit, &QLineEdit::textChanged, this, &LoginDialog::onFormChanged);
    connect(m_passwordEdit, &QLineEdit::textChanged, this, &LoginDialog::onFormChanged);
    connect(m_regUsernameEdit, &QLineEdit::textChanged, this, &LoginDialog::onFormChanged);
    connect(m_regPasswordEdit, &QLineEdit::textChanged, this, &LoginDialog::onFormChanged);
    connect(m_regConfirmPasswordEdit, &QLineEdit::textChanged, this, &LoginDialog::onFormChanged);
    connect(m_serverHostEdit, &QLineEdit::textChanged, this, &LoginDialog::onFormChanged);
    connect(m_serverPortEdit, &QLineEdit::textChanged, this, &LoginDialog::onFormChanged);
    
    // Enter key handling
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    connect(m_regConfirmPasswordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onRegisterClicked);
}

void LoginDialog::setupValidation() {
    // Username validation (alphanumeric + underscore)
    QRegularExpression usernameRegex("^[a-zA-Z0-9_]{3,32}$");
    QRegularExpressionValidator *usernameValidator = new QRegularExpressionValidator(usernameRegex, this);
    m_usernameEdit->setValidator(usernameValidator);
    m_regUsernameEdit->setValidator(usernameValidator);
    
    // Email validation
    QRegularExpression emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    QRegularExpressionValidator *emailValidator = new QRegularExpressionValidator(emailRegex, this);
    m_regEmailEdit->setValidator(emailValidator);
    
    // Host validation (IP or hostname)
    QRegularExpression hostRegex("^[a-zA-Z0-9.-]+$");
    QRegularExpressionValidator *hostValidator = new QRegularExpressionValidator(hostRegex, this);
    m_serverHostEdit->setValidator(hostValidator);
}

void LoginDialog::onLoginClicked() {
    if (!validateForm()) {
        return;
    }
    
    clearError();
    setLoginInProgress(true);
    
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();
    QString host = m_serverHostEdit->text().trimmed();
    int port = m_serverPortEdit->text().toInt();
    
    m_logger.info("Login attempt for user: {}", username);
    
    emit loginRequested(username, password, host, port);
}

void LoginDialog::onRegisterClicked() {
    if (!validateForm()) {
        return;
    }
    
    clearError();
    setLoginInProgress(true);
    
    QString username = m_regUsernameEdit->text().trimmed();
    QString password = m_regPasswordEdit->text();
    QString email = m_regEmailEdit->text().trimmed();
    QString host = m_serverHostEdit->text().trimmed();
    int port = m_serverPortEdit->text().toInt();
    
    m_logger.info("Registration attempt for user: {}", username);
    
    emit registerRequested(username, password, email, host, port);
}

void LoginDialog::onCancelClicked() {
    reject();
}

void LoginDialog::onShowRegisterToggled() {
    m_isLoginMode = !m_isLoginMode;
    
    if (m_isLoginMode) {
        // Switch to login mode
        m_loginGroupBox->setVisible(true);
        m_registerGroupBox->setVisible(false);
        m_loginButton->setVisible(true);
        m_registerButton->setVisible(false);
        m_showRegisterButton->setText(tr("Create Account"));
        setWindowTitle(tr("SecureChat - Login"));
    } else {
        // Switch to register mode
        m_loginGroupBox->setVisible(false);
        m_registerGroupBox->setVisible(true);
        m_loginButton->setVisible(false);
        m_registerButton->setVisible(true);
        m_showRegisterButton->setText(tr("Back to Login"));
        setWindowTitle(tr("SecureChat - Register"));
    }
    
    clearError();
    updateButtonStates();
    focusFirstEmptyField();
}

void LoginDialog::onAdvancedToggled() {
    m_isAdvancedMode = m_advancedButton->isChecked();
    m_serverGroupBox->setVisible(m_isAdvancedMode);
    
    if (m_isAdvancedMode) {
        setFixedSize(400, 750);
    } else {
        setFixedSize(400, 600);
    }
    
    adjustSize();
}

void LoginDialog::onShowPasswordToggled() {
    m_isPasswordVisible = m_showPasswordButton->isChecked();
    
    if (m_isPasswordVisible) {
        m_passwordEdit->setEchoMode(QLineEdit::Normal);
        m_showPasswordButton->setIcon(QIcon(":/icons/eye-open.png"));
    } else {
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        m_showPasswordButton->setIcon(QIcon(":/icons/eye-closed.png"));
    }
}

void LoginDialog::onTestConnectionClicked() {
    QString host = m_serverHostEdit->text().trimmed();
    int port = m_serverPortEdit->text().toInt();
    
    if (host.isEmpty() || port <= 0) {
        m_connectionStatusLabel->setText(tr("Invalid host or port"));
        m_connectionStatusLabel->setStyleSheet("QLabel { color: #e74c3c; }");
        return;
    }
    
    m_connectionStatusLabel->setText(tr("Testing..."));
    m_connectionStatusLabel->setStyleSheet("QLabel { color: #f39c12; }");
    m_testConnectionButton->setEnabled(false);
    
    // Create test connection
    if (!m_testConnection) {
        m_testConnection = std::make_unique<network::ClientConnection>(this);
        
        connect(m_testConnection.get(), &network::ClientConnection::connected, this, [this]() {
            m_connectionStatusLabel->setText(tr("✓ Connected"));
            m_connectionStatusLabel->setStyleSheet("QLabel { color: #27ae60; }");
            m_testConnectionButton->setEnabled(true);
            m_testConnection->disconnectFromServer();
        });
        
        connect(m_testConnection.get(), &network::ClientConnection::connectionError, this, [this](const QString &error) {
            m_connectionStatusLabel->setText(tr("✗ Failed: %1").arg(error));
            m_connectionStatusLabel->setStyleSheet("QLabel { color: #e74c3c; }");
            m_testConnectionButton->setEnabled(true);
        });
    }
    
    m_testConnection->connectToServer(host, port);
}

void LoginDialog::onFormChanged() {
    clearError();
    updateButtonStates();
}

bool LoginDialog::validateForm() {
    if (m_isLoginMode) {
        // Validate login form
        QString username = m_usernameEdit->text().trimmed();
        QString password = m_passwordEdit->text();
        
        if (username.isEmpty()) {
            setLoginError(tr("Please enter your username"));
            m_usernameEdit->setFocus();
            return false;
        }
        
        if (username.length() < 3) {
            setLoginError(tr("Username must be at least 3 characters long"));
            m_usernameEdit->setFocus();
            return false;
        }
        
        if (password.isEmpty()) {
            setLoginError(tr("Please enter your password"));
            m_passwordEdit->setFocus();
            return false;
        }
        
        if (password.length() < MIN_PASSWORD_LENGTH) {
            setLoginError(tr("Password must be at least %1 characters long").arg(MIN_PASSWORD_LENGTH));
            m_passwordEdit->setFocus();
            return false;
        }
    } else {
        // Validate registration form
        QString username = m_regUsernameEdit->text().trimmed();
        QString password = m_regPasswordEdit->text();
        QString confirmPassword = m_regConfirmPasswordEdit->text();
        QString email = m_regEmailEdit->text().trimmed();
        
        if (username.isEmpty()) {
            setLoginError(tr("Please choose a username"));
            m_regUsernameEdit->setFocus();
            return false;
        }
        
        if (username.length() < 3) {
            setLoginError(tr("Username must be at least 3 characters long"));
            m_regUsernameEdit->setFocus();
            return false;
        }
        
        if (password.isEmpty()) {
            setLoginError(tr("Please choose a password"));
            m_regPasswordEdit->setFocus();
            return false;
        }
        
        if (password.length() < MIN_PASSWORD_LENGTH) {
            setLoginError(tr("Password must be at least %1 characters long").arg(MIN_PASSWORD_LENGTH));
            m_regPasswordEdit->setFocus();
            return false;
        }
        
        if (password != confirmPassword) {
            setLoginError(tr("Passwords do not match"));
            m_regConfirmPasswordEdit->setFocus();
            return false;
        }
        
        if (!email.isEmpty() && !m_regEmailEdit->hasAcceptableInput()) {
            setLoginError(tr("Please enter a valid email address"));
            m_regEmailEdit->setFocus();
            return false;
        }
    }
    
    // Validate server settings if advanced mode is enabled
    if (m_isAdvancedMode) {
        QString host = m_serverHostEdit->text().trimmed();
        int port = m_serverPortEdit->text().toInt();
        
        if (host.isEmpty()) {
            setLoginError(tr("Please enter server address"));
            m_serverHostEdit->setFocus();
            return false;
        }
        
        if (port <= 0 || port > 65535) {
            setLoginError(tr("Please enter a valid port number (1-65535)"));
            m_serverPortEdit->setFocus();
            return false;
        }
    }
    
    return true;
}

void LoginDialog::setLoginInProgress(bool inProgress) {
    m_loginInProgress = inProgress;
    
    m_progressBar->setVisible(inProgress);
    if (inProgress) {
        m_progressBar->setRange(0, 0); // Indeterminate progress
    }
    
    setFormEnabled(!inProgress);
    updateButtonStates();
}

void LoginDialog::setLoginError(const QString &error) {
    m_errorLabel->setText(error);
    m_errorLabel->setVisible(true);
    m_errorTimer->start();
    
    m_logger.warn("Login error: {}", error);
}

void LoginDialog::clearError() {
    m_errorLabel->setVisible(false);
    m_errorTimer->stop();
}

void LoginDialog::clearErrorAfterDelay() {
    clearError();
}

void LoginDialog::setFormEnabled(bool enabled) {
    // Login form
    m_usernameEdit->setEnabled(enabled);
    m_passwordEdit->setEnabled(enabled);
    m_rememberCheckBox->setEnabled(enabled);
    m_autoConnectCheckBox->setEnabled(enabled);
    
    // Registration form
    m_regUsernameEdit->setEnabled(enabled);
    m_regPasswordEdit->setEnabled(enabled);
    m_regConfirmPasswordEdit->setEnabled(enabled);
    m_regEmailEdit->setEnabled(enabled);
    
    // Server settings
    m_serverHostEdit->setEnabled(enabled);
    m_serverPortEdit->setEnabled(enabled);
    m_testConnectionButton->setEnabled(enabled);
    
    // Buttons
    m_showRegisterButton->setEnabled(enabled);
    m_advancedButton->setEnabled(enabled);
}

void LoginDialog::updateButtonStates() {
    bool formValid = false;
    
    if (m_isLoginMode) {
        formValid = !m_usernameEdit->text().trimmed().isEmpty() && 
                   !m_passwordEdit->text().isEmpty();
    } else {
        formValid = !m_regUsernameEdit->text().trimmed().isEmpty() && 
                   !m_regPasswordEdit->text().isEmpty() &&
                   !m_regConfirmPasswordEdit->text().isEmpty();
    }
    
    m_loginButton->setEnabled(formValid && !m_loginInProgress);
    m_registerButton->setEnabled(formValid && !m_loginInProgress);
}

void LoginDialog::focusFirstEmptyField() {
    if (m_isLoginMode) {
        if (m_usernameEdit->text().isEmpty()) {
            m_usernameEdit->setFocus();
        } else {
            m_passwordEdit->setFocus();
        }
    } else {
        if (m_regUsernameEdit->text().isEmpty()) {
            m_regUsernameEdit->setFocus();
        } else if (m_regPasswordEdit->text().isEmpty()) {
            m_regPasswordEdit->setFocus();
        } else {
            m_regConfirmPasswordEdit->setFocus();
        }
    }
}

void LoginDialog::loadSettings() {
    QSettings settings;
    
    // Server settings
    m_serverHostEdit->setText(settings.value("connection/host", "localhost").toString());
    m_serverPortEdit->setText(settings.value("connection/port", 8080).toString());
    
    // UI settings
    m_rememberCheckBox->setChecked(settings.value("login/rememberCredentials", false).toBool());
    m_autoConnectCheckBox->setChecked(settings.value("connection/autoConnect", false).toBool());
    
    // Load saved credentials if remember is enabled
    if (m_rememberCheckBox->isChecked()) {
        m_usernameEdit->setText(settings.value("login/username", "").toString());
        // Note: We don't save passwords for security reasons
    }
}

void LoginDialog::saveSettings() {
    QSettings settings;
    
    // Server settings
    settings.setValue("connection/host", m_serverHostEdit->text());
    settings.setValue("connection/port", m_serverPortEdit->text().toInt());
    
    // UI settings
    settings.setValue("login/rememberCredentials", m_rememberCheckBox->isChecked());
    settings.setValue("connection/autoConnect", m_autoConnectCheckBox->isChecked());
    
    // Save username if remember is enabled
    if (m_rememberCheckBox->isChecked()) {
        settings.setValue("login/username", m_usernameEdit->text());
    } else {
        settings.remove("login/username");
    }
}

void LoginDialog::applyStyles() {
    setStyleSheet(R"(
        QDialog {
            background-color: #f8f9fa;
        }
        
        QGroupBox {
            font-weight: bold;
            border: 2px solid #dee2e6;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 10px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        
        QLineEdit {
            padding: 8px;
            border: 1px solid #ced4da;
            border-radius: 4px;
            font-size: 14px;
        }
        
        QLineEdit:focus {
            border-color: #007bff;
            outline: none;
        }
        
        QPushButton {
            padding: 8px 16px;
            border: none;
            border-radius: 4px;
            font-weight: bold;
            min-width: 80px;
        }
        
        QPushButton:default {
            background-color: #007bff;
            color: white;
        }
        
        QPushButton:default:hover {
            background-color: #0056b3;
        }
        
        QPushButton:default:pressed {
            background-color: #004085;
        }
        
        QPushButton:!default {
            background-color: #6c757d;
            color: white;
        }
        
        QPushButton:!default:hover {
            background-color: #545b62;
        }
        
        QPushButton:disabled {
            background-color: #e9ecef;
            color: #6c757d;
        }
        
        QCheckBox {
            font-size: 14px;
        }
        
        QProgressBar {
            border: 1px solid #ced4da;
            border-radius: 4px;
            text-align: center;
        }
        
        QProgressBar::chunk {
            background-color: #007bff;
            border-radius: 3px;
        }
    )");
}

void LoginDialog::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(event);
    }
}

void LoginDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    focusFirstEmptyField();
}

// Getters
QString LoginDialog::getUsername() const {
    return m_isLoginMode ? m_usernameEdit->text().trimmed() : m_regUsernameEdit->text().trimmed();
}

QString LoginDialog::getPassword() const {
    return m_isLoginMode ? m_passwordEdit->text() : m_regPasswordEdit->text();
}

QString LoginDialog::getServerHost() const {
    return m_serverHostEdit->text().trimmed();
}

int LoginDialog::getServerPort() const {
    return m_serverPortEdit->text().toInt();
}

bool LoginDialog::getRememberCredentials() const {
    return m_rememberCheckBox->isChecked();
}

bool LoginDialog::getAutoConnect() const {
    return m_autoConnectCheckBox->isChecked();
}

// Setters
void LoginDialog::setServerHost(const QString &host) {
    m_serverHostEdit->setText(host);
}

void LoginDialog::setServerPort(int port) {
    m_serverPortEdit->setText(QString::number(port));
}

void LoginDialog::setRememberCredentials(bool remember) {
    m_rememberCheckBox->setChecked(remember);
}

void LoginDialog::setAutoConnect(bool autoConnect) {
    m_autoConnectCheckBox->setChecked(autoConnect);
}

} // namespace securechat::client