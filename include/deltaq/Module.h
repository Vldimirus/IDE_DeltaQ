#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QVector>
#include <QMap>

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

struct ModuleProvenance {
    QString sourceKind;
    QString manifestId;
    QString sourceRef;
    QString symbol;
    QString toolVersion;

    bool isEmpty() const {
        return sourceKind.isEmpty()
            && manifestId.isEmpty()
            && sourceRef.isEmpty()
            && symbol.isEmpty()
            && toolVersion.isEmpty();
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        if (!sourceKind.isEmpty())
            obj["source_kind"] = sourceKind;
        if (!manifestId.isEmpty())
            obj["manifest_id"] = manifestId;
        if (!sourceRef.isEmpty())
            obj["source_ref"] = sourceRef;
        if (!symbol.isEmpty())
            obj["symbol"] = symbol;
        if (!toolVersion.isEmpty())
            obj["tool_version"] = toolVersion;
        return obj;
    }

    static ModuleProvenance fromJson(const QJsonObject &obj) {
        ModuleProvenance provenance;
        provenance.sourceKind = obj["source_kind"].toString();
        provenance.manifestId = obj["manifest_id"].toString();
        provenance.sourceRef = obj["source_ref"].toString();
        provenance.symbol = obj["symbol"].toString();
        provenance.toolVersion = obj["tool_version"].toString();
        return provenance;
    }
};

struct ModuleVerificationScenario {
    QString id;
    QString name;
    QMap<QString, QString> inputValues;
    QMap<QString, QString> expectedOutputValues;

    bool operator==(const ModuleVerificationScenario &other) const {
        return id == other.id
            && name == other.name
            && inputValues == other.inputValues
            && expectedOutputValues == other.expectedOutputValues;
    }

    // Возвращает человекочитаемое имя сценария для summary и selector-ов.
    QString displayName() const {
        const QString trimmedName = name.trimmed();
        if (!trimmedName.isEmpty())
            return trimmedName;
        return id.trimmed();
    }

    // Сериализует verification scenario в явный schema-backed JSON.
    QJsonObject toJson() const {
        QJsonObject obj;
        if (!id.isEmpty())
            obj["id"] = id;
        if (!name.isEmpty())
            obj["name"] = name;

        QJsonObject inputs;
        for (auto it = inputValues.begin(); it != inputValues.end(); ++it)
            inputs[it.key()] = it.value();
        obj["inputs"] = inputs;

        QJsonObject expectedOutputs;
        for (auto it = expectedOutputValues.begin(); it != expectedOutputValues.end(); ++it)
            expectedOutputs[it.key()] = it.value();
        obj["expected_outputs"] = expectedOutputs;
        return obj;
    }

    // Восстанавливает verification scenario из JSON без ухода в свободную metadata-схему.
    static ModuleVerificationScenario fromJson(const QJsonObject &obj) {
        ModuleVerificationScenario scenario;
        scenario.id = obj["id"].toString();
        scenario.name = obj["name"].toString();

        const QJsonObject inputs = obj["inputs"].toObject();
        for (auto it = inputs.begin(); it != inputs.end(); ++it)
            scenario.inputValues[it.key()] = it.value().toVariant().toString();

        const QJsonObject expectedOutputs = obj["expected_outputs"].toObject();
        for (auto it = expectedOutputs.begin(); it != expectedOutputs.end(); ++it)
            scenario.expectedOutputValues[it.key()] = it.value().toVariant().toString();
        return scenario;
    }
};

struct ModuleTraceEvent {
    int stepIndex = 0;
    QString eventKind;
    int sourceLine = -1;
    int sourceColumn = -1;
    QString sourceSnippet;
    QMap<QString, QString> variableSnapshotDelta;
    QMap<QString, QString> outputSnapshot;
    QString note;

    bool operator==(const ModuleTraceEvent &other) const {
        return stepIndex == other.stepIndex
            && eventKind == other.eventKind
            && sourceLine == other.sourceLine
            && sourceColumn == other.sourceColumn
            && sourceSnippet == other.sourceSnippet
            && variableSnapshotDelta == other.variableSnapshotDelta
            && outputSnapshot == other.outputSnapshot
            && note == other.note;
    }

    // Сериализует один trace step в стабильный schema-backed JSON.
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["step_index"] = stepIndex;
        obj["event_kind"] = eventKind;
        if (sourceLine >= 0)
            obj["source_line"] = sourceLine;
        if (sourceColumn >= 0)
            obj["source_column"] = sourceColumn;
        if (!sourceSnippet.isEmpty())
            obj["source_snippet"] = sourceSnippet;

        QJsonObject variableDelta;
        for (auto it = variableSnapshotDelta.begin(); it != variableSnapshotDelta.end(); ++it)
            variableDelta[it.key()] = it.value();
        if (!variableDelta.isEmpty())
            obj["variable_delta"] = variableDelta;

        QJsonObject outputs;
        for (auto it = outputSnapshot.begin(); it != outputSnapshot.end(); ++it)
            outputs[it.key()] = it.value();
        if (!outputs.isEmpty())
            obj["output_snapshot"] = outputs;

        if (!note.isEmpty())
            obj["note"] = note;
        return obj;
    }

    // Восстанавливает trace step из сохранённого trace artifact JSON.
    static ModuleTraceEvent fromJson(const QJsonObject &obj) {
        ModuleTraceEvent event;
        event.stepIndex = obj["step_index"].toInt();
        event.eventKind = obj["event_kind"].toString();
        event.sourceLine = obj.contains("source_line") ? obj["source_line"].toInt() : -1;
        event.sourceColumn = obj.contains("source_column") ? obj["source_column"].toInt() : -1;
        event.sourceSnippet = obj["source_snippet"].toString();
        const QJsonObject variableDelta = obj["variable_delta"].toObject();
        for (auto it = variableDelta.begin(); it != variableDelta.end(); ++it)
            event.variableSnapshotDelta[it.key()] = it.value().toVariant().toString();
        const QJsonObject outputs = obj["output_snapshot"].toObject();
        for (auto it = outputs.begin(); it != outputs.end(); ++it)
            event.outputSnapshot[it.key()] = it.value().toVariant().toString();
        event.note = obj["note"].toString();
        return event;
    }
};

struct ModuleTraceArtifact {
    QString id;
    QString scenarioId;
    QString createdAt;
    QString status;
    QString unavailableReason;
    QVector<ModuleTraceEvent> events;

    bool operator==(const ModuleTraceArtifact &other) const {
        return id == other.id
            && scenarioId == other.scenarioId
            && createdAt == other.createdAt
            && status == other.status
            && unavailableReason == other.unavailableReason
            && events == other.events;
    }

    // Возвращает строку-ссылку, которую можно использовать в verification summary.
    QString summaryRef() const {
        if (!id.trimmed().isEmpty())
            return id.trimmed();
        return scenarioId.trimmed();
    }

    // Сериализует trace artifact вместе с детальными step events.
    QJsonObject toJson() const {
        QJsonObject obj;
        if (!id.isEmpty())
            obj["id"] = id;
        if (!scenarioId.isEmpty())
            obj["scenario_id"] = scenarioId;
        if (!createdAt.isEmpty())
            obj["created_at"] = createdAt;
        if (!status.isEmpty())
            obj["status"] = status;
        if (!unavailableReason.isEmpty())
            obj["unavailable_reason"] = unavailableReason;
        if (!events.isEmpty()) {
            QJsonArray eventArray;
            for (const auto &event : events)
                eventArray.append(event.toJson());
            obj["events"] = eventArray;
        }
        return obj;
    }

    // Восстанавливает trace artifact из JSON для повторного review и future viewer surface.
    static ModuleTraceArtifact fromJson(const QJsonObject &obj) {
        ModuleTraceArtifact artifact;
        artifact.id = obj["id"].toString();
        artifact.scenarioId = obj["scenario_id"].toString();
        artifact.createdAt = obj["created_at"].toString();
        artifact.status = obj["status"].toString();
        artifact.unavailableReason = obj["unavailable_reason"].toString();
        for (const auto &value : obj["events"].toArray())
            artifact.events.append(ModuleTraceEvent::fromJson(value.toObject()));
        return artifact;
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
    ModuleProvenance provenance;
    QString trustState;
    QString lastVerifiedAt;
    QStringList verificationScenarioRefs;
    QStringList verificationTraceArtifactRefs;
    QVector<ModuleVerificationScenario> verificationScenarios;
    QVector<ModuleTraceArtifact> verificationTraceArtifacts;

    static constexpr int SchemaVersion = 2;

    ModuleProvenance effectiveProvenance() const {
        ModuleProvenance resolved = provenance;

        if (resolved.sourceKind.isEmpty()) {
            if (isImportedPackModule())
                resolved.sourceKind = "imported_pack";
            else if (origin == "core" || id.startsWith("core."))
                resolved.sourceKind = "core";
            else if (origin == "ui")
                resolved.sourceKind = "ui_contract";
            else if (origin == "graph")
                resolved.sourceKind = "graph";
            else
                resolved.sourceKind = "manual";
        }

        if (resolved.sourceRef.isEmpty()) {
            if (isImportedPackModule())
                resolved.sourceRef = importedPackName();
            else if (!graphId.isEmpty())
                resolved.sourceRef = graphId;
            else if (!sourcePath.isEmpty())
                resolved.sourceRef = sourcePath;
            else if (!headerPath.isEmpty())
                resolved.sourceRef = headerPath;
        }

        if (resolved.symbol.isEmpty() && isImportedPackModule())
            resolved.symbol = importedOriginalSymbol();

        return resolved;
    }

    QString effectiveTrustState() const {
        const QString explicitState = trustState.trimmed();
        if (!explicitState.isEmpty())
            return explicitState;

        const ModuleProvenance resolvedProvenance = effectiveProvenance();
        if (resolvedProvenance.sourceKind == "core"
            || resolvedProvenance.sourceKind == "ui_contract") {
            return "verified";
        }

        if (resolvedProvenance.sourceKind == "imported_pack") {
            const QString role = importedCurationRole();
            if (role == "curated_entry" || role == "adapter")
                return "curated";
            return "generated_unverified";
        }

        if (resolvedProvenance.sourceKind == "graph")
            return "curated";

        if (compileStatus == "passed" && testStatus == "passed")
            return "smoke_passed";

        return "draft";
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["schema_version"] = SchemaVersion;
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
        const ModuleProvenance resolvedProvenance = effectiveProvenance();
        if (!resolvedProvenance.isEmpty())
            obj["provenance"] = resolvedProvenance.toJson();
        const QString resolvedTrustState = effectiveTrustState();
        if (!resolvedTrustState.isEmpty())
            obj["trust_state"] = resolvedTrustState;
        QStringList resolvedScenarioRefs = verificationScenarioRefs;
        if (resolvedScenarioRefs.isEmpty()) {
            for (const auto &scenario : verificationScenarios) {
                const QString displayName = scenario.displayName();
                if (!displayName.isEmpty())
                    resolvedScenarioRefs.append(displayName);
            }
        }
        QStringList resolvedTraceArtifactRefs = verificationTraceArtifactRefs;
        if (resolvedTraceArtifactRefs.isEmpty()) {
            for (const auto &artifact : verificationTraceArtifacts) {
                const QString ref = artifact.summaryRef();
                if (!ref.isEmpty())
                    resolvedTraceArtifactRefs.append(ref);
            }
        }
        if ((!compileStatus.isEmpty() && compileStatus != "unknown")
            || (!testStatus.isEmpty() && testStatus != "untested")
            || !lastVerifiedAt.isEmpty()
            || !resolvedScenarioRefs.isEmpty()
            || !resolvedTraceArtifactRefs.isEmpty()
            || !verificationScenarios.isEmpty()
            || !verificationTraceArtifacts.isEmpty()) {
            QJsonObject verification;
            if (!compileStatus.isEmpty() && compileStatus != "unknown")
                verification["compile_status"] = compileStatus;
            if (!testStatus.isEmpty() && testStatus != "untested")
                verification["test_status"] = testStatus;
            if (!lastVerifiedAt.isEmpty())
                verification["last_verified_at"] = lastVerifiedAt;
            if (!resolvedScenarioRefs.isEmpty()) {
                QJsonArray scenarioRefs;
                for (const auto &scenarioRef : resolvedScenarioRefs)
                    scenarioRefs.append(scenarioRef);
                verification["scenario_refs"] = scenarioRefs;
            }
            if (!verificationScenarios.isEmpty()) {
                QJsonArray scenarios;
                for (const auto &scenario : verificationScenarios)
                    scenarios.append(scenario.toJson());
                verification["scenarios"] = scenarios;
            }
            if (!resolvedTraceArtifactRefs.isEmpty()) {
                QJsonArray traceRefs;
                for (const auto &traceRef : resolvedTraceArtifactRefs)
                    traceRefs.append(traceRef);
                verification["trace_artifact_refs"] = traceRefs;
            }
            if (!verificationTraceArtifacts.isEmpty()) {
                QJsonArray traceArtifacts;
                for (const auto &artifact : verificationTraceArtifacts)
                    traceArtifacts.append(artifact.toJson());
                verification["trace_artifacts"] = traceArtifacts;
            }
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
        m.provenance = ModuleProvenance::fromJson(obj["provenance"].toObject());
        m.trustState = obj["trust_state"].toString();
        if (obj.contains("verification")) {
            const QJsonObject verification = obj["verification"].toObject();
            m.compileStatus = verification["compile_status"].toString("unknown");
            m.testStatus = verification["test_status"].toString("untested");
            m.lastVerifiedAt = verification["last_verified_at"].toString();
            for (const auto &v : verification["scenario_refs"].toArray())
                m.verificationScenarioRefs.append(v.toString());
            for (const auto &v : verification["scenarios"].toArray())
                m.verificationScenarios.append(ModuleVerificationScenario::fromJson(v.toObject()));
            for (const auto &v : verification["trace_artifact_refs"].toArray())
                m.verificationTraceArtifactRefs.append(v.toString());
            for (const auto &v : verification["trace_artifacts"].toArray())
                m.verificationTraceArtifacts.append(ModuleTraceArtifact::fromJson(v.toObject()));
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

        if (m.trustState.isEmpty())
            m.trustState = m.effectiveTrustState();
        if (m.verificationScenarioRefs.isEmpty()) {
            for (const auto &scenario : m.verificationScenarios) {
                const QString displayName = scenario.displayName();
                if (!displayName.isEmpty())
                    m.verificationScenarioRefs.append(displayName);
            }
        }
        if (m.verificationTraceArtifactRefs.isEmpty()) {
            for (const auto &artifact : m.verificationTraceArtifacts) {
                const QString ref = artifact.summaryRef();
                if (!ref.isEmpty())
                    m.verificationTraceArtifactRefs.append(ref);
            }
        }
        m.provenance = m.effectiveProvenance();

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
