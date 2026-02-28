// Тесты UILayoutStore — хранилище UI-макетов, файловый I/O, сигналы
#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include "UILayoutStore.h"

using namespace DeltaQ;

class TestUILayoutStore : public QObject {
    Q_OBJECT

private slots:
    void registerAndFind()
    {
        UILayoutStore store;
        auto l = UILayout::create("MainWindow");
        QVERIFY(store.registerLayout(l));
        QCOMPARE(store.count(), 1);

        auto *found = store.findLayout(l.id);
        QVERIFY(found != nullptr);
        QCOMPARE(found->name, "MainWindow");
    }

    void registerEmptyId_fails()
    {
        UILayoutStore store;
        UILayout l;
        QVERIFY(!store.registerLayout(l));
        QCOMPARE(store.count(), 0);
    }

    void registerDuplicate_updates()
    {
        UILayoutStore store;
        auto l = UILayout::create("Original");
        store.registerLayout(l);
        l.name = "Updated";
        QSignalSpy spy(&store, &UILayoutStore::layoutUpdated);
        store.registerLayout(l);
        QCOMPARE(store.count(), 1);
        QCOMPARE(store.findLayout(l.id)->name, "Updated");
        QCOMPARE(spy.count(), 1);
    }

    void unregister()
    {
        UILayoutStore store;
        auto l = UILayout::create("L");
        store.registerLayout(l);
        QSignalSpy spy(&store, &UILayoutStore::layoutUnregistered);

        QVERIFY(store.unregisterLayout(l.id));
        QCOMPARE(store.count(), 0);
        QCOMPARE(spy.count(), 1);

        QVERIFY(!store.unregisterLayout(l.id));
    }

    void findByName()
    {
        UILayoutStore store;
        auto l1 = UILayout::create("Settings");
        auto l2 = UILayout::create("Dashboard");
        store.registerLayout(l1);
        store.registerLayout(l2);

        auto *found = store.findLayoutByName("Dashboard");
        QVERIFY(found != nullptr);
        QCOMPARE(found->id, l2.id);

        QVERIFY(store.findLayoutByName("Missing") == nullptr);
    }

    void allLayouts()
    {
        UILayoutStore store;
        store.registerLayout(UILayout::create("A"));
        store.registerLayout(UILayout::create("B"));

        auto all = store.allLayouts();
        QCOMPARE(all.size(), 2);
    }

    void clear()
    {
        UILayoutStore store;
        store.registerLayout(UILayout::create("L"));
        QSignalSpy spy(&store, &UILayoutStore::storeCleared);

        store.clear();
        QCOMPARE(store.count(), 0);
        QCOMPARE(spy.count(), 1);
    }

    void saveAndLoadFile()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        UILayoutStore store;
        auto l = UILayout::create("TestLayout");
        auto btn = UIWidget::create("Button", "btn_ok");
        btn.geometry = QRectF(10, 20, 80, 30);
        btn.events["on_click"] = "handle_ok";
        l.window.children = {btn};

        QString path = tmpDir.path() + "/test.dqui";
        QVERIFY(store.saveLayoutFile(l, path));

        UILayoutStore store2;
        QVERIFY(store2.loadLayoutFile(path));
        QCOMPARE(store2.count(), 1);

        auto *loaded = store2.findLayout(l.id);
        QVERIFY(loaded != nullptr);
        QCOMPARE(loaded->name, "TestLayout");
        QCOMPARE(loaded->window.children.size(), 1);
        QCOMPARE(loaded->window.children[0].name, "btn_ok");
    }

    void loadFromDirectory()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QDir(tmpDir.path()).mkpath("ui");

        UILayoutStore writer;
        auto l1 = UILayout::create("Main");
        auto l2 = UILayout::create("Settings");
        writer.saveLayoutFile(l1, tmpDir.path() + "/ui/main.dqui");
        writer.saveLayoutFile(l2, tmpDir.path() + "/ui/settings.dqui");

        UILayoutStore reader;
        QVERIFY(reader.loadFromDirectory(tmpDir.path()));
        QCOMPARE(reader.count(), 2);
    }

    void saveAll()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        UILayoutStore store;
        store.registerLayout(UILayout::create("X"));
        store.registerLayout(UILayout::create("Y"));

        QVERIFY(store.saveAll(tmpDir.path()));

        QDir uiDir(tmpDir.path() + "/ui");
        QVERIFY(uiDir.exists());
        auto files = uiDir.entryList({"*.dqui"}, QDir::Files);
        QCOMPARE(files.size(), 2);
    }
};

QTEST_MAIN(TestUILayoutStore)
#include "test_UILayoutStore.moc"
