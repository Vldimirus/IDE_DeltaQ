#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QVector>

namespace DeltaQ {

struct Port {
    QString name;
    QString type;
    QString defaultValue;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["type"] = type;
        if (!defaultValue.isEmpty())
            obj["default"] = defaultValue;
        return obj;
    }

    static Port fromJson(const QJsonObject &obj) {
        return {
            obj["name"].toString(),
            obj["type"].toString(),
            obj["default"].toString()
        };
    }
};

enum class PortDirection { Input, Output };
enum class PortKind { Data, Execution };

struct PortDefinition {
    QString name;
    QString type;
    PortDirection direction;
    PortKind kind = PortKind::Data;
};

struct Module {
    QString id;
    QString name;
    QString version;
    QString language;     // "c" or "cpp"
    QString description;
    QVector<Port> inputs;
    QVector<Port> outputs;
    QString sourcePath;   // relative to project
    QString headerPath;   // relative to project
    QStringList dependencies;
    QString origin;       // "user", "library", "graph"

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["version"] = version;
        obj["language"] = language;
        obj["description"] = description;
        obj["origin"] = origin;

        QJsonObject ports;
        QJsonArray inputArr, outputArr;
        for (const auto &p : inputs)
            inputArr.append(p.toJson());
        for (const auto &p : outputs)
            outputArr.append(p.toJson());
        ports["input"] = inputArr;
        ports["output"] = outputArr;
        obj["ports"] = ports;

        obj["source"] = sourcePath;
        obj["header"] = headerPath;

        QJsonArray deps;
        for (const auto &d : dependencies)
            deps.append(d);
        obj["dependencies"] = deps;

        return obj;
    }

    static Module fromJson(const QJsonObject &obj) {
        Module m;
        m.id = obj["id"].toString();
        m.name = obj["name"].toString();
        m.version = obj["version"].toString();
        m.language = obj["language"].toString();
        m.description = obj["description"].toString();
        m.origin = obj["origin"].toString();
        m.sourcePath = obj["source"].toString();
        m.headerPath = obj["header"].toString();

        auto ports = obj["ports"].toObject();
        for (const auto &v : ports["input"].toArray())
            m.inputs.append(Port::fromJson(v.toObject()));
        for (const auto &v : ports["output"].toArray())
            m.outputs.append(Port::fromJson(v.toObject()));

        for (const auto &v : obj["dependencies"].toArray())
            m.dependencies.append(v.toString());

        return m;
    }

    static Module create(const QString &name, const QString &lang = "c") {
        Module m;
        m.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m.name = name;
        m.version = "1.0.0";
        m.language = lang;
        m.origin = "user";
        return m;
    }
};

} // namespace DeltaQ
