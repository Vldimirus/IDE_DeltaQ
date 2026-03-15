// Тесты Module/Port — сериализация, operator==, helper-методы, валидация
#include <QtTest>
#include <deltaq/Module.h>

using namespace DeltaQ;

class TestModule : public QObject {
    Q_OBJECT

private slots:
    // --- Port ---

    void portToJson()
    {
        Port p{"input1", "int", "42"};
        auto json = p.toJson();
        QCOMPARE(json["name"].toString(), "input1");
        QCOMPARE(json["type"].toString(), "int");
        QCOMPARE(json["default"].toString(), "42");
    }

    void portFromJson()
    {
        QJsonObject obj;
        obj["name"] = "out";
        obj["type"] = "float";
        obj["default"] = "3.14";
        auto p = Port::fromJson(obj);
        QCOMPARE(p.name, "out");
        QCOMPARE(p.type, "float");
        QCOMPARE(p.defaultValue, "3.14");
    }

    void portRoundtrip()
    {
        Port original{"data", "string", "hello"};
        auto restored = Port::fromJson(original.toJson());
        QCOMPARE(restored, original);
    }

    void portEquality()
    {
        Port a{"x", "int", "0"};
        Port b{"x", "int", "0"};
        Port c{"x", "float", "0"};
        QVERIFY(a == b);
        QVERIFY(!(a == c));
    }

    // --- Module ---

    void moduleCreate()
    {
        auto m = Module::create("MyModule", "cpp");
        QVERIFY(!m.id.isEmpty());
        QCOMPARE(m.name, "MyModule");
        QCOMPARE(m.language, "cpp");
        QCOMPARE(m.version, "1.0.0");
        QCOMPARE(m.origin, "user");
    }

    void moduleToJsonFromJson()
    {
        Module m;
        m.id = "mod-1";
        m.name = "Фильтр";
        m.version = "2.0";
        m.language = "c";
        m.description = "Фильтрация данных";
        m.origin = "user";
        m.inputs = {{"in", "int", "0"}};
        m.outputs = {{"out", "int", ""}};
        m.sourcePath = "src/filter.c";
        m.headerPath = "include/filter.h";
        m.dependencies = {"math_utils"};

        auto json = m.toJson();
        auto restored = Module::fromJson(json);
        QCOMPARE(restored.id, m.id);
        QCOMPARE(restored.name, m.name);
        QCOMPARE(restored.version, m.version);
        QCOMPARE(restored.language, m.language);
        QCOMPARE(restored.description, m.description);
        QCOMPARE(restored.origin, m.origin);
        QCOMPARE(restored.inputs.size(), 1);
        QCOMPARE(restored.outputs.size(), 1);
        QCOMPARE(restored.inputs[0], m.inputs[0]);
        QCOMPARE(restored.outputs[0], m.outputs[0]);
        QCOMPARE(restored.sourcePath, m.sourcePath);
        QCOMPARE(restored.headerPath, m.headerPath);
        QCOMPARE(restored.dependencies, m.dependencies);
    }

    void moduleRoundtrip()
    {
        auto m = Module::create("RoundTrip");
        m.inputs = {{"a", "int", ""}, {"b", "float", "1.0"}};
        m.outputs = {{"result", "double", ""}};

        auto json = m.toJson();
        auto restored = Module::fromJson(json);
        // operator== сравнивает по id
        QCOMPARE(restored, m);
        // Дополнительно — порты совпали
        QCOMPARE(restored.inputs, m.inputs);
        QCOMPARE(restored.outputs, m.outputs);
    }

    void moduleEquality()
    {
        Module a, b;
        a.id = "same-id";
        a.name = "A";
        b.id = "same-id";
        b.name = "B";
        QVERIFY(a == b); // сравнение по id

        b.id = "other-id";
        QVERIFY(!(a == b));
    }

    void moduleFindPort()
    {
        auto m = Module::create("Test");
        m.inputs = {{"x", "int", ""}, {"y", "float", ""}};
        m.outputs = {{"result", "double", ""}};

        auto *p = m.findInput("x");
        QVERIFY(p != nullptr);
        QCOMPARE(p->type, "int");

        QVERIFY(m.findInput("nonexistent") == nullptr);

        auto *o = m.findOutput("result");
        QVERIFY(o != nullptr);
        QCOMPARE(o->type, "double");

        QVERIFY(m.findOutput("x") == nullptr);
    }

    void compositeBoundaryRoundtrip()
    {
        Module m = Module::create("Composite");
        m.origin = "graph";
        m.inputs = {Port::exec("flow_in"), {"value", "int", "0"}};
        m.outputs = {Port::exec("flow_out"), {"result", "int", ""}};
        m.graphId = "inner-graph-1";
        m.generatedFileBaseName = "submodule_01";
        m.boundaryInputs = {
            {"flow_in", "node_step", "flow_in", PortKind::Execution},
            {"value", "node_step", "value", PortKind::Data}
        };
        m.boundaryOutputs = {
            {"flow_out", "node_step", "flow_out", PortKind::Execution},
            {"result", "node_step", "result", PortKind::Data}
        };

        auto restored = Module::fromJson(m.toJson());
        QCOMPARE(restored.graphId, QString("inner-graph-1"));
        QCOMPARE(restored.generatedFileBaseName, QString("submodule_01"));
        QCOMPARE(restored.boundaryInputs, m.boundaryInputs);
        QCOMPARE(restored.boundaryOutputs, m.boundaryOutputs);

        const auto *flowIn = restored.findBoundaryInput("flow_in");
        const auto *result = restored.findBoundaryOutput("result");
        QVERIFY(flowIn != nullptr);
        QVERIFY(result != nullptr);
        QCOMPARE(flowIn->internalNodeId, QString("node_step"));
        QCOMPARE(result->internalPortName, QString("result"));
    }

    void verificationRoundtrip()
    {
        Module m = Module::create("Verified");
        m.compileStatus = "passed";
        m.testStatus = "modified";

        auto restored = Module::fromJson(m.toJson());
        QCOMPARE(restored.compileStatus, QString("passed"));
        QCOMPARE(restored.testStatus, QString("modified"));
    }

    void schemaV2RoundtripSeparatesTrustAndVerification()
    {
        Module m = Module::create("Traceable");
        m.origin = "local";
        m.sourcePath = "src/traceable.c";
        m.provenance.sourceKind = "manual";
        m.provenance.sourceRef = "src/traceable.c";
        m.provenance.manifestId = "manual-seed";
        m.trustState = "curated";
        m.compileStatus = "passed";
        m.testStatus = "modified";
        m.lastVerifiedAt = "2026-03-15T14:20:00Z";
        m.verificationScenarioRefs = {"smoke/basic", "smoke/edge"};
        m.verificationTraceArtifactRefs = {"trace://traceable/basic"};

        const QJsonObject json = m.toJson();
        QCOMPARE(json["schema_version"].toInt(), Module::SchemaVersion);
        QCOMPARE(json["trust_state"].toString(), QString("curated"));
        QCOMPARE(json["provenance"].toObject()["source_kind"].toString(), QString("manual"));
        QCOMPARE(json["verification"].toObject()["last_verified_at"].toString(),
                 QString("2026-03-15T14:20:00Z"));
        QCOMPARE(json["verification"].toObject()["scenario_refs"].toArray().size(), 2);
        QCOMPARE(json["verification"].toObject()["trace_artifact_refs"].toArray().size(), 1);

        const Module restored = Module::fromJson(json);
        QCOMPARE(restored.trustState, QString("curated"));
        QCOMPARE(restored.provenance.sourceKind, QString("manual"));
        QCOMPARE(restored.provenance.sourceRef, QString("src/traceable.c"));
        QCOMPARE(restored.lastVerifiedAt, QString("2026-03-15T14:20:00Z"));
        QCOMPARE(restored.verificationScenarioRefs, QStringList({"smoke/basic", "smoke/edge"}));
        QCOMPARE(restored.verificationTraceArtifactRefs, QStringList({"trace://traceable/basic"}));
        QCOMPARE(restored.compileStatus, QString("passed"));
        QCOMPARE(restored.testStatus, QString("modified"));
    }

    void verificationScenariosRoundtrip()
    {
        Module m = Module::create("ScenarioCarrier");
        m.inputs = {{"value", "int", "1"}};
        m.outputs = {{"result", "int", ""}};

        ModuleVerificationScenario scenario;
        scenario.id = "scenario_1";
        scenario.name = "basic";
        scenario.inputValues["value"] = "2";
        scenario.expectedOutputValues["result"] = "3";
        m.verificationScenarios.append(scenario);

        const QJsonObject json = m.toJson();
        QCOMPARE(json["verification"].toObject()["scenarios"].toArray().size(), 1);
        QCOMPARE(json["verification"].toObject()["scenario_refs"].toArray().at(0).toString(),
                 QString("basic"));

        const Module restored = Module::fromJson(json);
        QCOMPARE(restored.verificationScenarios.size(), 1);
        QCOMPARE(restored.verificationScenarios.first().id, QString("scenario_1"));
        QCOMPARE(restored.verificationScenarios.first().name, QString("basic"));
        QCOMPARE(restored.verificationScenarios.first().inputValues.value("value"), QString("2"));
        QCOMPARE(restored.verificationScenarios.first().expectedOutputValues.value("result"),
                 QString("3"));
        QCOMPARE(restored.verificationScenarioRefs, QStringList({"basic"}));
    }

    // Проверяет, что trace artifacts и step events проходят стабильный schema-backed round-trip.
    void verificationTraceArtifactsRoundtrip()
    {
        Module m = Module::create("TraceCarrier");
        m.inputs = {{"value", "int", "1"}};
        m.outputs = {{"result", "int", ""}};

        ModuleVerificationScenario scenario;
        scenario.id = "scenario_1";
        scenario.name = "basic";
        scenario.inputValues["value"] = "2";
        scenario.expectedOutputValues["result"] = "3";
        m.verificationScenarios.append(scenario);

        ModuleTraceArtifact artifact;
        artifact.id = "trace://TraceCarrier/scenario_1";
        artifact.scenarioId = "scenario_1";
        artifact.createdAt = "2026-03-15T15:30:00Z";
        artifact.status = "captured";

        ModuleTraceEvent enterEvent;
        enterEvent.stepIndex = 0;
        enterEvent.eventKind = "enter";
        enterEvent.sourceLine = 1;
        enterEvent.sourceSnippet = "int dq_trace_carrier(int value) {";
        enterEvent.variableSnapshotDelta["value"] = "2";
        artifact.events.append(enterEvent);

        ModuleTraceEvent returnEvent;
        returnEvent.stepIndex = 1;
        returnEvent.eventKind = "return";
        returnEvent.sourceLine = 2;
        returnEvent.sourceSnippet = "return value + 1;";
        returnEvent.outputSnapshot["result"] = "3";
        artifact.events.append(returnEvent);

        m.verificationTraceArtifacts.append(artifact);

        const QJsonObject json = m.toJson();
        const QJsonObject verification = json["verification"].toObject();
        QCOMPARE(verification["trace_artifact_refs"].toArray().size(), 1);
        QCOMPARE(verification["trace_artifact_refs"].toArray().at(0).toString(),
                 QString("trace://TraceCarrier/scenario_1"));
        QCOMPARE(verification["trace_artifacts"].toArray().size(), 1);

        const Module restored = Module::fromJson(json);
        QCOMPARE(restored.verificationTraceArtifactRefs,
                 QStringList({"trace://TraceCarrier/scenario_1"}));
        QCOMPARE(restored.verificationTraceArtifacts.size(), 1);
        QCOMPARE(restored.verificationTraceArtifacts.first().scenarioId, QString("scenario_1"));
        QCOMPARE(restored.verificationTraceArtifacts.first().status, QString("captured"));
        QCOMPARE(restored.verificationTraceArtifacts.first().events.size(), 2);
        QCOMPARE(restored.verificationTraceArtifacts.first().events.first().eventKind,
                 QString("enter"));
        QCOMPARE(restored.verificationTraceArtifacts.first().events.last().outputSnapshot.value("result"),
                 QString("3"));
    }

    void metadataRoundtrip()
    {
        Module m = Module::create("UiContract");
        m.metadata["deltaq.kind"] = "ui_contract";
        m.metadata["deltaq.ui.widget_type"] = "button";

        const Module restored = Module::fromJson(m.toJson());
        QCOMPARE(restored.metadataString("deltaq.kind"), QString("ui_contract"));
        QCOMPARE(restored.uiWidgetType(), QString("button"));
        QVERIFY(restored.isUIContractModule());
    }

    void legacyTestingStatusPromotesCompileStatus()
    {
        QJsonObject json;
        json["id"] = "legacy-module";
        json["name"] = "Legacy";
        json["language"] = "c";
        json["origin"] = "local";

        QJsonObject ports;
        ports["input"] = QJsonArray{};
        ports["output"] = QJsonArray{};
        json["ports"] = ports;

        QJsonObject testing;
        testing["status"] = "passed";
        json["testing"] = testing;

        const Module restored = Module::fromJson(json);
        QCOMPARE(restored.testStatus, QString("passed"));
        QCOMPARE(restored.compileStatus, QString("passed"));
    }

    void legacyModulesDeriveCanonicalTrustAndProvenance()
    {
        QJsonObject json;
        json["id"] = "legacy-verified";
        json["name"] = "LegacyVerified";
        json["language"] = "c";
        json["origin"] = "local";
        json["source"] = "src/legacy_verified.c";

        QJsonObject ports;
        ports["input"] = QJsonArray{};
        ports["output"] = QJsonArray{};
        json["ports"] = ports;

        QJsonObject verification;
        verification["compile_status"] = "passed";
        verification["test_status"] = "passed";
        json["verification"] = verification;

        const Module restored = Module::fromJson(json);
        QCOMPARE(restored.provenance.sourceKind, QString("manual"));
        QCOMPARE(restored.provenance.sourceRef, QString("src/legacy_verified.c"));
        QCOMPARE(restored.trustState, QString("smoke_passed"));
    }

    void importedPackLegacyModulesDeriveImportedTrust()
    {
        QJsonObject json;
        json["id"] = "sensor-read";
        json["name"] = "sensor_read";
        json["language"] = "c";
        json["origin"] = "extension";

        QJsonObject ports;
        ports["input"] = QJsonArray{};
        ports["output"] = QJsonArray{};
        json["ports"] = ports;

        QJsonObject metadata;
        metadata["deltaq.import.kind"] = "library_pack_module";
        metadata["deltaq.import.pack_name"] = "sensor_sdk";
        metadata["deltaq.import.original_symbol"] = "sensor_read";
        metadata["deltaq.import.curation_role"] = "curated_entry";
        json["metadata"] = metadata;

        const Module restored = Module::fromJson(json);
        QCOMPARE(restored.provenance.sourceKind, QString("imported_pack"));
        QCOMPARE(restored.provenance.sourceRef, QString("sensor_sdk"));
        QCOMPARE(restored.provenance.symbol, QString("sensor_read"));
        QCOMPARE(restored.trustState, QString("curated"));
    }

    void isCompositeDependsOnGraphId()
    {
        Module atomic = Module::create("Atomic");
        QVERIFY(!atomic.isComposite());

        Module composite = Module::create("Composite");
        composite.graphId = "inner-graph";
        QVERIFY(composite.isComposite());
    }

    void moduleHasPort()
    {
        auto m = Module::create("Test");
        m.inputs = {{"in1", "int", ""}};
        m.outputs = {{"out1", "float", ""}};

        QVERIFY(m.hasInput("in1"));
        QVERIFY(!m.hasInput("out1"));
        QVERIFY(m.hasOutput("out1"));
        QVERIFY(!m.hasOutput("in1"));
    }

    void moduleIsValid()
    {
        auto m = Module::create("Valid");
        m.inputs = {{"a", "int", ""}};
        QVERIFY(m.isValid());
    }

    void moduleIsValidEmpty()
    {
        Module m;
        QVERIFY(!m.isValid()); // id и name пусты

        m.id = "id1";
        QVERIFY(!m.isValid()); // name пуст

        m.name = "ok";
        QVERIFY(m.isValid());

        // Порт без типа — невалидно
        m.inputs = {{"port", "", ""}};
        QVERIFY(!m.isValid());

        // Порт без имени — невалидно
        m.inputs = {{"", "int", ""}};
        QVERIFY(!m.isValid());

        // Корректный порт
        m.inputs = {{"port", "int", ""}};
        QVERIFY(m.isValid());
    }
};

QTEST_MAIN(TestModule)
#include "test_Module.moc"
