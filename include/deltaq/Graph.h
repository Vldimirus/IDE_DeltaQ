#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QPointF>
#include <QUuid>
#include <QVector>
#include <QMap>
#include <QSet>
#include <algorithm>

namespace DeltaQ {

struct GraphNode {
    QString id;
    QString moduleId;
    QPointF position;
    QMap<QString, QString> properties;

    bool operator==(const GraphNode &other) const {
        return id == other.id;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["module_id"] = moduleId;
        QJsonObject pos;
        pos["x"] = position.x();
        pos["y"] = position.y();
        obj["position"] = pos;
        if (!properties.isEmpty()) {
            QJsonObject props;
            for (auto it = properties.begin(); it != properties.end(); ++it)
                props[it.key()] = it.value();
            obj["properties"] = props;
        }
        return obj;
    }

    static GraphNode fromJson(const QJsonObject &obj) {
        GraphNode n;
        n.id = obj["id"].toString();
        n.moduleId = obj["module_id"].toString();
        auto pos = obj["position"].toObject();
        n.position = QPointF(pos["x"].toDouble(), pos["y"].toDouble());
        auto props = obj["properties"].toObject();
        for (auto it = props.begin(); it != props.end(); ++it)
            n.properties[it.key()] = it.value().toString();
        return n;
    }

    static GraphNode create(const QString &moduleId, QPointF pos = {}) {
        GraphNode n;
        n.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        n.moduleId = moduleId;
        n.position = pos;
        return n;
    }
};

struct GraphConnection {
    struct Endpoint {
        QString nodeId;
        QString portName;

        bool operator==(const Endpoint &other) const {
            return nodeId == other.nodeId && portName == other.portName;
        }

        QJsonObject toJson() const {
            QJsonObject obj;
            obj["node"] = nodeId;
            obj["port"] = portName;
            return obj;
        }

        static Endpoint fromJson(const QJsonObject &obj) {
            return {obj["node"].toString(), obj["port"].toString()};
        }
    };

    Endpoint from;
    Endpoint to;

    bool operator==(const GraphConnection &other) const {
        return from == other.from && to == other.to;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["from"] = from.toJson();
        obj["to"] = to.toJson();
        return obj;
    }

    static GraphConnection fromJson(const QJsonObject &obj) {
        return {
            Endpoint::fromJson(obj["from"].toObject()),
            Endpoint::fromJson(obj["to"].toObject())
        };
    }
};

struct Graph {
    QString id;
    QString name;
    QVector<GraphNode> nodes;
    QVector<GraphConnection> connections;

    bool operator==(const Graph &other) const {
        return id == other.id;
    }

    // Поиск узла по id
    GraphNode *findNode(const QString &nodeId) {
        for (auto &n : nodes)
            if (n.id == nodeId) return &n;
        return nullptr;
    }

    const GraphNode *findNode(const QString &nodeId) const {
        for (const auto &n : nodes)
            if (n.id == nodeId) return &n;
        return nullptr;
    }

    // Добавление узла (false если id дублируется)
    bool addNode(GraphNode node) {
        if (findNode(node.id) != nullptr) return false;
        nodes.append(std::move(node));
        return true;
    }

    // Удаление узла И всех его соединений
    bool removeNode(const QString &nodeId) {
        auto it = std::find_if(nodes.begin(), nodes.end(),
            [&](const GraphNode &n) { return n.id == nodeId; });
        if (it == nodes.end()) return false;
        nodes.erase(it);
        // Удаляем все соединения, связанные с этим узлом
        connections.erase(
            std::remove_if(connections.begin(), connections.end(),
                [&](const GraphConnection &c) {
                    return c.from.nodeId == nodeId || c.to.nodeId == nodeId;
                }),
            connections.end());
        return true;
    }

    // Добавление соединения
    bool addConnection(GraphConnection conn) {
        connections.append(std::move(conn));
        return true;
    }

    // Удаление конкретного соединения
    bool removeConnection(const QString &fromNodeId, const QString &fromPort,
                          const QString &toNodeId, const QString &toPort) {
        auto it = std::find_if(connections.begin(), connections.end(),
            [&](const GraphConnection &c) {
                return c.from.nodeId == fromNodeId && c.from.portName == fromPort
                    && c.to.nodeId == toNodeId && c.to.portName == toPort;
            });
        if (it == connections.end()) return false;
        connections.erase(it);
        return true;
    }

    // Все соединения, связанные с узлом
    QVector<GraphConnection> connectionsForNode(const QString &nodeId) const {
        QVector<GraphConnection> result;
        for (const auto &c : connections)
            if (c.from.nodeId == nodeId || c.to.nodeId == nodeId)
                result.append(c);
        return result;
    }

    int nodeCount() const { return nodes.size(); }
    int connectionCount() const { return connections.size(); }

    // Валидация: id и name не пусты, нет дубликатов узлов
    bool isValid() const {
        if (id.isEmpty() || name.isEmpty()) return false;
        QSet<QString> ids;
        for (const auto &n : nodes) {
            if (ids.contains(n.id)) return false;
            ids.insert(n.id);
        }
        return true;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        QJsonArray nodeArr, connArr;
        for (const auto &n : nodes)
            nodeArr.append(n.toJson());
        for (const auto &c : connections)
            connArr.append(c.toJson());
        obj["nodes"] = nodeArr;
        obj["connections"] = connArr;
        return obj;
    }

    static Graph fromJson(const QJsonObject &obj) {
        Graph g;
        g.id = obj["id"].toString();
        g.name = obj["name"].toString();
        for (const auto &v : obj["nodes"].toArray())
            g.nodes.append(GraphNode::fromJson(v.toObject()));
        for (const auto &v : obj["connections"].toArray())
            g.connections.append(GraphConnection::fromJson(v.toObject()));
        return g;
    }

    static Graph create(const QString &name) {
        Graph g;
        g.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        g.name = name;
        return g;
    }
};

} // namespace DeltaQ
