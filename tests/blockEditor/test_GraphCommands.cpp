// Тесты GraphCommands — undo/redo для всех операций с графом
#include <QtTest>
#include <QApplication>
#include <deltaq/Graph.h>
#include <deltaq/Module.h>
#include "../../src/core/ModuleRegistry.h"
#include "../../src/core/CommandBus.h"
#include "../../src/blockEditor/BlockScene.h"
#include "../../src/blockEditor/NodeItem.h"
#include "../../src/blockEditor/GraphCommands.h"

using namespace DeltaQ;

class TestGraphCommands : public QObject {
    Q_OBJECT

private:
    ModuleRegistry *m_registry = nullptr;
    CommandBus *m_bus = nullptr;
    Graph m_graph;

    void registerTestModules()
    {
        Module add;
        add.id = "add"; add.name = "Add"; add.category = "math";
        add.language = "c"; add.version = "1.0"; add.origin = "user";
        add.inputs = {{"a", "int", "0"}, {"b", "int", "0"}};
        add.outputs = {{"result", "int", ""}};
        m_registry->registerModule(add);

        Module print;
        print.id = "print"; print.name = "Print"; print.category = "io";
        print.language = "c"; print.version = "1.0"; print.origin = "user";
        print.inputs = {{"value", "int", "0"}};
        m_registry->registerModule(print);
    }

private slots:
    void init()
    {
        m_registry = new ModuleRegistry(this);
        m_bus = new CommandBus(this);
        m_graph = Graph::create("TestGraph");
        registerTestModules();
    }

    void cleanup()
    {
        delete m_bus;
        delete m_registry;
        m_registry = nullptr;
        m_bus = nullptr;
    }

    void addNodeAndUndo()
    {
        BlockScene scene(m_registry, m_bus);
        auto node = GraphNode::create("add", QPointF(50, 50));

        auto cmd = std::make_unique<AddNodeCommand>(&scene, &m_graph, node);
        m_bus->execute(std::move(cmd));

        QCOMPARE(m_graph.nodeCount(), 1);
        QVERIFY(scene.nodeItem(node.id) != nullptr);

        m_bus->undo();
        QCOMPARE(m_graph.nodeCount(), 0);
        QVERIFY(scene.nodeItem(node.id) == nullptr);

        m_bus->redo();
        QCOMPARE(m_graph.nodeCount(), 1);
        QVERIFY(scene.nodeItem(node.id) != nullptr);
    }

    void removeNodeAndUndo()
    {
        BlockScene scene(m_registry, m_bus);
        auto node = GraphNode::create("add", QPointF(50, 50));
        m_graph.addNode(node);
        scene.addNodeItem(node);

        auto cmd = std::make_unique<RemoveNodeCommand>(&scene, &m_graph, node.id);
        m_bus->execute(std::move(cmd));

        QCOMPARE(m_graph.nodeCount(), 0);
        QVERIFY(scene.nodeItem(node.id) == nullptr);

        m_bus->undo();
        QCOMPARE(m_graph.nodeCount(), 1);
        QVERIFY(scene.nodeItem(node.id) != nullptr);
    }

    void removeNodeWithConnections()
    {
        BlockScene scene(m_registry, m_bus);
        auto n1 = GraphNode::create("add", QPointF(0, 0));
        auto n2 = GraphNode::create("print", QPointF(300, 0));
        m_graph.addNode(n1);
        m_graph.addNode(n2);
        scene.addNodeItem(n1);
        scene.addNodeItem(n2);

        GraphConnection conn{{"", ""}, {"", ""}};
        conn.from = {n1.id, "result"};
        conn.to = {n2.id, "value"};
        m_graph.addConnection(conn);
        scene.addConnectionItem(n1.id, "result", n2.id, "value");

        // Удаляем n1 — должно удалить и соединение
        auto cmd = std::make_unique<RemoveNodeCommand>(&scene, &m_graph, n1.id);
        m_bus->execute(std::move(cmd));

        QCOMPARE(m_graph.nodeCount(), 1);
        QCOMPARE(m_graph.connectionCount(), 0);

        // Undo — восстанавливает узел и соединение
        m_bus->undo();
        QCOMPARE(m_graph.nodeCount(), 2);
        QCOMPARE(m_graph.connectionCount(), 1);
        QCOMPARE(scene.connectionItems().size(), 1);
    }

    void moveNodeAndUndo()
    {
        BlockScene scene(m_registry, m_bus);
        auto node = GraphNode::create("add", QPointF(100, 100));
        m_graph.addNode(node);
        scene.addNodeItem(node);

        auto cmd = std::make_unique<MoveNodeCommand>(&scene, &m_graph, node.id,
                                                      QPointF(100, 100), QPointF(200, 300));
        m_bus->execute(std::move(cmd));

        auto *graphNode = m_graph.findNode(node.id);
        QCOMPARE(graphNode->position, QPointF(200, 300));
        QCOMPARE(scene.nodeItem(node.id)->pos(), QPointF(200, 300));

        m_bus->undo();
        QCOMPARE(graphNode->position, QPointF(100, 100));
        QCOMPARE(scene.nodeItem(node.id)->pos(), QPointF(100, 100));
    }

    void connectAndUndo()
    {
        BlockScene scene(m_registry, m_bus);
        auto n1 = GraphNode::create("add", QPointF(0, 0));
        auto n2 = GraphNode::create("print", QPointF(300, 0));
        m_graph.addNode(n1);
        m_graph.addNode(n2);
        scene.addNodeItem(n1);
        scene.addNodeItem(n2);

        GraphConnection conn;
        conn.from = {n1.id, "result"};
        conn.to = {n2.id, "value"};

        auto cmd = std::make_unique<ConnectCommand>(&scene, &m_graph, conn);
        m_bus->execute(std::move(cmd));

        QCOMPARE(m_graph.connectionCount(), 1);
        QCOMPARE(scene.connectionItems().size(), 1);

        m_bus->undo();
        QCOMPARE(m_graph.connectionCount(), 0);
        QCOMPARE(scene.connectionItems().size(), 0);
    }

    void disconnectAndUndo()
    {
        BlockScene scene(m_registry, m_bus);
        auto n1 = GraphNode::create("add", QPointF(0, 0));
        auto n2 = GraphNode::create("print", QPointF(300, 0));
        m_graph.addNode(n1);
        m_graph.addNode(n2);
        scene.addNodeItem(n1);
        scene.addNodeItem(n2);

        GraphConnection conn;
        conn.from = {n1.id, "result"};
        conn.to = {n2.id, "value"};
        m_graph.addConnection(conn);
        scene.addConnectionItem(n1.id, "result", n2.id, "value");

        auto cmd = std::make_unique<DisconnectCommand>(&scene, &m_graph, conn);
        m_bus->execute(std::move(cmd));

        QCOMPARE(m_graph.connectionCount(), 0);
        QCOMPARE(scene.connectionItems().size(), 0);

        m_bus->undo();
        QCOMPARE(m_graph.connectionCount(), 1);
        QCOMPARE(scene.connectionItems().size(), 1);
    }

    void moveMerge()
    {
        BlockScene scene(m_registry, m_bus);
        auto node = GraphNode::create("add", QPointF(0, 0));
        m_graph.addNode(node);
        scene.addNodeItem(node);

        // Два последовательных перемещения одного узла — должны склеиться
        auto cmd1 = std::make_unique<MoveNodeCommand>(&scene, &m_graph, node.id,
                                                       QPointF(0, 0), QPointF(50, 50));
        auto cmd2 = std::make_unique<MoveNodeCommand>(&scene, &m_graph, node.id,
                                                       QPointF(50, 50), QPointF(100, 100));
        // Проверяем mergeWith
        QVERIFY(cmd1->mergeWith(cmd2.get()));

        // После мерджа cmd1 должна знать финальную позицию
        cmd1->execute();
        auto *graphNode = m_graph.findNode(node.id);
        QCOMPARE(graphNode->position, QPointF(100, 100));

        // Undo должен вернуть в начальную позицию
        cmd1->undo();
        QCOMPARE(graphNode->position, QPointF(0, 0));
    }

    void changeProperty()
    {
        auto node = GraphNode::create("add", QPointF(0, 0));
        m_graph.addNode(node);

        auto cmd = std::make_unique<ChangePropertyCommand>(&m_graph, node.id, "label", "test");
        m_bus->execute(std::move(cmd));

        QCOMPARE(m_graph.findNode(node.id)->properties["label"], "test");

        m_bus->undo();
        QVERIFY(!m_graph.findNode(node.id)->properties.contains("label"));
    }
};

QTEST_MAIN(TestGraphCommands)
#include "test_GraphCommands.moc"
