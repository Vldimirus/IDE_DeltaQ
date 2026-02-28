// Компилятор графов — реализация
#include "GraphCompiler.h"
#include "IR.h"
#include "../core/ModuleRegistry.h"
#include <deltaq/Graph.h>
#include <deltaq/Module.h>

#include <QSet>

namespace DeltaQ {

GraphCompiler::GraphCompiler(ModuleRegistry *registry)
    : m_registry(registry)
{
}

CompilationResult GraphCompiler::compile(const Graph &graph)
{
    CompilationResult result;

    // 1. Валидация: проверяем что все модули существуют
    for (const auto &node : graph.nodes) {
        const Module *mod = m_registry->findModule(node.moduleId);
        if (!mod) {
            result.errors.append(
                QObject::tr("Node '%1': module '%2' not found in registry")
                .arg(node.id, node.moduleId));
        }
    }

    // Проверяем, что все соединения ведут к существующим узлам и портам
    for (const auto &conn : graph.connections) {
        const auto *fromNode = graph.findNode(conn.from.nodeId);
        const auto *toNode = graph.findNode(conn.to.nodeId);

        if (!fromNode) {
            result.errors.append(
                QObject::tr("Connection: source node '%1' not found").arg(conn.from.nodeId));
            continue;
        }
        if (!toNode) {
            result.errors.append(
                QObject::tr("Connection: target node '%1' not found").arg(conn.to.nodeId));
            continue;
        }

        // Проверяем порты
        const Module *fromMod = m_registry->findModule(fromNode->moduleId);
        const Module *toMod = m_registry->findModule(toNode->moduleId);

        if (fromMod && !fromMod->hasOutput(conn.from.portName)) {
            result.errors.append(
                QObject::tr("Connection: port '%1' not found in module '%2' outputs")
                .arg(conn.from.portName, fromMod->name));
        }
        if (toMod && !toMod->hasInput(conn.to.portName)) {
            result.errors.append(
                QObject::tr("Connection: port '%1' not found in module '%2' inputs")
                .arg(conn.to.portName, toMod->name));
        }

        // Проверяем совместимость типов
        if (fromMod && toMod) {
            const Port *outPort = fromMod->findOutput(conn.from.portName);
            const Port *inPort = toMod->findInput(conn.to.portName);
            if (outPort && inPort && !areTypesCompatible(outPort->type, inPort->type)) {
                result.errors.append(
                    QObject::tr("Type mismatch: '%1' (%2) → '%3' (%4)")
                    .arg(conn.from.portName, outPort->type, conn.to.portName, inPort->type));
            }
        }
    }

    if (!result.errors.isEmpty()) {
        result.success = false;
        return result;
    }

    // 2. Топологическая сортировка
    QStringList sortedNodes = topologicalSort(graph, result.errors);
    if (!result.errors.isEmpty()) {
        result.success = false;
        return result;
    }

    // 3. Генерация IR
    IR ir = generateIR(graph, sortedNodes, result);
    if (!result.errors.isEmpty()) {
        result.success = false;
        return result;
    }

    // 4. Генерация C-кода
    result.generatedCode = ir.emitCCode();

    // 5. Построение sourceMap (строка → nodeId)
    int lineNum = 0;
    for (const auto &line : result.generatedCode.split('\n')) {
        lineNum++;
        Q_UNUSED(line)
    }

    // Построение sourceMap из инструкций
    int codeLineOffset = ir.includes.size() + 2; // includes + пустая строка + main()
    for (int i = 0; i < ir.instructions.size(); ++i) {
        const auto &instr = ir.instructions[i];
        if (!instr.sourceNodeId.isEmpty()) {
            result.sourceMap[codeLineOffset + i + 1] = instr.sourceNodeId;
        }
    }

    result.success = true;
    return result;
}

QStringList GraphCompiler::topologicalSort(const Graph &graph, QStringList &errors)
{
    // Построение графа зависимостей: для каждого узла — от каких узлов он зависит
    QMap<QString, QSet<QString>> deps;  // nodeId → множество зависимостей
    QMap<QString, QSet<QString>> rdeps; // nodeId → кто от него зависит

    for (const auto &node : graph.nodes) {
        deps[node.id] = {};
        rdeps[node.id] = {};
    }

    for (const auto &conn : graph.connections) {
        deps[conn.to.nodeId].insert(conn.from.nodeId);
        rdeps[conn.from.nodeId].insert(conn.to.nodeId);
    }

    // Алгоритм Кана
    QStringList result;
    QStringList queue;

    // Начинаем с узлов без зависимостей
    for (auto it = deps.begin(); it != deps.end(); ++it) {
        if (it.value().isEmpty())
            queue.append(it.key());
    }

    while (!queue.isEmpty()) {
        QString nodeId = queue.takeFirst();
        result.append(nodeId);

        for (const auto &dependent : rdeps[nodeId]) {
            deps[dependent].remove(nodeId);
            if (deps[dependent].isEmpty())
                queue.append(dependent);
        }
    }

    // Если не все узлы обработаны — есть цикл
    if (result.size() != graph.nodes.size()) {
        errors.append(QObject::tr("Cycle detected in graph — cannot compile"));
    }

    return result;
}

IR GraphCompiler::generateIR(const Graph &graph, const QStringList &sortedNodes,
                              CompilationResult &result)
{
    IR ir;
    ir.addInclude("<stdio.h>");

    // Маппинг: nodeId:portName → имя переменной в C-коде
    QMap<QString, QString> portVarMap;

    for (const auto &nodeId : sortedNodes) {
        const GraphNode *node = graph.findNode(nodeId);
        if (!node) continue;

        const Module *mod = m_registry->findModule(node->moduleId);
        if (!mod) continue;

        ir.addInstruction(IRInstruction::makeComment(
            QObject::tr("Node: %1 (%2)").arg(mod->name, nodeId.left(8))));

        // Объявляем переменные для output-портов
        for (const auto &outPort : mod->outputs) {
            QString varName = QString("var_%1_%2").arg(
                nodeId.left(8).replace('-', '_'),
                outPort.name);
            QString cType = toCType(outPort.type);

            ir.addInstruction(IRInstruction::makeDeclareVar(varName, cType, nodeId));
            ir.declareVariable(varName, cType);
            portVarMap[nodeId + ":" + outPort.name] = varName;
        }

        // Подготавливаем аргументы (входные порты)
        QStringList callArgs;
        for (const auto &inPort : mod->inputs) {
            QString argValue;
            bool found = false;

            // Ищем входящее соединение для этого порта
            for (const auto &conn : graph.connections) {
                if (conn.to.nodeId == nodeId && conn.to.portName == inPort.name) {
                    QString sourceKey = conn.from.nodeId + ":" + conn.from.portName;
                    argValue = portVarMap.value(sourceKey);

                    // Проверяем необходимость конверсии типа
                    const GraphNode *srcNode = graph.findNode(conn.from.nodeId);
                    if (srcNode) {
                        const Module *srcMod = m_registry->findModule(srcNode->moduleId);
                        if (srcMod) {
                            const Port *srcPort = srcMod->findOutput(conn.from.portName);
                            if (srcPort && needsTypeConversion(srcPort->type, inPort.type)) {
                                QString convertedVar = argValue + "_conv";
                                QString cType = toCType(inPort.type);
                                ir.addInstruction(IRInstruction::makeTypeConvert(
                                    convertedVar, argValue, cType, nodeId));
                                ir.declareVariable(convertedVar, cType);
                                argValue = convertedVar;
                            }
                        }
                    }

                    found = true;
                    break;
                }
            }

            if (!found) {
                // Используем значение по умолчанию
                argValue = inPort.defaultValue.isEmpty() ? "0" : inPort.defaultValue;
            }

            callArgs.append(argValue);
        }

        // Генерируем вызов функции
        QString funcName = QString("dq_%1").arg(mod->name.toLower().replace(' ', '_'));

        if (mod->outputs.isEmpty()) {
            // Функция без возвращаемого значения
            ir.addInstruction(IRInstruction::makeCall({}, funcName, callArgs, nodeId));
        } else {
            // Функция с возвращаемым значением
            QString targetVar = portVarMap.value(nodeId + ":" + mod->outputs.first().name);
            ir.addInstruction(IRInstruction::makeCall(targetVar, funcName, callArgs, nodeId));
        }
    }

    ir.addInstruction(IRInstruction::makeReturn("0"));

    return ir;
}

bool GraphCompiler::areTypesCompatible(const QString &from, const QString &to) const
{
    if (from == to) return true;

    // Неявные преобразования
    if (from == "int" && (to == "float" || to == "double")) return true;
    if (from == "float" && to == "double") return true;
    if (from == "bool" && to == "int") return true;

    return false;
}

bool GraphCompiler::needsTypeConversion(const QString &from, const QString &to) const
{
    return from != to && areTypesCompatible(from, to);
}

QString GraphCompiler::toCType(const QString &portType) const
{
    if (portType == "int")    return "int";
    if (portType == "float")  return "float";
    if (portType == "double") return "double";
    if (portType == "bool")   return "int"; // C не имеет bool без stdbool.h
    if (portType == "string") return "const char*";
    return "int"; // fallback
}

} // namespace DeltaQ
