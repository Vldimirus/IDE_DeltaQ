// Тесты SubModuleFactory — создание составного модуля и его boundary metadata
#include <QtTest>
#include <QApplication>

#include <deltaq/Graph.h>
#include <deltaq/Module.h>

#include "../../src/blockEditor/SubModuleFactory.h"
#include "../../src/core/GraphStore.h"
#include "../../src/core/ModuleRegistry.h"

using namespace DeltaQ;

class TestSubModuleFactory : public QObject {
    Q_OBJECT

private:
    ModuleRegistry *m_registry = nullptr;
    GraphStore *m_graphStore = nullptr;

    // Регистрирует модуль с минимальной текстовой реализацией для проверки допуска.
    void registerModule(const QString &id, const QString &name,
                        const QVector<Port> &inputs,
                        const QVector<Port> &outputs)
    {
        Module mod;
        mod.id = id;
        mod.name = name;
        mod.category = "test";
        mod.language = "c";
        mod.version = "1.0";
        mod.origin = "user";
        mod.inputs = inputs;
        mod.outputs = outputs;
        mod.compileStatus = "passed";
        mod.testStatus = "passed";
        mod.sourceCode = "int dq_test_module(int value) {\n"
                         "    return value;\n"
                         "}";
        QVERIFY(m_registry->registerModule(mod));
    }

private slots:
    void init()
    {
        m_registry = new ModuleRegistry(this);
        m_graphStore = new GraphStore(this);
        m_registry->setGraphStore(m_graphStore);

        registerModule("start", "Start",
                       {},
                       {Port::exec("flow_out"), {"value", "int", ""}});
        registerModule("step", "Step",
                       {Port::exec("flow_in"), {"value", "int", "0"}},
                       {Port::exec("flow_out"), {"result", "int", ""}});
        registerModule("finish", "Finish",
                       {Port::exec("flow_in"), {"value", "int", "0"}},
                       {});
    }

    void cleanup()
    {
        delete m_graphStore;
        m_graphStore = nullptr;
        delete m_registry;
        m_registry = nullptr;
    }

    void createFromSelectionCapturesBoundaryMappings()
    {
        Graph parent = Graph::create("ParentGraph");

        GraphNode start = GraphNode::create("start");
        start.id = "node_start";
        GraphNode step = GraphNode::create("step");
        step.id = "node_step";
        GraphNode finish = GraphNode::create("finish");
        finish.id = "node_finish";

        QVERIFY(parent.addNode(start));
        QVERIFY(parent.addNode(step));
        QVERIFY(parent.addNode(finish));

        QVERIFY(parent.addConnection({{start.id, "flow_out"}, {step.id, "flow_in"}, PortKind::Execution}));
        QVERIFY(parent.addConnection({{start.id, "value"}, {step.id, "value"}, PortKind::Data}));
        QVERIFY(parent.addConnection({{step.id, "flow_out"}, {finish.id, "flow_in"}, PortKind::Execution}));
        QVERIFY(parent.addConnection({{step.id, "result"}, {finish.id, "value"}, PortKind::Data}));

        const auto result = SubModuleFactory::createFromSelection(
            parent, {step.id}, "StepUnit", m_registry);

        QCOMPARE(result.module.origin, QString("graph"));
        QCOMPARE(result.module.graphId, result.innerGraph.id);
        QCOMPARE(result.innerGraph.parentModuleId, result.module.id);
        QVERIFY(result.module.hasInput("flow_in"));
        QVERIFY(result.module.hasInput("value"));
        QVERIFY(result.module.hasOutput("flow_out"));
        QVERIFY(result.module.hasOutput("result"));

        const auto *flowIn = result.module.findBoundaryInput("flow_in");
        const auto *valueIn = result.module.findBoundaryInput("value");
        const auto *flowOut = result.module.findBoundaryOutput("flow_out");
        const auto *resultOut = result.module.findBoundaryOutput("result");
        QVERIFY(flowIn != nullptr);
        QVERIFY(valueIn != nullptr);
        QVERIFY(flowOut != nullptr);
        QVERIFY(resultOut != nullptr);

        QCOMPARE(flowIn->internalNodeId, step.id);
        QCOMPARE(flowIn->internalPortName, QString("flow_in"));
        QCOMPARE(flowIn->kind, PortKind::Execution);
        QCOMPARE(valueIn->internalPortName, QString("value"));
        QCOMPARE(valueIn->kind, PortKind::Data);
        QCOMPARE(flowOut->internalPortName, QString("flow_out"));
        QCOMPARE(flowOut->kind, PortKind::Execution);
        QCOMPARE(resultOut->internalPortName, QString("result"));
        QCOMPARE(resultOut->kind, PortKind::Data);

        // После сворачивания выделения внешние data/exec связи должны перейти на новый submodule node.
        QCOMPARE(result.updatedParentGraph.id, parent.id);
        QCOMPARE(result.updatedParentGraph.nodes.size(), 3);
        QVERIFY(result.updatedParentGraph.findNode(start.id) != nullptr);
        QVERIFY(result.updatedParentGraph.findNode(finish.id) != nullptr);
        const GraphNode *subNode = result.updatedParentGraph.findNode(result.createdNode.id);
        QVERIFY(subNode != nullptr);
        QCOMPARE(subNode->moduleId, result.module.id);

        bool hasExecIn = false;
        bool hasDataIn = false;
        bool hasExecOut = false;
        bool hasDataOut = false;
        for (const auto &conn : result.updatedParentGraph.connections) {
            if (conn.from.nodeId == start.id &&
                conn.from.portName == "flow_out" &&
                conn.to.nodeId == result.createdNode.id &&
                conn.to.portName == "flow_in" &&
                conn.kind == PortKind::Execution) {
                hasExecIn = true;
            }
            if (conn.from.nodeId == start.id &&
                conn.from.portName == "value" &&
                conn.to.nodeId == result.createdNode.id &&
                conn.to.portName == "value" &&
                conn.kind == PortKind::Data) {
                hasDataIn = true;
            }
            if (conn.from.nodeId == result.createdNode.id &&
                conn.from.portName == "flow_out" &&
                conn.to.nodeId == finish.id &&
                conn.to.portName == "flow_in" &&
                conn.kind == PortKind::Execution) {
                hasExecOut = true;
            }
            if (conn.from.nodeId == result.createdNode.id &&
                conn.from.portName == "result" &&
                conn.to.nodeId == finish.id &&
                conn.to.portName == "value" &&
                conn.kind == PortKind::Data) {
                hasDataOut = true;
            }
        }

        QVERIFY(hasExecIn);
        QVERIFY(hasDataIn);
        QVERIFY(hasExecOut);
        QVERIFY(hasDataOut);

        QVERIFY(m_graphStore->registerGraph(result.innerGraph));
        QVERIFY(m_registry->registerModule(result.module));

        QString reason;
        QVERIFY(m_registry->isModuleAdmittedForComposition(result.module, &reason));
        QVERIFY(reason.isEmpty());
    }
};

QTEST_MAIN(TestSubModuleFactory)
#include "test_SubModuleFactory.moc"
