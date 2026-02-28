// Визуальная отладка графов — реализация
#include "GraphDebugger.h"
#include "BlockScene.h"
#include "NodeItem.h"
#include "PortItem.h"
#include "../debug/DebugManager.h"
#include "../debug/DebugTypes.h"

namespace DeltaQ {

GraphDebugger::GraphDebugger(BlockScene *scene, DebugManager *debugManager,
                             QObject *parent)
    : QObject(parent)
    , m_scene(scene)
    , m_debugManager(debugManager)
{
    connect(debugManager, &DebugManager::breakpointHit,
            this, &GraphDebugger::onBreakpointHit);
    connect(debugManager, &DebugManager::stepped,
            this, &GraphDebugger::onStepped);
    connect(debugManager, &DebugManager::debugStopped,
            this, &GraphDebugger::onDebugStopped);
    connect(debugManager, &DebugManager::variablesUpdated,
            this, &GraphDebugger::onVariablesUpdated);
}

void GraphDebugger::setSourceMap(const QMap<int, QString> &sourceMap)
{
    m_lineToNode = sourceMap;
}

void GraphDebugger::clearDebugState()
{
    // Сбрасываем все узлы
    for (auto *node : m_scene->nodeItems()) {
        node->setHighlighted(false);
        node->setCompleted(false);
        node->setError(false);
    }
    m_currentNodeId.clear();
    m_completedNodes.clear();
}

void GraphDebugger::onBreakpointHit(const QString &file, int line)
{
    Q_UNUSED(file)
    QString nodeId = m_lineToNode.value(line);
    if (!nodeId.isEmpty())
        highlightNode(nodeId);
}

void GraphDebugger::onStepped(const QString &file, int line)
{
    Q_UNUSED(file)

    // Предыдущий узел помечаем как выполненный
    if (!m_currentNodeId.isEmpty()) {
        auto *prevNode = m_scene->nodeItem(m_currentNodeId);
        if (prevNode) {
            prevNode->setHighlighted(false);
            prevNode->setCompleted(true);
        }
        if (!m_completedNodes.contains(m_currentNodeId))
            m_completedNodes.append(m_currentNodeId);
    }

    // Подсвечиваем новый узел
    QString nodeId = m_lineToNode.value(line);
    if (!nodeId.isEmpty())
        highlightNode(nodeId);
}

void GraphDebugger::onDebugStopped()
{
    clearDebugState();
}

void GraphDebugger::onVariablesUpdated(const QVector<Variable> &vars)
{
    updatePortValues(vars);
}

void GraphDebugger::highlightNode(const QString &nodeId)
{
    // Снимаем подсветку с предыдущего
    if (!m_currentNodeId.isEmpty()) {
        auto *prevNode = m_scene->nodeItem(m_currentNodeId);
        if (prevNode)
            prevNode->setHighlighted(false);
    }

    m_currentNodeId = nodeId;

    // Подсвечиваем текущий
    auto *node = m_scene->nodeItem(nodeId);
    if (node)
        node->setHighlighted(true);
}

void GraphDebugger::updatePortValues(const QVector<Variable> &vars)
{
    // Ищем переменные вида var_{nodeId}_{portName}
    for (const auto &var : vars) {
        if (!var.name.startsWith("var_")) continue;

        // Парсим имя переменной: var_{shortId}_{portName}
        // Ищем узел, чей id начинается с shortId
        QString rest = var.name.mid(4); // убираем "var_"
        int underscorePos = rest.indexOf('_');
        if (underscorePos < 0) continue;

        QString shortId = rest.left(underscorePos);
        QString portName = rest.mid(underscorePos + 1);

        // Находим узел по короткому id
        for (auto it = m_scene->nodeItems().begin(); it != m_scene->nodeItems().end(); ++it) {
            QString nodeShort = it.key().left(8).replace('-', '_');
            if (nodeShort == shortId) {
                auto *node = it.value();
                // Находим порт и обновляем tooltip
                for (auto *port : node->outputPorts()) {
                    if (port->portName() == portName) {
                        port->setToolTip(QString("%1 (%2) = %3")
                            .arg(portName, port->portType(), var.value));
                    }
                }
                break;
            }
        }
    }
}

} // namespace DeltaQ
