// Графическая сцена блочного редактора — узлы, соединения, drag&drop
#pragma once

#include <QGraphicsScene>
#include <QMap>
#include <QList>
#include <QString>

namespace DeltaQ {

struct Graph;
struct GraphNode;
struct Module;
class NodeItem;
class PortItem;
class ConnectionItem;
class ModuleRegistry;
class CommandBus;

class BlockScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit BlockScene(ModuleRegistry *registry, CommandBus *bus,
                        QObject *parent = nullptr);

    // Работа с узлами
    NodeItem *addNodeItem(const GraphNode &node);
    void removeNodeItem(const QString &nodeId);
    NodeItem *nodeItem(const QString &nodeId) const;
    QMap<QString, NodeItem *> nodeItems() const { return m_nodes; }

    // Работа с соединениями
    ConnectionItem *addConnectionItem(const QString &fromNodeId, const QString &fromPort,
                                      const QString &toNodeId, const QString &toPort);
    void removeConnectionItem(const QString &fromNodeId, const QString &fromPort,
                              const QString &toNodeId, const QString &toPort);
    QList<ConnectionItem *> connectionItems() const { return m_connections; }

    // Валидация соединения
    bool canConnect(PortItem *source, PortItem *dest) const;

    // Загрузка/выгрузка графа
    void loadFromGraph(const Graph &graph);
    Graph toGraph(const QString &graphId, const QString &graphName) const;
    void clearScene();

    // Используемый граф
    void setCurrentGraphId(const QString &id) { m_currentGraphId = id; }
    QString currentGraphId() const { return m_currentGraphId; }

signals:
    // Сигналы для создания команд (из подэтапа 3.3)
    void nodeMovedByUser(const QString &nodeId, const QPointF &oldPos, const QPointF &newPos);
    void connectionRequested(const QString &fromNodeId, const QString &fromPort,
                             const QString &toNodeId, const QString &toPort);
    void nodeDropped(const QString &moduleId, const QPointF &scenePos);

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    PortItem *portItemAt(const QPointF &scenePos) const;

    ModuleRegistry *m_registry;
    CommandBus *m_commandBus;

    QMap<QString, NodeItem *> m_nodes;
    QList<ConnectionItem *> m_connections;

    // Перетаскивание соединения
    ConnectionItem *m_draggingConnection = nullptr;
    PortItem *m_draggingSourcePort = nullptr;

    QString m_currentGraphId;
};

} // namespace DeltaQ
