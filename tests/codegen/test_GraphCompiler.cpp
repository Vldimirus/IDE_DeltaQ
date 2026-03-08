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

        Graph g = Graph::create("main");

        GraphNode title; title.id = "n_title"; title.moduleId = "core.io.string_constant";
        title.properties["value"] = "\"window1\"";
        GraphNode width; width.id = "n_width"; width.moduleId = "core.io.int_constant";
        width.properties["value"] = "640";
        GraphNode height; height.id = "n_height"; height.moduleId = "core.io.int_constant";
        height.properties["value"] = "480";
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

        GraphCompiler compiler(m_registry);
        auto result = compiler.compile(g);

        QVERIFY(result.success);
        QVERIFY(result.generatedCode.contains("SDL_Init(SDL_INIT_VIDEO)"));
        QVERIFY(result.generatedCode.contains("TTF_Init()"));
        QVERIFY(result.generatedCode.contains("SDL_CreateWindow("));
        QVERIFY(result.generatedCode.contains("SDL_CreateRenderer("));
        QVERIFY(result.generatedCode.contains("ui_init(&var_n_ui_ui_state);"));
        QVERIFY(result.generatedCode.contains("ui_render(&var_n_ui_ui_state, var_n_renderer_renderer);"));
        QVERIFY(result.generatedCode.contains("TTF_CloseFont(var_n_ui_ui_state.font);"));
        QVERIFY(result.generatedCode.contains("SDL_DestroyRenderer(var_n_renderer_renderer);"));
        QVERIFY(result.generatedCode.contains("SDL_DestroyWindow(var_n_window_window);"));
        QVERIFY(result.generatedCode.contains("#include \"ui/window1.h\""));
        QVERIFY(result.generatedCode.contains("#include \"ui/window1_events.h\""));
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
