// Pre-build процессор — генерация исходников из графов и UI-макетов
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace DeltaQ {

class ModuleRegistry;
class GraphStore;
class UILayoutStore;
class GraphCompiler;

enum class PreBuildArtifactKind {
    GraphSource,
    SubmoduleHeader,
    SubmoduleSource,
    UIHeader,
    UISource,
    UIEventsHeader,
    UIEventsSource
};

struct PreBuildArtifact {
    QString path;
    PreBuildArtifactKind kind = PreBuildArtifactKind::GraphSource;
    QString sourceName;
    QString sourceId;
};

struct PreBuildResult {
    bool success = true;
    QVector<PreBuildArtifact> generatedArtifacts;
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
    void fileGenerated(const PreBuildArtifact &artifact);

private:
    // Генерация C-кода из всех графов
    void processGraphs(const QString &projectDir, PreBuildResult &result);

    // Генерация отдельных compilation units для составных модулей
    void processSubmodules(const QString &projectDir, PreBuildResult &result, GraphCompiler &compiler);

    // Генерация SDL2-кода из всех UI-макетов
    void processUILayouts(const QString &projectDir, PreBuildResult &result);

    ModuleRegistry *m_registry;
    GraphStore *m_graphStore;
    UILayoutStore *m_uiLayoutStore;
};

} // namespace DeltaQ

Q_DECLARE_METATYPE(DeltaQ::PreBuildArtifact)
