#include "ProjectManager.h"
#include "ModuleRegistry.h"
#include "GraphStore.h"
#include "UILayoutStore.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QFileInfo>

namespace DeltaQ {

ProjectManager::ProjectManager(ModuleRegistry *registry,
                               GraphStore *graphStore,
                               UILayoutStore *uiLayoutStore,
                               QObject *parent)
    : QObject(parent)
    , m_registry(registry)
    , m_graphStore(graphStore)
    , m_uiLayoutStore(uiLayoutStore)
{
}

bool ProjectManager::createProject(const QString &name, const QString &dir)
{
    if (m_isOpen)
        closeProject();

    QDir d(dir);
    if (!d.exists() && !d.mkpath("."))
        return false;

    m_project = Project::createNew(name);
    m_project.projectDir = dir;
    m_project.projectFilePath = dir + "/" + name + ".dqproj";
    m_localSettings = ProjectLocalSettings();

    if (!ensureDirectories(dir))
        return false;

    if (!saveProjectMetadataOnly())
        return false;

    m_isOpen = true;
    emit projectOpened(name);
    return true;
}

bool ProjectManager::createProject(const QString &name, const QString &dir,
                                   const QString &type)
{
    if (m_isOpen)
        closeProject();

    QDir d(dir);
    if (!d.exists() && !d.mkpath("."))
        return false;

    m_project = Project::createNew(name);
    m_project.projectDir = dir;
    m_project.projectFilePath = dir + "/" + name + ".dqproj";
    m_project.projectType = type;
    m_localSettings = ProjectLocalSettings();

    if (!ensureDirectories(dir))
        return false;

    if (!saveProjectMetadataOnly())
        return false;

    m_isOpen = true;
    emit projectOpened(name);
    return true;
}

bool ProjectManager::openProject(const QString &dqprojPath)
{
    QFile file(dqprojPath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError)
        return false;

    if (m_isOpen)
        closeProject();

    const QJsonObject projectJson = doc.object();
    m_project = Project::fromJson(projectJson);
    m_project.projectFilePath = dqprojPath;
    m_project.projectDir = QFileInfo(dqprojPath).absolutePath();
    m_localSettings = ProjectLocalSettings();

    QFile localFile(projectLocalSettingsPath());
    if (localFile.exists() && localFile.open(QIODevice::ReadOnly)) {
        QJsonParseError localError;
        const QJsonDocument localDoc = QJsonDocument::fromJson(localFile.readAll(), &localError);
        if (localError.error == QJsonParseError::NoError)
            m_localSettings = ProjectLocalSettings::fromJson(localDoc.object());
    } else {
        loadLocalSettingsFromLegacyProjectJson(projectJson);
    }

    // Загрузка модулей, графов и макетов из директории проекта
    m_registry->loadRegistry(m_project.projectDir);
    if (m_graphStore)
        m_graphStore->loadFromDirectory(m_project.projectDir);
    if (m_uiLayoutStore)
        m_uiLayoutStore->loadFromDirectory(m_project.projectDir);

    m_isOpen = true;
    emit projectOpened(m_project.name);
    return true;
}

bool ProjectManager::saveProjectMetadataOnly()
{
    if (m_project.projectFilePath.isEmpty())
        return false;

    if (!writeProjectMetadata())
        return false;
    if (!writeLocalSettings())
        return false;

    emit projectSaved();
    return true;
}

bool ProjectManager::saveProject()
{
    if (m_project.projectFilePath.isEmpty())
        return false;

    if (!writeProjectMetadata())
        return false;
    if (!writeLocalSettings())
        return false;

    // Сохранение модулей, графов и макетов
    m_registry->saveRegistry(m_project.projectDir);
    if (m_graphStore)
        m_graphStore->saveAll(m_project.projectDir);
    if (m_uiLayoutStore)
        m_uiLayoutStore->saveAll(m_project.projectDir);

    emit projectSaved();
    return true;
}

bool ProjectManager::closeProject()
{
    if (!m_isOpen)
        return false;

    m_registry->clear();
    if (m_graphStore)
        m_graphStore->clear();
    if (m_uiLayoutStore)
        m_uiLayoutStore->clear();
    m_project = Project();
    m_localSettings = ProjectLocalSettings();
    m_isOpen = false;

    emit projectClosed();
    return true;
}

bool ProjectManager::ensureDirectories(const QString &dir)
{
    QDir d(dir);
    return d.mkpath("graphs") &&
           d.mkpath("ui") &&
           d.mkpath("src") &&
           d.mkpath("src/ui") &&
           d.mkpath("build") &&
           d.mkpath("dqmods");
}

QString ProjectManager::projectLocalSettingsPath() const
{
    if (m_project.projectFilePath.isEmpty())
        return {};
    return m_project.projectFilePath + ".user";
}

bool ProjectManager::writeProjectMetadata()
{
    QFile file(m_project.projectFilePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    const QJsonDocument doc(m_project.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool ProjectManager::writeLocalSettings()
{
    const QString path = projectLocalSettingsPath();
    if (path.isEmpty())
        return false;

    if (m_localSettings.isDefault()) {
        QFile localFile(path);
        return !localFile.exists() || localFile.remove();
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    const QJsonDocument doc(m_localSettings.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

void ProjectManager::loadLocalSettingsFromLegacyProjectJson(const QJsonObject &projectJson)
{
    m_localSettings = ProjectLocalSettings();

    const QJsonObject build = projectJson["build"].toObject();
    const QString legacyCompilerPath = build["compiler_path"].toString();
    const QString legacyBuildTool = build["build_tool"].toString().trimmed().toLower();

    if (!legacyCompilerPath.isEmpty()) {
        m_localSettings.selectionMode = "manual";
        m_localSettings.manualOverride.enabled = true;
        m_localSettings.manualOverride.cCompilerPath = legacyCompilerPath;
    }

    if (legacyBuildTool == "ninja" || legacyBuildTool == "make") {
        m_localSettings.selectionMode = "manual";
        m_localSettings.manualOverride.enabled = true;
        m_localSettings.manualOverride.generator =
            (legacyBuildTool == "ninja") ? "ninja" : "unix_makefiles";
    }
}

} // namespace DeltaQ
