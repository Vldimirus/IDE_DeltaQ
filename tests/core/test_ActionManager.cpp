// Тесты ActionManager — реестр действий, группы
#include <QtTest>
#include "ActionManager.h"

using namespace DeltaQ;

class TestActionManager : public QObject {
    Q_OBJECT

private slots:
    void registerAction_basic()
    {
        ActionManager mgr;
        auto *act = mgr.registerAction("test.action", "Test", QKeySequence("Ctrl+T"));
        QVERIFY(act != nullptr);
        QCOMPARE(act->text(), "Test");
        QCOMPARE(act->shortcut(), QKeySequence("Ctrl+T"));
    }

    void registerAction_withGroup()
    {
        ActionManager mgr;
        auto *act = mgr.registerAction("grp.action", "Grouped", QKeySequence(), "mygroup");
        QVERIFY(act != nullptr);
        auto group = mgr.actionsInGroup("mygroup");
        QCOMPARE(group.size(), 1);
        QCOMPARE(group.first(), act);
    }

    void addToGroup_addsExistingAction()
    {
        ActionManager mgr;
        auto *act = mgr.registerAction("some.action", "Some");
        mgr.addToGroup("some.action", "group1");
        auto group = mgr.actionsInGroup("group1");
        QCOMPARE(group.size(), 1);
        QCOMPARE(group.first(), act);
    }

    void enableGroup_enablesAll()
    {
        ActionManager mgr;
        auto *a = mgr.registerAction("a", "A", QKeySequence(), "grp");
        auto *b = mgr.registerAction("b", "B", QKeySequence(), "grp");
        mgr.disableGroup("grp");
        QVERIFY(!a->isEnabled());
        QVERIFY(!b->isEnabled());
        mgr.enableGroup("grp");
        QVERIFY(a->isEnabled());
        QVERIFY(b->isEnabled());
    }

    void disableGroup_disablesAll()
    {
        ActionManager mgr;
        auto *a = mgr.registerAction("x", "X", QKeySequence(), "grp2");
        auto *b = mgr.registerAction("y", "Y", QKeySequence(), "grp2");
        QVERIFY(a->isEnabled());
        QVERIFY(b->isEnabled());
        mgr.disableGroup("grp2");
        QVERIFY(!a->isEnabled());
        QVERIFY(!b->isEnabled());
    }

    void actionsInGroup_emptyForUnknown()
    {
        ActionManager mgr;
        auto group = mgr.actionsInGroup("nonexistent");
        QVERIFY(group.isEmpty());
    }

    void allActions_returnsAll()
    {
        ActionManager mgr;
        mgr.registerAction("a1", "A1");
        mgr.registerAction("a2", "A2");
        mgr.registerAction("a3", "A3");
        QCOMPARE(mgr.allActions().size(), 3);
    }

    void setupStandardActions_createsExpected()
    {
        ActionManager mgr;
        mgr.setupStandardActions();

        // Проверяем ключевые действия
        QVERIFY(mgr.action("file.newProject") != nullptr);
        QVERIFY(mgr.action("file.save") != nullptr);
        QVERIFY(mgr.action("edit.undo") != nullptr);
        QVERIFY(mgr.action("edit.redo") != nullptr);
        QVERIFY(mgr.action("build.build") != nullptr);
        QVERIFY(mgr.action("view.codeEditor") != nullptr);

        // Проверяем группы
        QVERIFY(!mgr.actionsInGroup("file").isEmpty());
        QVERIFY(!mgr.actionsInGroup("edit").isEmpty());
        QVERIFY(!mgr.actionsInGroup("build").isEmpty());
        QVERIFY(!mgr.actionsInGroup("view").isEmpty());
    }

    void unknownAction_returnsNull()
    {
        ActionManager mgr;
        QVERIFY(mgr.action("nonexistent.action") == nullptr);
    }
};

QTEST_MAIN(TestActionManager)
#include "test_ActionManager.moc"
