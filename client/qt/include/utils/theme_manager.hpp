#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QSettings>
#include <QtGui/QPalette>
#include <QtWidgets/QApplication>

namespace securechat::client::utils {

enum class Theme {
    Light,
    Dark,
    Auto
};

class ThemeManager : public QObject {
    Q_OBJECT

public:
    static ThemeManager& instance();
    
    // Initialization
    static void initialize();
    
    // Theme management
    void setTheme(Theme theme);
    Theme getCurrentTheme() const { return m_currentTheme; }
    Theme getEffectiveTheme() const;
    
    // Style sheets
    QString getStyleSheet() const;
    QString getStyleSheet(const QString &component) const;
    
    // Colors
    QColor getPrimaryColor() const;
    QColor getSecondaryColor() const;
    QColor getBackgroundColor() const;
    QColor getTextColor() const;
    QColor getAccentColor() const;
    QColor getBorderColor() const;
    
    // Specific component colors
    QColor getChatBubbleColor(bool own = false) const;
    QColor getOnlineStatusColor() const;
    QColor getOfflineStatusColor() const;
    QColor getTypingIndicatorColor() const;
    
    // Settings
    void saveSettings();
    void loadSettings();

signals:
    void themeChanged(Theme theme);

private slots:
    void onSystemThemeChanged();

private:
    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager() override = default;
    
    void applyTheme();
    void setupSystemThemeDetection();
    bool isSystemDarkMode() const;
    
    QString getLightStyleSheet() const;
    QString getDarkStyleSheet() const;
    QString getComponentStyleSheet(const QString &component, bool darkMode) const;
    
    Theme m_currentTheme;
    static ThemeManager *s_instance;
};

} // namespace securechat::client::utils