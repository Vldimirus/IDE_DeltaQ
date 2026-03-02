// Pre-build процессор — реализация
#include "PreBuildProcessor.h"
#include "GraphCompiler.h"
#include "../core/ModuleRegistry.h"
#include "../core/GraphStore.h"
#include "../core/UILayoutStore.h"
#include "../uiDesigner/SDL2CodeGenerator.h"

#include <deltaq/Graph.h>
#include <deltaq/UILayout.h>

#include <QDir>
#include <QFile>

namespace DeltaQ {

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
    PreBuildResult result;

    emit progressMessage(tr("Pre-build: генерация исходников..."));

    processGraphs(projectDir, result);
    processUILayouts(projectDir, result);

    if (result.errors.isEmpty()) {
        emit progressMessage(tr("Pre-build завершён: %1 файлов сгенерировано")
                             .arg(result.generatedFiles.size()));
    } else {
        result.success = false;
        emit progressMessage(tr("Pre-build завершён с ошибками: %1")
                             .arg(result.errors.size()));
    }

    return result;
}

void PreBuildProcessor::processGraphs(const QString &projectDir, PreBuildResult &result)
{
    QString srcDir = projectDir + "/src";
    QDir().mkpath(srcDir);

    GraphCompiler compiler(m_registry);
    compiler.setGraphStore(m_graphStore);

    for (auto *graph : m_graphStore->allGraphs()) {
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
            result.generatedFiles.append(filePath);
            emit fileGenerated(filePath);
        } else {
            result.errors.append(tr("Не удалось записать файл: %1").arg(filePath));
        }
    }
}

void PreBuildProcessor::processUILayouts(const QString &projectDir, PreBuildResult &result)
{
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

        auto writeFile = [&](const QString &path, const QString &content) -> bool {
            QFile f(path);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(content.toUtf8());
                f.close();
                result.generatedFiles.append(path);
                emit fileGenerated(path);
                return true;
            }
            result.errors.append(tr("Не удалось записать файл: %1").arg(path));
            return false;
        };

        QString baseName = layout->name;
        writeFile(uiDir + "/" + baseName + ".h", code.uiHeader);
        writeFile(uiDir + "/" + baseName + ".c", code.uiSource);
        writeFile(uiDir + "/" + baseName + "_events.h", code.eventsHeader);
        writeFile(uiDir + "/" + baseName + "_events.c", code.eventsSource);
    }
}

} // namespace DeltaQ
