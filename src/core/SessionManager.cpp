#include "SessionManager.h"
#include <QDir>

namespace DeltaQ {

SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
    , m_settings("DeltaQ", "IDE")
{
}

SessionManager::SessionManager(const QString &org, const QString &app, QObject *parent)
    : QObject(parent)
    , m_settings(org, app)
{
}

QStringList SessionManager::recentProjects() const
{
    return m_settings.value("session/recentProjects").toStringList();
}

void SessionManager::addRecentProject(const QString &path)
{
    QStringList recent = recentProjects();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > MaxRecentProjects)
        recent.removeLast();
    m_settings.setValue("session/recentProjects", recent);
}

void SessionManager::clearRecentProjects()
{
    m_settings.remove("session/recentProjects");
}

QStringList SessionManager::openTabs() const
{
    return m_settings.value("session/openTabs").toStringList();
}

void SessionManager::setOpenTabs(const QStringList &tabs)
{
    m_settings.setValue("session/openTabs", tabs);
}

QString SessionManager::lastOpenedProject() const
{
    return m_settings.value("session/lastOpenedProject").toString();
}

void SessionManager::setLastOpenedProject(const QString &path)
{
    m_settings.setValue("session/lastOpenedProject", path);
}

QString SessionManager::language() const
{
    return m_settings.value("app/language", "en").toString();
}

void SessionManager::setLanguage(const QString &lang)
{
    m_settings.setValue("app/language", lang);
}

QByteArray SessionManager::windowGeometry() const
{
    return m_settings.value("window/geometry").toByteArray();
}

void SessionManager::setWindowGeometry(const QByteArray &geometry)
{
    m_settings.setValue("window/geometry", geometry);
}

QByteArray SessionManager::windowState() const
{
    return m_settings.value("window/state").toByteArray();
}

void SessionManager::setWindowState(const QByteArray &state)
{
    m_settings.setValue("window/state", state);
}

QString SessionManager::defaultProjectDir() const
{
    return m_settings.value("defaultProjectDir", QDir::homePath()).toString();
}

void SessionManager::setDefaultProjectDir(const QString &path)
{
    m_settings.setValue("defaultProjectDir", path);
}

int SessionManager::tabWidth() const
{
    return m_settings.value("editor/tabWidth", 4).toInt();
}

void SessionManager::setTabWidth(int width)
{
    m_settings.setValue("editor/tabWidth", width);
    emit settingsChanged();
}

bool SessionManager::useSpaces() const
{
    return m_settings.value("editor/useSpaces", true).toBool();
}

void SessionManager::setUseSpaces(bool use)
{
    m_settings.setValue("editor/useSpaces", use);
    emit settingsChanged();
}

QString SessionManager::fontFamily() const
{
    return m_settings.value("editor/fontFamily", "Monospace").toString();
}

void SessionManager::setFontFamily(const QString &family)
{
    m_settings.setValue("editor/fontFamily", family);
    emit settingsChanged();
}

int SessionManager::fontSize() const
{
    return m_settings.value("editor/fontSize", 12).toInt();
}

void SessionManager::setFontSize(int size)
{
    m_settings.setValue("editor/fontSize", size);
    emit settingsChanged();
}

} // namespace DeltaQ
