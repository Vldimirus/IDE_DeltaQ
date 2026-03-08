// Тесты BlockScene — управление узлами и соединениями на сцене
#include <QtTest>
#include <QApplication>
#include <deltaq/Graph.h>
#include <deltaq/Module.h>
#include "../../src/core/ModuleRegistry.h"
#include "../../src/core/CommandBus.h"
#include "../../src/blockEditor/BlockScene.h"
#include "../../src/blockEditor/NodeItem.h"
#include "../../src/blockEditor/PortItem.h"
#include "../../src/blockEditor/ConnectionItem.h"

using namespace DeltaQ;

class TestBlockScene : public QObject {
    Q_OBJECT

private:
    ModuleRegistry *m_registry = nullptr;
    CommandBus *m_bus = nullptr;

    // Вспомогательный: регистрация модуля с портами
    void registerTestModule(const QString &id, const QString &name,
                            const QString &category,
                            const QVector<Port> &inputs,
                            const QVector<Port> &outputs)
    {
        Module mod;
        mod.id = id;
        mod.name = name;
        mod.category = category;
        mod.language = "c";
        mod.version = "1.0";
        mod.origin = "user";
        mod.inputs = inputs;
        mod.outputs = outputs;
        m_registry->registerModule(mod);
    }

private slots:
    void init()
    {
        m_registry = new ModuleRegistry(this);
        m_bus = new CommandBus(this);

        // Регистрируем тестовые модули
        registerTestModule("add", "Add", "math",
                           {{"a", "int", "0"}, {"b", "int", "0"}},
                           {{"result", "int", ""}});
        registerTestModule("print", "Print", "io",
                           {{"value", "int", "0"}},
                           {});
        registerTestModule("exec_chain", "ExecChain", "control",
                           {Port::exec("flow_in"), {"value", "int", "0"}},
                           {Port::exec("flow_out"), {"result", "int", ""}});
        registerTestModule("wide_node", "create_window", "desktop",
                           {Port::exec("flow_in"),
                            {"title", "string", "\"window1\""},
                            {"width", "int", "640"},
                            {"height", "int", "480"}},
                           {Port::exec("flow_out"),
                            {"window", "sdl_window", ""}});
    }

    void cleanup()
    {
        delete m_bus;
        delete m_registry;
        m_registry = nullptr;
        m_bus = nullptr;
    }

    void addNode()
    {
        BlockScene scene(m_registry, m_bus);
        auto node = GraphNode::create("add", QPointF(100, 200));

        auto *item = scene.addNodeItem(node);
        QVERIFY(item != nullptr);
        QCOMPARE(item->nodeId(), node.id);
        QCOMPARE(item->pos(), QPointF(100, 200));
        QCOMPARE(item->inputPorts().size(), 2);
        QCOMPARE(item->outputPorts().size(), 1);
    }

    void removeNode()
    {
        BlockScene scene(m_registry, m_bus);
        auto node = GraphNode::create("add", QPointF(0, 0));

        scene.addNodeItem(node);
        QVERIFY(scene.nodeItem(node.id) != nullptr);

        scene.removeNodeItem(node.id);
        QVERIFY(scene.nodeItem(node.id) == nullptr);
    }

    void addConnection()
    {
        BlockScene scene(m_registry, m_bus);
        auto n1 = GraphNode::create("add", QPointF(0, 0));
        auto n2 = GraphNode::create("print", QPointF(300, 0));

        scene.addNodeItem(n1);
        scene.addNodeItem(n2);

        auto *conn = scene.addConnectionItem(n1.id, "result", n2.id, "value");
        QVERIFY(conn != nullptr);
        QCOMPARE(scene.connectionItems().size(), 1);
    }

    void removeConnection()
    {
        BlockScene scene(m_registry, m_bus);
        auto n1 = GraphNode::create("add", QPointF(0, 0));
        auto n2 = GraphNode::create("print", QPointF(300, 0));

        scene.addNodeItem(n1);
        scene.addNodeItem(n2);
        scene.addConnectionItem(n1.id, "result", n2.id, "value");

        scene.removeConnectionItem(n1.id, "result", n2.id, "value");
        QCOMPARE(scene.connectionItems().size(), 0);
    }

    void canConnectValid()
    {
        BlockScene scene(m_registry, m_bus);
        auto n1 = GraphNode::create("add", QPointF(0, 0));
        auto n2 = GraphNode::create("print", QPointF(300, 0));

        auto *item1 = scene.addNodeItem(n1);
        auto *item2 = scene.addNodeItem(n2);

        auto *outPort = item1->findPort("result", PortDirection::Output);
        auto *inPort = item2->findPort("value", PortDirection::Input);
        QVERIFY(outPort != nullptr);
        QVERIFY(inPort != nullptr);
        QVERIFY(scene.canConnect(outPort, inPort));
    }

    void canConnectSameDirection()
    {
        BlockScene scene(m_registry, m_bus);
        auto n1 = GraphNode::create("add", QPointF(0, 0));
        auto n2 = GraphNode::create("add", QPointF(300, 0));

        auto *item1 = scene.addNodeItem(n1);
        auto *item2 = scene.addNodeItem(n2);

        auto *out1 = item1->findPort("result", PortDirection::Output);
        auto *out2 = item2->findPort("result", PortDirection::Output);
        QVERIFY(!scene.canConnect(out1, out2));
    }

    void canConnectSameNode()
    {
        BlockScene scene(m_registry, m_bus);
        auto n1 = GraphNode::create("add", QPointF(0, 0));
        auto *item1 = scene.addNodeItem(n1);

        auto *outPort = item1->findPort("result", PortDirection::Output);
        auto *inPort = item1->findPort("a", PortDirection::Input);
        QVERIFY(!scene.canConnect(outPort, inPort));
    }

    void cascadeRemove()
    {
        BlockScene scene(m_registry, m_bus);
        auto n1 = GraphNode::create("add", QPointF(0, 0));
        auto n2 = GraphNode::create("print", QPointF(300, 0));

        scene.addNodeItem(n1);
        scene.addNodeItem(n2);
        scene.addConnectionItem(n1.id, "result", n2.id, "value");
        QCOMPARE(scene.connectionItems().size(), 1);

        // Удаление узла должно удалить все его соединения
        scene.removeNodeItem(n1.id);
        QCOMPARE(scene.connectionItems().size(), 0);
    }

    void loadFromGraphAndToGraph()
    {
        BlockScene scene(m_registry, m_bus);

        Graph g = Graph::create("TestGraph");
        GraphNode n1;
        n1.id = "n1"; n1.moduleId = "add"; n1.position = QPointF(100, 100);
        GraphNode n2;
        n2.id = "n2"; n2.moduleId = "print"; n2.position = QPointF(300, 100);
        g.addNode(n1);
        g.addNode(n2);
        g.addConnection({{"n1", "result"}, {"n2", "value"}});

        scene.loadFromGraph(g);
        QCOMPARE(scene.nodeItems().size(), 2);
        QCOMPARE(scene.connectionItems().size(), 1);

        // Конвертируем обратно
        Graph result = scene.toGraph(g.id, g.name);
        QCOMPARE(result.id, g.id);
        QCOMPARE(result.name, g.name);
        QCOMPARE(result.nodeCount(), 2);
        QCOMPARE(result.connectionCount(), 1);
    }

    void clearScene()
    {
        BlockScene scene(m_registry, m_bus);
        auto n1 = GraphNode::create("add", QPointF(0, 0));
        scene.addNodeItem(n1);

        scene.clearScene();
        QCOMPARE(scene.nodeItems().size(), 0);
        QCOMPARE(scene.connectionItems().size(), 0);
    }

    void duplicateNodePrevention()
    {
        BlockScene scene(m_registry, m_bus);
        auto node = GraphNode::create("add", QPointF(0, 0));

        auto *item1 = scene.addNodeItem(node);
        auto *item2 = scene.addNodeItem(node); // дубликат
        QVERIFY(item1 != nullptr);
        QVERIFY(item2 == item1); // возвращает существующий
    }

    void executionPortsAreVertical()
    {
        BlockScene scene(m_registry, m_bus);
        auto node = GraphNode::create("exec_chain", QPointF(0, 0));
        auto *item = scene.addNodeItem(node);
        QVERIFY(item != nullptr);

        auto *execIn = item->findPort("flow_in", PortDirection::Input);
        auto *execOut = item->findPort("flow_out", PortDirection::Output);
        auto *dataIn = item->findPort("value", PortDirection::Input);
        auto *dataOut = item->findPort("result", PortDirection::Output);
        QVERIFY(execIn != nullptr);
        QVERIFY(execOut != nullptr);
        QVERIFY(dataIn != nullptr);
        QVERIFY(dataOut != nullptr);

        QCOMPARE(execIn->portKind(), PortKind::Execution);
        QCOMPARE(execOut->portKind(), PortKind::Execution);
        QVERIFY(qAbs(execIn->pos().x() - item->boundingRect().center().x()) < 1.0);
        QVERIFY(qAbs(execOut->pos().x() - item->boundingRect().center().x()) < 1.0);
        QVERIFY(execIn->pos().y() < dataIn->pos().y());
        QVERIFY(execOut->pos().y() > dataOut->pos().y());
    }

    void longPortLabelsDoNotOverlap()
    {
        BlockScene scene(m_registry, m_bus);
        auto node = GraphNode::create("wide_node", QPointF(0, 0));
        auto *item = scene.addNodeItem(node);
        QVERIFY(item != nullptr);
        QVERIFY(item->boundingRect().width() > 180.0);

        auto *execIn = item->findPort("flow_in", PortDirection::Input);
        auto *execOut = item->findPort("flow_out", PortDirection::Output);
        auto *title = item->findPort("title", PortDirection::Input);
        auto *window = item->findPort("window", PortDirection::Output);
        QVERIFY(execIn != nullptr);
        QVERIFY(execOut != nullptr);
        QVERIFY(title != nullptr);
        QVERIFY(window != nullptr);

        QVERIFY(title->labelRectInNode().right() < window->labelRectInNode().left());
        QVERIFY(execIn->labelRectInNode().bottom() < 0.0);
        QVERIFY(execOut->labelRectInNode().top() > item->boundingRect().height());
    }
};

QTEST_MAIN(TestBlockScene)
#include "test_BlockScene.moc"
