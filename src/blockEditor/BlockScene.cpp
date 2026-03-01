// Графическая сцена блочного редактора — реализация
#include "BlockScene.h"
#include "NodeItem.h"
#include "PortItem.h"
#include "ConnectionItem.h"

#include "../core/ModuleRegistry.h"
#include "../core/CommandBus.h"
#include <deltaq/Graph.h>
#include <deltaq/Module.h>

#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>

namespace DeltaQ {

BlockScene::BlockScene(ModuleRegistry *registry, CommandBus *bus, QObject *parent)
    : QGraphicsScene(parent)
    , m_registry(registry)
    , m_commandBus(bus)
{
    setBackgroundBrush(QColor(35, 35, 35)); // Тёмный фон
}

// --- Узлы ---

NodeItem *BlockScene::addNodeItem(const GraphNode &node)
{
    if (m_nodes.contains(node.id))
        return m_nodes[node.id];

    const Module *mod = m_registry->findModule(node.moduleId);
    QString moduleName = mod ? mod->name : node.moduleId;
    QString category = mod ? mod->category : "custom";

    auto *item = new NodeItem(node.id, moduleName, category);
    item->setPos(node.position);

    // Добавляем порты из определения модуля
    if (mod) {
        for (const auto &p : mod->inputs)
            item->addInputPort(p.name, p.type);
        for (const auto &p : mod->outputs)
            item->addOutputPort(p.name, p.type);
    }

    addItem(item);
    m_nodes[node.id] = item;

    // Отслеживаем перемещение для MoveCommand
    connect(item, &NodeItem::positionChanged, this,
            [this, startPos = node.position](const QString &nodeId, const QPointF &newPos) {
        NodeItem *ni = m_nodes.value(nodeId);
        if (ni)
            emit nodeMovedByUser(nodeId, startPos, newPos);
    });

    return item;
}

void BlockScene::removeNodeItem(const QString &nodeId)
{
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) return;

    NodeItem *node = it.value();

    // Удаляем все соединения, связанные с узлом
    QList<ConnectionItem *> toRemove;
    for (auto *conn : m_connections) {
        if ((conn->sourcePort() && conn->sourcePort()->parentNode() == node) ||
            (conn->destPort() && conn->destPort()->parentNode() == node))
            toRemove.append(conn);
    }
    for (auto *conn : toRemove) {
        if (conn->sourcePort())
            conn->sourcePort()->removeConnection(conn);
        if (conn->destPort())
            conn->destPort()->removeConnection(conn);
        m_connections.removeOne(conn);
        removeItem(conn);
        delete conn;
    }

    removeItem(node);
    m_nodes.erase(it);
    delete node;
}

NodeItem *BlockScene::nodeItem(const QString &nodeId) const
{
    return m_nodes.value(nodeId, nullptr);
}

// --- Соединения ---

ConnectionItem *BlockScene::addConnectionItem(const QString &fromNodeId, const QString &fromPort,
                                               const QString &toNodeId, const QString &toPort)
{
    NodeItem *srcNode = m_nodes.value(fromNodeId);
    NodeItem *dstNode = m_nodes.value(toNodeId);
    if (!srcNode || !dstNode) return nullptr;

    PortItem *srcPort = srcNode->findPort(fromPort, PortDirection::Output);
    PortItem *dstPort = dstNode->findPort(toPort, PortDirection::Input);
    if (!srcPort || !dstPort) return nullptr;

    auto *conn = new ConnectionItem(srcPort, dstPort);
    addItem(conn);
    m_connections.append(conn);

    srcPort->addConnection(conn);
    dstPort->addConnection(conn);

    return conn;
}

void BlockScene::removeConnectionItem(const QString &fromNodeId, const QString &fromPort,
                                       const QString &toNodeId, const QString &toPort)
{
    for (int i = 0; i < m_connections.size(); ++i) {
        auto *conn = m_connections[i];
        if (conn->sourcePort() && conn->destPort() &&
            conn->sourcePort()->parentNode()->nodeId() == fromNodeId &&
            conn->sourcePort()->portName() == fromPort &&
            conn->destPort()->parentNode()->nodeId() == toNodeId &&
            conn->destPort()->portName() == toPort)
        {
            conn->sourcePort()->removeConnection(conn);
            conn->destPort()->removeConnection(conn);
            m_connections.removeAt(i);
            removeItem(conn);
            delete conn;
            return;
        }
    }
}

bool BlockScene::canConnect(PortItem *source, PortItem *dest) const
{
    if (!source || !dest) return false;

    // Нельзя соединять порты одного направления
    if (source->direction() == dest->direction()) return false;

    // Нельзя соединять порты одного узла
    if (source->parentNode() == dest->parentNode()) return false;

    // Убедимся, что source — output, dest — input
    PortItem *out = (source->direction() == PortDirection::Output) ? source : dest;
    PortItem *in = (source->direction() == PortDirection::Input) ? source : dest;

    // Проверяем совместимость типов
    QString srcType = out->portType();
    QString dstType = in->portType();
    if (srcType != dstType) {
        // Неявные преобразования
        bool compatible = false;
        if ((srcType == "int" && (dstType == "float" || dstType == "double")) ||
            (srcType == "float" && dstType == "double") ||
            (srcType == "bool" && dstType == "int"))
            compatible = true;
        if (!compatible) return false;
    }

    // Проверка на дубликат
    for (auto *conn : m_connections) {
        if (conn->sourcePort() == out && conn->destPort() == in)
            return false;
    }

    return true;
}

// --- Загрузка/выгрузка ---

void BlockScene::loadFromGraph(const Graph &graph)
{
    clearScene();
    m_currentGraphId = graph.id;

    // Сначала все узлы
    for (const auto &node : graph.nodes)
        addNodeItem(node);

    // Затем все соединения
    for (const auto &conn : graph.connections)
        addConnectionItem(conn.from.nodeId, conn.from.portName,
                          conn.to.nodeId, conn.to.portName);
}

Graph BlockScene::toGraph(const QString &graphId, const QString &graphName) const
{
    Graph g;
    g.id = graphId;
    g.name = graphName;

    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        GraphNode node;
        node.id = it.key();
        node.position = it.value()->pos();
        // Находим moduleId через ModuleRegistry (по имени модуля)
        auto *mod = m_registry->findModuleByName(it.value()->moduleName());
        node.moduleId = mod ? mod->id : it.value()->moduleName();
        g.nodes.append(node);
    }

    for (auto *conn : m_connections) {
        if (!conn->sourcePort() || !conn->destPort()) continue;
        GraphConnection gc;
        gc.from.nodeId = conn->sourcePort()->parentNode()->nodeId();
        gc.from.portName = conn->sourcePort()->portName();
        gc.to.nodeId = conn->destPort()->parentNode()->nodeId();
        gc.to.portName = conn->destPort()->portName();
        g.connections.append(gc);
    }

    return g;
}

void BlockScene::clearScene()
{
    m_draggingConnection = nullptr;
    m_draggingSourcePort = nullptr;
    m_connections.clear();
    m_nodes.clear();
    clear(); // QGraphicsScene::clear()
}

// --- Drag & Drop из палитры ---

void BlockScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dqmodule"))
        event->acceptProposedAction();
    else
        QGraphicsScene::dragEnterEvent(event);
}

void BlockScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dqmodule"))
        event->acceptProposedAction();
    else
        QGraphicsScene::dragMoveEvent(event);
}

void BlockScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dqmodule")) {
        QString moduleId = QString::fromUtf8(event->mimeData()->data("application/x-dqmodule"));
        emit nodeDropped(moduleId, event->scenePos());
        event->acceptProposedAction();
    } else {
        QGraphicsScene::dropEvent(event);
    }
}

// --- Соединение мышью ---

void BlockScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        PortItem *port = portItemAt(event->scenePos());
        if (port) {
            // Начинаем перетаскивание соединения
            m_draggingSourcePort = port;
            m_draggingConnection = new ConnectionItem(port);
            m_draggingConnection->setTempEndPoint(event->scenePos());
            addItem(m_draggingConnection);
            return; // Не передаём дальше
        }
    }
    QGraphicsScene::mousePressEvent(event);
}

void BlockScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_draggingConnection) {
        m_draggingConnection->setTempEndPoint(event->scenePos());

        // Подсвечиваем порт, если он подходит
        PortItem *port = portItemAt(event->scenePos());
        if (port && canConnect(m_draggingSourcePort, port))
            m_draggingConnection->setHighlighted(true);
        else
            m_draggingConnection->setHighlighted(false);
        return;
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void BlockScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_draggingConnection && event->button() == Qt::LeftButton) {
        PortItem *dest = portItemAt(event->scenePos());

        if (dest && canConnect(m_draggingSourcePort, dest)) {
            // Определяем направление: output → input
            PortItem *out = (m_draggingSourcePort->direction() == PortDirection::Output)
                            ? m_draggingSourcePort : dest;
            PortItem *in = (m_draggingSourcePort->direction() == PortDirection::Input)
                           ? m_draggingSourcePort : dest;

            emit connectionRequested(
                out->parentNode()->nodeId(), out->portName(),
                in->parentNode()->nodeId(), in->portName());
        }

        // Удаляем временное соединение
        removeItem(m_draggingConnection);
        delete m_draggingConnection;
        m_draggingConnection = nullptr;
        m_draggingSourcePort = nullptr;
        return;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

PortItem *BlockScene::portItemAt(const QPointF &scenePos) const
{
    // Ищем порт в области 28×28px вокруг курсора для удобства попадания
    constexpr qreal hitRadius = 14.0;
    QRectF hitArea(scenePos.x() - hitRadius, scenePos.y() - hitRadius,
                   hitRadius * 2, hitRadius * 2);

    PortItem *closest = nullptr;
    qreal closestDist = hitRadius * hitRadius;

    auto hitItems = this->items(hitArea);
    for (auto *item : hitItems) {
        auto *port = dynamic_cast<PortItem *>(item);
        if (!port) continue;

        QPointF center = port->centerInScene();
        qreal dx = center.x() - scenePos.x();
        qreal dy = center.y() - scenePos.y();
        qreal dist2 = dx * dx + dy * dy;
        if (dist2 < closestDist) {
            closestDist = dist2;
            closest = port;
        }
    }
    return closest;
}

} // namespace DeltaQ
