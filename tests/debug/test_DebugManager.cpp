// Тесты менеджера отладки — парсинг GDB/MI и управление breakpoints
#include <QTest>
#include <QSignalSpy>

#include "../../src/debug/DebugManager.h"
#include "../../src/debug/DebugTypes.h"

using namespace DeltaQ;

class TestDebugManager : public QObject {
    Q_OBJECT

private slots:
    // Парсинг MI вывода

    void testParseResultDone()
    {
        auto record = DebugManager::parseMIOutput("5^done,value=\"42\"");
        QCOMPARE(record.type, DebugManager::MIRecord::Result);
        QCOMPARE(record.token, 5);
        QCOMPARE(record.resultClass, "done");
        QVERIFY(record.payload.contains("value"));
    }

    void testParseResultRunning()
    {
        auto record = DebugManager::parseMIOutput("3^running");
        QCOMPARE(record.type, DebugManager::MIRecord::Result);
        QCOMPARE(record.token, 3);
        QCOMPARE(record.resultClass, "running");
        QVERIFY(record.payload.isEmpty());
    }

    void testParseResultError()
    {
        auto record = DebugManager::parseMIOutput("7^error,msg=\"No symbol table\"");
        QCOMPARE(record.type, DebugManager::MIRecord::Result);
        QCOMPARE(record.token, 7);
        QCOMPARE(record.resultClass, "error");
        QVERIFY(record.payload.contains("msg"));
    }

    void testParseExecAsyncStopped()
    {
        QString line = "*stopped,reason=\"breakpoint-hit\","
                       "frame={addr=\"0x400500\",func=\"main\",file=\"main.c\",line=\"10\"}";
        auto record = DebugManager::parseMIOutput(line);
        QCOMPARE(record.type, DebugManager::MIRecord::ExecAsync);
        QCOMPARE(record.resultClass, "stopped");
        QVERIFY(record.payload.contains("breakpoint-hit"));
        QVERIFY(record.payload.contains("main.c"));
    }

    void testParseExecAsyncRunning()
    {
        auto record = DebugManager::parseMIOutput("*running,thread-id=\"all\"");
        QCOMPARE(record.type, DebugManager::MIRecord::ExecAsync);
        QCOMPARE(record.resultClass, "running");
    }

    void testParseConsoleStream()
    {
        auto record = DebugManager::parseMIOutput("~\"Hello world\\n\"");
        QCOMPARE(record.type, DebugManager::MIRecord::ConsoleStream);
        QVERIFY(record.payload.contains("Hello world"));
    }

    void testParseTargetStream()
    {
        auto record = DebugManager::parseMIOutput("@\"output from target\"");
        QCOMPARE(record.type, DebugManager::MIRecord::TargetStream);
    }

    void testParseLogStream()
    {
        auto record = DebugManager::parseMIOutput("&\"log message\"");
        QCOMPARE(record.type, DebugManager::MIRecord::LogStream);
    }

    void testParseNotifyAsync()
    {
        auto record = DebugManager::parseMIOutput("=thread-group-added,id=\"i1\"");
        QCOMPARE(record.type, DebugManager::MIRecord::NotifyAsync);
        QCOMPARE(record.resultClass, "thread-group-added");
    }

    void testParseNoToken()
    {
        auto record = DebugManager::parseMIOutput("^done");
        QCOMPARE(record.type, DebugManager::MIRecord::Result);
        QCOMPARE(record.token, -1);
        QCOMPARE(record.resultClass, "done");
    }

    // Парсинг MI tuple

    void testParseMITupleSimple()
    {
        auto fields = DebugManager::parseMITuple("name=\"x\",type=\"int\",value=\"5\"");
        QCOMPARE(fields.value("name"), "x");
        QCOMPARE(fields.value("type"), "int");
        QCOMPARE(fields.value("value"), "5");
    }

    void testParseMITupleNested()
    {
        auto fields = DebugManager::parseMITuple(
            "number=\"1\",file=\"main.c\",line=\"10\",frame={func=\"main\"}");
        QCOMPARE(fields.value("number"), "1");
        QCOMPARE(fields.value("file"), "main.c");
        QVERIFY(fields.value("frame").contains("func"));
    }

    void testParseMITupleEscaped()
    {
        auto fields = DebugManager::parseMITuple("msg=\"Undefined \\\"foo\\\"\"");
        // Значение должно содержать экранированные кавычки
        QVERIFY(fields.contains("msg"));
    }

    // Unquote MI строки

    void testUnquoteMI()
    {
        QCOMPARE(DebugManager::unquoteMI("\"hello\""), "hello");
        QCOMPARE(DebugManager::unquoteMI("\"line1\\nline2\""), "line1\nline2");
        QCOMPARE(DebugManager::unquoteMI("\"tab\\there\""), "tab\there");
        QCOMPARE(DebugManager::unquoteMI("plain"), "plain");
    }

    void testUnquoteMIEmpty()
    {
        QCOMPARE(DebugManager::unquoteMI(""), "");
        QCOMPARE(DebugManager::unquoteMI("\"\""), "");
    }

    // Парсинг MI списка

    void testParseMIList()
    {
        QString list = "{name=\"x\",value=\"1\"},{name=\"y\",value=\"2\"}";
        auto items = DebugManager::parseMIList(list);
        QCOMPARE(items.size(), 2);
        QCOMPARE(items[0].value("name"), "x");
        QCOMPARE(items[1].value("name"), "y");
    }

    // Управление breakpoints (без запуска GDB)

    void testAddBreakpoint()
    {
        DebugManager mgr;
        mgr.addBreakpoint("/tmp/main.c", 10);
        QCOMPARE(mgr.breakpoints().size(), 1);
        QCOMPARE(mgr.breakpoints()[0].file, "/tmp/main.c");
        QCOMPARE(mgr.breakpoints()[0].line, 10);
    }

    void testAddDuplicateBreakpoint()
    {
        DebugManager mgr;
        mgr.addBreakpoint("/tmp/main.c", 10);
        mgr.addBreakpoint("/tmp/main.c", 10); // дубликат
        QCOMPARE(mgr.breakpoints().size(), 1);
    }

    void testRemoveBreakpoint()
    {
        DebugManager mgr;
        mgr.addBreakpoint("/tmp/main.c", 10);
        mgr.addBreakpoint("/tmp/main.c", 20);
        mgr.removeBreakpoint("/tmp/main.c", 10);
        QCOMPARE(mgr.breakpoints().size(), 1);
        QCOMPARE(mgr.breakpoints()[0].line, 20);
    }

    void testToggleBreakpoint()
    {
        DebugManager mgr;
        mgr.toggleBreakpoint("/tmp/main.c", 10);
        QCOMPARE(mgr.breakpoints().size(), 1);
        mgr.toggleBreakpoint("/tmp/main.c", 10);
        QCOMPARE(mgr.breakpoints().size(), 0);
    }

    void testInitialState()
    {
        DebugManager mgr;
        QCOMPARE(mgr.state(), DebugState::Idle);
        QVERIFY(!mgr.isRunning());
        QVERIFY(mgr.breakpoints().isEmpty());
        QVERIFY(mgr.localVariables().isEmpty());
        QVERIFY(mgr.callStack().isEmpty());
    }

    void testBreakpointSignal()
    {
        DebugManager mgr;
        QSignalSpy spy(&mgr, &DebugManager::breakpointAdded);
        mgr.addBreakpoint("/tmp/test.c", 5);
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(TestDebugManager)
#include "test_DebugManager.moc"
