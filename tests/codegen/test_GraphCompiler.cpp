// Тесты GraphCompiler — компиляция графов в C-код
#include <QtTest>
#include <QApplication>
#include <deltaq/Graph.h>
#include <deltaq/Module.h>
#include "../../src/core/ModuleRegistry.h"
#include "../../src/codegen/GraphCompiler.h"

using namespace DeltaQ;

class TestGraphCompiler : public QObject {
    Q_OBJECT

private:
    ModuleRegistry *m_registry = nullptr;

    void registerTestModules()
    {
        Module add;
        add.id = "add"; add.name = "Add"; add.category = "math";
        add.language = "c"; add.version = "1.0"; add.origin = "user";
        add.inputs = {{"a", "int", "0"}, {"b", "int", "0"}};
        add.outputs = {{"result", "int", ""}};
        m_registry->registerModule(add);

        Module print;
        print.id = "print"; print.name = "Print"; print.category = "io";
        print.language = "c"; print.version = "1.0"; print.origin = "user";
        print.inputs = {{"value", "int", "0"}};
        m_registry->registerModule(print);

        Module mul;
        mul.id = "mul"; mul.name = "Multiply"; mul.category = "math";
        mul.language = "c"; mul.version = "1.0"; mul.origin = "user";
        mul.inputs = {{"a", "int", "0"}, {"b", "int", "0"}};
        mul.outputs = {{"result", "int", ""}};
        m_registry->registerModule(mul);

        Module tofloat;
        tofloat.id = "tofloat"; tofloat.name = "ToFloat"; tofloat.category = "math";
        tofloat.language = "c"; tofloat.version = "1.0"; tofloat.origin = "user";
        tofloat.inputs = {{"value", "int", "0"}};
        tofloat.outputs = {{"result", "float", ""}};
        m_registry->registerModule(tofloat);

        Module addf;
        addf.id = "addf"; addf.name = "AddFloat"; addf.category = "math";
        addf.language = "c"; addf.version = "1.0"; addf.origin = "user";
        addf.inputs = {{"a", "float", "0"}, {"b", "float", "0"}};
        addf.outputs = {{"result", "float", ""}};
        m_registry->registerModule(addf);
    }

private slots:
    void init()
    {
        m_registry = new ModuleRegistry(this);
        registerTestModules();
    }

    void cleanup()
    {
        delete m_registry;
        m_registry = nullptr;
    }

    void emptyGraph()
    {
        Graph g = Graph::create("Empty");
        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(result.success);
        QVERIFY(result.generatedCode.contains("int main(void)"));
        QVERIFY(result.errors.isEmpty());
    }

    void simpleGraph()
    {
        Graph g = Graph::create("Simple");
        GraphNode n;
        n.id = "n1"; n.moduleId = "add"; n.position = QPointF(0, 0);
        g.addNode(n);

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(result.success);
        QVERIFY(result.generatedCode.contains("dq_add"));
    }

    void linearChain()
    {
        Graph g = Graph::create("Chain");

        GraphNode n1; n1.id = "n1"; n1.moduleId = "add"; n1.position = QPointF(0, 0);
        GraphNode n2; n2.id = "n2"; n2.moduleId = "mul"; n2.position = QPointF(200, 0);
        GraphNode n3; n3.id = "n3"; n3.moduleId = "print"; n3.position = QPointF(400, 0);
        g.addNode(n1);
        g.addNode(n2);
        g.addNode(n3);

        g.addConnection({{"n1", "result"}, {"n2", "a"}});
        g.addConnection({{"n2", "result"}, {"n3", "value"}});

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(result.success);
        QVERIFY(result.generatedCode.contains("dq_add"));
        QVERIFY(result.generatedCode.contains("dq_multiply"));
        QVERIFY(result.generatedCode.contains("dq_print"));
    }

    void cycleDetection()
    {
        Graph g = Graph::create("Cycle");

        GraphNode n1; n1.id = "n1"; n1.moduleId = "add"; n1.position = QPointF(0, 0);
        GraphNode n2; n2.id = "n2"; n2.moduleId = "mul"; n2.position = QPointF(200, 0);
        g.addNode(n1);
        g.addNode(n2);

        // Создаём цикл
        g.addConnection({{"n1", "result"}, {"n2", "a"}});
        g.addConnection({{"n2", "result"}, {"n1", "a"}});

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(!result.success);
        QVERIFY(!result.errors.isEmpty());
        // Должно содержать сообщение о цикле
        bool hasCycleError = false;
        for (const auto &err : result.errors)
            if (err.contains("ycle", Qt::CaseInsensitive))
                hasCycleError = true;
        QVERIFY(hasCycleError);
    }

    void missingModule()
    {
        Graph g = Graph::create("Missing");
        GraphNode n;
        n.id = "n1"; n.moduleId = "nonexistent"; n.position = QPointF(0, 0);
        g.addNode(n);

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(!result.success);
        QVERIFY(!result.errors.isEmpty());
    }

    void typeConversion()
    {
        Graph g = Graph::create("TypeConv");

        GraphNode n1; n1.id = "n1"; n1.moduleId = "tofloat"; n1.position = QPointF(0, 0);
        GraphNode n2; n2.id = "n2"; n2.moduleId = "addf"; n2.position = QPointF(200, 0);
        g.addNode(n1);
        g.addNode(n2);

        // int output → float input (автоматический каст не нужен — tofloat уже float)
        g.addConnection({{"n1", "result"}, {"n2", "a"}});

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(result.success);
    }

    void incompatibleTypes()
    {
        Graph g = Graph::create("Incompatible");

        // Создаём модуль со string output
        Module strmod;
        strmod.id = "strmod"; strmod.name = "StrMod"; strmod.category = "string";
        strmod.language = "c"; strmod.version = "1.0"; strmod.origin = "user";
        strmod.outputs = {{"result", "string", ""}};
        m_registry->registerModule(strmod);

        GraphNode n1; n1.id = "n1"; n1.moduleId = "strmod"; n1.position = QPointF(0, 0);
        GraphNode n2; n2.id = "n2"; n2.moduleId = "add"; n2.position = QPointF(200, 0);
        g.addNode(n1);
        g.addNode(n2);

        // string → int — несовместимо
        g.addConnection({{"n1", "result"}, {"n2", "a"}});

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(!result.success);
    }

    void multiOutput()
    {
        Graph g = Graph::create("MultiOut");

        // Add имеет один output, подключаем его к двум узлам
        GraphNode n1; n1.id = "n1"; n1.moduleId = "add"; n1.position = QPointF(0, 0);
        GraphNode n2; n2.id = "n2"; n2.moduleId = "print"; n2.position = QPointF(200, 0);
        GraphNode n3; n3.id = "n3"; n3.moduleId = "mul"; n3.position = QPointF(200, 200);
        g.addNode(n1);
        g.addNode(n2);
        g.addNode(n3);

        g.addConnection({{"n1", "result"}, {"n2", "value"}});
        g.addConnection({{"n1", "result"}, {"n3", "a"}});

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(result.success);
    }

    void sourceMapGenerated()
    {
        Graph g = Graph::create("SourceMap");
        GraphNode n;
        n.id = "n1"; n.moduleId = "add"; n.position = QPointF(0, 0);
        g.addNode(n);

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(result.success);
        // sourceMap должен содержать маппинг для этого узла
        bool found = false;
        for (auto it = result.sourceMap.begin(); it != result.sourceMap.end(); ++it) {
            if (it.value() == "n1") {
                found = true;
                break;
            }
        }
        QVERIFY(found);
    }

    void execAndDataOutputs()
    {
        Module readExec;
        readExec.id = "read_exec";
        readExec.name = "read_exec";
        readExec.category = "io";
        readExec.language = "c";
        readExec.version = "1.0";
        readExec.origin = "user";
        readExec.inputs = {Port::exec("flow_in")};
        readExec.outputs = {Port::exec("flow_out"), {"text", "string", ""}};
        readExec.sourceCode = "const char *dq_read_exec(void) {\n"
                              "    return \"ok\";\n"
                              "}";
        m_registry->registerModule(readExec);

        Module printStr;
        printStr.id = "print_str";
        printStr.name = "print_str";
        printStr.category = "io";
        printStr.language = "c";
        printStr.version = "1.0";
        printStr.origin = "user";
        printStr.inputs = {Port::exec("flow_in"), {"text", "string", ""}};
        printStr.outputs = {Port::exec("flow_out")};
        printStr.sourceCode = "void dq_print_str(const char *text) {\n"
                              "    (void)text;\n"
                              "}";
        m_registry->registerModule(printStr);

        Graph g = Graph::create("ExecAndData");
        GraphNode n1; n1.id = "n1"; n1.moduleId = "read_exec"; n1.position = QPointF(0, 0);
        GraphNode n2; n2.id = "n2"; n2.moduleId = "print_str"; n2.position = QPointF(200, 0);
        g.addNode(n1);
        g.addNode(n2);
        g.addConnection({{"n1", "text"}, {"n2", "text"}});
        g.addConnection({{"n1", "flow_out"}, {"n2", "flow_in"}, PortKind::Execution});

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(result.success);
        QVERIFY(result.generatedCode.contains("var_n1_text = dq_read_exec("));
        QVERIFY(result.generatedCode.contains("dq_print_str(var_n1_text)"));
    }
};

QTEST_MAIN(TestGraphCompiler)
#include "test_GraphCompiler.moc"
