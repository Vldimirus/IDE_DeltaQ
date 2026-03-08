// Фабрика подмодулей — реализация
#include "SubModuleFactory.h"
#include "../core/ModuleRegistry.h"

#include <QSet>
#include <QUuid>

namespace DeltaQ {

namespace {

// Вычисляет среднюю позицию выделенных узлов, чтобы новый подмодуль вставлялся
// примерно в то же место, где была исходная группа блоков.
QPointF averageSelectionPosition(const Graph &graph, const QSet<QString> &selectedSet)
{
    QPointF sum;
    int count = 0;

    for (const auto &node : graph.nodes) {
        if (!selectedSet.contains(node.id))
            continue;
        sum += node.position;
        count++;
    }

    return count > 0 ? sum / count : QPointF();
}

} // namespace

SubModuleFactory::Result SubModuleFactory::createFromSelection(
    const Graph &parentGraph,
    const QStringList &selectedNodeIds,
    const QString &name,
    const ModuleRegistry *registry)
{
    Result result;
    QSet<QString> selectedSet(selectedNodeIds.begin(), selectedNodeIds.end());
    QSet<QString> usedExternalInputNames;
    QSet<QString> usedExternalOutputNames;

    // Модуль создаём заранее, чтобы по мере анализа сразу наполнять его boundary metadata.
    result.module = Module::create(name);

    // Делает имя внешнего порта уникальным в контракте нового подмодуля.
    const auto makeUniqueExternalName = [](QSet<QString> &usedNames, const QString &baseName) {
        QString candidate = baseName;
        int suffix = 1;
        while (usedNames.contains(candidate)) {
            candidate = baseName + "_" + QString::number(suffix);
            suffix++;
        }
        usedNames.insert(candidate);
        return candidate;
    };

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
                extPort.name = makeUniqueExternalName(usedExternalInputNames, conn.to.portName);
                extPort.type = port->type;
                extPort.defaultValue = port->defaultValue;
                extPort.kind = port->kind;

                inputPortMap[key] = extPort.name;
                externalInputs.append(extPort);
                result.module.boundaryInputs.append({
                    extPort.name,
                    conn.to.nodeId,
                    conn.to.portName,
                    port->kind
                });
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
                extPort.name = makeUniqueExternalName(usedExternalOutputNames, conn.from.portName);
                extPort.type = port->type;
                extPort.kind = port->kind;

                outputPortMap[key] = extPort.name;
                externalOutputs.append(extPort);
                result.module.boundaryOutputs.append({
                    extPort.name,
                    conn.from.nodeId,
                    conn.from.portName,
                    port->kind
                });
            }
        }
    }

    // --- 3. Создаём модуль-обёртку ---
    result.module.origin = "graph";
    result.module.category = "composite";
    result.module.description = QString("Композитный модуль '%1'").arg(name);
    result.module.inputs = externalInputs;
    result.module.outputs = externalOutputs;
    result.module.graphId = result.innerGraph.id;

    // --- 4. Обратная ссылка на модуль ---
    result.innerGraph.parentModuleId = result.module.id;

    // --- 5. Обновлённый родительский граф с подстановкой одного узла подмодуля ---
    result.updatedParentGraph = parentGraph;
    result.createdNode = GraphNode::create(
        result.module.id,
        averageSelectionPosition(parentGraph, selectedSet));

    // Удаляем исходные узлы из копии родительского графа — removeNode автоматически
    // убирает и все внутренние/граничные связи выбранной группы.
    for (const auto &nodeId : selectedNodeIds)
        result.updatedParentGraph.removeNode(nodeId);

    result.updatedParentGraph.addNode(result.createdNode);

    // Возвращаем внешние связи на новый узел подмодуля, сохраняя и data, и exec границу.
    for (const auto &conn : parentGraph.connections) {
        if (!selectedSet.contains(conn.from.nodeId) &&
            selectedSet.contains(conn.to.nodeId)) {
            const QString key = conn.to.nodeId + ":" + conn.to.portName;
            const QString submodulePort = inputPortMap.value(key);
            if (!submodulePort.isEmpty()) {
                result.updatedParentGraph.addConnection({
                    conn.from,
                    {result.createdNode.id, submodulePort},
                    conn.kind
                });
            }
            continue;
        }

        if (selectedSet.contains(conn.from.nodeId) &&
            !selectedSet.contains(conn.to.nodeId)) {
            const QString key = conn.from.nodeId + ":" + conn.from.portName;
            const QString submodulePort = outputPortMap.value(key);
            if (!submodulePort.isEmpty()) {
                result.updatedParentGraph.addConnection({
                    {result.createdNode.id, submodulePort},
                    conn.to,
                    conn.kind
                });
            }
        }
    }

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
