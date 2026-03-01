// Детектор циклов в композитных модулях — реализация
#include "CycleDetector.h"
#include "../core/ModuleRegistry.h"
#include "../core/GraphStore.h"

#include <deltaq/Module.h>
#include <deltaq/Graph.h>
#include <QSet>

namespace DeltaQ {

bool CycleDetector::wouldCreateCycle(
    const QString &graphId,
    const QString &moduleId,
    const ModuleRegistry &registry,
    const GraphStore &store)
{
    QSet<QString> visited;
    return dfs(graphId, moduleId, registry, store, visited);
}

bool CycleDetector::dfs(
    const QString &targetGraphId,
    const QString &currentModuleId,
    const ModuleRegistry &registry,
    const GraphStore &store,
    QSet<QString> &visited)
{
    if (visited.contains(currentModuleId))
        return false;
    visited.insert(currentModuleId);

    const Module *mod = registry.findModule(currentModuleId);
    if (!mod) return false;

    // Если модуль не композитный — цикла нет
    if (mod->graphId.isEmpty()) return false;

    // Если внутренний граф совпадает с целевым — цикл!
    if (mod->graphId == targetGraphId) return true;

    // Смотрим внутренний граф — проверяем все его узлы
    const Graph *innerGraph = store.findGraph(mod->graphId);
    if (!innerGraph) return false;

    for (const auto &node : innerGraph->nodes) {
        if (dfs(targetGraphId, node.moduleId, registry, store, visited))
            return true;
    }

    return false;
}

} // namespace DeltaQ
