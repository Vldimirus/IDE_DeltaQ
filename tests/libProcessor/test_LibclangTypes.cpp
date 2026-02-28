// Тесты LibclangTypes
#include <QTest>
#include "../../src/libProcessor/LibclangTypes.h"

using namespace DeltaQ;

class TestLibclangTypes : public QObject {
    Q_OBJECT

private slots:
    void testFunctionDecl()
    {
        FunctionDecl f;
        f.name = "my_func";
        f.returnType = "int";
        f.parameters.append({"x", "int", ""});
        f.parameters.append({"y", "float", "1.0"});

        QCOMPARE(f.name, QString("my_func"));
        QCOMPARE(f.returnType, QString("int"));
        QCOMPARE(f.parameters.size(), 2);
        QCOMPARE(f.parameters[0].name, QString("x"));
        QCOMPARE(f.parameters[1].defaultValue, QString("1.0"));
    }

    void testStructDecl()
    {
        StructDecl s;
        s.name = "MyStruct";
        s.fields.append({"x", "int", ""});
        s.fields.append({"y", "float", ""});
        s.fields.append({"name", "const char *", ""});

        QCOMPARE(s.name, QString("MyStruct"));
        QCOMPARE(s.fields.size(), 3);
        QCOMPARE(s.fields[2].type, QString("const char *"));
    }

    void testEnumDecl()
    {
        EnumDecl e;
        e.name = "Color";
        e.constants.append({"RED", 0});
        e.constants.append({"GREEN", 1});
        e.constants.append({"BLUE", 2});

        QCOMPARE(e.name, QString("Color"));
        QCOMPARE(e.constants.size(), 3);
        QCOMPARE(e.constants[2].value, 2LL);
    }

    void testClassDecl()
    {
        ClassDecl c;
        c.name = "MyClass";
        c.baseClasses.append("BaseClass");

        ConstructorDecl ctor;
        ctor.accessSpecifier = "public";
        ctor.parameters.append({"value", "int", ""});
        c.constructors.append(ctor);

        MethodDecl m;
        m.name = "getValue";
        m.returnType = "int";
        m.isConst = true;
        m.accessSpecifier = "public";
        c.methods.append(m);

        MethodDecl m2;
        m2.name = "setValue";
        m2.returnType = "void";
        m2.parameters.append({"v", "int", ""});
        m2.accessSpecifier = "public";
        c.methods.append(m2);

        QCOMPARE(c.name, QString("MyClass"));
        QCOMPARE(c.baseClasses.size(), 1);
        QCOMPARE(c.constructors.size(), 1);
        QCOMPARE(c.methods.size(), 2);
        QVERIFY(c.methods[0].isConst);
    }

    void testTypedefDecl()
    {
        TypedefDecl td;
        td.name = "uint32";
        td.underlyingType = "unsigned int";

        QCOMPARE(td.name, QString("uint32"));
        QCOMPARE(td.underlyingType, QString("unsigned int"));
    }
};

QTEST_APPLESS_MAIN(TestLibclangTypes)
#include "test_LibclangTypes.moc"
