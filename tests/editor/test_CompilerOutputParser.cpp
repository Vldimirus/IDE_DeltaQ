// Тесты парсера вывода компилятора
#include <QTest>
#include <QSignalSpy>

#include "../../src/editor/CompilerOutputParser.h"

using namespace DeltaQ;

class TestCompilerOutputParser : public QObject {
    Q_OBJECT

private slots:
    void testGccError()
    {
        CompilerOutputParser parser;
        bool found = parser.parseLine("/home/user/main.cpp:10:5: error: unknown type name 'foo'");
        QVERIFY(found);
        QCOMPARE(parser.errors().size(), 1);

        const auto &err = parser.errors().first();
        QCOMPARE(err.file, "/home/user/main.cpp");
        QCOMPARE(err.line, 10);
        QCOMPARE(err.column, 5);
        QCOMPARE(err.severity, CompilerSeverity::Error);
        QCOMPARE(err.message, "unknown type name 'foo'");
    }

    void testGccWarning()
    {
        CompilerOutputParser parser;
        parser.parseLine("src/test.c:25:12: warning: unused variable 'x' [-Wunused-variable]");
        QCOMPARE(parser.errors().size(), 1);

        const auto &err = parser.errors().first();
        QCOMPARE(err.file, "src/test.c");
        QCOMPARE(err.line, 25);
        QCOMPARE(err.column, 12);
        QVERIFY(err.isWarning());
        QVERIFY(err.message.contains("unused variable"));
    }

    void testGccNote()
    {
        CompilerOutputParser parser;
        parser.parseLine("main.cpp:5:1: note: candidate function not viable");
        QCOMPARE(parser.errors().size(), 1);
        QCOMPARE(parser.errors().first().severity, CompilerSeverity::Note);
    }

    void testClangError()
    {
        CompilerOutputParser parser;
        parser.parseLine("/tmp/project/src/app.cpp:42:10: error: use of undeclared identifier 'bar'");
        QCOMPARE(parser.errors().size(), 1);
        QCOMPARE(parser.errors().first().file, "/tmp/project/src/app.cpp");
        QCOMPARE(parser.errors().first().line, 42);
        QVERIFY(parser.errors().first().isError());
    }

    void testMultipleLines()
    {
        CompilerOutputParser parser;
        parser.parseLine("a.cpp:1:1: error: first");
        parser.parseLine("This is just info text");
        parser.parseLine("b.cpp:2:3: warning: second");
        parser.parseLine("c.cpp:5:1: note: third");

        QCOMPARE(parser.errors().size(), 3);
        QCOMPARE(parser.errorCount(), 1);
        QCOMPARE(parser.warningCount(), 1);
    }

    void testParseAll()
    {
        CompilerOutputParser parser;
        QString output =
            "a.cpp:1:1: error: first error\n"
            "some other output\n"
            "b.cpp:10:2: warning: some warning\n"
            "c.cpp:20:5: error: second error\n";

        auto errors = parser.parseAll(output);
        QCOMPARE(errors.size(), 3);
        QCOMPARE(parser.errorCount(), 2);
        QCOMPARE(parser.warningCount(), 1);
    }

    void testNonMatchingLine()
    {
        CompilerOutputParser parser;
        bool found = parser.parseLine("Linking CXX executable test_app");
        QVERIFY(!found);
        QCOMPARE(parser.errors().size(), 0);
    }

    void testCmakeError()
    {
        CompilerOutputParser parser;
        parser.parseLine("CMake Error at CMakeLists.txt:10: message goes here");
        QCOMPARE(parser.errors().size(), 1);
        QCOMPARE(parser.errors().first().file, "CMakeLists.txt");
        QCOMPARE(parser.errors().first().line, 10);
        QVERIFY(parser.errors().first().isError());
    }

    void testCmakeWarning()
    {
        CompilerOutputParser parser;
        parser.parseLine("CMake Warning at CMakeLists.txt:5: deprecated command");
        QCOMPARE(parser.errors().size(), 1);
        QVERIFY(parser.errors().first().isWarning());
    }

    void testClear()
    {
        CompilerOutputParser parser;
        parser.parseLine("a.cpp:1:1: error: test");
        QCOMPARE(parser.errors().size(), 1);
        parser.clear();
        QCOMPARE(parser.errors().size(), 0);
        QCOMPARE(parser.errorCount(), 0);
        QCOMPARE(parser.warningCount(), 0);
    }

    void testSignalEmitted()
    {
        CompilerOutputParser parser;
        QSignalSpy spy(&parser, &CompilerOutputParser::errorFound);
        parser.parseLine("test.cpp:1:1: error: boom");
        QCOMPARE(spy.count(), 1);
    }

    void testWindowsPathStyle()
    {
        // Windows-стиль пути с диском (C:/...)
        CompilerOutputParser parser;
        parser.parseLine("C:/Users/dev/main.cpp:15:8: error: missing semicolon");
        QCOMPARE(parser.errors().size(), 1);
        QCOMPARE(parser.errors().first().line, 15);
    }
};

QTEST_MAIN(TestCompilerOutputParser)
#include "test_CompilerOutputParser.moc"
