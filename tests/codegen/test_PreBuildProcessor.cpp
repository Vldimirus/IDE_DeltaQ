#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QJsonArray>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTest>

#include <deltaq/Graph.h>
#include <deltaq/UILayout.h>

#include "../../src/codegen/PreBuildProcessor.h"
#include "../../src/core/GraphStore.h"
#include "../../src/core/ModuleRegistry.h"
#include "../../src/core/UILayoutStore.h"
#include "../../src/editor/CMakeGenerator.h"
#include "../../src/libProcessor/LibraryDecomposer.h"
#include "../../src/libProcessor/LibraryPackager.h"

using namespace DeltaQ;

class TestPreBuildProcessor : public QObject {
    Q_OBJECT

private:
    // Определяет корень репозитория относительно каталога test-бинарника.
    static QString repoRootPath()
    {
        QStringList candidates = {
            QDir::cleanPath(QDir::currentPath()),
            QDir::cleanPath(QDir(QDir::currentPath()).absoluteFilePath("..")),
            QDir::cleanPath(QDir(QDir::currentPath()).absoluteFilePath("../.."))
        };

        if (QCoreApplication::instance()) {
            const QDir appDir(QCoreApplication::applicationDirPath());
            candidates.append(QDir::cleanPath(appDir.absoluteFilePath("../..")));
            candidates.append(QDir::cleanPath(appDir.absoluteFilePath("../../..")));
        }

        for (const auto &candidate : candidates) {
            if (QFile::exists(candidate + "/modules/core/pack.json") &&
                QFile::exists(candidate + "/resources/templates/desktop/graphs/main.dqgraph")) {
                return candidate;
            }
        }

        return {};
    }

    // Копирует шаблонный проект в temporary directory для интеграционного теста.
    static bool copyDirectory(const QString &src, const QString &dst)
    {
        QDir srcDir(src);
        if (!srcDir.exists())
            return false;

        QDir().mkpath(dst);
        QDirIterator it(src, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();

            const QString relPath = srcDir.relativeFilePath(it.filePath());
            const QString dstPath = dst + "/" + relPath;
            QDir().mkpath(QFileInfo(dstPath).absolutePath());
            if (QFile::exists(dstPath))
                QFile::remove(dstPath);
            if (!QFile::copy(it.filePath(), dstPath))
                return false;
        }

        return true;
    }

    // Ищет собранный исполняемый файл по имени таргета внутри build-dir.
    static QString findBuiltExecutable(const QString &buildDir, const QString &targetName)
    {
        const QString directPath = buildDir + "/" + targetName;
        if (QFileInfo::exists(directPath))
            return directPath;

        const QString exePath = directPath + ".exe";
        if (QFileInfo::exists(exePath))
            return exePath;

        QDirIterator it(buildDir, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QFileInfo info(it.fileInfo());
            if (info.fileName() == targetName || info.fileName() == targetName + ".exe")
                return info.absoluteFilePath();
        }

        return {};
    }

    // Запускает внешний процесс и возвращает его stdout/stderr для диагностики теста.
    static bool runProcess(const QString &program, const QStringList &arguments,
                           const QString &workingDir, QByteArray *stdoutData,
                           QByteArray *stderrData, int timeoutMs = 120000)
    {
        QProcess process;
        process.setProgram(program);
        process.setArguments(arguments);
        process.setWorkingDirectory(workingDir);
        process.start();
        if (!process.waitForStarted(30000))
            return false;
        if (!process.waitForFinished(timeoutMs))
            return false;

        if (stdoutData)
            *stdoutData = process.readAllStandardOutput();
        if (stderrData)
            *stderrData = process.readAllStandardError();
        return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    }

    // Собирает фиксированный ParseResult для fixture SDK, чтобы curated pack verification
    // не зависела от наличия libclang в текущем окружении.
    static ParseResult makeMiniSensorParseResult()
    {
        ParseResult result;
        result.success = true;

        FunctionDecl init;
        init.name = "mini_sensor_init";
        init.returnType = "mini_sensor_context *";
        init.parameters.append({"device_name", "const char *", ""});
        result.functions.append(init);

        FunctionDecl read;
        read.name = "mini_sensor_read";
        read.returnType = "int";
        read.parameters.append({"ctx", "mini_sensor_context *", ""});
        result.functions.append(read);

        FunctionDecl scale;
        scale.name = "mini_sensor_scale";
        scale.returnType = "int";
        scale.parameters.append({"ctx", "mini_sensor_context *", ""});
        scale.parameters.append({"factor", "int", ""});
        result.functions.append(scale);

        FunctionDecl lastError;
        lastError.name = "mini_sensor_last_error";
        lastError.returnType = "const char *";
        lastError.parameters.append({"ctx", "mini_sensor_context *", ""});
        result.functions.append(lastError);

        FunctionDecl shutdown;
        shutdown.name = "mini_sensor_shutdown";
        shutdown.returnType = "void";
        shutdown.parameters.append({"ctx", "mini_sensor_context *", ""});
        result.functions.append(shutdown);

        return result;
    }

    // Собирает fixture SDK как внешнюю библиотеку, чтобы imported pack можно было
    // линковать в реальном build/run сценарии через metadata pack-а.
    static bool buildMiniSensorFixture(const QString &repoRoot, const QString &workDir,
                                       QString *includeDir, QString *libraryPath,
                                       QString *failureLog = nullptr)
    {
        const QString fixtureSrc =
            repoRoot + "/resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk";
        if (!copyDirectory(fixtureSrc, workDir)) {
            if (failureLog)
                *failureLog = "Failed to copy mini_sensor_sdk fixture";
            return false;
        }

        QByteArray out;
        QByteArray err;
        if (!runProcess("cmake", {"-S", ".", "-B", "build"}, workDir, &out, &err)) {
            if (failureLog)
                *failureLog = QString::fromUtf8(out + err);
            return false;
        }

        out.clear();
        err.clear();
        if (!runProcess("cmake", {"--build", "build", "--parallel"}, workDir, &out, &err)) {
            if (failureLog)
                *failureLog = QString::fromUtf8(out + err);
            return false;
        }

        if (includeDir)
            *includeDir = QDir::cleanPath(workDir + "/include");
        if (libraryPath)
            *libraryPath = QDir::cleanPath(workDir + "/build/libmini_sensor_sdk.a");
        return QFile::exists(QDir::cleanPath(workDir + "/build/libmini_sensor_sdk.a"));
    }

    // Находит модуль по id в реестре для последующей curation/verification подготовки.
    static Module *requireModule(ModuleRegistry &registry, const QString &moduleId)
    {
        return registry.findModule(moduleId);
    }

    // Готовит verified curated pack поверх raw wrapper-ов и добавляет adapter entry-модули.
    static bool createCuratedMiniSensorPack(const QString &modulesRoot,
                                            const QString &headerIncludeDir,
                                            const QString &libraryPath,
                                            QString *failureLog = nullptr)
    {
        DecompositionOptions options;
        options.category = "sensor_raw";
        options.language = "c";
        const DecompositionResult decomposition =
            LibraryDecomposer::decompose(makeMiniSensorParseResult(), options);

        ImportedLibraryPackSpec spec;
        spec.packName = "mini_sensor_sdk_curated";
        spec.displayName = "Mini Sensor SDK Curated";
        spec.author = "DeltaQ";
        spec.description = "Verified curated pack for mini_sensor_sdk";
        spec.category = "sensor_raw";
        spec.language = "c";
        spec.standard = "c17";
        spec.headerPaths = {"mini_sensor_sdk.h"};
        spec.includePaths = {headerIncludeDir};
        spec.linkLibraries = {libraryPath};

        const ImportedLibraryPackResult packResult =
            LibraryPackager::writeImportedPack(modulesRoot, spec, decomposition.modules);
        if (!packResult.success()) {
            if (failureLog)
                *failureLog = packResult.errors.join(" | ");
            return false;
        }

        ModuleRegistry registry;
        registry.loadGlobalModules(modulesRoot);

        // Сырые wrapper-ы в curated pack остаются доступными для аудита, но скрываются из палитры.
        const QStringList rawModuleIds = {
            "ext.mini_sensor_sdk_curated.mini_sensor_init",
            "ext.mini_sensor_sdk_curated.mini_sensor_read",
            "ext.mini_sensor_sdk_curated.mini_sensor_scale",
            "ext.mini_sensor_sdk_curated.mini_sensor_last_error",
            "ext.mini_sensor_sdk_curated.mini_sensor_shutdown"
        };
        for (const auto &moduleId : rawModuleIds) {
            Module *module = requireModule(registry, moduleId);
            if (!module) {
                if (failureLog)
                    *failureLog = "Curated pack is missing raw module: " + moduleId;
                return false;
            }
            module->compileStatus = "passed";
            module->testStatus = "passed";
            module->metadata["deltaq.import.display_name"] = module->name;
            module->metadata["deltaq.import.curation_role"] = "hidden";
            if (!registry.saveModuleFile(*module, module->storagePath)) {
                if (failureLog)
                    *failureLog = "Failed to save curated raw wrapper: " + moduleId;
                return false;
            }
        }

        auto makeAdapter = [&](const QString &id,
                               const QString &name,
                               const QString &displayName,
                               const QString &sourceCode) {
            Module adapter = Module::create(name, "c");
            adapter.id = id;
            adapter.name = name;
            adapter.origin = "extension";
            adapter.category = "sensor_curated";
            adapter.version = "1.0";
            adapter.compileStatus = "passed";
            adapter.testStatus = "passed";
            adapter.inputs = {{"device_name", "string", "\"sensor-A\""},
                              {"factor", "int", "1"}};
            adapter.outputs = {{"result", "int", ""}};
            adapter.includes = {"<stdio.h>", "<mini_sensor_sdk.h>"};
            adapter.sourceCode = sourceCode;
            adapter.metadata["deltaq.import.kind"] = "library_pack_module";
            adapter.metadata["deltaq.import.pack_name"] = "mini_sensor_sdk_curated";
            adapter.metadata["deltaq.import.pack_title"] = "Mini Sensor SDK Curated";
            adapter.metadata["deltaq.import.original_symbol"] = name;
            adapter.metadata["deltaq.import.display_name"] = displayName;
            adapter.metadata["deltaq.import.curation_role"] = "curated_entry";
            adapter.metadata["deltaq.import.include_paths"] = QJsonArray{headerIncludeDir};
            adapter.metadata["deltaq.import.link_libraries"] = QJsonArray{libraryPath};
            return adapter;
        };

        const Module scaleReport = makeAdapter(
            "ext.mini_sensor_sdk_curated.sensor_scale_report",
            "sensor_scale_report",
            "Sensor Scale Report",
            "int dq_sensor_scale_report(const char *device_name, int factor) {\n"
            "    mini_sensor_context *ctx = mini_sensor_init(device_name);\n"
            "    if (!ctx) {\n"
            "        printf(\"INIT_FAILED\\n\");\n"
            "        return -999;\n"
            "    }\n"
            "    int read_value = mini_sensor_read(ctx);\n"
            "    int scaled_value = mini_sensor_scale(ctx, factor);\n"
            "    printf(\"READ=%d\\n\", read_value);\n"
            "    printf(\"SCALED=%d\\n\", scaled_value);\n"
            "    mini_sensor_shutdown(ctx);\n"
            "    printf(\"SHUTDOWN=done\\n\");\n"
            "    return scaled_value;\n"
            "}");

        const Module errorReport = makeAdapter(
            "ext.mini_sensor_sdk_curated.sensor_error_report",
            "sensor_error_report",
            "Sensor Error Report",
            "int dq_sensor_error_report(const char *device_name, int factor) {\n"
            "    mini_sensor_context *ctx = mini_sensor_init(device_name);\n"
            "    if (!ctx) {\n"
            "        printf(\"INIT_FAILED\\n\");\n"
            "        return -999;\n"
            "    }\n"
            "    int failed_value = mini_sensor_scale(ctx, factor);\n"
            "    printf(\"FAILED_SCALE=%d\\n\", failed_value);\n"
            "    printf(\"LAST_ERROR=%s\\n\", mini_sensor_last_error(ctx));\n"
            "    mini_sensor_shutdown(ctx);\n"
            "    printf(\"SHUTDOWN=done\\n\");\n"
            "    return failed_value;\n"
            "}");

        const QString curatedDir = modulesRoot + "/mini_sensor_sdk_curated/sensor_curated";
        if (!QDir().mkpath(curatedDir)) {
            if (failureLog)
                *failureLog = "Failed to create curated pack category directory";
            return false;
        }

        for (const Module &adapter : {scaleReport, errorReport}) {
            if (!registry.registerModule(adapter)) {
                if (failureLog)
                    *failureLog = "Failed to register curated adapter: " + adapter.id;
                return false;
            }
            const QString adapterPath = curatedDir + "/" + adapter.name + ".dqmod";
            if (!registry.saveModuleFile(adapter, adapterPath)) {
                if (failureLog)
                    *failureLog = "Failed to save curated adapter: " + adapter.name;
                return false;
            }
        }

        return true;
    }

private slots:
    void processReportsArtifactOrigins()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        ModuleRegistry registry;
        GraphStore graphStore;
        UILayoutStore layoutStore;

        Graph graph = Graph::create("main");
        QVERIFY(graphStore.registerGraph(graph));

        UILayout layout = UILayout::create("window1");
        QVERIFY(layoutStore.registerLayout(layout));

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult result = processor.process(tmpDir.path());

        QVERIFY(result.success);
        QCOMPARE(result.generatedArtifacts.size(), 5);

        bool sawGraphSource = false;
        bool sawUiHeader = false;
        bool sawUiSource = false;
        bool sawUiEventsHeader = false;
        bool sawUiEventsSource = false;

        for (const auto &artifact : result.generatedArtifacts) {
            QVERIFY(QFile::exists(artifact.path));
            switch (artifact.kind) {
            case PreBuildArtifactKind::GraphSource:
                sawGraphSource = true;
                QCOMPARE(artifact.sourceName, QString("main"));
                QCOMPARE(artifact.sourceId, graph.id);
                QVERIFY(artifact.path.endsWith("/src/main.c"));
                break;
            case PreBuildArtifactKind::UIHeader:
                sawUiHeader = true;
                QCOMPARE(artifact.sourceName, QString("window1"));
                QCOMPARE(artifact.sourceId, layout.id);
                QVERIFY(artifact.path.endsWith("/src/ui/window1.h"));
                break;
            case PreBuildArtifactKind::UISource:
                sawUiSource = true;
                QCOMPARE(artifact.sourceName, QString("window1"));
                QCOMPARE(artifact.sourceId, layout.id);
                QVERIFY(artifact.path.endsWith("/src/ui/window1.c"));
                break;
            case PreBuildArtifactKind::UIEventsHeader:
                sawUiEventsHeader = true;
                QCOMPARE(artifact.sourceName, QString("window1"));
                QCOMPARE(artifact.sourceId, layout.id);
                QVERIFY(artifact.path.endsWith("/src/ui/window1_events.h"));
                break;
            case PreBuildArtifactKind::UIEventsSource:
                sawUiEventsSource = true;
                QCOMPARE(artifact.sourceName, QString("window1"));
                QCOMPARE(artifact.sourceId, layout.id);
                QVERIFY(artifact.path.endsWith("/src/ui/window1_events.c"));
                break;
            }
        }

        QVERIFY(sawGraphSource);
        QVERIFY(sawUiHeader);
        QVERIFY(sawUiSource);
        QVERIFY(sawUiEventsHeader);
        QVERIFY(sawUiEventsSource);

        QFile graphFile(tmpDir.path() + "/src/main.c");
        QVERIFY(graphFile.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(QString::fromUtf8(graphFile.readAll()).contains("Source: graph 'main'"));

        QFile uiHeaderFile(tmpDir.path() + "/src/ui/window1.h");
        QVERIFY(uiHeaderFile.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(QString::fromUtf8(uiHeaderFile.readAll()).contains("Source: UI layout 'window1'"));
    }

    void processesDesktopTemplateEndToEnd()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString repoRoot = repoRootPath();
        QVERIFY2(!repoRoot.isEmpty(), "Repository root was not found from test binary location");

        const QString projectDir = tmpDir.path() + "/desktop_flow";
        QVERIFY(copyDirectory(repoRoot + "/resources/templates/desktop", projectDir));

        ModuleRegistry registry;
        registry.loadGlobalModules(repoRoot + "/modules");
        const Module *eventLoopModule = registry.findModule("core.desktop.event_loop");
        QVERIFY(eventLoopModule != nullptr);
        QCOMPARE(eventLoopModule->metadataString("deltaq.kind"), QString("ui_backend_runtime"));
        QCOMPARE(eventLoopModule->metadataString("deltaq.ui.backend"), QString("sdl2"));

        GraphStore graphStore;
        UILayoutStore layoutStore;
        QVERIFY(graphStore.loadFromDirectory(projectDir));
        QVERIFY(layoutStore.loadFromDirectory(projectDir));
        QCOMPARE(graphStore.count(), 1);
        QCOMPARE(layoutStore.count(), 1);

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult result = processor.process(projectDir);

        QVERIFY2(result.success, qPrintable(result.errors.join('\n')));
        QCOMPARE(result.generatedArtifacts.size(), 5);

        bool sawMainGraph = false;
        bool sawWindowUi = false;
        for (const auto &artifact : result.generatedArtifacts) {
            if (artifact.kind == PreBuildArtifactKind::GraphSource &&
                artifact.sourceName == "main" &&
                artifact.path.endsWith("/src/main.c")) {
                sawMainGraph = true;
            }
            if ((artifact.kind == PreBuildArtifactKind::UIHeader ||
                 artifact.kind == PreBuildArtifactKind::UISource ||
                 artifact.kind == PreBuildArtifactKind::UIEventsHeader ||
                 artifact.kind == PreBuildArtifactKind::UIEventsSource) &&
                artifact.sourceName == "window1") {
                sawWindowUi = true;
            }
        }

        QVERIFY(sawMainGraph);
        QVERIFY(sawWindowUi);

        QFile mainFile(projectDir + "/src/main.c");
        QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString mainCode = QString::fromUtf8(mainFile.readAll());
        QVERIFY(mainCode.contains("Source: graph 'main'"));
        QVERIFY(mainCode.contains("DQ_UIBackendContext dq_ui_backend_ctx = {0};"));
        QVERIFY(mainCode.contains("dq_ui_backend_init(&dq_ui_backend_ctx"));
        QVERIFY(mainCode.contains("dq_ui_backend_poll_event(&event)"));
        QVERIFY(mainCode.contains("dq_ui_backend_window_size(&dq_ui_backend_ctx, &window_width, &window_height);"));
        QVERIFY(mainCode.contains("ui_apply_layout(&"));
        QVERIFY(mainCode.contains("window_width, window_height);"));
        QVERIFY(mainCode.contains("dq_ui_backend_begin_frame(&dq_ui_backend_ctx);"));
        QVERIFY(mainCode.contains("dq_ui_backend_shutdown(&dq_ui_backend_ctx);"));
        QVERIFY(!mainCode.contains("SDL_CreateWindow("));

        QFile uiSource(projectDir + "/src/ui/window1.c");
        QVERIFY(uiSource.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString uiCode = QString::fromUtf8(uiSource.readAll());
        QVERIFY(uiCode.contains("Source: UI layout 'window1'"));
        QVERIFY(uiCode.contains("void ui_init(UIState *ui)"));
        QVERIFY(uiCode.contains("bool dq_ui_backend_init(DQ_UIBackendContext *backend"));
        QVERIFY(uiCode.contains("void dq_ui_backend_window_size(DQ_UIBackendContext *backend, int *width, int *height)"));
        QVERIFY(uiCode.contains("void ui_apply_layout(UIState *ui, int window_width, int window_height)"));
        QVERIFY(uiCode.contains("void dq_ui_backend_begin_frame(DQ_UIBackendContext *backend)"));

        QFile uiEventsSource(projectDir + "/src/ui/window1_events.c");
        QVERIFY(uiEventsSource.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString eventsCode = QString::fromUtf8(uiEventsSource.readAll());
        QVERIFY(eventsCode.contains("counter++"));
        QVERIFY(eventsCode.contains("Counter: %d"));
        QVERIFY(eventsCode.contains("on_btnCopyToLog_click"));
        QVERIFY(!eventsCode.contains("TODO: implement"));
    }

    void processesConsoleTemplateEndToEnd()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString repoRoot = repoRootPath();
        QVERIFY2(!repoRoot.isEmpty(), "Repository root was not found from test binary location");

        const QString projectDir = tmpDir.path() + "/console_flow";
        QVERIFY(copyDirectory(repoRoot + "/resources/templates/internal_console_graph", projectDir));

        ModuleRegistry registry;
        registry.loadGlobalModules(repoRoot + "/modules");

        GraphStore graphStore;
        UILayoutStore layoutStore;
        QVERIFY(graphStore.loadFromDirectory(projectDir));
        QCOMPARE(graphStore.count(), 1);
        QCOMPARE(layoutStore.count(), 0);

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult result = processor.process(projectDir);

        QVERIFY2(result.success, qPrintable(result.errors.join('\n')));
        QCOMPARE(result.generatedArtifacts.size(), 1);
        QCOMPARE(result.generatedArtifacts.first().kind, PreBuildArtifactKind::GraphSource);
        QCOMPARE(result.generatedArtifacts.first().sourceName, QString("main"));
        QVERIFY(result.generatedArtifacts.first().path.endsWith("/src/main.c"));

        QFile mainFile(projectDir + "/src/main.c");
        QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString mainCode = QString::fromUtf8(mainFile.readAll());
        QVERIFY(mainCode.contains("Source: graph 'main'"));
        QVERIFY(mainCode.contains("dq_string_constant"));
        QVERIFY(mainCode.contains("dq_println"));
        QVERIFY(mainCode.contains("dq_read_line"));
        QVERIFY(mainCode.contains("int main(void)"));
    }

    void generatesSeparateCompilationUnitsForCompositeSubmodule()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        ModuleRegistry registry;
        GraphStore graphStore;
        UILayoutStore layoutStore;
        registry.setGraphStore(&graphStore);

        // Атомарный модуль используется внутри составного подмодуля.
        Module identity;
        identity.id = "identity";
        identity.name = "Identity";
        identity.category = "custom";
        identity.language = "c";
        identity.version = "1.0";
        identity.origin = "local";
        identity.compileStatus = "passed";
        identity.testStatus = "passed";
        identity.inputs = {{"value", "int", "5"}};
        identity.outputs = {{"result", "int", ""}};
        identity.sourceCode = "int dq_identity(int value) {\n"
                              "    return value;\n"
                              "}";
        QVERIFY(registry.registerModule(identity));

        // Внутренний граф подмодуля.
        Graph inner = Graph::create("IdentityInner");
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
        composite.inputs = {{"value", "int", "5"}};
        composite.outputs = {{"result", "int", ""}};
        composite.boundaryInputs = {{"value", innerNode.id, "value", PortKind::Data}};
        composite.boundaryOutputs = {{"result", innerNode.id, "result", PortKind::Data}};
        QVERIFY(registry.registerModule(composite));

        // Корневой граф использует составной модуль как обычный callable block.
        Graph root = Graph::create("main");
        GraphNode compositeNode;
        compositeNode.id = "root_node";
        compositeNode.moduleId = composite.id;
        compositeNode.properties["value"] = "11";
        QVERIFY(root.addNode(compositeNode));
        QVERIFY(graphStore.registerGraph(root));

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult result = processor.process(tmpDir.path());

        QVERIFY2(result.success, qPrintable(result.errors.join('\n')));
        QCOMPARE(result.generatedArtifacts.size(), 3);

        QString submoduleHeaderPath;
        QString submoduleSourcePath;
        QString rootSourcePath;
        for (const auto &artifact : result.generatedArtifacts) {
            QVERIFY(QFile::exists(artifact.path));
            switch (artifact.kind) {
            case PreBuildArtifactKind::GraphSource:
                rootSourcePath = artifact.path;
                QCOMPARE(artifact.sourceName, QString("main"));
                break;
            case PreBuildArtifactKind::SubmoduleHeader:
                submoduleHeaderPath = artifact.path;
                QCOMPARE(artifact.sourceName, QString("IdentityInner"));
                break;
            case PreBuildArtifactKind::SubmoduleSource:
                submoduleSourcePath = artifact.path;
                QCOMPARE(artifact.sourceName, QString("IdentityInner"));
                break;
            default:
                QFAIL("Unexpected artifact kind in composite submodule test");
            }
        }

        QVERIFY(!rootSourcePath.isEmpty());
        QVERIFY(!submoduleHeaderPath.isEmpty());
        QVERIFY(!submoduleSourcePath.isEmpty());

        const QString fileBaseName = QFileInfo(submoduleHeaderPath).completeBaseName();
        QCOMPARE(fileBaseName, QString("submodule_01"));
        QFile rootFile(rootSourcePath);
        QVERIFY(rootFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString rootCode = QString::fromUtf8(rootFile.readAll());
        QVERIFY(rootCode.contains("Source: graph 'main'"));
        QVERIFY(rootCode.contains(
            QString("#include \"generated/submodules/%1.h\"").arg(fileBaseName)));
        QVERIFY(rootCode.contains("dq_sub_"));
        QVERIFY(rootCode.contains("(11)"));

        QFile headerFile(submoduleHeaderPath);
        QVERIFY(headerFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString headerCode = QString::fromUtf8(headerFile.readAll());
        QVERIFY(headerCode.contains("Source: graph 'IdentityInner'"));
        QVERIFY(headerCode.contains("Generated by DeltaQ IDE."));
        QVERIFY(headerCode.contains("dq_sub_"));

        QFile sourceFile(submoduleSourcePath);
        QVERIFY(sourceFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString sourceCode = QString::fromUtf8(sourceFile.readAll());
        QVERIFY(sourceCode.contains("Source: graph 'IdentityInner'"));
        QVERIFY(sourceCode.contains(
            QString("#include \"generated/submodules/%1.h\"").arg(fileBaseName)));
        QVERIFY(sourceCode.contains("int dq_identity(int value)"));
        QVERIFY(sourceCode.contains("return var_inner_node_result;"));
    }

    void reusesExecAwareCompositeSubmoduleInRootGraph()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        ModuleRegistry registry;
        GraphStore graphStore;
        UILayoutStore layoutStore;
        registry.setGraphStore(&graphStore);

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
        QVERIFY(registry.registerModule(identityExec));

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
        QVERIFY(registry.registerModule(intConst));

        Graph inner = Graph::create("ExecReuseInner");
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
        QVERIFY(registry.registerModule(composite));

        Graph root = Graph::create("main");
        GraphNode constNode;
        constNode.id = "n_const";
        constNode.moduleId = intConst.id;
        constNode.properties["value"] = "21";
        GraphNode firstUse;
        firstUse.id = "n_first";
        firstUse.moduleId = composite.id;
        GraphNode secondUse;
        secondUse.id = "n_second";
        secondUse.moduleId = composite.id;
        QVERIFY(root.addNode(constNode));
        QVERIFY(root.addNode(firstUse));
        QVERIFY(root.addNode(secondUse));
        QVERIFY(root.addConnection({{"n_const", "out"}, {"n_first", "value"}}));
        QVERIFY(root.addConnection({{"n_first", "flow_out"}, {"n_second", "flow_in"},
                                    PortKind::Execution}));
        QVERIFY(root.addConnection({{"n_first", "result"}, {"n_second", "value"}}));
        QVERIFY(graphStore.registerGraph(root));

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult result = processor.process(tmpDir.path());

        QVERIFY2(result.success, qPrintable(result.errors.join('\n')));
        QCOMPARE(result.generatedArtifacts.size(), 3);

        QString rootSourcePath;
        QString submoduleHeaderPath;
        QString submoduleSourcePath;
        for (const auto &artifact : result.generatedArtifacts) {
            switch (artifact.kind) {
            case PreBuildArtifactKind::GraphSource:
                rootSourcePath = artifact.path;
                break;
            case PreBuildArtifactKind::SubmoduleHeader:
                submoduleHeaderPath = artifact.path;
                break;
            case PreBuildArtifactKind::SubmoduleSource:
                submoduleSourcePath = artifact.path;
                break;
            default:
                break;
            }
        }

        QVERIFY(!rootSourcePath.isEmpty());
        QVERIFY(!submoduleHeaderPath.isEmpty());
        QVERIFY(!submoduleSourcePath.isEmpty());
        QVERIFY(submoduleHeaderPath.endsWith("/src/generated/submodules/submodule_exec_01.h"));
        QVERIFY(submoduleSourcePath.endsWith("/src/generated/submodules/submodule_exec_01.c"));

        QFile rootFile(rootSourcePath);
        QVERIFY(rootFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString rootCode = QString::fromUtf8(rootFile.readAll());
        QVERIFY(rootCode.contains("#include \"generated/submodules/submodule_exec_01.h\""));
        QCOMPARE(rootCode.count("dq_sub_submodule_exec_01("), 2);
        QVERIFY(rootCode.contains("var_n_first_result = dq_sub_submodule_exec_01(var_n_const_out);"));
        QVERIFY(rootCode.contains("var_n_second_result = dq_sub_submodule_exec_01(var_n_first_result);"));
    }

    void consoleTemplateBuildsAndRunsEndToEnd()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString repoRoot = repoRootPath();
        QVERIFY2(!repoRoot.isEmpty(), "Repository root was not found from test binary location");

        const QString projectDir = tmpDir.path() + "/console_run_flow";
        QVERIFY(copyDirectory(repoRoot + "/resources/templates/internal_console_graph", projectDir));

        ModuleRegistry registry;
        registry.loadGlobalModules(repoRoot + "/modules");

        GraphStore graphStore;
        UILayoutStore layoutStore;
        QVERIFY(graphStore.loadFromDirectory(projectDir));
        QCOMPARE(graphStore.count(), 1);

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult prebuild = processor.process(projectDir);
        QVERIFY2(prebuild.success, qPrintable(prebuild.errors.join('\n')));

        CMakeGenerator generator;
        const QString targetName = "ConsoleRunFlow";
        generator.generate(projectDir, targetName, "17", "20", {}, "console");
        QVERIFY2(generator.configure(projectDir), "CMake configure failed for console end-to-end flow");

        QByteArray buildOut;
        QByteArray buildErr;
        QVERIFY2(runProcess("cmake", {"--build", "build", "--parallel"}, projectDir,
                            &buildOut, &buildErr),
                 qPrintable(QString::fromUtf8(buildOut + buildErr)));

        const QString executablePath = findBuiltExecutable(projectDir + "/build", targetName);
        QVERIFY2(!executablePath.isEmpty(), "Built executable was not found");

        // Запускаем готовую программу и подаём ввод, чтобы проверить полный цикл до runtime.
        QProcess app;
        app.setProgram(executablePath);
        app.setWorkingDirectory(projectDir + "/build");
        app.start();
        QVERIFY2(app.waitForStarted(30000), "Built executable failed to start");
        app.write("DeltaQ\n");
        app.closeWriteChannel();
        QVERIFY2(app.waitForFinished(30000), "Built executable did not finish in time");

        // Считываем вывод один раз после завершения процесса, чтобы проверки работали стабильно.
        const QByteArray stdoutData = app.readAllStandardOutput();
        const QByteArray stderrData = app.readAllStandardError();
        QVERIFY2(app.exitStatus() == QProcess::NormalExit && app.exitCode() == 0,
                 qPrintable(QString::fromUtf8(stdoutData + stderrData)));

        const QString appOutput = QString::fromUtf8(stdoutData);
        const QString appError = QString::fromUtf8(stderrData);
        const QString runtimeLog = QString("STDOUT:\n%1\nSTDERR:\n%2")
                                       .arg(appOutput, appError);
        QVERIFY2(appOutput.contains("Hello"), qPrintable(runtimeLog));
        QVERIFY2(appOutput.contains("DeltaQ"), qPrintable(runtimeLog));
    }

    void minimalConsoleExampleBuildsAndRunsEndToEnd()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString repoRoot = repoRootPath();
        QVERIFY2(!repoRoot.isEmpty(), "Repository root was not found from test binary location");

        const QString projectDir = tmpDir.path() + "/minimal_console_flow";
        QVERIFY(copyDirectory(repoRoot + "/resources/examples/minimal_console_flow", projectDir));

        ModuleRegistry registry;
        registry.loadGlobalModules(repoRoot + "/modules");

        GraphStore graphStore;
        UILayoutStore layoutStore;
        QVERIFY(graphStore.loadFromDirectory(projectDir));
        QCOMPARE(graphStore.count(), 1);

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult prebuild = processor.process(projectDir);
        QVERIFY2(prebuild.success, qPrintable(prebuild.errors.join('\n')));
        QCOMPARE(prebuild.generatedArtifacts.size(), 1);
        QCOMPARE(prebuild.generatedArtifacts.first().kind, PreBuildArtifactKind::GraphSource);
        QVERIFY(prebuild.generatedArtifacts.first().path.endsWith("/src/main.c"));

        QFile mainFile(projectDir + "/src/main.c");
        QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString mainCode = QString::fromUtf8(mainFile.readAll());
        QVERIFY(mainCode.contains("Source: graph 'main'"));
        QVERIFY(mainCode.contains("dq_string_constant"));
        QVERIFY(mainCode.contains("dq_println"));
        QVERIFY(mainCode.contains("dq_read_line"));
        QVERIFY(mainCode.contains("Hello from DeltaQ!"));

        CMakeGenerator generator;
        const QString targetName = "MinimalConsoleFlow";
        generator.generate(projectDir, targetName, "17", "20", {}, "console");
        QVERIFY2(generator.configure(projectDir), "CMake configure failed for minimal console flow");

        QByteArray buildOut;
        QByteArray buildErr;
        QVERIFY2(runProcess("cmake", {"--build", "build", "--parallel"}, projectDir,
                            &buildOut, &buildErr),
                 qPrintable(QString::fromUtf8(buildOut + buildErr)));

        const QString executablePath = findBuiltExecutable(projectDir + "/build", targetName);
        QVERIFY2(!executablePath.isEmpty(), "Built minimal console flow executable was not found");

        QProcess app;
        app.setProgram(executablePath);
        app.setWorkingDirectory(projectDir + "/build");
        app.start();
        QVERIFY2(app.waitForStarted(30000), "Built minimal console flow executable failed to start");
        app.write("DeltaQ\n");
        app.closeWriteChannel();
        QVERIFY2(app.waitForFinished(30000), "Built minimal console flow executable did not finish in time");

        const QString stdoutText = QString::fromUtf8(app.readAllStandardOutput());
        const QString stderrText = QString::fromUtf8(app.readAllStandardError());
        const QString runtimeLog = QString("STDOUT:\n%1\nSTDERR:\n%2")
                                       .arg(stdoutText, stderrText);
        QVERIFY2(app.exitStatus() == QProcess::NormalExit && app.exitCode() == 0,
                 qPrintable(runtimeLog));

        QString normalizedStdout = stdoutText;
        normalizedStdout.replace("\r\n", "\n");
        const QStringList lines = normalizedStdout.split('\n', Qt::SkipEmptyParts);
        QCOMPARE(lines.size(), 2);
        QCOMPARE(lines.at(0), QString("Hello from DeltaQ!"));
        QCOMPARE(lines.at(1), QString("DeltaQ"));
    }

    void desktopUiExampleBuildsAndRunsHeadless()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString repoRoot = repoRootPath();
        QVERIFY2(!repoRoot.isEmpty(), "Repository root was not found from test binary location");

        const QString projectDir = tmpDir.path() + "/desktop_ui_flow";
        QVERIFY(copyDirectory(repoRoot + "/resources/examples/desktop_ui_flow", projectDir));

        ModuleRegistry registry;
        registry.loadGlobalModules(repoRoot + "/modules");

        GraphStore graphStore;
        UILayoutStore layoutStore;
        QVERIFY(graphStore.loadFromDirectory(projectDir));
        QVERIFY(layoutStore.loadFromDirectory(projectDir));
        QCOMPARE(graphStore.count(), 1);
        QCOMPARE(layoutStore.count(), 1);

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult prebuild = processor.process(projectDir);
        QVERIFY2(prebuild.success, qPrintable(prebuild.errors.join('\n')));
        QCOMPARE(prebuild.generatedArtifacts.size(), 5);

        QFile uiSource(projectDir + "/src/ui/window1.c");
        QVERIFY(uiSource.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString uiCode = QString::fromUtf8(uiSource.readAll());
        QVERIFY(uiCode.contains("SDL_RENDERER_SOFTWARE"));

        QFile eventsSource(projectDir + "/src/ui/window1_events.c");
        QVERIFY(eventsSource.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString eventsCode = QString::fromUtf8(eventsSource.readAll());
        QVERIFY(eventsCode.contains("DQ_DESKTOP_UI_FLOW_AUTOCLOSE_MS"));
        QVERIFY(eventsCode.contains("SDL_PushEvent"));
        QVERIFY(!eventsCode.contains("TODO: implement"));

        CMakeGenerator generator;
        const QString targetName = "DesktopUIFlow";
        generator.generate(projectDir, targetName, "17", "20", {}, "desktop");
        QVERIFY2(generator.configure(projectDir), "CMake configure failed for desktop UI flow");

        QByteArray buildOut;
        QByteArray buildErr;
        QVERIFY2(runProcess("cmake", {"--build", "build", "--parallel"}, projectDir,
                            &buildOut, &buildErr),
                 qPrintable(QString::fromUtf8(buildOut + buildErr)));

        const QString executablePath = findBuiltExecutable(projectDir + "/build", targetName);
        QVERIFY2(!executablePath.isEmpty(), "Built desktop UI flow executable was not found");

        QProcess app;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("SDL_VIDEODRIVER", "dummy");
        env.insert("SDL_RENDER_DRIVER", "software");
        env.insert("DQ_DESKTOP_UI_FLOW_AUTOCLOSE_MS", "1200");
        app.setProcessEnvironment(env);
        app.setProgram(executablePath);
        app.setWorkingDirectory(projectDir + "/build");
        app.start();
        QVERIFY2(app.waitForStarted(30000), "Built desktop UI flow executable failed to start");
        QVERIFY2(app.waitForFinished(10000), "Built desktop UI flow executable did not finish in time");

        const QString stdoutText = QString::fromUtf8(app.readAllStandardOutput());
        const QString stderrText = QString::fromUtf8(app.readAllStandardError());
        const QString runtimeLog = QString("STDOUT:\n%1\nSTDERR:\n%2")
                                       .arg(stdoutText, stderrText);
        QVERIFY2(app.exitStatus() == QProcess::NormalExit && app.exitCode() == 0,
                 qPrintable(runtimeLog));
    }

    void importedPackBuildRequirementsReachCMakeAndRuntime()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString projectDir = tmpDir.path() + "/imported_pack_flow";
        QVERIFY(QDir().mkpath(projectDir));

        ModuleRegistry registry;
        GraphStore graphStore;
        UILayoutStore layoutStore;
        registry.setGraphStore(&graphStore);

        // Импортированный модуль эмулирует pack от внешней C-библиотеки: код вызывает cos(),
        // а сборка обязана подтянуть -lm через metadata imported pack-а.
        Module importedCos;
        importedCos.id = "ext.math_pack.cos_value";
        importedCos.name = "cos_value";
        importedCos.category = "imported";
        importedCos.language = "c";
        importedCos.version = "1.0";
        importedCos.origin = "extension";
        importedCos.compileStatus = "passed";
        importedCos.testStatus = "passed";
        importedCos.inputs = {{"value", "double", "0"}};
        importedCos.outputs = {{"result", "double", ""}};
        importedCos.includes = {"<math.h>"};
        importedCos.sourceCode = "double dq_cos_value(double value) {\n"
                                 "    return cos(value);\n"
                                 "}";
        importedCos.metadata["deltaq.import.pack_name"] = "math_pack";
        importedCos.metadata["deltaq.import.kind"] = "library_pack_module";
        importedCos.metadata["deltaq.import.include_paths"] = QJsonArray{"/usr/include"};
        importedCos.metadata["deltaq.import.defines"] = QJsonArray{"USE_SYSTEM_MATH=1"};
        importedCos.metadata["deltaq.import.link_libraries"] = QJsonArray{"m"};
        QVERIFY(registry.registerModule(importedCos));

        Module printDouble;
        printDouble.id = "print_double";
        printDouble.name = "print_double";
        printDouble.category = "io";
        printDouble.language = "c";
        printDouble.version = "1.0";
        printDouble.origin = "local";
        printDouble.compileStatus = "passed";
        printDouble.testStatus = "passed";
        printDouble.inputs = {{"value", "double", "0"}};
        printDouble.outputs = {};
        printDouble.includes = {"<stdio.h>"};
        printDouble.sourceCode = "void dq_print_double(double value) {\n"
                                 "    printf(\"COS=%.6f\\n\", value);\n"
                                 "}";
        QVERIFY(registry.registerModule(printDouble));

        Graph root = Graph::create("main");
        GraphNode cosNode;
        cosNode.id = "cos_node";
        cosNode.moduleId = importedCos.id;
        cosNode.properties["value"] = "0.5";
        GraphNode printNode;
        printNode.id = "print_node";
        printNode.moduleId = printDouble.id;
        QVERIFY(root.addNode(cosNode));
        QVERIFY(root.addNode(printNode));
        QVERIFY(root.addConnection({{"cos_node", "result"}, {"print_node", "value"}}));
        QVERIFY(graphStore.registerGraph(root));

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult prebuild = processor.process(projectDir);
        QVERIFY2(prebuild.success, qPrintable(prebuild.errors.join('\n')));
        QCOMPARE(prebuild.generatedArtifacts.size(), 1);

        QFile mainFile(projectDir + "/src/main.c");
        QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString mainCode = QString::fromUtf8(mainFile.readAll());
        QVERIFY(mainCode.contains("#include <math.h>"));
        QVERIFY(mainCode.contains("double dq_cos_value(double value)"));
        QVERIFY(mainCode.contains("dq_print_double"));

        CMakeGenerator generator;
        generator.setModuleRegistry(&registry);
        generator.setGraphStore(&graphStore);
        const QString targetName = "ImportedPackFlow";
        generator.generate(projectDir, targetName, "17", "20", {}, "console");

        QFile cmakeFile(projectDir + "/CMakeLists.txt");
        QVERIFY(cmakeFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString cmakeCode = QString::fromUtf8(cmakeFile.readAll());
        QVERIFY(cmakeCode.contains("#   - math_pack"));
        QVERIFY(cmakeCode.contains("target_compile_definitions(ImportedPackFlow PRIVATE"));
        QVERIFY(cmakeCode.contains("USE_SYSTEM_MATH=1"));
        QVERIFY(cmakeCode.contains("target_link_libraries(ImportedPackFlow PRIVATE"));
        QVERIFY(cmakeCode.contains("\n    m\n"));

        QVERIFY2(generator.configure(projectDir), "CMake configure failed for imported pack flow");

        QByteArray buildOut;
        QByteArray buildErr;
        QVERIFY2(runProcess("cmake", {"--build", "build", "--parallel"}, projectDir,
                            &buildOut, &buildErr),
                 qPrintable(QString::fromUtf8(buildOut + buildErr)));

        const QString executablePath = findBuiltExecutable(projectDir + "/build", targetName);
        QVERIFY2(!executablePath.isEmpty(), "Built imported pack executable was not found");

        QProcess app;
        app.setProgram(executablePath);
        app.setWorkingDirectory(projectDir + "/build");
        app.start();
        QVERIFY2(app.waitForStarted(30000), "Built imported pack executable failed to start");
        QVERIFY2(app.waitForFinished(30000), "Built imported pack executable did not finish in time");

        const QByteArray stdoutData = app.readAllStandardOutput();
        const QByteArray stderrData = app.readAllStandardError();
        QVERIFY2(app.exitStatus() == QProcess::NormalExit && app.exitCode() == 0,
                 qPrintable(QString::fromUtf8(stdoutData + stderrData)));

        const QString runtimeLog = QString("STDOUT:\n%1\nSTDERR:\n%2")
                                       .arg(QString::fromUtf8(stdoutData),
                                            QString::fromUtf8(stderrData));
        QVERIFY2(QString::fromUtf8(stdoutData).contains("COS=0.877583"), qPrintable(runtimeLog));
    }

    void curatedImportedPackBuildsAndRunsMiniSensorFlow()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString repoRoot = repoRootPath();
        QVERIFY2(!repoRoot.isEmpty(), "Repository root was not found from test binary location");

        QString includeDir;
        QString libraryPath;
        QString fixtureFailure;
        QVERIFY2(buildMiniSensorFixture(repoRoot,
                                        tmpDir.path() + "/mini_sensor_sdk_build",
                                        &includeDir,
                                        &libraryPath,
                                        &fixtureFailure),
                 qPrintable(fixtureFailure));

        QString curatedFailure;
        const QString modulesRoot = tmpDir.path() + "/modules";
        QVERIFY2(createCuratedMiniSensorPack(modulesRoot, includeDir, libraryPath, &curatedFailure),
                 qPrintable(curatedFailure));

        const QString projectDir = tmpDir.path() + "/mini_sensor_curated_flow";
        QVERIFY(QDir().mkpath(projectDir));

        ModuleRegistry registry;
        registry.loadGlobalModules(modulesRoot);

        GraphStore graphStore;
        UILayoutStore layoutStore;
        registry.setGraphStore(&graphStore);

        const Module *scaleModule =
            registry.findModule("ext.mini_sensor_sdk_curated.sensor_scale_report");
        const Module *errorModule =
            registry.findModule("ext.mini_sensor_sdk_curated.sensor_error_report");
        QVERIFY(scaleModule != nullptr);
        QVERIFY(errorModule != nullptr);
        QVERIFY(scaleModule->metadataString("deltaq.import.curation_role") == "curated_entry");
        QVERIFY(errorModule->metadataString("deltaq.import.curation_role") == "curated_entry");

        Graph root = Graph::create("main");
        GraphNode scaleNode;
        scaleNode.id = "scale_report";
        scaleNode.moduleId = scaleModule->id;
        scaleNode.properties["device_name"] = "\"sensor-A\"";
        scaleNode.properties["factor"] = "3";

        GraphNode errorNode;
        errorNode.id = "error_report";
        errorNode.moduleId = errorModule->id;
        errorNode.properties["device_name"] = "\"sensor-A\"";
        errorNode.properties["factor"] = "0";

        QVERIFY(root.addNode(scaleNode));
        QVERIFY(root.addNode(errorNode));
        QVERIFY(graphStore.registerGraph(root));

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult prebuild = processor.process(projectDir);
        QVERIFY2(prebuild.success, qPrintable(prebuild.errors.join('\n')));
        QCOMPARE(prebuild.generatedArtifacts.size(), 1);

        QFile mainFile(projectDir + "/src/main.c");
        QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString mainCode = QString::fromUtf8(mainFile.readAll());
        QVERIFY(mainCode.contains("#include <mini_sensor_sdk.h>"));
        QVERIFY(mainCode.contains("int dq_sensor_scale_report(const char *device_name, int factor)"));
        QVERIFY(mainCode.contains("int dq_sensor_error_report(const char *device_name, int factor)"));

        CMakeGenerator generator;
        generator.setModuleRegistry(&registry);
        generator.setGraphStore(&graphStore);
        const QString targetName = "CuratedMiniSensorFlow";
        generator.generate(projectDir, targetName, "17", "20", {}, "console");

        QFile cmakeFile(projectDir + "/CMakeLists.txt");
        QVERIFY(cmakeFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString cmakeCode = QString::fromUtf8(cmakeFile.readAll());
        QVERIFY(cmakeCode.contains("#   - mini_sensor_sdk_curated"));
        QVERIFY(cmakeCode.contains("target_include_directories(CuratedMiniSensorFlow PRIVATE"));
        QVERIFY(cmakeCode.contains(QDir::fromNativeSeparators(includeDir)));
        QVERIFY(cmakeCode.contains("target_link_libraries(CuratedMiniSensorFlow PRIVATE"));
        QVERIFY(cmakeCode.contains(QDir::fromNativeSeparators(libraryPath)));

        QVERIFY2(generator.configure(projectDir), "CMake configure failed for curated mini_sensor flow");

        QByteArray buildOut;
        QByteArray buildErr;
        QVERIFY2(runProcess("cmake", {"--build", "build", "--parallel"}, projectDir,
                            &buildOut, &buildErr),
                 qPrintable(QString::fromUtf8(buildOut + buildErr)));

        const QString executablePath = findBuiltExecutable(projectDir + "/build", targetName);
        QVERIFY2(!executablePath.isEmpty(), "Built curated mini_sensor executable was not found");

        QProcess app;
        app.setProgram(executablePath);
        app.setWorkingDirectory(projectDir + "/build");
        app.start();
        QVERIFY2(app.waitForStarted(30000), "Built curated mini_sensor executable failed to start");
        QVERIFY2(app.waitForFinished(30000), "Built curated mini_sensor executable did not finish in time");

        const QString stdoutText = QString::fromUtf8(app.readAllStandardOutput());
        const QString stderrText = QString::fromUtf8(app.readAllStandardError());
        const QString runtimeLog = QString("STDOUT:\n%1\nSTDERR:\n%2")
                                       .arg(stdoutText, stderrText);
        QVERIFY2(app.exitStatus() == QProcess::NormalExit && app.exitCode() == 0,
                 qPrintable(runtimeLog));

        QVERIFY2(stdoutText.contains("READ=48"), qPrintable(runtimeLog));
        QVERIFY2(stdoutText.contains("SCALED=144"), qPrintable(runtimeLog));
        QVERIFY2(stdoutText.contains("FAILED_SCALE=-1"), qPrintable(runtimeLog));
        QVERIFY2(stdoutText.contains("LAST_ERROR=scale factor must be positive"),
                 qPrintable(runtimeLog));
        QCOMPARE(stdoutText.count("SHUTDOWN=done"), 2);
    }

    void importedPackSensorConsoleExampleBuildsAndRunsEndToEnd()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString repoRoot = repoRootPath();
        QVERIFY2(!repoRoot.isEmpty(), "Repository root was not found from test binary location");

        const QString projectDir = tmpDir.path() + "/imported_pack_sensor_console";
        QVERIFY(copyDirectory(repoRoot + "/resources/examples/imported_pack_sensor_console", projectDir));

        ModuleRegistry registry;
        registry.loadGlobalModules(repoRoot + "/modules");

        GraphStore graphStore;
        UILayoutStore layoutStore;
        QVERIFY(graphStore.loadFromDirectory(projectDir));
        registry.loadLocalModules(projectDir + "/dqmods");
        registry.setGraphStore(&graphStore);

        QCOMPARE(graphStore.count(), 1);
        QVERIFY(registry.findModule("ext.mini_sensor_sdk_curated.sensor_scale_report") != nullptr);
        QVERIFY(registry.findModule("ext.mini_sensor_sdk_curated.sensor_error_report") != nullptr);

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult prebuild = processor.process(projectDir);
        QVERIFY2(prebuild.success, qPrintable(prebuild.errors.join('\n')));
        QCOMPARE(prebuild.generatedArtifacts.size(), 1);

        QFile mainFile(projectDir + "/src/main.c");
        QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString mainCode = QString::fromUtf8(mainFile.readAll());
        QVERIFY(mainCode.contains("dq_sensor_scale_report"));
        QVERIFY(mainCode.contains("dq_sensor_error_report"));
        QVERIFY(mainCode.contains("#include <mini_sensor_sdk.h>"));

        CMakeGenerator generator;
        generator.setModuleRegistry(&registry);
        generator.setGraphStore(&graphStore);
        const QString targetName = "ImportedPackSensorConsole";
        generator.generate(projectDir, targetName, "17", "20", {}, "console");

        QFile cmakeFile(projectDir + "/CMakeLists.txt");
        QVERIFY(cmakeFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString cmakeCode = QString::fromUtf8(cmakeFile.readAll());
        QVERIFY(cmakeCode.contains("#   - mini_sensor_sdk_curated"));
        QVERIFY(cmakeCode.contains("${CMAKE_SOURCE_DIR}/vendor/mini_sensor_sdk/include"));
        QVERIFY(!cmakeCode.contains("target_link_libraries(ImportedPackSensorConsole PRIVATE\n    mini_sensor_sdk"));

        QVERIFY2(generator.configure(projectDir), "CMake configure failed for imported pack sensor console example");

        QByteArray buildOut;
        QByteArray buildErr;
        QVERIFY2(runProcess("cmake", {"--build", "build", "--parallel"}, projectDir,
                            &buildOut, &buildErr),
                 qPrintable(QString::fromUtf8(buildOut + buildErr)));

        const QString executablePath = findBuiltExecutable(projectDir + "/build", targetName);
        QVERIFY2(!executablePath.isEmpty(), "Built imported pack sensor console executable was not found");

        QProcess app;
        app.setProgram(executablePath);
        app.setWorkingDirectory(projectDir + "/build");
        app.start();
        QVERIFY2(app.waitForStarted(30000), "Built imported pack sensor console executable failed to start");
        QVERIFY2(app.waitForFinished(30000), "Built imported pack sensor console executable did not finish in time");

        const QString stdoutText = QString::fromUtf8(app.readAllStandardOutput());
        const QString stderrText = QString::fromUtf8(app.readAllStandardError());
        const QString runtimeLog = QString("STDOUT:\n%1\nSTDERR:\n%2")
                                       .arg(stdoutText, stderrText);
        QVERIFY2(app.exitStatus() == QProcess::NormalExit && app.exitCode() == 0,
                 qPrintable(runtimeLog));

        QVERIFY2(stdoutText.contains("READ=48"), qPrintable(runtimeLog));
        QVERIFY2(stdoutText.contains("SCALED=144"), qPrintable(runtimeLog));
        QVERIFY2(stdoutText.contains("FAILED_SCALE=-1"), qPrintable(runtimeLog));
        QVERIFY2(stdoutText.contains("LAST_ERROR=scale factor must be positive"),
                 qPrintable(runtimeLog));
        QCOMPARE(stdoutText.count("SHUTDOWN=done"), 2);
    }

    void importedPackChecksumConsoleExampleBuildsAndRunsEndToEnd()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString repoRoot = repoRootPath();
        QVERIFY2(!repoRoot.isEmpty(), "Repository root was not found from test binary location");

        const QString projectDir = tmpDir.path() + "/imported_pack_checksum_console";
        QVERIFY(copyDirectory(repoRoot + "/resources/examples/imported_pack_checksum_console", projectDir));

        ModuleRegistry registry;
        registry.loadGlobalModules(repoRoot + "/modules");

        GraphStore graphStore;
        UILayoutStore layoutStore;
        QVERIFY(graphStore.loadFromDirectory(projectDir));
        registry.loadLocalModules(projectDir + "/dqmods");
        registry.setGraphStore(&graphStore);

        QCOMPARE(graphStore.count(), 1);
        QVERIFY(registry.findModule("ext.mini_checksum_sdk_curated.checksum_report") != nullptr);
        QVERIFY(registry.findModule("ext.mini_checksum_sdk_curated.checksum_match_report") != nullptr);

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult prebuild = processor.process(projectDir);
        QVERIFY2(prebuild.success, qPrintable(prebuild.errors.join('\n')));
        QCOMPARE(prebuild.generatedArtifacts.size(), 1);

        QFile mainFile(projectDir + "/src/main.c");
        QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString mainCode = QString::fromUtf8(mainFile.readAll());
        QVERIFY(mainCode.contains("dq_checksum_report"));
        QVERIFY(mainCode.contains("dq_checksum_match_report"));
        QVERIFY(mainCode.contains("#include <mini_checksum_sdk.h>"));

        CMakeGenerator generator;
        generator.setModuleRegistry(&registry);
        generator.setGraphStore(&graphStore);
        const QString targetName = "ImportedPackChecksumConsole";
        generator.generate(projectDir, targetName, "17", "20", {}, "console");

        QFile cmakeFile(projectDir + "/CMakeLists.txt");
        QVERIFY(cmakeFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString cmakeCode = QString::fromUtf8(cmakeFile.readAll());
        QVERIFY(cmakeCode.contains("#   - mini_checksum_sdk_curated"));
        QVERIFY(cmakeCode.contains("${CMAKE_SOURCE_DIR}/vendor/mini_checksum_sdk/include"));

        QVERIFY2(generator.configure(projectDir), "CMake configure failed for imported pack checksum console example");

        QByteArray buildOut;
        QByteArray buildErr;
        QVERIFY2(runProcess("cmake", {"--build", "build", "--parallel"}, projectDir,
                            &buildOut, &buildErr),
                 qPrintable(QString::fromUtf8(buildOut + buildErr)));

        const QString executablePath = findBuiltExecutable(projectDir + "/build", targetName);
        QVERIFY2(!executablePath.isEmpty(), "Built imported pack checksum console executable was not found");

        QProcess app;
        app.setProgram(executablePath);
        app.setWorkingDirectory(projectDir + "/build");
        app.start();
        QVERIFY2(app.waitForStarted(30000), "Built imported pack checksum console executable failed to start");
        QVERIFY2(app.waitForFinished(30000), "Built imported pack checksum console executable did not finish in time");

        const QString stdoutText = QString::fromUtf8(app.readAllStandardOutput());
        const QString stderrText = QString::fromUtf8(app.readAllStandardError());
        const QString runtimeLog = QString("STDOUT:\n%1\nSTDERR:\n%2")
                                       .arg(stdoutText, stderrText);
        QVERIFY2(app.exitStatus() == QProcess::NormalExit && app.exitCode() == 0,
                 qPrintable(runtimeLog));

        QVERIFY2(stdoutText.contains("TEXT=DeltaQ"), qPrintable(runtimeLog));
        QVERIFY2(stdoutText.contains("CHECKSUM=2115045471"), qPrintable(runtimeLog));
        QVERIFY2(stdoutText.contains("HEX=7E11085F"), qPrintable(runtimeLog));
        QVERIFY2(stdoutText.contains("EXPECTED=00000000"), qPrintable(runtimeLog));
        QVERIFY2(stdoutText.contains("ACTUAL=7E11085F"), qPrintable(runtimeLog));
        QVERIFY2(stdoutText.contains("MATCH=0"), qPrintable(runtimeLog));
        QCOMPARE(stdoutText.count("SHUTDOWN=done"), 2);
    }

    void reusableCompositionExampleBuildsAndRunsEndToEnd()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString repoRoot = repoRootPath();
        QVERIFY2(!repoRoot.isEmpty(), "Repository root was not found from test binary location");

        const QString projectDir = tmpDir.path() + "/reusable_composition_console";
        QVERIFY(copyDirectory(repoRoot + "/resources/examples/reusable_composition_console", projectDir));

        ModuleRegistry registry;
        registry.loadGlobalModules(repoRoot + "/modules");

        GraphStore graphStore;
        UILayoutStore layoutStore;
        QVERIFY(graphStore.loadFromDirectory(projectDir));
        registry.loadLocalModules(projectDir + "/dqmods");
        registry.setGraphStore(&graphStore);

        QCOMPARE(graphStore.count(), 3);
        QVERIFY(registry.findModule("example.graph.echo_with_prefix") != nullptr);
        QVERIFY(registry.findModule("example.graph.measure_text") != nullptr);

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult prebuild = processor.process(projectDir);
        QVERIFY2(prebuild.success, qPrintable(prebuild.errors.join('\n')));
        QCOMPARE(prebuild.generatedArtifacts.size(), 5);

        bool sawEchoHeader = false;
        bool sawEchoSource = false;
        bool sawMeasureHeader = false;
        bool sawMeasureSource = false;
        bool sawRootSource = false;
        for (const auto &artifact : prebuild.generatedArtifacts) {
            if (artifact.kind == PreBuildArtifactKind::SubmoduleHeader &&
                artifact.path.endsWith("/src/generated/submodules/echo_with_prefix.h")) {
                sawEchoHeader = true;
            } else if (artifact.kind == PreBuildArtifactKind::SubmoduleSource &&
                       artifact.path.endsWith("/src/generated/submodules/echo_with_prefix.c")) {
                sawEchoSource = true;
            } else if (artifact.kind == PreBuildArtifactKind::SubmoduleHeader &&
                       artifact.path.endsWith("/src/generated/submodules/measure_text.h")) {
                sawMeasureHeader = true;
            } else if (artifact.kind == PreBuildArtifactKind::SubmoduleSource &&
                       artifact.path.endsWith("/src/generated/submodules/measure_text.c")) {
                sawMeasureSource = true;
            } else if (artifact.kind == PreBuildArtifactKind::GraphSource &&
                       artifact.path.endsWith("/src/main.c")) {
                sawRootSource = true;
            }
        }

        QVERIFY(sawEchoHeader);
        QVERIFY(sawEchoSource);
        QVERIFY(sawMeasureHeader);
        QVERIFY(sawMeasureSource);
        QVERIFY(sawRootSource);

        QFile mainFile(projectDir + "/src/main.c");
        QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString mainCode = QString::fromUtf8(mainFile.readAll());
        QVERIFY(mainCode.contains("#include \"generated/submodules/echo_with_prefix.h\""));
        QVERIFY(mainCode.contains("#include \"generated/submodules/measure_text.h\""));
        QCOMPARE(mainCode.count("dq_sub_echo_with_prefix("), 2);
        QVERIFY(mainCode.contains("dq_sub_measure_text("));

        CMakeGenerator generator;
        const QString targetName = "ReusableCompositionConsole";
        generator.generate(projectDir, targetName, "17", "20", {}, "console");
        QVERIFY2(generator.configure(projectDir), "CMake configure failed for reusable composition flow");

        QByteArray buildOut;
        QByteArray buildErr;
        QVERIFY2(runProcess("cmake", {"--build", "build", "--parallel"}, projectDir,
                            &buildOut, &buildErr),
                 qPrintable(QString::fromUtf8(buildOut + buildErr)));

        const QString executablePath = findBuiltExecutable(projectDir + "/build", targetName);
        QVERIFY2(!executablePath.isEmpty(), "Built reusable composition executable was not found");

        // Проверяем реальный showcase-сценарий reuse: один подмодуль печатает greeting и length.
        QProcess app;
        app.setProgram(executablePath);
        app.setWorkingDirectory(projectDir + "/build");
        app.start();
        QVERIFY2(app.waitForStarted(30000), "Built reusable composition executable failed to start");
        app.write("DeltaQ\n");
        app.closeWriteChannel();
        QVERIFY2(app.waitForFinished(30000), "Built reusable composition executable did not finish in time");

        const QByteArray stdoutData = app.readAllStandardOutput();
        const QByteArray stderrData = app.readAllStandardError();
        QVERIFY2(app.exitStatus() == QProcess::NormalExit && app.exitCode() == 0,
                 qPrintable(QString::fromUtf8(stdoutData + stderrData)));

        const QString appOutput = QString::fromUtf8(stdoutData);
        const QString runtimeLog = QString("STDOUT:\n%1\nSTDERR:\n%2")
                                       .arg(appOutput, QString::fromUtf8(stderrData));
        QVERIFY2(appOutput.contains("Hello, DeltaQ"), qPrintable(runtimeLog));
        QVERIFY2(appOutput.contains("Length: 6"), qPrintable(runtimeLog));
    }
};

QTEST_APPLESS_MAIN(TestPreBuildProcessor)
#include "test_PreBuildProcessor.moc"
