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

    if (!ensureDirectories(dir))
        return false;

    if (!saveProject())
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

    if (!ensureDirectories(dir))
        return false;

    if (!saveProject())
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

    m_project = Project::fromJson(doc.object());
    m_project.projectFilePath = dqprojPath;
    m_project.projectDir = QFileInfo(dqprojPath).absolutePath();

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

bool ProjectManager::saveProject()
{
    if (m_project.projectFilePath.isEmpty())
        return false;

    QFile file(m_project.projectFilePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QJsonDocument doc(m_project.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));

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
    m_isOpen = false;

    emit projectClosed();
    return true;
}

bool ProjectManager::ensureDirectories(const QString &dir)
{
    QDir d(dir);
    return d.mkpath("modules") &&
           d.mkpath("graphs") &&
           d.mkpath("ui") &&
           d.mkpath("src") &&
           d.mkpath("build") &&
           d.mkpath("include");
}

} // namespace DeltaQ
