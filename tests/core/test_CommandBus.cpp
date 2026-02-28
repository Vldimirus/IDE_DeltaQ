// Тесты CommandBus — шина команд с undo/redo, макрокоманды, потокобезопасность
#include <QtTest>
#include <QSignalSpy>
#include <QThread>
#include "CommandBus.h"

using namespace DeltaQ;

// Тестовая команда — увеличивает/уменьшает счётчик
class TestCommand : public Command {
public:
    TestCommand(int &counter, QString desc = "test")
        : m_counter(counter), m_desc(std::move(desc)) {}

    void execute() override { ++m_counter; }
    void undo() override { --m_counter; }
    QString description() const override { return m_desc; }

private:
    int &m_counter;
    QString m_desc;
};

class TestCommandBus : public QObject {
    Q_OBJECT

private slots:
    // --- Базовые операции ---

    void execute_incrementsCounter()
    {
        CommandBus bus;
        int counter = 0;
        bus.execute(std::make_unique<TestCommand>(counter, "cmd1"));
        QCOMPARE(counter, 1);
    }

    void undo_decrementsCounter()
    {
        CommandBus bus;
        int counter = 0;
        bus.execute(std::make_unique<TestCommand>(counter));
        bus.undo();
        QCOMPARE(counter, 0);
    }

    void redo_incrementsAgain()
    {
        CommandBus bus;
        int counter = 0;
        bus.execute(std::make_unique<TestCommand>(counter));
        bus.undo();
        bus.redo();
        QCOMPARE(counter, 1);
    }

    void undoEmpty_doesNothing()
    {
        CommandBus bus;
        bus.undo(); // не должно упасть
        QVERIFY(!bus.canUndo());
    }

    void redoEmpty_doesNothing()
    {
        CommandBus bus;
        bus.redo(); // не должно упасть
        QVERIFY(!bus.canRedo());
    }

    void redo_clearedOnNewCommand()
    {
        CommandBus bus;
        int counter = 0;
        bus.execute(std::make_unique<TestCommand>(counter, "a"));
        bus.undo();
        QVERIFY(bus.canRedo());
        bus.execute(std::make_unique<TestCommand>(counter, "b"));
        QVERIFY(!bus.canRedo());
    }

    void description_returnsCorrectText()
    {
        CommandBus bus;
        int counter = 0;
        bus.execute(std::make_unique<TestCommand>(counter, "Добавить блок"));
        QCOMPARE(bus.undoText(), "Добавить блок");
    }

    void maxUndoDepth_limitsStack()
    {
        CommandBus bus;
        int counter = 0;
        // MaxUndoDepth = 100, добавляем 105 команд
        for (int i = 0; i < 105; ++i)
            bus.execute(std::make_unique<TestCommand>(counter, QString::number(i)));

        // Стек не должен превышать 100
        int undoCount = 0;
        while (bus.canUndo()) {
            bus.undo();
            ++undoCount;
        }
        QCOMPARE(undoCount, 100);
    }

    void clear_emptiesBothStacks()
    {
        CommandBus bus;
        int counter = 0;
        bus.execute(std::make_unique<TestCommand>(counter));
        bus.undo();
        QVERIFY(bus.canRedo());
        bus.execute(std::make_unique<TestCommand>(counter));
        QVERIFY(bus.canUndo());
        bus.clear();
        QVERIFY(!bus.canUndo());
        QVERIFY(!bus.canRedo());
    }

    // --- Сигналы ---

    void signal_commandExecuted()
    {
        CommandBus bus;
        QSignalSpy spy(&bus, &CommandBus::commandExecuted);
        int counter = 0;
        bus.execute(std::make_unique<TestCommand>(counter, "test_signal"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toString(), "test_signal");
    }

    void signal_undoPerformed()
    {
        CommandBus bus;
        int counter = 0;
        bus.execute(std::make_unique<TestCommand>(counter, "undo_me"));
        QSignalSpy spy(&bus, &CommandBus::undoPerformed);
        bus.undo();
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toString(), "undo_me");
    }

    void signal_redoPerformed()
    {
        CommandBus bus;
        int counter = 0;
        bus.execute(std::make_unique<TestCommand>(counter, "redo_me"));
        bus.undo();
        QSignalSpy spy(&bus, &CommandBus::redoPerformed);
        bus.redo();
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toString(), "redo_me");
    }

    void signal_stateChanged()
    {
        CommandBus bus;
        QSignalSpy spy(&bus, &CommandBus::stateChanged);
        int counter = 0;
        bus.execute(std::make_unique<TestCommand>(counter));
        bus.undo();
        bus.redo();
        // execute + undo + redo = 3 сигнала stateChanged
        QCOMPARE(spy.count(), 3);
    }

    // --- Макрокоманды ---

    void macro_basicGrouping()
    {
        CommandBus bus;
        int counter = 0;
        bus.beginMacro("Макро");
        bus.execute(std::make_unique<TestCommand>(counter, "a"));
        bus.execute(std::make_unique<TestCommand>(counter, "b"));
        bus.execute(std::make_unique<TestCommand>(counter, "c"));
        bus.endMacro();
        QCOMPARE(counter, 3);
        // Одна отмена должна откатить все 3 команды
        bus.undo();
        QCOMPARE(counter, 0);
    }

    void macro_emptyDoesNotAddToStack()
    {
        CommandBus bus;
        bus.beginMacro("Пустая макро");
        bus.endMacro();
        QVERIFY(!bus.canUndo());
    }

    void macro_nestedMacros()
    {
        CommandBus bus;
        int counter = 0;
        bus.beginMacro("Внешняя");
        bus.execute(std::make_unique<TestCommand>(counter, "a"));
        bus.beginMacro("Внутренняя"); // вложенная — игнорируется
        bus.execute(std::make_unique<TestCommand>(counter, "b"));
        bus.endMacro(); // завершение внутренней — ничего не происходит
        bus.execute(std::make_unique<TestCommand>(counter, "c"));
        bus.endMacro(); // завершение внешней
        QCOMPARE(counter, 3);
        bus.undo();
        QCOMPARE(counter, 0);
    }

    void isInMacro_returnsCorrectState()
    {
        CommandBus bus;
        QVERIFY(!bus.isInMacro());
        bus.beginMacro("test");
        QVERIFY(bus.isInMacro());
        bus.endMacro();
        QVERIFY(!bus.isInMacro());
    }

    // --- executeNoHistory ---

    void executeNoHistory_doesNotAffectStacks()
    {
        CommandBus bus;
        int counter = 0;
        bus.executeNoHistory(std::make_unique<TestCommand>(counter, "ghost"));
        QCOMPARE(counter, 1);
        QVERIFY(!bus.canUndo());
        QVERIFY(!bus.canRedo());
    }

    // --- Потокобезопасность ---

    void threadSafety_concurrentExecute()
    {
        CommandBus bus;
        // Атомарный счётчик для корректности в многопоточной среде
        std::atomic<int> atomicCounter{0};

        const int threadCount = 10;
        const int opsPerThread = 100;
        QList<QThread *> threads;

        for (int t = 0; t < threadCount; ++t) {
            auto *thread = QThread::create([&bus, &atomicCounter, opsPerThread]() {
                for (int i = 0; i < opsPerThread; ++i) {
                    int localVal = 0; // каждая команда с отдельным счётчиком
                    auto cmd = std::make_unique<LambdaCommand>(
                        "thread_cmd",
                        [&atomicCounter]() { ++atomicCounter; },
                        [&atomicCounter]() { --atomicCounter; }
                    );
                    bus.execute(std::move(cmd));
                }
            });
            threads.append(thread);
        }

        for (auto *t : threads)
            t->start();
        for (auto *t : threads)
            t->wait();
        for (auto *t : threads)
            delete t;

        // Все команды должны были выполниться
        QCOMPARE(atomicCounter.load(), threadCount * opsPerThread);
    }
};

QTEST_MAIN(TestCommandBus)
#include "test_CommandBus.moc"
