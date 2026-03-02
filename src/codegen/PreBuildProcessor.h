// Pre-build процессор — генерация исходников из графов и UI-макетов
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace DeltaQ {

class ModuleRegistry;
class GraphStore;
class UILayoutStore;

struct PreBuildResult {
    bool success = true;
    QStringList generatedFiles;
    QStringList errors;
};

class PreBuildProcessor : public QObject {
    Q_OBJECT

public:
    PreBuildProcessor(ModuleRegistry *registry, GraphStore *graphStore,
                      UILayoutStore *uiLayoutStore, QObject *parent = nullptr);

    // Полный цикл pre-build: графы + UI-макеты → исходники
    PreBuildResult process(const QString &projectDir);

signals:
    void progressMessage(const QString &text);
    void fileGenerated(const QString &path);

private:
    // Генерация C-кода из всех графов
    void processGraphs(const QString &projectDir, PreBuildResult &result);

    // Генерация SDL2-кода из всех UI-макетов
    void processUILayouts(const QString &projectDir, PreBuildResult &result);

    ModuleRegistry *m_registry;
    GraphStore *m_graphStore;
    UILayoutStore *m_uiLayoutStore;
};

} // namespace DeltaQ
