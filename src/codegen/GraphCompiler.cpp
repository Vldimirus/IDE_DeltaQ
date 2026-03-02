// Компилятор графов — реализация
#include "GraphCompiler.h"
#include "IR.h"
#include "../core/ModuleRegistry.h"
#include "../core/GraphStore.h"
#include "../uiDesigner/UIModuleFactory.h"
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

    // 1.5. Проверка совместимости языков модулей
    QSet<QString> usedLanguages;
    for (const auto &node : graph.nodes) {
        const Module *mod = m_registry->findModule(node.moduleId);
        if (mod && !mod->language.isEmpty())
            usedLanguages.insert(mod->language);
    }
    // C и C++ совместимы, но Python/Rust — нет
    QSet<QString> incompatible = usedLanguages;
    incompatible.remove("c");
    incompatible.remove("cpp");
    if (!incompatible.isEmpty() && (usedLanguages.contains("c") || usedLanguages.contains("cpp"))) {
        for (const auto &lang : incompatible) {
            result.errors.append(
                QObject::tr("Language '%1' is not compatible with C/C++ modules in this graph")
                .arg(lang));
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
    // Проверяем наличие execution-связей
    bool hasExecConnections = false;
    for (const auto &conn : graph.connections) {
        if (conn.kind == PortKind::Execution) {
            hasExecConnections = true;
            break;
        }
    }

    // Если есть exec-связи — используем exec-aware сортировку
    if (hasExecConnections)
        return topologicalSortByExecution(graph, errors);

    // Иначе — обычная data flow сортировка (обратная совместимость)
    return topologicalSortByData(graph, errors);
}

QStringList GraphCompiler::topologicalSortByData(const Graph &graph, QStringList &errors)
{
    // Построение графа зависимостей: для каждого узла — от каких узлов он зависит
    QMap<QString, QSet<QString>> deps;  // nodeId → множество зависимостей
    QMap<QString, QSet<QString>> rdeps; // nodeId → кто от него зависит

    for (const auto &node : graph.nodes) {
        deps[node.id] = {};
        rdeps[node.id] = {};
    }

    for (const auto &conn : graph.connections) {
        if (conn.kind == PortKind::Execution)
            continue; // Игнорируем exec-связи для data-сортировки
        deps[conn.to.nodeId].insert(conn.from.nodeId);
        rdeps[conn.from.nodeId].insert(conn.to.nodeId);
    }

    // Алгоритм Кана
    QStringList result;
    QStringList queue;

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

    if (result.size() != graph.nodes.size()) {
        errors.append(QObject::tr("Cycle detected in graph — cannot compile"));
    }

    return result;
}

QStringList GraphCompiler::topologicalSortByExecution(const Graph &graph, QStringList &errors)
{
    // Строим граф data-зависимостей (без exec-связей)
    QMap<QString, QSet<QString>> dataDeps;
    for (const auto &node : graph.nodes)
        dataDeps[node.id] = {};

    for (const auto &conn : graph.connections) {
        if (conn.kind != PortKind::Execution)
            dataDeps[conn.to.nodeId].insert(conn.from.nodeId);
    }

    // Строим exec-граф: exec-зависимости
    QMap<QString, QSet<QString>> execDeps;   // nodeId → входящие exec-связи
    QMap<QString, QStringList> execNext;     // nodeId → исходящие exec-связи (в порядке)
    for (const auto &node : graph.nodes) {
        execDeps[node.id] = {};
    }
    for (const auto &conn : graph.connections) {
        if (conn.kind == PortKind::Execution) {
            execDeps[conn.to.nodeId].insert(conn.from.nodeId);
            execNext[conn.from.nodeId].append(conn.to.nodeId);
        }
    }

    // Находим стартовые exec-узлы (участвуют в exec-связях, но без входящих exec)
    QStringList execStarts;
    QSet<QString> execParticipants;
    for (const auto &conn : graph.connections) {
        if (conn.kind == PortKind::Execution) {
            execParticipants.insert(conn.from.nodeId);
            execParticipants.insert(conn.to.nodeId);
        }
    }
    for (const auto &nodeId : execParticipants) {
        if (execDeps[nodeId].isEmpty())
            execStarts.append(nodeId);
    }

    QStringList result;
    QSet<QString> visited;

    // Обходим exec-цепочку — перед каждым узлом вставляем его data-зависимости
    QStringList execQueue = execStarts;
    QSet<QString> execProcessed;

    while (!execQueue.isEmpty()) {
        QString nodeId = execQueue.takeFirst();
        if (execProcessed.contains(nodeId))
            continue;
        execProcessed.insert(nodeId);

        // Вставляем data-зависимости рекурсивно
        insertWithDataDeps(nodeId, graph, visited, result, dataDeps);

        // Следуем по exec-выходам
        for (const auto &next : execNext[nodeId])
            execQueue.append(next);
    }

    // Добавляем оставшиеся data-only узлы (не участвующие в exec-flow)
    QStringList remainingErrors;
    QStringList dataOnly = topologicalSortByData(graph, remainingErrors);
    for (const auto &nodeId : dataOnly) {
        if (!visited.contains(nodeId)) {
            visited.insert(nodeId);
            result.append(nodeId);
        }
    }

    if (result.size() != graph.nodes.size()) {
        errors.append(QObject::tr("Cycle detected in graph — cannot compile"));
    }

    return result;
}

void GraphCompiler::insertWithDataDeps(const QString &nodeId, const Graph &graph,
                                        QSet<QString> &visited, QStringList &result,
                                        const QMap<QString, QSet<QString>> &dataDeps)
{
    if (visited.contains(nodeId))
        return;

    // Сначала вставляем все data-зависимости
    for (const auto &dep : dataDeps[nodeId]) {
        Q_UNUSED(graph)
        insertWithDataDeps(dep, graph, visited, result, dataDeps);
    }

    visited.insert(nodeId);
    result.append(nodeId);
}

IR GraphCompiler::generateIR(const Graph &graph, const QStringList &sortedNodes,
                              CompilationResult &result)
{
    IR ir;
    ir.addInclude("<stdio.h>");

    // Сбор уникальных модулей — дедупликация includes и определений
    QSet<QString> processedModuleIds;
    for (const auto &node : graph.nodes) {
        if (processedModuleIds.contains(node.moduleId))
            continue;
        processedModuleIds.insert(node.moduleId);

        const Module *mod = m_registry->findModule(node.moduleId);
        if (!mod) continue;

        // Собираем includes модуля
        for (const auto &inc : mod->includes) {
            QString header = inc;
            if (!header.startsWith('<') && !header.startsWith('"'))
                header = QString("<%1>").arg(inc);
            ir.addInclude(header);
        }

        // Собираем sourceCode модуля (определение функции)
        if (!mod->sourceCode.isEmpty()) {
            ir.moduleSources.append(mod->sourceCode);
        }
    }

    // Маппинг: nodeId:portName → имя переменной в C-коде
    QMap<QString, QString> portVarMap;

    for (const auto &nodeId : sortedNodes) {
        const GraphNode *node = graph.findNode(nodeId);
        if (!node) continue;

        const Module *mod = m_registry->findModule(node->moduleId);
        if (!mod) continue;

        ir.addInstruction(IRInstruction::makeComment(
            QObject::tr("Node: %1 (%2)").arg(mod->name, nodeId.left(8))));

        // Объявляем переменные для output-портов (пропускаем exec-порты)
        for (const auto &outPort : mod->outputs) {
            if (outPort.kind == PortKind::Execution)
                continue;
            QString varName = QString("var_%1_%2").arg(
                nodeId.left(8).replace('-', '_'),
                outPort.name);
            QString cType = toCType(outPort.type);

            ir.addInstruction(IRInstruction::makeDeclareVar(varName, cType, nodeId));
            ir.declareVariable(varName, cType);
            portVarMap[nodeId + ":" + outPort.name] = varName;
        }

        // Подготавливаем аргументы (входные порты, пропускаем exec-порты)
        QStringList callArgs;
        for (const auto &inPort : mod->inputs) {
            if (inPort.kind == PortKind::Execution)
                continue;
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
        if (UIModuleFactory::isUIModule(node->moduleId) ||
            UIModuleFactory::isUIModuleByName(mod->name)) {
            // UI-модуль: SDL2-специфичный код
            QString widgetVar = QString("widget_%1").arg(nodeId.left(8).replace('-', '_'));

            if (mod->name == "UI_Button") {
                ir.addInstruction(IRInstruction::makeComment(
                    QString("UI: Button '%1'").arg(callArgs.value(0))));
                // onClick → будет обработан как callback
                if (!mod->outputs.isEmpty()) {
                    QString targetVar = portVarMap.value(nodeId + ":" + mod->outputs.first().name);
                    ir.addInstruction(IRInstruction::makeAssign(
                        targetVar, "0 /* onClick callback */", nodeId));
                }
            } else if (mod->name == "UI_Label") {
                ir.addInstruction(IRInstruction::makeComment(
                    QString("UI: Label, text=%1").arg(callArgs.value(0))));
                QString funcName = "ui_label_set_text";
                ir.addInstruction(IRInstruction::makeCall(
                    {}, funcName, {"&" + widgetVar, callArgs.value(0)}, nodeId));
            } else if (mod->name == "UI_Slider") {
                ir.addInstruction(IRInstruction::makeComment(
                    QString("UI: Slider [%1..%2]").arg(callArgs.value(0), callArgs.value(1))));
                if (mod->outputs.size() > 1) {
                    QString targetVar = portVarMap.value(nodeId + ":" + mod->outputs[1].name);
                    ir.addInstruction(IRInstruction::makeAssign(
                        targetVar, callArgs.value(2), nodeId));
                }
            } else {
                // Общий вариант для остальных UI-модулей
                QString funcName = QString("ui_%1_create").arg(
                    mod->name.toLower().replace("ui_", ""));
                ir.addInstruction(IRInstruction::makeCall({}, funcName, callArgs, nodeId));
            }
        } else if (mod->origin == "graph" && !mod->graphId.isEmpty() && m_graphStore) {
            // Композитный подмодуль — рекурсивная компиляция
            QString subFuncName = compileSubModule(*mod, result);
            if (!subFuncName.isEmpty()) {
                if (mod->outputs.isEmpty()) {
                    ir.addInstruction(IRInstruction::makeCall({}, subFuncName, callArgs, nodeId));
                } else {
                    QString targetVar = portVarMap.value(nodeId + ":" + mod->outputs.first().name);
                    ir.addInstruction(IRInstruction::makeCall(targetVar, subFuncName, callArgs, nodeId));
                }
            }
        } else {
            // Обычный модуль
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
    }

    ir.addInstruction(IRInstruction::makeReturn("0"));

    return ir;
}

QString GraphCompiler::compileSubModule(const Module &mod, CompilationResult &result)
{
    // Имя функции для подмодуля
    QString funcName = QString("dq_sub_%1").arg(mod.name.toLower().replace(' ', '_'));

    // Проверяем, не был ли он уже скомпилирован
    if (m_compiledSubModules.contains(mod.id))
        return funcName;
    m_compiledSubModules.insert(mod.id);

    // Находим внутренний граф
    const Graph *innerGraph = m_graphStore->findGraph(mod.graphId);
    if (!innerGraph) {
        result.errors.append(
            QObject::tr("Подмодуль '%1': внутренний граф '%2' не найден")
            .arg(mod.name, mod.graphId));
        return {};
    }

    // Рекурсивно компилируем внутренний граф
    CompilationResult subResult = compile(*innerGraph);
    if (!subResult.success) {
        for (const auto &err : subResult.errors)
            result.errors.append(QObject::tr("В подмодуле '%1': %2").arg(mod.name, err));
        return {};
    }

    return funcName;
}

bool GraphCompiler::areTypesCompatible(const QString &from, const QString &to) const
{
    if (from == to) return true;

    // Exec-тип совместим только с exec
    if (from == "exec" || to == "exec") return false;

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
