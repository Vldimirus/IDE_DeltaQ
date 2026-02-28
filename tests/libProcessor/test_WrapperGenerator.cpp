// Тесты WrapperGenerator
#include <QTest>
#include "../../src/libProcessor/WrapperGenerator.h"

using namespace DeltaQ;

class TestWrapperGenerator : public QObject {
    Q_OBJECT

private:
    ClassDecl createTestClass()
    {
        ClassDecl cls;
        cls.name = "Calculator";

        ConstructorDecl ctor;
        ctor.accessSpecifier = "public";
        ctor.parameters.append({"initial", "int", ""});
        cls.constructors.append(ctor);

        MethodDecl add;
        add.name = "add";
        add.returnType = "void";
        add.parameters.append({"value", "int", ""});
        add.accessSpecifier = "public";
        add.isConst = false;
        add.isStatic = false;
        cls.methods.append(add);

        MethodDecl getValue;
        getValue.name = "getValue";
        getValue.returnType = "int";
        getValue.isConst = true;
        getValue.isStatic = false;
        getValue.accessSpecifier = "public";
        cls.methods.append(getValue);

        return cls;
    }

private slots:
    void testHeaderContainsExternC()
    {
        ClassDecl cls = createTestClass();
        WrapperCode code = WrapperGenerator::generateCppClassWrapper(cls, "Calculator.h");

        QVERIFY(code.header.contains("extern \"C\""));
        QVERIFY(code.header.contains("#ifdef __cplusplus"));
        QVERIFY(code.header.contains("#endif"));
    }

    void testHeaderContainsHandleTypedef()
    {
        ClassDecl cls = createTestClass();
        WrapperCode code = WrapperGenerator::generateCppClassWrapper(cls, "Calculator.h");

        QVERIFY(code.header.contains("typedef void* DQ_Calculator"));
        QVERIFY(code.header.contains("dq_calculator_create"));
        QVERIFY(code.header.contains("dq_calculator_destroy"));
    }

    void testHeaderContainsMethodPrototypes()
    {
        ClassDecl cls = createTestClass();
        WrapperCode code = WrapperGenerator::generateCppClassWrapper(cls, "Calculator.h");

        QVERIFY(code.header.contains("dq_calculator_add"));
        QVERIFY(code.header.contains("dq_calculator_getValue")); // toLower только для prefix
    }

    void testSourceContainsStaticCast()
    {
        ClassDecl cls = createTestClass();
        WrapperCode code = WrapperGenerator::generateCppClassWrapper(cls, "Calculator.h");

        QVERIFY(code.source.contains("static_cast"));
        QVERIFY(code.source.contains("new Calculator"));
        QVERIFY(code.source.contains("delete static_cast<Calculator *>(handle)"));
    }

    void testSourceContainsOriginalInclude()
    {
        ClassDecl cls = createTestClass();
        WrapperCode code = WrapperGenerator::generateCppClassWrapper(cls, "Calculator.h");

        QVERIFY(code.source.contains("#include \"Calculator.h\""));
        QVERIFY(code.source.contains("extern \"C\""));
        QCOMPARE(code.className, QString("Calculator"));
    }

    void testGenerateCWrapperMultipleClasses()
    {
        ClassDecl cls1;
        cls1.name = "Foo";
        ConstructorDecl ctor1;
        ctor1.accessSpecifier = "public";
        cls1.constructors.append(ctor1);

        ClassDecl cls2;
        cls2.name = "Bar";
        ConstructorDecl ctor2;
        ctor2.accessSpecifier = "public";
        cls2.constructors.append(ctor2);

        QVector<ClassDecl> classes = {cls1, cls2};
        auto result = WrapperGenerator::generateCWrapper(classes, "mylib.h");

        QCOMPARE(result.size(), 2);
        QCOMPARE(result[0].className, QString("Foo"));
        QCOMPARE(result[1].className, QString("Bar"));
        QVERIFY(result[0].header.contains("DQ_Foo"));
        QVERIFY(result[1].header.contains("DQ_Bar"));
    }

    void testPrivateMethodsExcluded()
    {
        ClassDecl cls;
        cls.name = "MyClass";
        ConstructorDecl ctor;
        ctor.accessSpecifier = "public";
        cls.constructors.append(ctor);

        MethodDecl pubMethod;
        pubMethod.name = "publicMethod";
        pubMethod.returnType = "void";
        pubMethod.accessSpecifier = "public";
        pubMethod.isStatic = false;
        cls.methods.append(pubMethod);

        MethodDecl privMethod;
        privMethod.name = "privateHelper";
        privMethod.returnType = "void";
        privMethod.accessSpecifier = "private";
        privMethod.isStatic = false;
        cls.methods.append(privMethod);

        WrapperCode code = WrapperGenerator::generateCppClassWrapper(cls, "MyClass.h");

        QVERIFY(code.header.contains("dq_myclass_publicMethod"));
        QVERIFY(!code.header.contains("privateHelper"));
    }
};

QTEST_APPLESS_MAIN(TestWrapperGenerator)
#include "test_WrapperGenerator.moc"
