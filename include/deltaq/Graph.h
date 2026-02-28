#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QPointF>
#include <QUuid>
#include <QVector>
#include <QMap>

namespace DeltaQ {

struct GraphNode {
    QString id;
    QString moduleId;
    QPointF position;
    QMap<QString, QString> properties;

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
