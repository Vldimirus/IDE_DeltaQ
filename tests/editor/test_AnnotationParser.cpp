// Тесты парсера аннотаций @dqmodule / @dqport
#include <QTest>
#include <QSignalSpy>

#include "../../src/editor/AnnotationParser.h"

using namespace DeltaQ;

class TestAnnotationParser : public QObject {
    Q_OBJECT

private slots:
    void testParseSimpleModule()
    {
        AnnotationParser parser;
        QString source =
            "// @dqmodule name=Add version=1.0 category=math description=\"Сложение\"\n"
            "// @dqport direction=input name=a type=int default=0\n"
            "// @dqport direction=input name=b type=int\n"
            "// @dqport direction=output name=result type=int\n"
            "int add(int a, int b) { return a + b; }\n";

        auto modules = parser.parseSource(source, "/tmp/add.c");
        QCOMPARE(modules.size(), 1);

        const auto &mod = modules.first();
        QCOMPARE(mod.name, "Add");
        QCOMPARE(mod.version, "1.0");
        QCOMPARE(mod.category, "math");
        QCOMPARE(mod.description, "Сложение");
        QCOMPARE(mod.language, "c");
        QCOMPARE(mod.inputs.size(), 2);
        QCOMPARE(mod.outputs.size(), 1);
        QCOMPARE(mod.inputs[0].name, "a");
        QCOMPARE(mod.inputs[0].type, "int");
        QCOMPARE(mod.inputs[0].defaultValue, "0");
        QCOMPARE(mod.inputs[1].name, "b");
        QCOMPARE(mod.outputs[0].name, "result");
    }

    void testParseMultipleModules()
    {
        AnnotationParser parser;
        QString source =
            "// @dqmodule name=Add category=math\n"
            "// @dqport direction=input name=a type=int\n"
            "// @dqport direction=output name=result type=int\n"
            "int add(int a) { return a + 1; }\n"
            "\n"
            "// @dqmodule name=Mul category=math\n"
            "// @dqport direction=input name=x type=float\n"
            "// @dqport direction=output name=result type=float\n"
            "float mul(float x) { return x * 2; }\n";

        auto modules = parser.parseSource(source, "/tmp/ops.c");
        QCOMPARE(modules.size(), 2);
        QCOMPARE(modules[0].name, "Add");
        QCOMPARE(modules[1].name, "Mul");
        QCOMPARE(modules[0].inputs.size(), 1);
        QCOMPARE(modules[1].inputs.size(), 1);
    }

    void testParseMultiLineComment()
    {
        AnnotationParser parser;
        QString source =
            "/*\n"
            " * @dqmodule name=Filter version=2.0 category=signal\n"
            " * @dqport direction=input name=signal type=float\n"
            " * @dqport direction=output name=filtered type=float\n"
            " */\n"
            "float filter(float s) { return s; }\n";

        auto modules = parser.parseSource(source, "/tmp/filter.c");
        QCOMPARE(modules.size(), 1);
        QCOMPARE(modules[0].name, "Filter");
        QCOMPARE(modules[0].version, "2.0");
        QCOMPARE(modules[0].category, "signal");
        QCOMPARE(modules[0].inputs.size(), 1);
        QCOMPARE(modules[0].outputs.size(), 1);
    }

    void testParseDescriptionWithSpaces()
    {
        AnnotationParser parser;
        QString source =
            "// @dqmodule name=Delay description=\"Задержка сигнала на N тактов\"\n";

        auto modules = parser.parseSource(source, "/tmp/delay.c");
        QCOMPARE(modules.size(), 1);
        QCOMPARE(modules[0].description, "Задержка сигнала на N тактов");
    }

    void testStableId()
    {
        // Один и тот же файл и имя модуля должны давать одинаковый ID
        QString id1 = AnnotationParser::generateModuleId("/tmp/add.c", "Add");
        QString id2 = AnnotationParser::generateModuleId("/tmp/add.c", "Add");
        QCOMPARE(id1, id2);

        // Другой файл — другой ID
        QString id3 = AnnotationParser::generateModuleId("/tmp/mul.c", "Add");
        QVERIFY(id1 != id3);

        // Другое имя — другой ID
        QString id4 = AnnotationParser::generateModuleId("/tmp/add.c", "Sub");
        QVERIFY(id1 != id4);
    }

    void testCppLanguageDetection()
    {
        AnnotationParser parser;
        QString source = "// @dqmodule name=Foo\n";

        auto mods1 = parser.parseSource(source, "/tmp/foo.cpp");
        QCOMPARE(mods1.size(), 1);
        QCOMPARE(mods1[0].language, "cpp");

        auto mods2 = parser.parseSource(source, "/tmp/foo.c");
        QCOMPARE(mods2.size(), 1);
        QCOMPARE(mods2[0].language, "c");
    }

    void testPortWithoutModule()
    {
        AnnotationParser parser;
        QString source =
            "// @dqport direction=input name=x type=int\n"
            "// @dqmodule name=Foo\n";

        auto modules = parser.parseSource(source);
        QCOMPARE(modules.size(), 1);
        // @dqport перед @dqmodule — должна быть ошибка
        QVERIFY(parser.hasErrors());
        QCOMPARE(parser.errors().size(), 1);
        QVERIFY(parser.errors()[0].message.contains("@dqport"));
    }

    void testEmptySource()
    {
        AnnotationParser parser;
        auto modules = parser.parseSource("int main() { return 0; }");
        QCOMPARE(modules.size(), 0);
        QVERIFY(!parser.hasErrors());
    }

    void testModuleWithoutName()
    {
        AnnotationParser parser;
        QString source = "// @dqmodule version=1.0\n";

        auto modules = parser.parseSource(source);
        QCOMPARE(modules.size(), 0);
        QVERIFY(parser.hasErrors());
    }

    void testPortWithoutNameOrType()
    {
        AnnotationParser parser;
        QString source =
            "// @dqmodule name=Bad\n"
            "// @dqport direction=input name=x\n"; // нет type

        auto modules = parser.parseSource(source);
        QVERIFY(parser.hasErrors());
        // Модуль создаётся, но порт пропущен
        QCOMPARE(modules.size(), 1);
        QCOMPARE(modules[0].inputs.size(), 0);
    }

    void testDqmodPath()
    {
        QString path = AnnotationParser::dqmodPathForSource("/home/user/project/src/add.c");
        QVERIFY(path.endsWith("add.dqmod"));
        QVERIFY(path.contains("/home/user/project/src/"));
    }

    void testSignalEmitted()
    {
        AnnotationParser parser;
        QSignalSpy spy(&parser, &AnnotationParser::moduleParsed);
        QString source =
            "// @dqmodule name=Sig\n"
            "// @dqport direction=input name=x type=int\n";

        parser.parseSource(source, "/tmp/sig.c");
        QCOMPARE(spy.count(), 1);
    }

    void testErrorSignalEmitted()
    {
        AnnotationParser parser;
        QSignalSpy spy(&parser, &AnnotationParser::parseError);
        QString source = "// @dqport direction=input name=x type=int\n";

        parser.parseSource(source);
        QCOMPARE(spy.count(), 1);
    }

    void testDefaultDirection()
    {
        // Если direction не указан — по умолчанию input
        AnnotationParser parser;
        QString source =
            "// @dqmodule name=DefDir\n"
            "// @dqport name=x type=int\n";

        auto modules = parser.parseSource(source, "/tmp/dd.c");
        QCOMPARE(modules.size(), 1);
        QCOMPARE(modules[0].inputs.size(), 1);
        QCOMPARE(modules[0].outputs.size(), 0);
    }
};

QTEST_MAIN(TestAnnotationParser)
#include "test_AnnotationParser.moc"
