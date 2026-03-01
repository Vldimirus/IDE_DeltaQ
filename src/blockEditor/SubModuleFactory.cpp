// Фабрика подмодулей — реализация
#include "SubModuleFactory.h"
#include "../core/ModuleRegistry.h"

#include <QSet>
#include <QUuid>

namespace DeltaQ {

SubModuleFactory::Result SubModuleFactory::createFromSelection(
    const Graph &parentGraph,
    const QStringList &selectedNodeIds,
    const QString &name,
    const ModuleRegistry *registry)
{
    Result result;
    QSet<QString> selectedSet(selectedNodeIds.begin(), selectedNodeIds.end());

    // --- 1. Создаём внутренний граф с копией выделенных узлов ---
    result.innerGraph = Graph::create(name + "_inner");

    for (const auto &node : parentGraph.nodes) {
        if (selectedSet.contains(node.id))
            result.innerGraph.addNode(node);
    }

    // Копируем внутренние связи (оба конца в выделении)
    for (const auto &conn : parentGraph.connections) {
        if (selectedSet.contains(conn.from.nodeId) &&
            selectedSet.contains(conn.to.nodeId)) {
            result.innerGraph.addConnection(conn);
        }
    }

    // --- 2. Определяем внешние порты (связи пересекающие границу выделения) ---
    // Входы: связи из невыделенных узлов в выделенные
    QVector<Port> externalInputs;
    QMap<QString, QString> inputPortMap; // "toNodeId:toPortName" → имя внешнего порта

    for (const auto &conn : parentGraph.connections) {
        if (!selectedSet.contains(conn.from.nodeId) &&
             selectedSet.contains(conn.to.nodeId)) {
            // Внешняя входящая связь
            const GraphNode *toNode = parentGraph.findNode(conn.to.nodeId);
            if (!toNode) continue;

            const Module *toMod = registry->findModule(toNode->moduleId);
            if (!toMod) continue;

            const Port *port = toMod->findInput(conn.to.portName);
            if (!port) continue;

            QString key = conn.to.nodeId + ":" + conn.to.portName;
            if (!inputPortMap.contains(key)) {
                Port extPort;
                extPort.name = conn.to.portName;
                extPort.type = port->type;
                extPort.defaultValue = port->defaultValue;

                // Обеспечиваем уникальность имён
                int suffix = 0;
                QString baseName = extPort.name;
                for (const auto &p : externalInputs) {
                    if (p.name == extPort.name) {
                        suffix++;
                        extPort.name = baseName + "_" + QString::number(suffix);
                    }
                }

                inputPortMap[key] = extPort.name;
                externalInputs.append(extPort);
            }
        }
    }

    // Выходы: связи из выделенных узлов в невыделенные
    QVector<Port> externalOutputs;
    QMap<QString, QString> outputPortMap; // "fromNodeId:fromPortName" → имя внешнего порта

    for (const auto &conn : parentGraph.connections) {
        if (selectedSet.contains(conn.from.nodeId) &&
            !selectedSet.contains(conn.to.nodeId)) {
            // Внешняя исходящая связь
            const GraphNode *fromNode = parentGraph.findNode(conn.from.nodeId);
            if (!fromNode) continue;

            const Module *fromMod = registry->findModule(fromNode->moduleId);
            if (!fromMod) continue;

            const Port *port = fromMod->findOutput(conn.from.portName);
            if (!port) continue;

            QString key = conn.from.nodeId + ":" + conn.from.portName;
            if (!outputPortMap.contains(key)) {
                Port extPort;
                extPort.name = conn.from.portName;
                extPort.type = port->type;

                // Обеспечиваем уникальность имён
                int suffix = 0;
                QString baseName = extPort.name;
                for (const auto &p : externalOutputs) {
                    if (p.name == extPort.name) {
                        suffix++;
                        extPort.name = baseName + "_" + QString::number(suffix);
                    }
                }

                outputPortMap[key] = extPort.name;
                externalOutputs.append(extPort);
            }
        }
    }

    // --- 3. Создаём модуль-обёртку ---
    result.module = Module::create(name);
    result.module.origin = "graph";
    result.module.category = "composite";
    result.module.description = QString("Композитный модуль '%1'").arg(name);
    result.module.inputs = externalInputs;
    result.module.outputs = externalOutputs;
    result.module.graphId = result.innerGraph.id;

    // --- 4. Обратная ссылка на модуль ---
    result.innerGraph.parentModuleId = result.module.id;

    return result;
}

QVector<Port> SubModuleFactory::findExternalInputs(
    const Graph &graph,
    const QStringList &selectedNodeIds,
    const ModuleRegistry *registry)
{
    Q_UNUSED(graph)
    Q_UNUSED(selectedNodeIds)
    Q_UNUSED(registry)
    return {};
}

QVector<Port> SubModuleFactory::findExternalOutputs(
    const Graph &graph,
    const QStringList &selectedNodeIds,
    const ModuleRegistry *registry)
{
    Q_UNUSED(graph)
    Q_UNUSED(selectedNodeIds)
    Q_UNUSED(registry)
    return {};
}

} // namespace DeltaQ
