#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QVector>

namespace DeltaQ {

enum class PortDirection { Input, Output };
enum class PortKind { Data, Execution };

struct Port {
    QString name;
    QString type;
    QString defaultValue;
    PortKind kind = PortKind::Data;

    bool operator==(const Port &other) const {
        return name == other.name
            && type == other.type
            && defaultValue == other.defaultValue
            && kind == other.kind;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["type"] = type;
        if (!defaultValue.isEmpty())
            obj["default"] = defaultValue;
        if (kind == PortKind::Execution)
            obj["kind"] = "execution";
        return obj;
    }

    static Port fromJson(const QJsonObject &obj) {
        Port p;
        p.name = obj["name"].toString();
        p.type = obj["type"].toString();
        p.defaultValue = obj["default"].toString();
        if (obj["kind"].toString() == "execution")
            p.kind = PortKind::Execution;
        return p;
    }

    // Фабричный метод для execution-порта
    static Port exec(const QString &name) {
        return {name, "exec", "", PortKind::Execution};
    }
};

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
    QString category;     // "math", "logic", "io", "string", ...
    QVector<Port> inputs;
    QVector<Port> outputs;
    QString sourcePath;   // relative to project
    QString headerPath;   // relative to project
    QStringList dependencies;
    QString origin;       // "user", "library", "graph", "ui", "local"
    QString graphId;      // ID внутреннего графа (пусто для атомарных модулей, заполнено для композитных)

    // Расширенные поля модульной системы
    QStringList includes;            // Зависимости (#include), хранятся отдельно от кода
    QString testStatus = "untested"; // passed | failed | untested | modified
    QString sourceCode;              // Тело функции (чистый код без #include)

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["version"] = version;
        obj["language"] = language;
        obj["description"] = description;
        if (!category.isEmpty())
            obj["category"] = category;
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

        // Композитный модуль
        if (!graphId.isEmpty())
            obj["graph_id"] = graphId;

        // Расширенные поля
        if (!includes.isEmpty()) {
            QJsonArray incArr;
            for (const auto &inc : includes)
                incArr.append(inc);
            obj["includes"] = incArr;
        }
        if (!sourceCode.isEmpty())
            obj["source_code"] = sourceCode;
        if (!testStatus.isEmpty() && testStatus != "untested") {
            QJsonObject testing;
            testing["status"] = testStatus;
            obj["testing"] = testing;
        }

        return obj;
    }

    static Module fromJson(const QJsonObject &obj) {
        Module m;
        m.id = obj["id"].toString();
        m.name = obj["name"].toString();
        m.version = obj["version"].toString();
        m.language = obj["language"].toString();
        m.description = obj["description"].toString();
        m.category = obj["category"].toString();
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

        // Композитный модуль
        m.graphId = obj["graph_id"].toString();

        // Расширенные поля
        for (const auto &v : obj["includes"].toArray())
            m.includes.append(v.toString());
        m.sourceCode = obj["source_code"].toString();
        if (obj.contains("testing"))
            m.testStatus = obj["testing"].toObject()["status"].toString("untested");

        return m;
    }

    bool operator==(const Module &other) const {
        return id == other.id;
    }

    // Поиск входного порта по имени
    const Port *findInput(const QString &portName) const {
        for (const auto &p : inputs)
            if (p.name == portName) return &p;
        return nullptr;
    }

    // Поиск выходного порта по имени
    const Port *findOutput(const QString &portName) const {
        for (const auto &p : outputs)
            if (p.name == portName) return &p;
        return nullptr;
    }

    bool hasInput(const QString &portName) const { return findInput(portName) != nullptr; }
    bool hasOutput(const QString &portName) const { return findOutput(portName) != nullptr; }

    // Фильтрация портов по kind
    QVector<Port> dataInputs() const {
        QVector<Port> result;
        for (const auto &p : inputs)
            if (p.kind == PortKind::Data) result.append(p);
        return result;
    }
    QVector<Port> dataOutputs() const {
        QVector<Port> result;
        for (const auto &p : outputs)
            if (p.kind == PortKind::Data) result.append(p);
        return result;
    }
    bool hasExecFlow() const {
        for (const auto &p : inputs)
            if (p.kind == PortKind::Execution) return true;
        for (const auto &p : outputs)
            if (p.kind == PortKind::Execution) return true;
        return false;
    }

    // Валидация: id и name не пусты, каждый порт имеет имя и тип
    bool isValid() const {
        if (id.isEmpty() || name.isEmpty()) return false;
        for (const auto &p : inputs)
            if (p.name.isEmpty() || p.type.isEmpty()) return false;
        for (const auto &p : outputs)
            if (p.name.isEmpty() || p.type.isEmpty()) return false;
        return true;
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
