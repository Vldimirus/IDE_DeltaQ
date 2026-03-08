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

// Граница составного модуля: какой внешний порт привязан к какому внутреннему узлу и порту.
struct CompositePortBinding {
    QString externalPortName;
    QString internalNodeId;
    QString internalPortName;
    PortKind kind = PortKind::Data;

    bool operator==(const CompositePortBinding &other) const {
        return externalPortName == other.externalPortName
            && internalNodeId == other.internalNodeId
            && internalPortName == other.internalPortName
            && kind == other.kind;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["external_port"] = externalPortName;
        obj["internal_node"] = internalNodeId;
        obj["internal_port"] = internalPortName;
        if (kind == PortKind::Execution)
            obj["kind"] = "execution";
        return obj;
    }

    static CompositePortBinding fromJson(const QJsonObject &obj) {
        CompositePortBinding binding;
        binding.externalPortName = obj["external_port"].toString();
        binding.internalNodeId = obj["internal_node"].toString();
        binding.internalPortName = obj["internal_port"].toString();
        if (obj["kind"].toString() == "execution")
            binding.kind = PortKind::Execution;
        return binding;
    }
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
    QString generatedFileBaseName; // Базовое имя generated .h/.c для составного модуля
    QString storagePath;  // Служебный путь к .dqmod на диске, не сериализуется

    // Расширенные поля модульной системы
    QStringList includes;               // Зависимости (#include), хранятся отдельно от кода
    QString compileStatus = "unknown";  // passed | failed | unknown | modified
    QString testStatus = "untested";    // passed | failed | untested | modified
    QString sourceCode;                 // Тело функции (чистый код без #include)
    QJsonObject metadata;               // Расширяемые метаданные модуля (UI contract, backend hints и т.п.)
    QVector<CompositePortBinding> boundaryInputs;  // Внешний input -> внутренний input
    QVector<CompositePortBinding> boundaryOutputs; // Внутренний output -> внешний output

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
        if (!generatedFileBaseName.isEmpty())
            obj["generated_file_base"] = generatedFileBaseName;
        if (!boundaryInputs.isEmpty() || !boundaryOutputs.isEmpty()) {
            QJsonObject boundary;
            QJsonArray inputBindings;
            QJsonArray outputBindings;
            for (const auto &binding : boundaryInputs)
                inputBindings.append(binding.toJson());
            for (const auto &binding : boundaryOutputs)
                outputBindings.append(binding.toJson());
            boundary["inputs"] = inputBindings;
            boundary["outputs"] = outputBindings;
            obj["boundary"] = boundary;
        }

        // Расширенные поля
        if (!includes.isEmpty()) {
            QJsonArray incArr;
            for (const auto &inc : includes)
                incArr.append(inc);
            obj["includes"] = incArr;
        }
        if (!sourceCode.isEmpty())
            obj["source_code"] = sourceCode;
        if (!metadata.isEmpty())
            obj["metadata"] = metadata;
        if ((!compileStatus.isEmpty() && compileStatus != "unknown")
            || (!testStatus.isEmpty() && testStatus != "untested")) {
            QJsonObject verification;
            if (!compileStatus.isEmpty() && compileStatus != "unknown")
                verification["compile_status"] = compileStatus;
            if (!testStatus.isEmpty() && testStatus != "untested")
                verification["test_status"] = testStatus;
            obj["verification"] = verification;

            // Сохраняем legacy-поле, чтобы старые данные с тестовым статусом не терялись.
            if (!testStatus.isEmpty() && testStatus != "untested") {
                QJsonObject testing;
                testing["status"] = testStatus;
                obj["testing"] = testing;
            }
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
        m.generatedFileBaseName = obj["generated_file_base"].toString();
        const QJsonObject boundary = obj["boundary"].toObject();
        for (const auto &v : boundary["inputs"].toArray())
            m.boundaryInputs.append(CompositePortBinding::fromJson(v.toObject()));
        for (const auto &v : boundary["outputs"].toArray())
            m.boundaryOutputs.append(CompositePortBinding::fromJson(v.toObject()));

        // Расширенные поля
        for (const auto &v : obj["includes"].toArray())
            m.includes.append(v.toString());
        m.sourceCode = obj["source_code"].toString();
        m.metadata = obj["metadata"].toObject();
        if (obj.contains("verification")) {
            const QJsonObject verification = obj["verification"].toObject();
            m.compileStatus = verification["compile_status"].toString("unknown");
            m.testStatus = verification["test_status"].toString("untested");
        }

        if (obj.contains("testing")) {
            const QString legacyTestStatus = obj["testing"].toObject()["status"].toString("untested");
            if (m.testStatus == "untested")
                m.testStatus = legacyTestStatus;

            // Для старых .dqmod успешный тест подразумевал успешно пройденную компиляцию.
            if (m.compileStatus == "unknown") {
                if (legacyTestStatus == "passed" || legacyTestStatus == "failed")
                    m.compileStatus = "passed";
                else if (legacyTestStatus == "modified")
                    m.compileStatus = "modified";
            }
        }

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

    // Поиск boundary mapping для внешнего input порта составного модуля.
    const CompositePortBinding *findBoundaryInput(const QString &portName) const {
        for (const auto &binding : boundaryInputs)
            if (binding.externalPortName == portName) return &binding;
        return nullptr;
    }

    // Поиск boundary mapping для внешнего output порта составного модуля.
    const CompositePortBinding *findBoundaryOutput(const QString &portName) const {
        for (const auto &binding : boundaryOutputs)
            if (binding.externalPortName == portName) return &binding;
        return nullptr;
    }

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

    // Чтение строкового metadata-поля без дублирования однотипного кода в UI/runtime слоях.
    QString metadataString(const QString &key, const QString &defaultValue = QString()) const {
        if (!metadata.contains(key))
            return defaultValue;
        const QString value = metadata.value(key).toString();
        return value.isEmpty() ? defaultValue : value;
    }

    // UI contract-модуль определяется явной metadata-маркировкой, а не backend-реализацией.
    bool isUIContractModule() const {
        return metadataString("deltaq.kind") == "ui_contract";
    }

    // Для UI contract-модулей храним канонический тип виджета отдельно от display name.
    QString uiWidgetType() const {
        return metadataString("deltaq.ui.widget_type");
    }

    // Imported pack module определяется по metadata converter-а, а не по origin == extension.
    bool isImportedPackModule() const {
        return metadataString("deltaq.import.kind") == "library_pack_module"
            || !metadataString("deltaq.import.pack_name").isEmpty();
    }

    // Возвращает имя imported pack-а для UI и build слоёв.
    QString importedPackName() const {
        return metadataString("deltaq.import.pack_name");
    }

    // Возвращает исходный символ библиотеки до curation и display rename.
    QString importedOriginalSymbol() const {
        return metadataString("deltaq.import.original_symbol", name);
    }

    // Display name imported-модуля может отличаться от внутреннего module.name.
    QString importedDisplayName() const {
        return metadataString("deltaq.import.display_name", name);
    }

    // Краткое назначение модуля: metadata может переопределить description,
    // но по умолчанию именно description остаётся главным human-readable summary.
    QString documentationSummary() const {
        return metadataString("deltaq.doc.summary", description).trimmed();
    }

    // Когда модуль рекомендуется использовать в графе или curation-flow.
    QString documentationWhenToUse() const {
        return metadataString("deltaq.doc.when_to_use").trimmed();
    }

    // Ограничения и caveats модуля, которые должны быть видны пользователю.
    QString documentationLimitations() const {
        return metadataString("deltaq.doc.limitations").trimmed();
    }

    // Быстрая проверка, есть ли у модуля хотя бы минимальная пользовательская документация.
    bool hasDocumentationDetails() const {
        return !documentationSummary().isEmpty()
            || !documentationWhenToUse().isEmpty()
            || !documentationLimitations().isEmpty();
    }

    // Формальная роль imported-модуля внутри v1 curation flow.
    QString importedCurationRole() const {
        if (!isImportedPackModule())
            return {};
        return metadataString("deltaq.import.curation_role", "raw_wrapper");
    }

    // Скрытые imported wrapper-ы не должны засорять палитру, но остаются редактируемыми в менеджере.
    bool isHiddenFromPalette() const {
        return isImportedPackModule() && importedCurationRole() == "hidden";
    }

    // Составной модуль определяется по наличию внутреннего графа, а не по origin.
    bool isComposite() const {
        return !graphId.trimmed().isEmpty();
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
