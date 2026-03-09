// Pre-build процессор — реализация
#include "PreBuildProcessor.h"
#include "GraphCompiler.h"
#include "../core/ModuleRegistry.h"
#include "../core/GraphStore.h"
#include "../core/UILayoutStore.h"
#include "../uiDesigner/SDL2CodeGenerator.h"

#include <deltaq/Graph.h>
#include <deltaq/Module.h>
#include <deltaq/UILayout.h>

#include <algorithm>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace DeltaQ {

namespace {

// Добавляет generated-артефакт в результат и сразу пробрасывает его в UI слоям IDE.
void appendArtifact(PreBuildResult &result, PreBuildProcessor *processor,
                    const PreBuildArtifact &artifact)
{
    result.generatedArtifacts.append(artifact);
    emit processor->fileGenerated(artifact);
}

} // namespace

PreBuildProcessor::PreBuildProcessor(ModuleRegistry *registry, GraphStore *graphStore,
                                     UILayoutStore *uiLayoutStore, QObject *parent)
    : QObject(parent)
    , m_registry(registry)
    , m_graphStore(graphStore)
    , m_uiLayoutStore(uiLayoutStore)
{
}

PreBuildResult PreBuildProcessor::process(const QString &projectDir)
{
    // Выполняет оба этапа pre-build: графы и UI layout-ы.
    PreBuildResult result;

    emit progressMessage(tr("Pre-build: генерация исходников..."));

    processGraphs(projectDir, result);
    processUILayouts(projectDir, result);

    if (result.errors.isEmpty()) {
        emit progressMessage(tr("Pre-build завершён: %1 файлов сгенерировано")
                             .arg(result.generatedArtifacts.size()));
    } else {
        result.success = false;
        emit progressMessage(tr("Pre-build завершён с ошибками: %1")
                             .arg(result.errors.size()));
    }

    return result;
}

void PreBuildProcessor::processGraphs(const QString &projectDir, PreBuildResult &result)
{
    // Сначала генерирует отдельные compilation units для составных модулей,
    // затем компилирует только корневые графы проекта в C-файлы.
    QString srcDir = projectDir + "/src";
    QDir().mkpath(srcDir);

    GraphCompiler compiler(m_registry);
    compiler.setGraphStore(m_graphStore);

    processSubmodules(projectDir, result, compiler);
    if (!result.errors.isEmpty())
        return;

    for (auto *graph : m_graphStore->allGraphs()) {
        if (!graph->parentModuleId.isEmpty())
            continue;

        emit progressMessage(tr("Компиляция графа: %1").arg(graph->name));

        CompilationResult cr = compiler.compile(*graph);

        if (!cr.success) {
            for (const auto &err : cr.errors)
                result.errors.append(tr("Граф '%1': %2").arg(graph->name, err));
            continue;
        }

        // Сохраняем сгенерированный код
        QString filePath = srcDir + "/" + graph->name + ".c";
        QFile f(filePath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(cr.generatedCode.toUtf8());
            f.close();
            appendArtifact(result, this, PreBuildArtifact{
                filePath,
                PreBuildArtifactKind::GraphSource,
                graph->name,
                graph->id
            });
        } else {
            result.errors.append(tr("Не удалось записать файл: %1").arg(filePath));
        }
    }
}

void PreBuildProcessor::processSubmodules(const QString &projectDir, PreBuildResult &result,
                                          GraphCompiler &compiler)
{
    // Для каждого составного модуля заранее генерируем отдельные .h/.c,
    // чтобы корневые графы могли подключать их как обычные исходники проекта.
    QString submoduleDir = projectDir + "/src/generated/submodules";
    QDir().mkpath(submoduleDir);

    QVector<const Module *> submodules;
    for (const auto *module : m_registry->allModules()) {
        if (module->isComposite())
            submodules.append(module);
    }

    std::sort(submodules.begin(), submodules.end(),
              [](const Module *lhs, const Module *rhs) {
        return lhs->name < rhs->name;
    });

    for (const auto *module : submodules) {
        emit progressMessage(tr("Генерация подмодуля: %1").arg(module->name));

        const SubmoduleUnitResult unit = compiler.compileSubmoduleUnit(*module);
        if (!unit.success) {
            for (const auto &err : unit.errors)
                result.errors.append(tr("Подмодуль '%1': %2").arg(module->name, err));
            continue;
        }

        const Graph *innerGraph = m_graphStore->findGraph(module->graphId);
        const QString sourceName = innerGraph ? innerGraph->name : module->name;
        const QString sourceId = innerGraph ? innerGraph->id : module->graphId;

        auto writeFile = [&](const QString &path, const QString &content,
                             PreBuildArtifactKind kind) -> bool {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                result.errors.append(tr("Не удалось записать файл: %1").arg(path));
                return false;
            }
            file.write(content.toUtf8());
            file.close();
            appendArtifact(result, this, PreBuildArtifact{
                path,
                kind,
                sourceName,
                sourceId
            });
            return true;
        };

        writeFile(submoduleDir + "/" + unit.fileBaseName + ".h", unit.headerCode,
                  PreBuildArtifactKind::SubmoduleHeader);
        writeFile(submoduleDir + "/" + unit.fileBaseName + ".c", unit.sourceCode,
                  PreBuildArtifactKind::SubmoduleSource);
    }
}

void PreBuildProcessor::processUILayouts(const QString &projectDir, PreBuildResult &result)
{
    // Генерирует SDL2-артефакты для всех UI layout-ов проекта.
    if (!m_uiLayoutStore)
        return;

    auto layouts = m_uiLayoutStore->allLayouts();
    if (layouts.isEmpty())
        return;

    QString uiDir = projectDir + "/src/ui";
    QDir().mkpath(uiDir);

    for (auto *layout : layouts) {
        emit progressMessage(tr("Генерация UI: %1").arg(layout->name));

        GeneratedCode code = SDL2CodeGenerator::generate(*layout, layout->name);

        // Общий путь записи для всех generated-файлов UI с сохранением их происхождения.
        auto writeFile = [&](const QString &path, const QString &content,
                             PreBuildArtifactKind kind) -> bool {
            if (kind == PreBuildArtifactKind::UIEventsSource && QFile::exists(path)) {
                // Пользовательский код обработчиков не должен теряться при повторном pre-build.
                appendArtifact(result, this, PreBuildArtifact{
                    path,
                    kind,
                    layout->name,
                    layout->id
                });
                emit progressMessage(tr("Сохранён пользовательский файл обработчиков: %1")
                                     .arg(QFileInfo(path).fileName()));
                return true;
            }

            QFile f(path);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(content.toUtf8());
                f.close();
                appendArtifact(result, this, PreBuildArtifact{
                    path,
                    kind,
                    layout->name,
                    layout->id
                });
                return true;
            }
            result.errors.append(tr("Не удалось записать файл: %1").arg(path));
            return false;
        };

        QString baseName = layout->name;
        writeFile(uiDir + "/" + baseName + ".h", code.uiHeader, PreBuildArtifactKind::UIHeader);
        writeFile(uiDir + "/" + baseName + ".c", code.uiSource, PreBuildArtifactKind::UISource);
        writeFile(uiDir + "/" + baseName + "_events.h", code.eventsHeader,
                  PreBuildArtifactKind::UIEventsHeader);
        writeFile(uiDir + "/" + baseName + "_events.c", code.eventsSource,
                  PreBuildArtifactKind::UIEventsSource);
    }
}

} // namespace DeltaQ
