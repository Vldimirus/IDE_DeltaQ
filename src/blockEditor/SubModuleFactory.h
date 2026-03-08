// Фабрика подмодулей — создание композитных модулей из выделенных узлов
#pragma once

#include <QString>
#include <QList>
#include <deltaq/Module.h>
#include <deltaq/Graph.h>

namespace DeltaQ {

class ModuleRegistry;

class SubModuleFactory {
public:
    struct Result {
        Module module;          // Обёрточный модуль (origin="graph")
        Graph innerGraph;       // Внутренний граф подмодуля
        Graph updatedParentGraph; // Родительский граф после замены выделения на один submodule node
        GraphNode createdNode;  // Узел подмодуля, вставленный в родительский граф
    };

    // Создать подмодуль из выделенных узлов
    // parentGraph — текущий граф, selectedNodeIds — ID выбранных узлов
    // registry — для поиска модулей (определения портов)
    static Result createFromSelection(
        const Graph &parentGraph,
        const QStringList &selectedNodeIds,
        const QString &name,
        const ModuleRegistry *registry
    );

private:
    // Определить внешние входы (связи входящие в выделение извне)
    static QVector<Port> findExternalInputs(
        const Graph &graph,
        const QStringList &selectedNodeIds,
        const ModuleRegistry *registry
    );

    // Определить внешние выходы (связи исходящие из выделения наружу)
    static QVector<Port> findExternalOutputs(
        const Graph &graph,
        const QStringList &selectedNodeIds,
        const ModuleRegistry *registry
    );
};

} // namespace DeltaQ
