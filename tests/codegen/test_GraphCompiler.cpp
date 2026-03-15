// Тесты GraphCompiler — компиляция графов в C-код
#include <QtTest>
#include <QApplication>
#include <deltaq/Graph.h>
#include <deltaq/Module.h>
#include "../../src/core/GraphStore.h"
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
        add.compileStatus = "passed";
        add.testStatus = "passed";
        add.inputs = {{"a", "int", "0"}, {"b", "int", "0"}};
        add.outputs = {{"result", "int", ""}};
        add.sourceCode = "int dq_add(int a, int b) {\n"
                         "    return a + b;\n"
                         "}";
        m_registry->registerModule(add);

        Module print;
        print.id = "print"; print.name = "Print"; print.category = "io";
        print.language = "c"; print.version = "1.0"; print.origin = "user";
        print.compileStatus = "passed";
        print.testStatus = "passed";
        print.inputs = {{"value", "int", "0"}};
        print.includes = {"stdio.h"};
        print.sourceCode = "void dq_print(int value) {\n"
                           "    printf(\"%d\", value);\n"
                           "}";
        m_registry->registerModule(print);

        Module mul;
        mul.id = "mul"; mul.name = "Multiply"; mul.category = "math";
        mul.language = "c"; mul.version = "1.0"; mul.origin = "user";
        mul.compileStatus = "passed";
        mul.testStatus = "passed";
        mul.inputs = {{"a", "int", "0"}, {"b", "int", "0"}};
        mul.outputs = {{"result", "int", ""}};
        mul.sourceCode = "int dq_multiply(int a, int b) {\n"
                         "    return a * b;\n"
                         "}";
        m_registry->registerModule(mul);

        Module tofloat;
        tofloat.id = "tofloat"; tofloat.name = "ToFloat"; tofloat.category = "math";
        tofloat.language = "c"; tofloat.version = "1.0"; tofloat.origin = "user";
        tofloat.compileStatus = "passed";
        tofloat.testStatus = "passed";
        tofloat.inputs = {{"value", "int", "0"}};
        tofloat.outputs = {{"result", "float", ""}};
        tofloat.sourceCode = "float dq_tofloat(int value) {\n"
                             "    return (float)value;\n"
                             "}";
        m_registry->registerModule(tofloat);

        Module addf;
        addf.id = "addf"; addf.name = "AddFloat"; addf.category = "math";
        addf.language = "c"; addf.version = "1.0"; addf.origin = "user";
        addf.compileStatus = "passed";
        addf.testStatus = "passed";
        addf.inputs = {{"a", "float", "0"}, {"b", "float", "0"}};
        addf.outputs = {{"result", "float", ""}};
        addf.sourceCode = "float dq_addfloat(float a, float b) {\n"
                          "    return a + b;\n"
                          "}";
        m_registry->registerModule(addf);

        Module stringConst;
        stringConst.id = "core.io.string_constant";
        stringConst.name = "string_constant";
        stringConst.category = "io";
        stringConst.language = "c";
        stringConst.version = "1.0";
        stringConst.origin = "core";
        stringConst.inputs = {{"value", "string", "\"Hello\""}};
        stringConst.outputs = {{"out", "string", ""}};
        stringConst.sourceCode = "const char *dq_string_constant(const char *value) {\n"
                                 "    return value;\n"
                                 "}";
        m_registry->registerModule(stringConst);

        Module intConst;
        intConst.id = "core.io.int_constant";
        intConst.name = "int_constant";
        intConst.category = "io";
        intConst.language = "c";
        intConst.version = "1.0";
        intConst.origin = "core";
        intConst.inputs = {{"value", "int", "0"}};
        intConst.outputs = {{"out", "int", ""}};
        intConst.sourceCode = "int dq_int_constant(int value) {\n"
                              "    return value;\n"
                              "}";
        m_registry->registerModule(intConst);
    }

    void registerDesktopModule(const QString &id, const QVector<Port> &inputs,
                               const QVector<Port> &outputs, const QStringList &includes = {})
    {
        Module mod;
        mod.id = id;
        mod.name = id.section('.', -1);
        mod.category = "desktop";
        mod.language = "c";
        mod.version = "1.0";
        mod.origin = "core";
        mod.inputs = inputs;
        mod.outputs = outputs;
        mod.includes = includes;
        m_registry->registerModule(mod);
    }

    void registerDesktopRuntimeModules()
    {
        registerDesktopModule("core.desktop.sdl_init",
                              {},
                              {Port::exec("flow_out")},
                              {"SDL2/SDL.h"});
        registerDesktopModule("core.desktop.ttf_init",
                              {Port::exec("flow_in")},
                              {Port::exec("flow_out")},
                              {"SDL2/SDL.h", "SDL2/SDL_ttf.h"});
        registerDesktopModule("core.desktop.create_window",
                              {Port::exec("flow_in"),
                               {"title", "string", "\"window1\""},
                               {"width", "int", "640"},
                               {"height", "int", "480"}},
                              {Port::exec("flow_out"), {"window", "sdl_window", ""}},
                              {"SDL2/SDL.h", "SDL2/SDL_ttf.h"});
        registerDesktopModule("core.desktop.create_renderer",
                              {Port::exec("flow_in"), {"window", "sdl_window", ""}},
                              {Port::exec("flow_out"), {"renderer", "sdl_renderer", ""}},
                              {"SDL2/SDL.h", "SDL2/SDL_ttf.h"});
        registerDesktopModule("core.desktop.ui_init",
                              {Port::exec("flow_in")},
                              {Port::exec("flow_out"), {"ui_state", "ui_state", ""}},
                              {"\"ui/window1.h\""});
        registerDesktopModule("core.desktop.event_loop",
                              {Port::exec("flow_in"),
                               {"renderer", "sdl_renderer", ""},
                               {"ui_state", "ui_state", ""}},
                              {Port::exec("flow_out")},
                              {"\"ui/window1_events.h\""});
        registerDesktopModule("core.desktop.ui_cleanup_font",
                              {Port::exec("flow_in"), {"ui_state", "ui_state", ""}},
                              {Port::exec("flow_out")},
                              {"SDL2/SDL_ttf.h", "\"ui/window1.h\""});
        registerDesktopModule("core.desktop.destroy_renderer",
                              {Port::exec("flow_in"), {"renderer", "sdl_renderer", ""}},
                              {Port::exec("flow_out")},
                              {"SDL2/SDL.h"});
        registerDesktopModule("core.desktop.destroy_window",
                              {Port::exec("flow_in"), {"window", "sdl_window", ""}},
                              {Port::exec("flow_out")},
                              {"SDL2/SDL.h"});
        registerDesktopModule("core.desktop.ttf_quit",
                              {Port::exec("flow_in")},
                              {Port::exec("flow_out")},
                              {"SDL2/SDL_ttf.h"});
        registerDesktopModule("core.desktop.sdl_quit",
                              {Port::exec("flow_in")},
                              {},
                              {"SDL2/SDL.h"});
    }

    Graph createDesktopRuntimeGraph(const QString &titleValue = "\"window1\"",
                                    const QString &widthValue = "640",
                                    const QString &heightValue = "480")
    {
        Graph g = Graph::create("main");

        GraphNode title; title.id = "n_title"; title.moduleId = "core.io.string_constant";
        title.properties["value"] = titleValue;
        GraphNode width; width.id = "n_width"; width.moduleId = "core.io.int_constant";
        width.properties["value"] = widthValue;
        GraphNode height; height.id = "n_height"; height.moduleId = "core.io.int_constant";
        height.properties["value"] = heightValue;
        GraphNode sdl; sdl.id = "n_sdl"; sdl.moduleId = "core.desktop.sdl_init";
        GraphNode ttf; ttf.id = "n_ttf"; ttf.moduleId = "core.desktop.ttf_init";
        GraphNode window; window.id = "n_window"; window.moduleId = "core.desktop.create_window";
        GraphNode renderer; renderer.id = "n_renderer"; renderer.moduleId = "core.desktop.create_renderer";
        GraphNode uiInit; uiInit.id = "n_ui"; uiInit.moduleId = "core.desktop.ui_init";
        GraphNode loop; loop.id = "n_loop"; loop.moduleId = "core.desktop.event_loop";
        GraphNode cleanupFont; cleanupFont.id = "n_font"; cleanupFont.moduleId = "core.desktop.ui_cleanup_font";
        GraphNode destroyRenderer; destroyRenderer.id = "n_dr"; destroyRenderer.moduleId = "core.desktop.destroy_renderer";
        GraphNode destroyWindow; destroyWindow.id = "n_dw"; destroyWindow.moduleId = "core.desktop.destroy_window";
        GraphNode ttfQuit; ttfQuit.id = "n_ttf_quit"; ttfQuit.moduleId = "core.desktop.ttf_quit";
        GraphNode sdlQuit; sdlQuit.id = "n_sdl_quit"; sdlQuit.moduleId = "core.desktop.sdl_quit";

        g.addNode(title);
        g.addNode(width);
        g.addNode(height);
        g.addNode(sdl);
        g.addNode(ttf);
        g.addNode(window);
        g.addNode(renderer);
        g.addNode(uiInit);
        g.addNode(loop);
        g.addNode(cleanupFont);
        g.addNode(destroyRenderer);
        g.addNode(destroyWindow);
        g.addNode(ttfQuit);
        g.addNode(sdlQuit);

        g.addConnection({{"n_title", "out"}, {"n_window", "title"}});
        g.addConnection({{"n_width", "out"}, {"n_window", "width"}});
        g.addConnection({{"n_height", "out"}, {"n_window", "height"}});
        g.addConnection({{"n_sdl", "flow_out"}, {"n_ttf", "flow_in"}, PortKind::Execution});
        g.addConnection({{"n_ttf", "flow_out"}, {"n_window", "flow_in"}, PortKind::Execution});
        g.addConnection({{"n_window", "window"}, {"n_renderer", "window"}});
        g.addConnection({{"n_window", "window"}, {"n_dw", "window"}});
        g.addConnection({{"n_window", "flow_out"}, {"n_renderer", "flow_in"}, PortKind::Execution});
        g.addConnection({{"n_renderer", "renderer"}, {"n_loop", "renderer"}});
        g.addConnection({{"n_renderer", "renderer"}, {"n_dr", "renderer"}});
        g.addConnection({{"n_renderer", "flow_out"}, {"n_ui", "flow_in"}, PortKind::Execution});
        g.addConnection({{"n_ui", "ui_state"}, {"n_loop", "ui_state"}});
        g.addConnection({{"n_ui", "ui_state"}, {"n_font", "ui_state"}});
        g.addConnection({{"n_ui", "flow_out"}, {"n_loop", "flow_in"}, PortKind::Execution});
        g.addConnection({{"n_loop", "flow_out"}, {"n_font", "flow_in"}, PortKind::Execution});
        g.addConnection({{"n_font", "flow_out"}, {"n_dr", "flow_in"}, PortKind::Execution});
        g.addConnection({{"n_dr", "flow_out"}, {"n_dw", "flow_in"}, PortKind::Execution});
        g.addConnection({{"n_dw", "flow_out"}, {"n_ttf_quit", "flow_in"}, PortKind::Execution});
        g.addConnection({{"n_ttf_quit", "flow_out"}, {"n_sdl_quit", "flow_in"}, PortKind::Execution});

        return g;
    }

    QString expectedInlineDesktopInitCall(const QString &title, int width, int height,
                                          int minWidth, int minHeight, bool resizable) const
    {
        return QString("dq_ui_backend_init(&dq_ui_backend_ctx, \"%1\", %2, %3, %4, %5, %6)")
            .arg(title)
            .arg(width)
            .arg(height)
            .arg(minWidth)
            .arg(minHeight)
            .arg(resizable ? "true" : "false");
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
        QVERIFY(result.generatedCode.contains("Generated by DeltaQ IDE."));
        QVERIFY(result.generatedCode.contains("Source: graph 'Empty'"));
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
        strmod.compileStatus = "passed";
        strmod.testStatus = "passed";
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

    void untestedAtomicModuleIsRejectedForComposition()
    {
        Module draft;
        draft.id = "draft";
        draft.name = "Draft";
        draft.category = "custom";
        draft.language = "c";
        draft.version = "1.0";
        draft.origin = "local";
        draft.compileStatus = "passed";
        draft.testStatus = "untested";
        draft.inputs = {{"value", "int", "0"}};
        draft.outputs = {{"result", "int", ""}};
        draft.sourceCode = "int dq_draft(int value) {\n"
                           "    return value;\n"
                           "}";
        m_registry->registerModule(draft);

        Graph g = Graph::create("DraftUse");
        GraphNode n;
        n.id = "n1";
        n.moduleId = "draft";
        g.addNode(n);

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(!result.success);
        QVERIFY(result.errors.join('\n').contains("not ready for composition"));
        QVERIFY(result.errors.join('\n').contains("test check passes"));
    }

    void compileUncheckedAtomicModuleIsRejectedForComposition()
    {
        Module draft;
        draft.id = "compile_draft";
        draft.name = "CompileDraft";
        draft.category = "custom";
        draft.language = "c";
        draft.version = "1.0";
        draft.origin = "local";
        draft.compileStatus = "unknown";
        draft.testStatus = "passed";
        draft.inputs = {{"value", "int", "0"}};
        draft.outputs = {{"result", "int", ""}};
        draft.sourceCode = "int dq_compile_draft(int value) {\n"
                           "    return value;\n"
                           "}";
        m_registry->registerModule(draft);

        Graph g = Graph::create("CompileDraftUse");
        GraphNode n;
        n.id = "n1";
        n.moduleId = "compile_draft";
        g.addNode(n);

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(!result.success);
        QVERIFY(result.errors.join('\n').contains("not ready for composition"));
        QVERIFY(result.errors.join('\n').contains("compile check passes"));
    }

    void brokenCompositeModuleIsRejectedForComposition()
    {
        GraphStore graphStore;
        m_registry->setGraphStore(&graphStore);

        Module composite;
        composite.id = "broken_composite";
        composite.name = "BrokenComposite";
        composite.category = "composite";
        composite.language = "c";
        composite.version = "1.0";
        composite.origin = "graph";
        composite.graphId = "missing_graph";
        composite.inputs = {{"value", "int", "0"}};
        composite.outputs = {{"result", "int", ""}};
        QVERIFY(m_registry->registerModule(composite));

        Graph g = Graph::create("BrokenCompositeUse");
        GraphNode n;
        n.id = "n1";
        n.moduleId = composite.id;
        g.addNode(n);

        GraphCompiler compiler(m_registry);
        compiler.setGraphStore(&graphStore);
        auto result = compiler.compile(g);

        QVERIFY(!result.success);
        QVERIFY(result.errors.join('\n').contains("not ready for composition"));
        QVERIFY(result.errors.join('\n').contains("inner graph"));
    }

    void compositeModuleWithoutBoundaryMappingIsRejected()
    {
        GraphStore graphStore;
        m_registry->setGraphStore(&graphStore);

        Module identity;
        identity.id = "identity";
        identity.name = "Identity";
        identity.category = "custom";
        identity.language = "c";
        identity.version = "1.0";
        identity.origin = "local";
        identity.compileStatus = "passed";
        identity.testStatus = "passed";
        identity.inputs = {{"value", "int", "0"}};
        identity.outputs = {{"result", "int", ""}};
        identity.sourceCode = "int dq_identity(int value) {\n"
                              "    return value;\n"
                              "}";
        QVERIFY(m_registry->registerModule(identity));

        Graph inner = Graph::create("InnerComposite");
        inner.parentModuleId = "composite_without_boundary";
        GraphNode innerNode;
        innerNode.id = "inner_node";
        innerNode.moduleId = identity.id;
        QVERIFY(inner.addNode(innerNode));
        QVERIFY(graphStore.registerGraph(inner));

        Module composite;
        composite.id = "composite_without_boundary";
        composite.name = "CompositeWithoutBoundary";
        composite.category = "composite";
        composite.language = "c";
        composite.version = "1.0";
        composite.origin = "graph";
        composite.graphId = inner.id;
        composite.inputs = {{"value", "int", "0"}};
        composite.outputs = {{"result", "int", ""}};
        QVERIFY(m_registry->registerModule(composite));

        Graph g = Graph::create("BrokenBoundaryUse");
        GraphNode n;
        n.id = "n1";
        n.moduleId = composite.id;
        g.addNode(n);

        GraphCompiler compiler(m_registry);
        compiler.setGraphStore(&graphStore);
        auto result = compiler.compile(g);

        QVERIFY(!result.success);
        QVERIFY(result.errors.join('\n').contains("not ready for composition"));
        QVERIFY(result.errors.join('\n').contains("boundary mapping"));
    }

    void compositeModuleCompilesIntoSeparateUnit()
    {
        GraphStore graphStore;
        m_registry->setGraphStore(&graphStore);

        Module identity;
        identity.id = "identity";
        identity.name = "Identity";
        identity.category = "custom";
        identity.language = "c";
        identity.version = "1.0";
        identity.origin = "local";
        identity.compileStatus = "passed";
        identity.testStatus = "passed";
        identity.inputs = {{"value", "int", "7"}};
        identity.outputs = {{"result", "int", ""}};
        identity.sourceCode = "int dq_identity(int value) {\n"
                              "    return value;\n"
                              "}";
        QVERIFY(m_registry->registerModule(identity));

        // Внутренний граф подмодуля берёт внешний input и возвращает его как output.
        Graph inner = Graph::create("InnerComposite");
        inner.parentModuleId = "identity_box";
        GraphNode innerNode;
        innerNode.id = "inner_node";
        innerNode.moduleId = identity.id;
        QVERIFY(inner.addNode(innerNode));
        QVERIFY(graphStore.registerGraph(inner));

        Module composite;
        composite.id = "identity_box";
        composite.name = "Identity Box";
        composite.category = "composite";
        composite.language = "c";
        composite.version = "1.0";
        composite.origin = "graph";
        composite.graphId = inner.id;
        composite.generatedFileBaseName = "submodule_01";
        composite.inputs = {{"value", "int", "7"}};
        composite.outputs = {{"result", "int", ""}};
        composite.boundaryInputs = {{"value", innerNode.id, "value", PortKind::Data}};
        composite.boundaryOutputs = {{"result", innerNode.id, "result", PortKind::Data}};
        QVERIFY(m_registry->registerModule(composite));

        GraphCompiler compiler(m_registry);
        compiler.setGraphStore(&graphStore);

        const SubmoduleUnitResult unit = compiler.compileSubmoduleUnit(composite);
        QVERIFY2(unit.success, qPrintable(unit.errors.join('\n')));
        QCOMPARE(unit.fileBaseName, QString("submodule_01"));
        QVERIFY(unit.functionName.startsWith("dq_sub_"));
        QVERIFY(unit.headerCode.contains("Generated by DeltaQ IDE."));
        QVERIFY(unit.headerCode.contains("Source: graph 'InnerComposite'"));
        QVERIFY(unit.headerCode.contains(QString("int %1(int value);").arg(unit.functionName)));
        QVERIFY(unit.sourceCode.contains(
            QString("#include \"generated/submodules/%1.h\"").arg(unit.fileBaseName)));
        QVERIFY(unit.sourceCode.contains("int dq_identity(int value)"));
        QVERIFY(unit.sourceCode.contains(QString("int %1(int value)").arg(unit.functionName)));
        QVERIFY(unit.sourceCode.contains("return var_inner_node_result;"));
    }

    void execAwareCompositeModuleCanBeReused()
    {
        GraphStore graphStore;
        m_registry->setGraphStore(&graphStore);

        Module identityExec;
        identityExec.id = "identity_exec";
        identityExec.name = "identity_exec";
        identityExec.category = "custom";
        identityExec.language = "c";
        identityExec.version = "1.0";
        identityExec.origin = "local";
        identityExec.compileStatus = "passed";
        identityExec.testStatus = "passed";
        identityExec.inputs = {Port::exec("flow_in"), {"value", "int", "0"}};
        identityExec.outputs = {Port::exec("flow_out"), {"result", "int", ""}};
        identityExec.sourceCode = "int dq_identity_exec(int value) {\n"
                                  "    return value;\n"
                                  "}";
        QVERIFY(m_registry->registerModule(identityExec));

        Graph inner = Graph::create("ExecAwareInner");
        inner.parentModuleId = "exec_identity_box";
        GraphNode innerNode;
        innerNode.id = "inner_exec_node";
        innerNode.moduleId = identityExec.id;
        QVERIFY(inner.addNode(innerNode));
        QVERIFY(graphStore.registerGraph(inner));

        Module composite;
        composite.id = "exec_identity_box";
        composite.name = "Exec Identity Box";
        composite.category = "composite";
        composite.language = "c";
        composite.version = "1.0";
        composite.origin = "graph";
        composite.graphId = inner.id;
        composite.generatedFileBaseName = "submodule_exec_01";
        composite.inputs = {Port::exec("flow_in"), {"value", "int", "0"}};
        composite.outputs = {Port::exec("flow_out"), {"result", "int", ""}};
        composite.boundaryInputs = {
            {"flow_in", innerNode.id, "flow_in", PortKind::Execution},
            {"value", innerNode.id, "value", PortKind::Data}
        };
        composite.boundaryOutputs = {
            {"flow_out", innerNode.id, "flow_out", PortKind::Execution},
            {"result", innerNode.id, "result", PortKind::Data}
        };
        QVERIFY(m_registry->registerModule(composite));

        Graph root = Graph::create("ExecReuse");
        GraphNode constNode; constNode.id = "n_const"; constNode.moduleId = "core.io.int_constant";
        constNode.properties["value"] = "7";
        GraphNode firstUse; firstUse.id = "n_first"; firstUse.moduleId = composite.id;
        GraphNode secondUse; secondUse.id = "n_second"; secondUse.moduleId = composite.id;
        root.addNode(constNode);
        root.addNode(firstUse);
        root.addNode(secondUse);

        root.addConnection({{"n_const", "out"}, {"n_first", "value"}});
        root.addConnection({{"n_first", "flow_out"}, {"n_second", "flow_in"}, PortKind::Execution});
        root.addConnection({{"n_first", "result"}, {"n_second", "value"}});

        GraphCompiler compiler(m_registry);
        compiler.setGraphStore(&graphStore);

        const SubmoduleUnitResult unit = compiler.compileSubmoduleUnit(composite);
        QVERIFY2(unit.success, qPrintable(unit.errors.join('\n')));
        QCOMPARE(unit.fileBaseName, QString("submodule_exec_01"));
        QVERIFY(unit.headerCode.contains(
            QString("int %1(int value);").arg(unit.functionName)));

        const CompilationResult result = compiler.compile(root);
        QVERIFY2(result.success, qPrintable(result.errors.join('\n')));
        QVERIFY(result.generatedCode.contains(
            "#include \"generated/submodules/submodule_exec_01.h\""));
        QCOMPARE(result.generatedCode.count(unit.functionName + "("), 2);
        QVERIFY(result.generatedCode.contains(
            QString("var_n_first_result = %1(var_n_const_out);").arg(unit.functionName)));
        QVERIFY(result.generatedCode.contains(
            QString("var_n_second_result = %1(var_n_first_result);").arg(unit.functionName)));
    }

    void nodePropertiesOverrideDefaultInputs()
    {
        Graph g = Graph::create("Properties");
        GraphNode n;
        n.id = "n1";
        n.moduleId = "core.io.int_constant";
        n.properties["value"] = "42";
        g.addNode(n);

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(result.success);
        QVERIFY(result.generatedCode.contains("dq_int_constant(42)"));
    }

    void desktopRuntimeGraphGeneratesSDLMain()
    {
        registerDesktopRuntimeModules();
        Graph g = createDesktopRuntimeGraph();

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(result.success);
        QVERIFY(result.generatedCode.contains("DQ_UIBackendContext dq_ui_backend_ctx = {0};"));
        QVERIFY(result.generatedCode.contains(
            "dq_ui_backend_init(&dq_ui_backend_ctx, var_n_title_out, var_n_width_out, var_n_height_out, 240, 180, true)"));
        QVERIFY(result.generatedCode.contains("var_n_window_window = dq_ui_backend_window(&dq_ui_backend_ctx);"));
        QVERIFY(result.generatedCode.contains("var_n_renderer_renderer = dq_ui_backend_renderer(&dq_ui_backend_ctx);"));
        QVERIFY(result.generatedCode.contains("DQ_UIBackendEvent event;"));
        QVERIFY(result.generatedCode.contains("dq_ui_backend_poll_event(&event)"));
        QVERIFY(result.generatedCode.contains("dq_ui_backend_window_size(&dq_ui_backend_ctx, &window_width, &window_height);"));
        QVERIFY(result.generatedCode.contains("ui_apply_layout(&var_n_ui_ui_state, window_width, window_height);"));
        QVERIFY(result.generatedCode.contains("dq_ui_backend_begin_frame(&dq_ui_backend_ctx);"));
        QVERIFY(result.generatedCode.contains("dq_ui_backend_end_frame(&dq_ui_backend_ctx);"));
        QVERIFY(result.generatedCode.contains("ui_init(&var_n_ui_ui_state);"));
        QVERIFY(result.generatedCode.contains("ui_render(&var_n_ui_ui_state, var_n_renderer_renderer);"));
        QVERIFY(result.generatedCode.contains("TTF_CloseFont(var_n_ui_ui_state.font);"));
        QVERIFY(result.generatedCode.contains("dq_ui_backend_release_renderer(&dq_ui_backend_ctx);"));
        QVERIFY(result.generatedCode.contains("dq_ui_backend_release_window(&dq_ui_backend_ctx);"));
        QVERIFY(result.generatedCode.contains("dq_ui_backend_shutdown(&dq_ui_backend_ctx);"));
        QVERIFY(result.generatedCode.contains("#include \"ui/window1.h\""));
        QVERIFY(result.generatedCode.contains("#include \"ui/window1_events.h\""));
        QVERIFY(result.generatedCode.contains("Source: graph 'main'"));
        QVERIFY(!result.generatedCode.contains("SDL_CreateWindow("));
    }

    void desktopRuntimeGraphCanUseInjectedWindowContract()
    {
        registerDesktopRuntimeModules();

        Graph g = createDesktopRuntimeGraph("\"graph-window\"", "320", "240");

        GraphCompiler compiler(m_registry);
        compiler.setInlineDesktopWindowContract({
            "Desktop UI Baseline",
            640,
            480,
            480,
            360,
            true
        });
        auto result = compiler.compile(g);

        QVERIFY(result.success);
        QVERIFY(result.generatedCode.contains(expectedInlineDesktopInitCall("Desktop UI Baseline",
                                                                            640, 480,
                                                                            480, 360,
                                                                            true)));
        QVERIFY(!result.generatedCode.contains(expectedInlineDesktopInitCall("graph-window",
                                                                             320, 240,
                                                                             240, 180,
                                                                             true)));
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
        readExec.compileStatus = "passed";
        readExec.testStatus = "passed";
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
        printStr.compileStatus = "passed";
        printStr.testStatus = "passed";
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

    void uiContractModuleUsesMetadataInsteadOfBackendImplementation()
    {
        Module button;
        button.id = "ui_module_button";
        button.name = "UI_Button";
        button.category = "ui";
        button.language = "c";
        button.version = "1.0";
        button.origin = "ui";
        button.metadata["deltaq.kind"] = "ui_contract";
        button.metadata["deltaq.ui.widget_type"] = "button";
        button.inputs = {{"text", "string", "\"OK\""}};
        button.outputs = {{"onClick", "signal", ""}};
        button.sourceCode =
            "// UI contract module: UI_Button\n"
            "// Backend v1: SDL2.\n";
        m_registry->registerModule(button);

        Graph g = Graph::create("UiContract");
        GraphNode n1;
        n1.id = "button1";
        n1.moduleId = "ui_module_button";
        n1.position = QPointF(0, 0);
        g.addNode(n1);

        GraphCompiler compiler(m_registry);
        const auto result = compiler.compile(g);

        QVERIFY(result.success);
        QVERIFY(result.generatedCode.contains("UI contract: button"));
        QVERIFY(result.generatedCode.contains("ui contract event placeholder"));
    }

    void uiContractStateOutputsFollowSharedVocabulary()
    {
        Module comboBox;
        comboBox.id = "ui_module_combobox";
        comboBox.name = "UI_ComboBox";
        comboBox.category = "ui";
        comboBox.language = "c";
        comboBox.version = "1.0";
        comboBox.origin = "ui";
        comboBox.metadata["deltaq.kind"] = "ui_contract";
        comboBox.metadata["deltaq.ui.widget_type"] = "combo_box";
        comboBox.metadata["deltaq.ui.events"] = QJsonArray{"onSelectionChanged"};
        comboBox.inputs = {{"items", "string", "\"A,B\""}, {"selected", "int", "2"}};
        comboBox.outputs = {{"onSelectionChanged", "signal", ""}, {"selected", "int", ""}};
        m_registry->registerModule(comboBox);

        Graph g = Graph::create("UiComboContract");
        GraphNode n1;
        n1.id = "combo1";
        n1.moduleId = "ui_module_combobox";
        n1.position = QPointF(0, 0);
        g.addNode(n1);

        GraphCompiler compiler(m_registry);
        const auto result = compiler.compile(g);

        QVERIFY(result.success);
        QVERIFY(result.generatedCode.contains("UI contract: combo_box"));
        QVERIFY(result.generatedCode.contains("ui contract event placeholder"));
        QVERIFY(result.generatedCode.contains("var_combo1_selected = 2"));
    }
};

QTEST_MAIN(TestGraphCompiler)
#include "test_GraphCompiler.moc"
