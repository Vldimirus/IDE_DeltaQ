// Двухфазная сборка — реализация
#include "BuildPipeline.h"
#include "PreBuildProcessor.h"
#include <deltaq/BuildTypes.h>
#include "../editor/BuildManager.h"

#include <QDir>

namespace DeltaQ {

namespace {

// Преобразует enum артефакта в человекочитаемую роль для build output.
QString artifactKindLabel(PreBuildArtifactKind kind)
{
    switch (kind) {
    case PreBuildArtifactKind::GraphSource:
        return QObject::tr("graph C source");
    case PreBuildArtifactKind::SubmoduleHeader:
        return QObject::tr("submodule header");
    case PreBuildArtifactKind::SubmoduleSource:
        return QObject::tr("submodule implementation");
    case PreBuildArtifactKind::UIHeader:
        return QObject::tr("UI header");
    case PreBuildArtifactKind::UISource:
        return QObject::tr("UI implementation");
    case PreBuildArtifactKind::UIEventsHeader:
        return QObject::tr("UI event header");
    case PreBuildArtifactKind::UIEventsSource:
        return QObject::tr("UI event implementation");
    }
    return QObject::tr("generated artifact");
}

// Показывает, из какого source-of-truth получен артефакт pre-build.
QString artifactSourceLabel(const PreBuildArtifact &artifact)
{
    switch (artifact.kind) {
    case PreBuildArtifactKind::GraphSource:
        return QObject::tr("graph '%1'").arg(artifact.sourceName);
    case PreBuildArtifactKind::SubmoduleHeader:
    case PreBuildArtifactKind::SubmoduleSource:
        return QObject::tr("submodule graph '%1'").arg(artifact.sourceName);
    case PreBuildArtifactKind::UIHeader:
    case PreBuildArtifactKind::UISource:
    case PreBuildArtifactKind::UIEventsHeader:
    case PreBuildArtifactKind::UIEventsSource:
        return QObject::tr("UI layout '%1'").arg(artifact.sourceName);
    }
    return artifact.sourceName;
}

// Собирает одну строку build output с путём, ролью и источником generated-файла.
QString formatArtifactLine(const QString &projectDir, const PreBuildArtifact &artifact)
{
    const QString relativePath = QDir(projectDir).relativeFilePath(artifact.path);
    return QObject::tr("  -> %1 [%2] <- %3")
        .arg(relativePath, artifactKindLabel(artifact.kind), artifactSourceLabel(artifact));
}

} // namespace

BuildPipeline::BuildPipeline(PreBuildProcessor *preBuild, BuildManager *buildManager,
                             QObject *parent)
    : QObject(parent)
    , m_preBuild(preBuild)
    , m_buildManager(buildManager)
{
    // Пробрасываем сообщения от pre-build процессора
    connect(m_preBuild, &PreBuildProcessor::progressMessage,
            this, &BuildPipeline::pipelineOutput);

    // Pipeline считается завершённым только после фактического окончания compile phase.
    connect(m_buildManager, &BuildManager::buildFinished,
            this, [this](bool success, int, int) {
        emit pipelineFinished(success);
    });
}

void BuildPipeline::run(const ProjectBuildRequest &request)
{
    emit pipelineStarted();
    emit pipelineOutput(tr("=== Фаза 1: Генерация исходников ==="));

    // Фаза 1: Pre-build
    PreBuildResult result = m_preBuild->process(request.projectDir);

    if (!result.success) {
        emit pipelineOutput(tr("Pre-build завершился с ошибками:"));
        for (const auto &err : result.errors)
            emit pipelineOutput("  ERROR: " + err);
        emit preBuildFinished(false);
        emit pipelineFinished(false);
        return;
    }

    emit pipelineOutput(tr("Сгенерировано файлов: %1").arg(result.generatedArtifacts.size()));
    for (const auto &artifact : result.generatedArtifacts)
        emit pipelineOutput(formatArtifactLine(request.projectDir, artifact));

    emit preBuildFinished(true);

    // Фаза 2: Build
    emit pipelineOutput(tr("\n=== Фаза 2: Компиляция ==="));
    emit buildStarted();

    m_buildManager->build(request);
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

void BuildPipeline::runBuild(const ProjectBuildRequest &request)
{
    emit pipelineOutput(tr("=== Build: Компиляция ==="));
    emit buildStarted();
    m_buildManager->build(request);
}

} // namespace DeltaQ
