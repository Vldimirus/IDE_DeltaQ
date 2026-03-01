// Детектор циклов в композитных модулях
#pragma once

#include <QString>

namespace DeltaQ {

class ModuleRegistry;
class GraphStore;

class CycleDetector {
public:
    // Проверить, создаст ли добавление модуля moduleId
    // в граф graphId циклическую зависимость
    // (модуль содержит подграф, который прямо или косвенно ссылается на graphId)
    static bool wouldCreateCycle(
        const QString &graphId,
        const QString &moduleId,
        const ModuleRegistry &registry,
        const GraphStore &store
    );

private:
    // Рекурсивный DFS по цепочке graphId → модули → их graphId → ...
    static bool dfs(
        const QString &targetGraphId,
        const QString &currentModuleId,
        const ModuleRegistry &registry,
        const GraphStore &store,
        QSet<QString> &visited
    );
};

} // namespace DeltaQ
