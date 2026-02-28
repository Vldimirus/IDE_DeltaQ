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
    bool openProject(const QString &dqprojPath);
    bool saveProject();
    bool closeProject();

    bool isProjectOpen() const { return m_isOpen; }
    const Project &currentProject() const { return m_project; }
    Project &currentProject() { return m_project; }
    QString projectDir() const { return m_project.projectDir; }

signals:
    void projectOpened(const QString &name);
    void projectClosed();
    void projectSaved();
    void projectModified();

private:
    bool ensureDirectories(const QString &dir);

    Project m_project;
    ModuleRegistry *m_registry;
    GraphStore *m_graphStore;
    UILayoutStore *m_uiLayoutStore;
    bool m_isOpen = false;
};

} // namespace DeltaQ
