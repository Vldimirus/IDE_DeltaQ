// Тесты GraphStore — хранилище графов, файловый I/O, сигналы
#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include "GraphStore.h"

using namespace DeltaQ;

class TestGraphStore : public QObject {
    Q_OBJECT

private slots:
    void registerAndFind()
    {
        GraphStore store;
        auto g = Graph::create("TestGraph");
        QVERIFY(store.registerGraph(g));
        QCOMPARE(store.count(), 1);

        auto *found = store.findGraph(g.id);
        QVERIFY(found != nullptr);
        QCOMPARE(found->name, "TestGraph");
    }

    void registerEmptyId_fails()
    {
        GraphStore store;
        Graph g;
        QVERIFY(!store.registerGraph(g));
        QCOMPARE(store.count(), 0);
    }

    void registerDuplicate_updates()
    {
        GraphStore store;
        auto g = Graph::create("Original");
        store.registerGraph(g);
        g.name = "Updated";
        QSignalSpy spy(&store, &GraphStore::graphUpdated);
        store.registerGraph(g);
        QCOMPARE(store.count(), 1);
        QCOMPARE(store.findGraph(g.id)->name, "Updated");
        QCOMPARE(spy.count(), 1);
    }

    void unregister()
    {
        GraphStore store;
        auto g = Graph::create("G");
        store.registerGraph(g);
        QSignalSpy spy(&store, &GraphStore::graphUnregistered);

        QVERIFY(store.unregisterGraph(g.id));
        QCOMPARE(store.count(), 0);
        QCOMPARE(spy.count(), 1);

        // Повторное удаление
        QVERIFY(!store.unregisterGraph(g.id));
    }

    void findByName()
    {
        GraphStore store;
        auto g1 = Graph::create("Alpha");
        auto g2 = Graph::create("Beta");
        store.registerGraph(g1);
        store.registerGraph(g2);

        auto *found = store.findGraphByName("Beta");
        QVERIFY(found != nullptr);
        QCOMPARE(found->id, g2.id);

        QVERIFY(store.findGraphByName("Gamma") == nullptr);
    }

    void allGraphs()
    {
        GraphStore store;
        store.registerGraph(Graph::create("A"));
        store.registerGraph(Graph::create("B"));
        store.registerGraph(Graph::create("C"));

        auto all = store.allGraphs();
        QCOMPARE(all.size(), 3);
    }

    void clear()
    {
        GraphStore store;
        store.registerGraph(Graph::create("G"));
        QSignalSpy spy(&store, &GraphStore::storeCleared);

        store.clear();
        QCOMPARE(store.count(), 0);
        QCOMPARE(spy.count(), 1);
    }

    void signals_registered()
    {
        GraphStore store;
        QSignalSpy spy(&store, &GraphStore::graphRegistered);
        auto g = Graph::create("Sig");
        store.registerGraph(g);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toString(), g.id);
    }

    void saveAndLoadFile()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        GraphStore store;
        auto g = Graph::create("Saved");
        GraphNode n;
        n.id = "n1"; n.moduleId = "m1"; n.position = QPointF(10, 20);
        g.addNode(n);
        g.addConnection({{"n1", "out"}, {"n1", "in"}});

        QString path = tmpDir.path() + "/test.dqgraph";
        QVERIFY(store.saveGraphFile(g, path));

        // Загружаем в другой store
        GraphStore store2;
        QVERIFY(store2.loadGraphFile(path));
        QCOMPARE(store2.count(), 1);

        auto *loaded = store2.findGraph(g.id);
        QVERIFY(loaded != nullptr);
        QCOMPARE(loaded->name, "Saved");
        QCOMPARE(loaded->nodeCount(), 1);
        QCOMPARE(loaded->connectionCount(), 1);
    }

    void loadFromDirectory()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Создаём поддиректорию graphs/
        QDir(tmpDir.path()).mkpath("graphs");

        // Сохраняем 2 графа
        GraphStore writer;
        auto g1 = Graph::create("Graph1");
        auto g2 = Graph::create("Graph2");
        writer.saveGraphFile(g1, tmpDir.path() + "/graphs/g1.dqgraph");
        writer.saveGraphFile(g2, tmpDir.path() + "/graphs/g2.dqgraph");

        // Загружаем из директории
        GraphStore reader;
        QVERIFY(reader.loadFromDirectory(tmpDir.path()));
        QCOMPARE(reader.count(), 2);
    }

    void saveAll()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        GraphStore store;
        store.registerGraph(Graph::create("A"));
        store.registerGraph(Graph::create("B"));

        QVERIFY(store.saveAll(tmpDir.path()));

        // Проверяем, что файлы созданы
        QDir graphsDir(tmpDir.path() + "/graphs");
        QVERIFY(graphsDir.exists());
        auto files = graphsDir.entryList({"*.dqgraph"}, QDir::Files);
        QCOMPARE(files.size(), 2);
    }
};

QTEST_MAIN(TestGraphStore)
#include "test_GraphStore.moc"
