// Двухфазная сборка: Pre-build (генерация исходников) + Build (компиляция)
#pragma once

#include <QObject>
#include <QString>

namespace DeltaQ {

class PreBuildProcessor;
class BuildManager;
struct ProjectBuildRequest;

class BuildPipeline : public QObject {
    Q_OBJECT

public:
    BuildPipeline(PreBuildProcessor *preBuild, BuildManager *buildManager,
                  QObject *parent = nullptr);

    // Полный pipeline: pre-build → build
    void run(const ProjectBuildRequest &request);

    // Только генерация исходников
    void runPreBuild(const QString &projectDir);

    // Только компиляция (предполагает что исходники уже сгенерированы)
    void runBuild(const ProjectBuildRequest &request);

signals:
    void pipelineStarted();
    void preBuildFinished(bool success);
    void buildStarted();
    void pipelineOutput(const QString &text);
    void pipelineFinished(bool success);

private:
    PreBuildProcessor *m_preBuild;
    BuildManager *m_buildManager;
};

} // namespace DeltaQ
