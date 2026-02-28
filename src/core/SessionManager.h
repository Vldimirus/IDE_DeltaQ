#pragma once

#include <QObject>
#include <QStringList>
#include <QSettings>

namespace DeltaQ {

class SessionManager : public QObject {
    Q_OBJECT

public:
    explicit SessionManager(QObject *parent = nullptr);

    // Тестовый конструктор — изолированные QSettings
    SessionManager(const QString &org, const QString &app, QObject *parent = nullptr);

    QStringList recentProjects() const;
    void addRecentProject(const QString &path);
    void clearRecentProjects();

    // Открытые вкладки
    QStringList openTabs() const;
    void setOpenTabs(const QStringList &tabs);

    // Последний открытый проект
    QString lastOpenedProject() const;
    void setLastOpenedProject(const QString &path);

    // Window geometry
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray &geometry);
    QByteArray windowState() const;
    void setWindowState(const QByteArray &state);

    // Language
    QString language() const;
    void setLanguage(const QString &lang);

    // Editor settings
    int tabWidth() const;
    void setTabWidth(int width);
    bool useSpaces() const;
    void setUseSpaces(bool use);
    QString fontFamily() const;
    void setFontFamily(const QString &family);
    int fontSize() const;
    void setFontSize(int size);

signals:
    void settingsChanged();

private:
    QSettings m_settings;
    static constexpr int MaxRecentProjects = 10;
};

} // namespace DeltaQ
