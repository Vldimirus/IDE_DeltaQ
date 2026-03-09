// Тесты генератора CMakeLists.txt
#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonArray>
#include <QTextStream>
#include <QDir>

#include <deltaq/Graph.h>
#include <deltaq/Module.h>

#include "../../src/core/GraphStore.h"
#include "../../src/core/ModuleRegistry.h"
#include "../../src/editor/CMakeGenerator.h"

using namespace DeltaQ;

class TestCMakeGenerator : public QObject {
    Q_OBJECT

private slots:
    void testGenerateBasic()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Создаём исходный файл
        QFile mainFile(tmpDir.path() + "/main.c");
        QVERIFY(mainFile.open(QIODevice::WriteOnly));
        mainFile.write("int main() { return 0; }\n");
        mainFile.close();

        CMakeGenerator gen;
        QString cmakePath = gen.generate(tmpDir.path(), "TestProject", "17", "20");

        QVERIFY(QFile::exists(cmakePath));

        QFile cmake(cmakePath);
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        QString content = cmake.readAll();
        cmake.close();

        QVERIFY(content.contains("cmake_minimum_required"));
        QVERIFY(content.contains("project(TestProject"));
        QVERIFY(content.contains("CMAKE_C_STANDARD 17"));
        QVERIFY(content.contains("CMAKE_CXX_STANDARD 20"));
        QVERIFY(content.contains("main.c"));
        QVERIFY(content.contains("add_executable(TestProject"));
    }

    void testGenerateWithMultipleFiles()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Создаём несколько файлов
        QDir(tmpDir.path()).mkpath("src");
        for (const auto &name : {"main.cpp", "src/utils.cpp", "src/helper.c"}) {
            QFile f(tmpDir.path() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("// source\n");
            f.close();
        }

        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "MyApp");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        QString content = cmake.readAll();

        QVERIFY(content.contains("main.cpp"));
        QVERIFY(content.contains("src/utils.cpp"));
        QVERIFY(content.contains("src/helper.c"));
    }

    void testGenerateNormalizesDialectLabels()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QFile mainFile(tmpDir.path() + "/main.c");
        QVERIFY(mainFile.open(QIODevice::WriteOnly));
        mainFile.write("int main(void) { return 0; }\n");
        mainFile.close();

        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "DialectTest", "c17", "c++20");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        const QString content = cmake.readAll();

        QVERIFY(content.contains("CMAKE_C_STANDARD 17"));
        QVERIFY(content.contains("CMAKE_CXX_STANDARD 20"));
        QVERIFY(!content.contains("CMAKE_C_STANDARD c17"));
        QVERIFY(!content.contains("CMAKE_CXX_STANDARD c++20"));
    }

    void testSkipBuildDir()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Создаём файлы в build/ — они должны быть пропущены
        QDir(tmpDir.path()).mkpath("build");
        QFile buildFile(tmpDir.path() + "/build/generated.cpp");
        buildFile.open(QIODevice::WriteOnly);
        buildFile.write("// generated\n");
        buildFile.close();

        QFile mainFile(tmpDir.path() + "/main.cpp");
        mainFile.open(QIODevice::WriteOnly);
        mainFile.write("int main() {}\n");
        mainFile.close();

        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "TestProj");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        QString content = cmake.readAll();

        QVERIFY(content.contains("main.cpp"));
        QVERIFY(!content.contains("generated.cpp"));
    }

    void testExtraFlags()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QFile mainFile(tmpDir.path() + "/main.c");
        mainFile.open(QIODevice::WriteOnly);
        mainFile.write("int main() { return 0; }\n");
        mainFile.close();

        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "FlagTest", "17", "20", {"-O2", "-DNDEBUG"});

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        QString content = cmake.readAll();

        QVERIFY(content.contains("-O2"));
        QVERIFY(content.contains("-DNDEBUG"));
        QVERIFY(content.contains("-Wall -Wextra"));
    }

    void testDesktopSDL2Dependencies()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QFile mainFile(tmpDir.path() + "/main.c");
        mainFile.open(QIODevice::WriteOnly);
        mainFile.write("int main() { return 0; }\n");
        mainFile.close();

        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "DesktopApp", "17", "20", {}, "desktop");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        QString content = cmake.readAll();

        QVERIFY(content.contains("find_package(SDL2 QUIET)"));
        QVERIFY(content.contains("find_package(SDL2_ttf QUIET)"));
        QVERIFY(content.contains("pkg_check_modules(PKG_SDL2 QUIET sdl2)"));
        QVERIFY(content.contains("pkg_check_modules(PKG_SDL2_TTF QUIET SDL2_ttf)"));
        QVERIFY(content.contains("Desktop template requires SDL2 and SDL2_ttf"));
    }

    void testEmptyProject()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Нет исходных файлов
        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "Empty");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        QString content = cmake.readAll();

        // CMakeLists.txt должен быть создан даже без исходников
        QVERIFY(content.contains("project(Empty"));
        QVERIFY(content.contains("set(SOURCES"));
    }

    void testAutoGeneratedComment()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QFile f(tmpDir.path() + "/main.c");
        f.open(QIODevice::WriteOnly);
        f.write("int main() {}\n");
        f.close();

        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "Test");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        cmake.open(QIODevice::ReadOnly);
        QString content = cmake.readAll();

        QVERIFY(content.startsWith("# Автоматически сгенерировано DeltaQ IDE"));
    }

    void testImportedPackBuildRequirementsFromUsedGraph()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QFile mainFile(tmpDir.path() + "/main.c");
        QVERIFY(mainFile.open(QIODevice::WriteOnly));
        mainFile.write("int main(void) { return 0; }\n");
        mainFile.close();

        ModuleRegistry registry;
        GraphStore graphStore;
        registry.setGraphStore(&graphStore);

        Module imported;
        imported.id = "ext.math_pack.cos_value";
        imported.name = "cos_value";
        imported.language = "c";
        imported.version = "1.0";
        imported.origin = "extension";
        imported.compileStatus = "passed";
        imported.testStatus = "passed";
        imported.inputs = {{"value", "double", "0"}};
        imported.outputs = {{"result", "double", ""}};
        imported.sourceCode = "double dq_cos_value(double value) {\n    return value;\n}";
        imported.metadata["deltaq.import.pack_name"] = "math_pack";
        imported.metadata["deltaq.import.include_paths"] = QJsonArray{
            "/opt/math sdk/include", "/usr/local/include/math_pack"
        };
        imported.metadata["deltaq.import.defines"] = QJsonArray{
            "USE_SYSTEM_MATH=1", "MATH_PACK_ENABLED"
        };
        imported.metadata["deltaq.import.link_libraries"] = QJsonArray{"m", "sensor_sdk"};
        QVERIFY(registry.registerModule(imported));

        Graph root = Graph::create("main");
        GraphNode node;
        node.id = "cos_node";
        node.moduleId = imported.id;
        QVERIFY(root.addNode(node));
        QVERIFY(graphStore.registerGraph(root));

        CMakeGenerator gen;
        gen.setModuleRegistry(&registry);
        gen.setGraphStore(&graphStore);
        gen.generate(tmpDir.path(), "ImportedPackApp");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        const QString content = QString::fromUtf8(cmake.readAll());

        QVERIFY(content.contains("# Imported DeltaQ module packs actually used by project graphs"));
        QVERIFY(content.contains("#   - math_pack"));
        QVERIFY(content.contains("target_include_directories(ImportedPackApp PRIVATE"));
        QVERIFY(content.contains("\"/opt/math sdk/include\""));
        QVERIFY(content.contains("/usr/local/include/math_pack"));
        QVERIFY(content.contains("target_compile_definitions(ImportedPackApp PRIVATE"));
        QVERIFY(content.contains("USE_SYSTEM_MATH=1"));
        QVERIFY(content.contains("MATH_PACK_ENABLED"));
        QVERIFY(content.contains("target_link_libraries(ImportedPackApp PRIVATE"));
        QVERIFY(content.contains("\n    m\n"));
        QVERIFY(content.contains("\n    sensor_sdk\n"));
    }

    void testImportedPackRequirementsPropagateThroughCompositeModule()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QFile mainFile(tmpDir.path() + "/main.c");
        QVERIFY(mainFile.open(QIODevice::WriteOnly));
        mainFile.write("int main(void) { return 0; }\n");
        mainFile.close();

        ModuleRegistry registry;
        GraphStore graphStore;
        registry.setGraphStore(&graphStore);

        Module imported;
        imported.id = "ext.math_pack.cos_value";
        imported.name = "cos_value";
        imported.language = "c";
        imported.version = "1.0";
        imported.origin = "extension";
        imported.compileStatus = "passed";
        imported.testStatus = "passed";
        imported.inputs = {{"value", "double", "0"}};
        imported.outputs = {{"result", "double", ""}};
        imported.sourceCode = "double dq_cos_value(double value) {\n    return value;\n}";
        imported.metadata["deltaq.import.pack_name"] = "math_pack";
        imported.metadata["deltaq.import.link_libraries"] = QJsonArray{"m"};
        QVERIFY(registry.registerModule(imported));

        Graph inner = Graph::create("Inner");
        inner.parentModuleId = "math_box";
        GraphNode innerNode;
        innerNode.id = "inner_cos";
        innerNode.moduleId = imported.id;
        QVERIFY(inner.addNode(innerNode));
        QVERIFY(graphStore.registerGraph(inner));

        Module composite;
        composite.id = "math_box";
        composite.name = "Math Box";
        composite.language = "c";
        composite.version = "1.0";
        composite.origin = "local";
        composite.graphId = inner.id;
        composite.inputs = {{"value", "double", "0"}};
        composite.outputs = {{"result", "double", ""}};
        composite.boundaryInputs = {{"value", innerNode.id, "value", PortKind::Data}};
        composite.boundaryOutputs = {{"result", innerNode.id, "result", PortKind::Data}};
        QVERIFY(registry.registerModule(composite));

        Graph root = Graph::create("main");
        GraphNode rootNode;
        rootNode.id = "root_math";
        rootNode.moduleId = composite.id;
        QVERIFY(root.addNode(rootNode));
        QVERIFY(graphStore.registerGraph(root));

        CMakeGenerator gen;
        gen.setModuleRegistry(&registry);
        gen.setGraphStore(&graphStore);
        gen.generate(tmpDir.path(), "CompositeImportedPackApp");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        const QString content = QString::fromUtf8(cmake.readAll());

        QVERIFY(content.contains("#   - math_pack"));
        QVERIFY(content.contains("target_link_libraries(CompositeImportedPackApp PRIVATE"));
        QVERIFY(content.contains("\n    m\n"));
    }
};

QTEST_MAIN(TestCMakeGenerator)
#include "test_CMakeGenerator.moc"
