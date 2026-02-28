// Тесты UndoManager — обёртка над CommandBus, clean state
#include <QtTest>
#include <QSignalSpy>
#include "CommandBus.h"
#include "UndoManager.h"

using namespace DeltaQ;

// Тестовая команда
class UMTestCommand : public Command {
public:
    UMTestCommand(int &counter, QString desc = "test")
        : m_counter(counter), m_desc(std::move(desc)) {}

    void execute() override { ++m_counter; }
    void undo() override { --m_counter; }
    QString description() const override { return m_desc; }

private:
    int &m_counter;
    QString m_desc;
};

class TestUndoManager : public QObject {
    Q_OBJECT

private slots:
    void delegation_canUndoCanRedo()
    {
        CommandBus bus;
        UndoManager mgr(&bus);
        int counter = 0;

        QVERIFY(!mgr.canUndo());
        QVERIFY(!mgr.canRedo());

        bus.execute(std::make_unique<UMTestCommand>(counter));
        QVERIFY(mgr.canUndo());
        QVERIFY(!mgr.canRedo());
    }

    void undoRedo_delegatesToBus()
    {
        CommandBus bus;
        UndoManager mgr(&bus);
        int counter = 0;

        bus.execute(std::make_unique<UMTestCommand>(counter));
        QCOMPARE(counter, 1);

        mgr.undo();
        QCOMPARE(counter, 0);

        mgr.redo();
        QCOMPARE(counter, 1);
    }

    void cleanInitially()
    {
        CommandBus bus;
        UndoManager mgr(&bus);
        QVERIFY(mgr.isClean());
    }

    void cleanAfterExecute_isDirty()
    {
        CommandBus bus;
        UndoManager mgr(&bus);
        int counter = 0;

        bus.execute(std::make_unique<UMTestCommand>(counter));
        QVERIFY(!mgr.isClean());
    }

    void setClean_marksClean()
    {
        CommandBus bus;
        UndoManager mgr(&bus);
        int counter = 0;

        bus.execute(std::make_unique<UMTestCommand>(counter));
        QVERIFY(!mgr.isClean());

        mgr.setClean();
        QVERIFY(mgr.isClean());
    }

    void cleanAfterUndo_returnsToPreviousClean()
    {
        CommandBus bus;
        UndoManager mgr(&bus);
        int counter = 0;

        // Изначально чисто (index 0 = clean 0)
        bus.execute(std::make_unique<UMTestCommand>(counter));
        // index = 1, clean = 0 → dirty
        bus.undo();
        // index = 0, clean = 0 → clean
        QVERIFY(mgr.isClean());
    }

    void cleanAfterUndoRedo()
    {
        CommandBus bus;
        UndoManager mgr(&bus);
        int counter = 0;

        bus.execute(std::make_unique<UMTestCommand>(counter));
        mgr.setClean(); // clean = 1
        bus.undo();      // index = 0 → dirty
        QVERIFY(!mgr.isClean());
        bus.redo();      // index = 1 → clean
        QVERIFY(mgr.isClean());
    }

    void cleanChanged_signalEmitted()
    {
        CommandBus bus;
        UndoManager mgr(&bus);
        QSignalSpy spy(&mgr, &UndoManager::cleanChanged);
        int counter = 0;

        bus.execute(std::make_unique<UMTestCommand>(counter));
        // Должен быть сигнал cleanChanged(false)
        QVERIFY(spy.count() >= 1);
        QCOMPARE(spy.last().first().toBool(), false);

        mgr.setClean();
        // Должен быть сигнал cleanChanged(true)
        QCOMPARE(spy.last().first().toBool(), true);
    }

    void clear_resetsState()
    {
        CommandBus bus;
        UndoManager mgr(&bus);
        int counter = 0;

        bus.execute(std::make_unique<UMTestCommand>(counter));
        mgr.setClean();
        mgr.clear();

        QVERIFY(!bus.canUndo());
        QVERIFY(!bus.canRedo());
        // После clear: cleanIndex = -1, currentIndex = 0 → dirty
        QVERIFY(!mgr.isClean());
    }

    void descriptions_returnCorrectText()
    {
        CommandBus bus;
        UndoManager mgr(&bus);
        int counter = 0;

        bus.execute(std::make_unique<UMTestCommand>(counter, "Описание"));
        QCOMPARE(mgr.undoText(), "Описание");
        QCOMPARE(mgr.undoDescription(), "Описание");

        bus.undo();
        QCOMPARE(mgr.redoText(), "Описание");
        QCOMPARE(mgr.redoDescription(), "Описание");
    }

    void stateChanged_signalForwarded()
    {
        CommandBus bus;
        UndoManager mgr(&bus);
        QSignalSpy spy(&mgr, &UndoManager::stateChanged);
        int counter = 0;

        bus.execute(std::make_unique<UMTestCommand>(counter));
        QVERIFY(spy.count() >= 1);
    }
};

QTEST_MAIN(TestUndoManager)
#include "test_UndoManager.moc"
