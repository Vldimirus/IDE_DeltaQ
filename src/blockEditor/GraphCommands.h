// Команды графа — undo/redo для всех операций с узлами и соединениями
#pragma once

#include "../core/CommandBus.h"
#include <deltaq/Graph.h>
#include <QPointF>
#include <QVector>

namespace DeltaQ {

class BlockScene;

// Добавление узла
class AddNodeCommand : public Command {
public:
    AddNodeCommand(BlockScene *scene, Graph *graph, const GraphNode &node);
    void execute() override;
    void undo() override;
    QString description() const override;

private:
    BlockScene *m_scene;
    Graph *m_graph;
    GraphNode m_node;
};

// Удаление узла (с сохранением связей для undo)
class RemoveNodeCommand : public Command {
public:
    RemoveNodeCommand(BlockScene *scene, Graph *graph, const QString &nodeId);
    void execute() override;
    void undo() override;
    QString description() const override;

private:
    BlockScene *m_scene;
    Graph *m_graph;
    QString m_nodeId;
    GraphNode m_savedNode;
    QVector<GraphConnection> m_savedConnections;
};

// Перемещение узла
class MoveNodeCommand : public Command {
public:
    MoveNodeCommand(BlockScene *scene, Graph *graph,
                    const QString &nodeId, const QPointF &oldPos, const QPointF &newPos);
    void execute() override;
    void undo() override;
    QString description() const override;
    bool mergeWith(const Command *other) override;

private:
    BlockScene *m_scene;
    Graph *m_graph;
    QString m_nodeId;
    QPointF m_oldPos;
    QPointF m_newPos;
};

// Создание соединения
class ConnectCommand : public Command {
public:
    ConnectCommand(BlockScene *scene, Graph *graph, const GraphConnection &conn);
    void execute() override;
    void undo() override;
    QString description() const override;

private:
    BlockScene *m_scene;
    Graph *m_graph;
    GraphConnection m_conn;
};

// Удаление соединения
class DisconnectCommand : public Command {
public:
    DisconnectCommand(BlockScene *scene, Graph *graph, const GraphConnection &conn);
    void execute() override;
    void undo() override;
    QString description() const override;

private:
    BlockScene *m_scene;
    Graph *m_graph;
    GraphConnection m_conn;
};

// Изменение свойства узла
class ChangePropertyCommand : public Command {
public:
    ChangePropertyCommand(Graph *graph, const QString &nodeId,
                          const QString &key, const QString &newValue);
    void execute() override;
    void undo() override;
    QString description() const override;

private:
    Graph *m_graph;
    QString m_nodeId;
    QString m_key;
    QString m_newValue;
    QString m_oldValue;
    bool m_hadOldValue = false;
};

} // namespace DeltaQ
