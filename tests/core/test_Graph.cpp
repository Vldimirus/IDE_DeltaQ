// Тесты Graph/GraphNode/GraphConnection — сериализация, манипуляция, валидация
#include <QtTest>
#include <deltaq/Graph.h>

using namespace DeltaQ;

class TestGraph : public QObject {
    Q_OBJECT

private slots:
    // --- GraphNode ---

    void nodeCreate()
    {
        auto n = GraphNode::create("mod-1", QPointF(100, 200));
        QVERIFY(!n.id.isEmpty());
        QCOMPARE(n.moduleId, "mod-1");
        QCOMPARE(n.position, QPointF(100, 200));
    }

    void nodeRoundtrip()
    {
        auto n = GraphNode::create("mod-2", QPointF(50, 75));
        n.properties["label"] = "Фильтр";
        n.properties["color"] = "#ff0000";

        auto json = n.toJson();
        auto restored = GraphNode::fromJson(json);
        QCOMPARE(restored.id, n.id);
        QCOMPARE(restored.moduleId, n.moduleId);
        QCOMPARE(restored.position, n.position);
        QCOMPARE(restored.properties, n.properties);
    }

    void nodeEquality()
    {
        GraphNode a, b;
        a.id = "node-1";
        b.id = "node-1";
        QVERIFY(a == b);

        b.id = "node-2";
        QVERIFY(!(a == b));
    }

    // --- GraphConnection ---

    void endpointEquality()
    {
        GraphConnection::Endpoint a{"n1", "out"};
        GraphConnection::Endpoint b{"n1", "out"};
        GraphConnection::Endpoint c{"n1", "in"};
        QVERIFY(a == b);
        QVERIFY(!(a == c));
    }

    void connectionRoundtrip()
    {
        GraphConnection conn{{"n1", "output"}, {"n2", "input"}};
        auto json = conn.toJson();
        auto restored = GraphConnection::fromJson(json);
        QCOMPARE(restored.from.nodeId, "n1");
        QCOMPARE(restored.from.portName, "output");
        QCOMPARE(restored.to.nodeId, "n2");
        QCOMPARE(restored.to.portName, "input");
    }

    void connectionEquality()
    {
        GraphConnection a{{"n1", "out"}, {"n2", "in"}};
        GraphConnection b{{"n1", "out"}, {"n2", "in"}};
        GraphConnection c{{"n1", "out"}, {"n3", "in"}};
        QVERIFY(a == b);
        QVERIFY(!(a == c));
    }

    // --- Graph ---

    void graphCreate()
    {
        auto g = Graph::create("MainGraph");
        QVERIFY(!g.id.isEmpty());
        QCOMPARE(g.name, "MainGraph");
        QCOMPARE(g.nodeCount(), 0);
        QCOMPARE(g.connectionCount(), 0);
    }

    void graphRoundtrip()
    {
        auto g = Graph::create("TestGraph");
        GraphNode n1;
        n1.id = "n1"; n1.moduleId = "m1"; n1.position = QPointF(10, 20);
        GraphNode n2;
        n2.id = "n2"; n2.moduleId = "m2"; n2.position = QPointF(30, 40);
        g.addNode(n1);
        g.addNode(n2);
        g.addConnection({{"n1", "out"}, {"n2", "in"}});

        auto json = g.toJson();
        auto restored = Graph::fromJson(json);
        QCOMPARE(restored.id, g.id);
        QCOMPARE(restored.name, g.name);
        QCOMPARE(restored.nodeCount(), 2);
        QCOMPARE(restored.connectionCount(), 1);
    }

    void graphEquality()
    {
        Graph a, b;
        a.id = "g1";
        b.id = "g1";
        QVERIFY(a == b);

        b.id = "g2";
        QVERIFY(!(a == b));
    }

    void graphAddNode()
    {
        auto g = Graph::create("G");
        GraphNode n;
        n.id = "n1"; n.moduleId = "m1";

        QVERIFY(g.addNode(n));
        QCOMPARE(g.nodeCount(), 1);

        // Дубликат id — не добавляется
        QVERIFY(!g.addNode(n));
        QCOMPARE(g.nodeCount(), 1);
    }

    void graphRemoveNode()
    {
        auto g = Graph::create("G");
        GraphNode n;
        n.id = "n1"; n.moduleId = "m1";
        g.addNode(n);

        QVERIFY(g.removeNode("n1"));
        QCOMPARE(g.nodeCount(), 0);

        // Удаление несуществующего
        QVERIFY(!g.removeNode("n1"));
    }

    void graphAddConnection()
    {
        auto g = Graph::create("G");
        g.addConnection({{"n1", "out"}, {"n2", "in"}});
        QCOMPARE(g.connectionCount(), 1);
    }

    void graphRemoveNodeCascade()
    {
        auto g = Graph::create("G");
        GraphNode n1, n2, n3;
        n1.id = "n1"; n1.moduleId = "m1";
        n2.id = "n2"; n2.moduleId = "m2";
        n3.id = "n3"; n3.moduleId = "m3";
        g.addNode(n1);
        g.addNode(n2);
        g.addNode(n3);

        g.addConnection({{"n1", "out"}, {"n2", "in"}});
        g.addConnection({{"n2", "out"}, {"n3", "in"}});
        g.addConnection({{"n1", "out2"}, {"n3", "in2"}});

        QCOMPARE(g.connectionCount(), 3);

        // Удаляем n2 — должны удалиться 2 соединения (n1→n2 и n2→n3)
        g.removeNode("n2");
        QCOMPARE(g.nodeCount(), 2);
        QCOMPARE(g.connectionCount(), 1);
        // Осталось только n1→n3
        QCOMPARE(g.connections[0].from.nodeId, "n1");
        QCOMPARE(g.connections[0].to.nodeId, "n3");
    }

    void graphFindNode()
    {
        auto g = Graph::create("G");
        GraphNode n;
        n.id = "n1"; n.moduleId = "m1";
        g.addNode(n);

        auto *found = g.findNode("n1");
        QVERIFY(found != nullptr);
        QCOMPARE(found->moduleId, "m1");

        QVERIFY(g.findNode("nonexistent") == nullptr);

        // const-версия
        const auto &cg = g;
        const auto *cfound = cg.findNode("n1");
        QVERIFY(cfound != nullptr);
    }

    void graphConnectionsForNode()
    {
        auto g = Graph::create("G");
        GraphNode n1, n2, n3;
        n1.id = "n1"; n1.moduleId = "m1";
        n2.id = "n2"; n2.moduleId = "m2";
        n3.id = "n3"; n3.moduleId = "m3";
        g.addNode(n1);
        g.addNode(n2);
        g.addNode(n3);

        g.addConnection({{"n1", "out"}, {"n2", "in"}});
        g.addConnection({{"n2", "out"}, {"n3", "in"}});
        g.addConnection({{"n1", "out2"}, {"n3", "in2"}});

        auto conns = g.connectionsForNode("n1");
        QCOMPARE(conns.size(), 2);

        conns = g.connectionsForNode("n2");
        QCOMPARE(conns.size(), 2); // входящее от n1 + исходящее к n3

        conns = g.connectionsForNode("n3");
        QCOMPARE(conns.size(), 2); // входящее от n2 + входящее от n1

        conns = g.connectionsForNode("nonexistent");
        QCOMPARE(conns.size(), 0);
    }

    void graphRemoveConnection()
    {
        auto g = Graph::create("G");
        g.addConnection({{"n1", "out"}, {"n2", "in"}});
        g.addConnection({{"n2", "out"}, {"n3", "in"}});

        QVERIFY(g.removeConnection("n1", "out", "n2", "in"));
        QCOMPARE(g.connectionCount(), 1);

        // Удаление несуществующего
        QVERIFY(!g.removeConnection("n1", "out", "n2", "in"));
    }

    void graphIsValid()
    {
        Graph g;
        QVERIFY(!g.isValid()); // id и name пусты

        g.id = "g1";
        QVERIFY(!g.isValid()); // name пуст

        g.name = "ok";
        QVERIFY(g.isValid());

        // Дубликаты узлов — невалидно
        GraphNode n1, n2;
        n1.id = "dup";
        n2.id = "dup";
        g.nodes = {n1, n2};
        QVERIFY(!g.isValid());
    }
};

QTEST_MAIN(TestGraph)
#include "test_Graph.moc"
