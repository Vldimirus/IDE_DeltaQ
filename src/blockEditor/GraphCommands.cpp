// Команды графа — реализация undo/redo
#include "GraphCommands.h"
#include "BlockScene.h"
#include "NodeItem.h"

namespace DeltaQ {

// --- AddNodeCommand ---

AddNodeCommand::AddNodeCommand(BlockScene *scene, Graph *graph, const GraphNode &node)
    : m_scene(scene), m_graph(graph), m_node(node)
{
}

void AddNodeCommand::execute()
{
    m_graph->addNode(m_node);
    m_scene->addNodeItem(m_node);
}

void AddNodeCommand::undo()
{
    m_scene->removeNodeItem(m_node.id);
    m_graph->removeNode(m_node.id);
}

QString AddNodeCommand::description() const
{
    return QObject::tr("Add node");
}

// --- RemoveNodeCommand ---

RemoveNodeCommand::RemoveNodeCommand(BlockScene *scene, Graph *graph, const QString &nodeId)
    : m_scene(scene), m_graph(graph), m_nodeId(nodeId)
{
}

void RemoveNodeCommand::execute()
{
    // Сохраняем узел и его соединения перед удалением
    const GraphNode *node = m_graph->findNode(m_nodeId);
    if (node)
        m_savedNode = *node;
    m_savedConnections = m_graph->connectionsForNode(m_nodeId);

    m_scene->removeNodeItem(m_nodeId);
    m_graph->removeNode(m_nodeId); // Каскадно удаляет соединения
}

void RemoveNodeCommand::undo()
{
    // Восстанавливаем узел
    m_graph->addNode(m_savedNode);
    m_scene->addNodeItem(m_savedNode);

    // Восстанавливаем все соединения
    for (const auto &conn : m_savedConnections) {
        m_graph->addConnection(conn);
        m_scene->addConnectionItem(conn.from.nodeId, conn.from.portName,
                                   conn.to.nodeId, conn.to.portName);
    }
}

QString RemoveNodeCommand::description() const
{
    return QObject::tr("Remove node");
}

// --- MoveNodeCommand ---

MoveNodeCommand::MoveNodeCommand(BlockScene *scene, Graph *graph,
                                 const QString &nodeId, const QPointF &oldPos, const QPointF &newPos)
    : m_scene(scene), m_graph(graph), m_nodeId(nodeId), m_oldPos(oldPos), m_newPos(newPos)
{
}

void MoveNodeCommand::execute()
{
    auto *item = m_scene->nodeItem(m_nodeId);
    if (item)
        item->setPos(m_newPos);

    auto *node = m_graph->findNode(m_nodeId);
    if (node)
        node->position = m_newPos;
}

void MoveNodeCommand::undo()
{
    auto *item = m_scene->nodeItem(m_nodeId);
    if (item)
        item->setPos(m_oldPos);

    auto *node = m_graph->findNode(m_nodeId);
    if (node)
        node->position = m_oldPos;
}

QString MoveNodeCommand::description() const
{
    return QObject::tr("Move node");
}

bool MoveNodeCommand::mergeWith(const Command *other)
{
    auto *move = dynamic_cast<const MoveNodeCommand *>(other);
    if (!move || move->m_nodeId != m_nodeId)
        return false;
    m_newPos = move->m_newPos;
    return true;
}

// --- ConnectCommand ---

ConnectCommand::ConnectCommand(BlockScene *scene, Graph *graph, const GraphConnection &conn)
    : m_scene(scene), m_graph(graph), m_conn(conn)
{
}

void ConnectCommand::execute()
{
    m_graph->addConnection(m_conn);
    m_scene->addConnectionItem(m_conn.from.nodeId, m_conn.from.portName,
                               m_conn.to.nodeId, m_conn.to.portName);
}

void ConnectCommand::undo()
{
    m_scene->removeConnectionItem(m_conn.from.nodeId, m_conn.from.portName,
                                  m_conn.to.nodeId, m_conn.to.portName);
    m_graph->removeConnection(m_conn.from.nodeId, m_conn.from.portName,
                              m_conn.to.nodeId, m_conn.to.portName);
}

QString ConnectCommand::description() const
{
    return QObject::tr("Connect ports");
}

// --- DisconnectCommand ---

DisconnectCommand::DisconnectCommand(BlockScene *scene, Graph *graph, const GraphConnection &conn)
    : m_scene(scene), m_graph(graph), m_conn(conn)
{
}

void DisconnectCommand::execute()
{
    m_scene->removeConnectionItem(m_conn.from.nodeId, m_conn.from.portName,
                                  m_conn.to.nodeId, m_conn.to.portName);
    m_graph->removeConnection(m_conn.from.nodeId, m_conn.from.portName,
                              m_conn.to.nodeId, m_conn.to.portName);
}

void DisconnectCommand::undo()
{
    m_graph->addConnection(m_conn);
    m_scene->addConnectionItem(m_conn.from.nodeId, m_conn.from.portName,
                               m_conn.to.nodeId, m_conn.to.portName);
}

QString DisconnectCommand::description() const
{
    return QObject::tr("Disconnect ports");
}

// --- ChangePropertyCommand ---

ChangePropertyCommand::ChangePropertyCommand(Graph *graph, const QString &nodeId,
                                             const QString &key, const QString &newValue)
    : m_graph(graph), m_nodeId(nodeId), m_key(key), m_newValue(newValue)
{
}

void ChangePropertyCommand::execute()
{
    auto *node = m_graph->findNode(m_nodeId);
    if (!node) return;

    m_hadOldValue = node->properties.contains(m_key);
    if (m_hadOldValue)
        m_oldValue = node->properties[m_key];

    node->properties[m_key] = m_newValue;
}

void ChangePropertyCommand::undo()
{
    auto *node = m_graph->findNode(m_nodeId);
    if (!node) return;

    if (m_hadOldValue)
        node->properties[m_key] = m_oldValue;
    else
        node->properties.remove(m_key);
}

QString ChangePropertyCommand::description() const
{
    return QObject::tr("Change property '%1'").arg(m_key);
}

} // namespace DeltaQ
