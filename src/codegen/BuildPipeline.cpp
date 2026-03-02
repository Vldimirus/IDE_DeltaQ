// Двухфазная сборка — реализация
#include "BuildPipeline.h"
#include "PreBuildProcessor.h"
#include "../editor/BuildManager.h"

namespace DeltaQ {

BuildPipeline::BuildPipeline(PreBuildProcessor *preBuild, BuildManager *buildManager,
                             QObject *parent)
    : QObject(parent)
    , m_preBuild(preBuild)
    , m_buildManager(buildManager)
{
    // Пробрасываем сообщения от pre-build процессора
    connect(m_preBuild, &PreBuildProcessor::progressMessage,
            this, &BuildPipeline::pipelineOutput);
}

void BuildPipeline::run(const QString &projectDir, const QString &projectName,
                        const QString &cStandard, const QString &cxxStandard,
                        const QString &projectType)
{
    emit pipelineStarted();
    emit pipelineOutput(tr("=== Фаза 1: Генерация исходников ==="));

    // Фаза 1: Pre-build
    PreBuildResult result = m_preBuild->process(projectDir);

    if (!result.success) {
        emit pipelineOutput(tr("Pre-build завершился с ошибками:"));
        for (const auto &err : result.errors)
            emit pipelineOutput("  ERROR: " + err);
        emit preBuildFinished(false);
        emit pipelineFinished(false);
        return;
    }

    emit pipelineOutput(tr("Сгенерировано файлов: %1").arg(result.generatedFiles.size()));
    for (const auto &f : result.generatedFiles)
        emit pipelineOutput(tr("  → %1").arg(f));

    emit preBuildFinished(true);

    // Фаза 2: Build
    emit pipelineOutput(tr("\n=== Фаза 2: Компиляция ==="));
    emit buildStarted();

    m_buildManager->build(projectDir, projectName, cStandard, cxxStandard, projectType);
}

void BuildPipeline::runPreBuild(const QString &projectDir)
{
    emit pipelineStarted();
    emit pipelineOutput(tr("=== Pre-build: Генерация исходников ==="));

    PreBuildResult result = m_preBuild->process(projectDir);

    emit preBuildFinished(result.success);

    if (!result.success) {
        for (const auto &err : result.errors)
            emit pipelineOutput("  ERROR: " + err);
    }

    emit pipelineFinished(result.success);
}

void BuildPipeline::runBuild(const QString &projectDir, const QString &projectName,
                             const QString &cStandard, const QString &cxxStandard,
                             const QString &projectType)
{
    emit pipelineOutput(tr("=== Build: Компиляция ==="));
    emit buildStarted();
    m_buildManager->build(projectDir, projectName, cStandard, cxxStandard, projectType);
}

} // namespace DeltaQ
