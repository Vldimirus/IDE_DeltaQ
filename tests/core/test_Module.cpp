// Тесты Module/Port — сериализация, operator==, helper-методы, валидация
#include <QtTest>
#include <deltaq/Module.h>

using namespace DeltaQ;

class TestModule : public QObject {
    Q_OBJECT

private slots:
    // --- Port ---

    void portToJson()
    {
        Port p{"input1", "int", "42"};
        auto json = p.toJson();
        QCOMPARE(json["name"].toString(), "input1");
        QCOMPARE(json["type"].toString(), "int");
        QCOMPARE(json["default"].toString(), "42");
    }

    void portFromJson()
    {
        QJsonObject obj;
        obj["name"] = "out";
        obj["type"] = "float";
        obj["default"] = "3.14";
        auto p = Port::fromJson(obj);
        QCOMPARE(p.name, "out");
        QCOMPARE(p.type, "float");
        QCOMPARE(p.defaultValue, "3.14");
    }

    void portRoundtrip()
    {
        Port original{"data", "string", "hello"};
        auto restored = Port::fromJson(original.toJson());
        QCOMPARE(restored, original);
    }

    void portEquality()
    {
        Port a{"x", "int", "0"};
        Port b{"x", "int", "0"};
        Port c{"x", "float", "0"};
        QVERIFY(a == b);
        QVERIFY(!(a == c));
    }

    // --- Module ---

    void moduleCreate()
    {
        auto m = Module::create("MyModule", "cpp");
        QVERIFY(!m.id.isEmpty());
        QCOMPARE(m.name, "MyModule");
        QCOMPARE(m.language, "cpp");
        QCOMPARE(m.version, "1.0.0");
        QCOMPARE(m.origin, "user");
    }

    void moduleToJsonFromJson()
    {
        Module m;
        m.id = "mod-1";
        m.name = "Фильтр";
        m.version = "2.0";
        m.language = "c";
        m.description = "Фильтрация данных";
        m.origin = "user";
        m.inputs = {{"in", "int", "0"}};
        m.outputs = {{"out", "int", ""}};
        m.sourcePath = "src/filter.c";
        m.headerPath = "include/filter.h";
        m.dependencies = {"math_utils"};

        auto json = m.toJson();
        auto restored = Module::fromJson(json);
        QCOMPARE(restored.id, m.id);
        QCOMPARE(restored.name, m.name);
        QCOMPARE(restored.version, m.version);
        QCOMPARE(restored.language, m.language);
        QCOMPARE(restored.description, m.description);
        QCOMPARE(restored.origin, m.origin);
        QCOMPARE(restored.inputs.size(), 1);
        QCOMPARE(restored.outputs.size(), 1);
        QCOMPARE(restored.inputs[0], m.inputs[0]);
        QCOMPARE(restored.outputs[0], m.outputs[0]);
        QCOMPARE(restored.sourcePath, m.sourcePath);
        QCOMPARE(restored.headerPath, m.headerPath);
        QCOMPARE(restored.dependencies, m.dependencies);
    }

    void moduleRoundtrip()
    {
        auto m = Module::create("RoundTrip");
        m.inputs = {{"a", "int", ""}, {"b", "float", "1.0"}};
        m.outputs = {{"result", "double", ""}};

        auto json = m.toJson();
        auto restored = Module::fromJson(json);
        // operator== сравнивает по id
        QCOMPARE(restored, m);
        // Дополнительно — порты совпали
        QCOMPARE(restored.inputs, m.inputs);
        QCOMPARE(restored.outputs, m.outputs);
    }

    void moduleEquality()
    {
        Module a, b;
        a.id = "same-id";
        a.name = "A";
        b.id = "same-id";
        b.name = "B";
        QVERIFY(a == b); // сравнение по id

        b.id = "other-id";
        QVERIFY(!(a == b));
    }

    void moduleFindPort()
    {
        auto m = Module::create("Test");
        m.inputs = {{"x", "int", ""}, {"y", "float", ""}};
        m.outputs = {{"result", "double", ""}};

        auto *p = m.findInput("x");
        QVERIFY(p != nullptr);
        QCOMPARE(p->type, "int");

        QVERIFY(m.findInput("nonexistent") == nullptr);

        auto *o = m.findOutput("result");
        QVERIFY(o != nullptr);
        QCOMPARE(o->type, "double");

        QVERIFY(m.findOutput("x") == nullptr);
    }

    void moduleHasPort()
    {
        auto m = Module::create("Test");
        m.inputs = {{"in1", "int", ""}};
        m.outputs = {{"out1", "float", ""}};

        QVERIFY(m.hasInput("in1"));
        QVERIFY(!m.hasInput("out1"));
        QVERIFY(m.hasOutput("out1"));
        QVERIFY(!m.hasOutput("in1"));
    }

    void moduleIsValid()
    {
        auto m = Module::create("Valid");
        m.inputs = {{"a", "int", ""}};
        QVERIFY(m.isValid());
    }

    void moduleIsValidEmpty()
    {
        Module m;
        QVERIFY(!m.isValid()); // id и name пусты

        m.id = "id1";
        QVERIFY(!m.isValid()); // name пуст

        m.name = "ok";
        QVERIFY(m.isValid());

        // Порт без типа — невалидно
        m.inputs = {{"port", "", ""}};
        QVERIFY(!m.isValid());

        // Порт без имени — невалидно
        m.inputs = {{"", "int", ""}};
        QVERIFY(!m.isValid());

        // Корректный порт
        m.inputs = {{"port", "int", ""}};
        QVERIFY(m.isValid());
    }
};

QTEST_MAIN(TestModule)
#include "test_Module.moc"
