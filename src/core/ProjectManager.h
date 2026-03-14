#pragma once

#include <QObject>
#include <QString>
#include <deltaq/Project.h>

namespace DeltaQ {

class ModuleRegistry;
class GraphStore;
class UILayoutStore;

class ProjectManager : public QObject {
    Q_OBJECT

public:
    explicit ProjectManager(ModuleRegistry *registry,
                            GraphStore *graphStore = nullptr,
                            UILayoutStore *uiLayoutStore = nullptr,
                            QObject *parent = nullptr);

    bool createProject(const QString &name, const QString &dir);
    bool createProject(const QString &name, const QString &dir, const QString &type);
    bool openProject(const QString &dqprojPath);
    bool saveProjectMetadataOnly();
    bool saveProject();
    bool closeProject();

    bool isProjectOpen() const { return m_isOpen; }
    const Project &currentProject() const { return m_project; }
    Project &currentProject() { return m_project; }
    const ProjectLocalSettings &currentLocalSettings() const { return m_localSettings; }
    ProjectLocalSettings &currentLocalSettings() { return m_localSettings; }
    QString projectDir() const { return m_project.projectDir; }
    QString projectLocalSettingsPath() const;

signals:
    void projectOpened(const QString &name);
    void projectClosed();
    void projectSaved();
    void projectModified();

private:
    bool ensureDirectories(const QString &dir);
    bool writeProjectMetadata();
    bool writeLocalSettings();
    void loadLocalSettingsFromLegacyProjectJson(const QJsonObject &projectJson);

    Project m_project;
    ProjectLocalSettings m_localSettings;
    ModuleRegistry *m_registry;
    GraphStore *m_graphStore;
    UILayoutStore *m_uiLayoutStore;
    bool m_isOpen = false;
};

} // namespace DeltaQ
