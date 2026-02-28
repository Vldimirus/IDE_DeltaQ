// Тесты LibraryDecomposer
#include <QTest>
#include "../../src/libProcessor/LibraryDecomposer.h"

using namespace DeltaQ;

class TestLibraryDecomposer : public QObject {
    Q_OBJECT

private slots:
    void testFunctionToModule()
    {
        ParseResult pr;
        FunctionDecl f;
        f.name = "add_numbers";
        f.returnType = "int";
        f.parameters.append({"a", "int", ""});
        f.parameters.append({"b", "int", ""});
        pr.functions.append(f);
        pr.success = true;

        auto result = LibraryDecomposer::decompose(pr);

        QCOMPARE(result.modules.size(), 1);
        QCOMPARE(result.modules[0].name, QString("add_numbers"));
        QCOMPARE(result.modules[0].inputs.size(), 2);
        QCOMPARE(result.modules[0].outputs.size(), 1);
        QCOMPARE(result.modules[0].outputs[0].name, QString("result"));
        QCOMPARE(result.modules[0].origin, QString("library"));
    }

    void testVoidFunction()
    {
        ParseResult pr;
        FunctionDecl f;
        f.name = "print_hello";
        f.returnType = "void";
        pr.functions.append(f);
        pr.success = true;

        auto result = LibraryDecomposer::decompose(pr);

        QCOMPARE(result.modules.size(), 1);
        QCOMPARE(result.modules[0].outputs.size(), 0);
    }

    void testClassToModules()
    {
        ParseResult pr;
        ClassDecl cls;
        cls.name = "Calculator";

        ConstructorDecl ctor;
        ctor.accessSpecifier = "public";
        cls.constructors.append(ctor);

        MethodDecl getValue;
        getValue.name = "getValue";
        getValue.returnType = "int";
        getValue.isConst = true;
        getValue.accessSpecifier = "public";
        cls.methods.append(getValue);

        MethodDecl add;
        add.name = "add";
        add.returnType = "void";
        add.parameters.append({"value", "int", ""});
        add.accessSpecifier = "public";
        cls.methods.append(add);

        pr.classes.append(cls);
        pr.success = true;

        DecompositionOptions opts;
        opts.language = "cpp";
        auto result = LibraryDecomposer::decompose(pr, opts);

        // create + destroy + getValue + add = 4 модуля
        QCOMPARE(result.modules.size(), 4);

        // Проверяем create
        QCOMPARE(result.modules[0].name, QString("Calculator_create"));
        QCOMPARE(result.modules[0].outputs.size(), 1);
        QCOMPARE(result.modules[0].outputs[0].type, QString("pointer"));

        // Проверяем destroy
        QCOMPARE(result.modules[1].name, QString("Calculator_destroy"));
        QCOMPARE(result.modules[1].inputs.size(), 1);
        QCOMPARE(result.modules[1].inputs[0].name, QString("instance"));

        // Проверяем getValue — instance вход + result + instance_out выходы
        QCOMPARE(result.modules[2].name, QString("Calculator_getValue"));
        QCOMPARE(result.modules[2].inputs.size(), 1); // instance
        QCOMPARE(result.modules[2].outputs.size(), 2); // result + instance_out
    }

    void testExcludePattern()
    {
        ParseResult pr;
        FunctionDecl f1;
        f1.name = "public_func";
        f1.returnType = "void";
        pr.functions.append(f1);

        FunctionDecl f2;
        f2.name = "_internal_func";
        f2.returnType = "void";
        pr.functions.append(f2);
        pr.success = true;

        DecompositionOptions opts;
        opts.excludePatterns = {"^_.*"}; // Исключить начинающиеся с _

        auto result = LibraryDecomposer::decompose(pr, opts);
        QCOMPARE(result.modules.size(), 1);
        QCOMPARE(result.modules[0].name, QString("public_func"));
    }

    void testCustomCategory()
    {
        ParseResult pr;
        FunctionDecl f;
        f.name = "custom_func";
        f.returnType = "void";
        pr.functions.append(f);
        pr.success = true;

        DecompositionOptions opts;
        opts.category = "mylib";

        auto result = LibraryDecomposer::decompose(pr, opts);
        QCOMPARE(result.modules[0].category, QString("mylib"));
    }

    void testEmptyResult()
    {
        ParseResult pr;
        pr.success = true;

        auto result = LibraryDecomposer::decompose(pr);
        QCOMPARE(result.modules.size(), 0);
    }
};

QTEST_APPLESS_MAIN(TestLibraryDecomposer)
#include "test_LibraryDecomposer.moc"
