#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QDirIterator>
#include <QGraphicsSceneMouseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSignalSpy>
#include <QStatusBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#include <QToolBar>
#include <QTextEdit>
#include <QStandardPaths>

#include "../../src/blockEditor/BlockEditorWidget.h"
#include "../../src/codegen/BuildPipeline.h"
#include "../../src/core/ActionManager.h"
#include "../../src/core/CommandBus.h"
#include "../../src/core/MainWindow.h"
#include "../../src/core/ModuleRegistry.h"
#include "../../src/core/ProjectManager.h"
#include "../../src/core/ProjectTemplates.h"
#include "../../src/editor/CodeEditorWidget.h"
#include "../../src/editor/ModuleEditorWidget.h"
#include "../../src/lsp/LSPClient.h"
#include "../../src/uiDesigner/DesignScene.h"
#include "../../src/uiDesigner/PropertyEditor.h"
#include "../../src/uiDesigner/UIDesignerWidget.h"

#include <functional>

using namespace DeltaQ;

class TestMainWindowEditorActions : public QObject {
    Q_OBJECT

private:
    struct ManualToolchainConfig {
        QString cmakePath;
        QString cCompilerPath;
        QString cxxCompilerPath;
        QString builderPath;
        QString generator;

        bool isValid() const
        {
            return !cmakePath.isEmpty()
                && !cCompilerPath.isEmpty()
                && !cxxCompilerPath.isEmpty()
                && !builderPath.isEmpty()
                && !generator.isEmpty();
        }
    };

    class ScopedEnvVar {
    public:
        ScopedEnvVar(const char *name, const QByteArray &value)
            : m_name(name)
            , m_hadValue(qEnvironmentVariableIsSet(name))
            , m_oldValue(qgetenv(name))
        {
            qputenv(name, value);
        }

        ~ScopedEnvVar()
        {
            if (m_hadValue)
                qputenv(m_name.constData(), m_oldValue);
            else
                qunsetenv(m_name.constData());
        }

    private:
        QByteArray m_name;
        bool m_hadValue = false;
        QByteArray m_oldValue;
    };

    static QString writeFile(const QString &dirPath, const QString &fileName, const QString &content)
    {
        const QString path = dirPath + "/" + fileName;
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return {};
        file.write(content.toUtf8());
        file.close();
        return path;
    }

    // Находит checked-in testdata-файл и возвращает его абсолютный путь.
    static QString findTestDataPath(const QString &relativePath)
    {
        return QFINDTESTDATA(relativePath.toUtf8().constData());
    }

    // Рекурсивно копирует fixture-директорию в temp workspace для изолированного proof.
    static bool copyDirectory(const QString &sourceDirPath, const QString &targetDirPath)
    {
        const QDir sourceDir(sourceDirPath);
        if (!sourceDir.exists())
            return false;

        if (!QDir().mkpath(targetDirPath))
            return false;

        QDirIterator it(sourceDirPath,
                        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString sourcePath = it.next();
            const QString relativePath = sourceDir.relativeFilePath(sourcePath);
            const QString targetPath = QDir(targetDirPath).filePath(relativePath);
            const QFileInfo sourceInfo(sourcePath);
            if (sourceInfo.isDir()) {
                if (!QDir().mkpath(targetPath))
                    return false;
                continue;
            }

            if (!QDir().mkpath(QFileInfo(targetPath).absolutePath()))
                return false;
            QFile::remove(targetPath);
            if (!QFile::copy(sourcePath, targetPath))
                return false;
        }

        return true;
    }

    // Ищет строку verification result table по имени порта, а не по жёсткому индексу.
    static int findTableRowByPortName(const QTableWidget *table, const QString &portName)
    {
        if (!table)
            return -1;

        for (int row = 0; row < table->rowCount(); ++row) {
            const auto *nameItem = table->item(row, 0);
            if (nameItem && nameItem->text() == portName)
                return row;
        }
        return -1;
    }

    // Пишет минимальный fake LSP server, чтобы проверить live JSON-RPC цикл без внешнего clangd.
    static QString writeFakeLspServerScript(const QString &dirPath)
    {
        // Минимальный локальный JSON-RPC сервер даёт deterministic LSP proof без внешних зависимостей.
        return writeFile(
            dirPath,
            "fake_lsp_server.py",
            R"PY(import json
import sys

log_path = sys.argv[1]

def log_event(method, uri=""):
    with open(log_path, "a", encoding="utf-8") as fh:
        fh.write(json.dumps({"method": method, "uri": uri}) + "\n")
        fh.flush()

def send(message):
    payload = json.dumps(message, separators=(",", ":")).encode("utf-8")
    header = f"Content-Length: {len(payload)}\r\n\r\n".encode("ascii")
    sys.stdout.buffer.write(header)
    sys.stdout.buffer.write(payload)
    sys.stdout.buffer.flush()

while True:
    content_length = None
    while True:
        line = sys.stdin.buffer.readline()
        if not line:
            sys.exit(0)
        if line in (b"\r\n", b"\n"):
            break
        if line.lower().startswith(b"content-length:"):
            content_length = int(line.split(b":", 1)[1].strip())

    if content_length is None:
        continue

    body = sys.stdin.buffer.read(content_length)
    if not body:
        sys.exit(0)

    message = json.loads(body.decode("utf-8"))
    method = message.get("method", "")

    if method == "initialize":
        send({
            "jsonrpc": "2.0",
            "id": message["id"],
            "result": {
                "capabilities": {
                    "documentFormattingProvider": True
                }
            }
        })
    elif method == "shutdown":
        send({"jsonrpc": "2.0", "id": message["id"], "result": None})
    elif method == "exit":
        sys.exit(0)
    elif method == "initialized":
        log_event("initialized")
    elif method == "textDocument/didOpen":
        uri = message["params"]["textDocument"]["uri"]
        log_event(method, uri)
        send({
            "jsonrpc": "2.0",
            "method": "textDocument/publishDiagnostics",
            "params": {
                "uri": uri,
                "diagnostics": [{
                    "range": {
                        "start": {"line": 1, "character": 19},
                        "end": {"line": 1, "character": 33}
                    },
                    "severity": 1,
                    "source": "fake-clangd",
                    "message": "use of undeclared identifier 'missing_symbol'"
                }]
            }
        })
    elif method == "textDocument/didChange":
        uri = message["params"]["textDocument"]["uri"]
        log_event(method, uri)
    elif method == "textDocument/didSave":
        uri = message["params"]["textDocument"]["uri"]
        log_event(method, uri)
    elif method == "textDocument/didClose":
        uri = message["params"]["textDocument"]["uri"]
        log_event(method, uri)
    elif method == "textDocument/formatting":
        uri = message["params"]["textDocument"]["uri"]
        log_event(method, uri)
        send({
            "jsonrpc": "2.0",
            "id": message["id"],
            "result": [{
                "range": {
                    "start": {"line": 0, "character": 0},
                    "end": {"line": 0, "character": 0}
                },
                "newText": "/* formatted */\n"
            }]
        })
)PY");
    }

    // Проверяет, что fake LSP server уже записал нужное событие в свой jsonl-лог.
    static bool logContainsEvent(const QString &logPath,
                                 const QString &method,
                                 const QString &uri = QString())
    {
        QFile file(logPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;

        const QStringList lines = QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QJsonObject obj = QJsonDocument::fromJson(line.toUtf8()).object();
            if (obj["method"].toString() != method)
                continue;
            if (uri.isEmpty() || obj["uri"].toString() == uri)
                return true;
        }
        return false;
    }

    static void processUi()
    {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
    }

    static bool waitForCondition(const std::function<bool()> &predicate, int timeoutMs = 10000)
    {
        QElapsedTimer timer;
        timer.start();

        while (timer.elapsed() < timeoutMs) {
            if (predicate())
                return true;
            processUi();
            QTest::qWait(20);
        }

        processUi();
        return predicate();
    }

    static QJsonObject loadJsonObject(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};
        return QJsonDocument::fromJson(file.readAll()).object();
    }

    static QString expectedInlineDesktopInitCall(const QString &title, int width, int height,
                                                 int minWidth, int minHeight, bool resizable)
    {
        return QString("dq_ui_backend_init(&dq_ui_backend_ctx, \"%1\", %2, %3, %4, %5, %6)")
            .arg(title)
            .arg(width)
            .arg(height)
            .arg(minWidth)
            .arg(minHeight)
            .arg(resizable ? "true" : "false");
    }

    static QString findFirstExecutable(std::initializer_list<const char *> candidates)
    {
        for (const char *candidate : candidates) {
            const QString path = QStandardPaths::findExecutable(QString::fromLatin1(candidate));
            if (!path.isEmpty())
                return path;
        }
        return {};
    }

    static ManualToolchainConfig localDesktopToolchain()
    {
        ManualToolchainConfig config;
        config.cmakePath = findFirstExecutable({"cmake"});
        config.cCompilerPath = findFirstExecutable({"gcc", "clang", "cc"});
        config.cxxCompilerPath = findFirstExecutable({"g++", "clang++", "c++"});
        config.builderPath = findFirstExecutable({"ninja"});
        config.generator = "ninja";
        if (config.builderPath.isEmpty()) {
            config.builderPath = findFirstExecutable({"make"});
            config.generator = config.builderPath.isEmpty() ? QString() : QStringLiteral("unix_makefiles");
        }
        return config;
    }

    static void dragWindowResizeHandle(DesignScene *scene,
                                       const QPointF &pressPos,
                                       const QPointF &releasePos)
    {
        QVERIFY(scene != nullptr);

        QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
        pressEvent.setButton(Qt::LeftButton);
        pressEvent.setButtons(Qt::LeftButton);
        pressEvent.setScenePos(pressPos);
        pressEvent.setLastScenePos(pressPos);
        pressEvent.setButtonDownScenePos(Qt::LeftButton, pressPos);
        QApplication::sendEvent(scene, &pressEvent);

        QGraphicsSceneMouseEvent moveEvent(QEvent::GraphicsSceneMouseMove);
        moveEvent.setButton(Qt::LeftButton);
        moveEvent.setButtons(Qt::LeftButton);
        moveEvent.setScenePos(releasePos);
        moveEvent.setLastScenePos(pressPos);
        moveEvent.setButtonDownScenePos(Qt::LeftButton, pressPos);
        QApplication::sendEvent(scene, &moveEvent);

        QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
        releaseEvent.setButton(Qt::LeftButton);
        releaseEvent.setButtons(Qt::NoButton);
        releaseEvent.setScenePos(releasePos);
        releaseEvent.setLastScenePos(releasePos);
        releaseEvent.setButtonDownScenePos(Qt::LeftButton, pressPos);
        QApplication::sendEvent(scene, &releaseEvent);
    }

    static void configureManualToolchain(ProjectManager *projectManager,
                                         const ManualToolchainConfig &toolchain)
    {
        QVERIFY(projectManager != nullptr);
        QVERIFY(toolchain.isValid());

        auto &settings = projectManager->currentLocalSettings();
        settings.selectionMode = "manual";
        settings.manualOverride.enabled = true;
        settings.manualOverride.cmakePath = toolchain.cmakePath;
        settings.manualOverride.cCompilerPath = toolchain.cCompilerPath;
        settings.manualOverride.cxxCompilerPath = toolchain.cxxCompilerPath;
        settings.manualOverride.builderPath = toolchain.builderPath;
        settings.manualOverride.generator = toolchain.generator;
    }

private slots:
    void regularTextFilesEnableEditingActions()
    {
        MainWindow window;
        auto *editor = window.findChild<CodeEditorWidget *>();
        QVERIFY(editor != nullptr);

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString path = writeFile(tmpDir.path(), "manual.c", "int main(void) { return 0; }\n");
        QVERIFY(!path.isEmpty());

        editor->openFile(path);
        processUi();

        QVERIFY(window.actionManager()->saveAction()->isEnabled());
        QVERIFY(window.actionManager()->action("edit.rename")->isEnabled());
        QVERIFY(window.actionManager()->action("edit.format")->isEnabled());
        QVERIFY(!window.actionManager()->action("edit.openGeneratedOrigin")->isEnabled());
    }

    void generatedTextFilesDisableEditingActions()
    {
        MainWindow window;
        auto *editor = window.findChild<CodeEditorWidget *>();
        QVERIFY(editor != nullptr);

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString path = writeFile(
            tmpDir.path(),
            "main.c",
            "// Generated by DeltaQ IDE.\n"
            "// Source: graph 'main'\n"
            "// File role: generated C source from a DeltaQ graph.\n"
            "// Warning: this file is generated and may be overwritten.\n\n"
            "int main(void) { return 0; }\n");
        QVERIFY(!path.isEmpty());

        editor->openFile(path);
        processUi();

        QVERIFY(!window.actionManager()->saveAction()->isEnabled());
        QVERIFY(!window.actionManager()->action("edit.rename")->isEnabled());
        QVERIFY(!window.actionManager()->action("edit.format")->isEnabled());
        QVERIFY(window.actionManager()->action("edit.openGeneratedOrigin")->isEnabled());
    }

    void graphTabsStaySavableButNotTextEditable()
    {
        MainWindow window;
        auto *editor = window.findChild<CodeEditorWidget *>();
        QVERIFY(editor != nullptr);

        auto *graphEditor = new BlockEditorWidget(window.moduleRegistry(), window.commandBus());
        editor->openCustomTab(graphEditor, "demo.dqgraph", "/tmp/demo.dqgraph");
        processUi();

        QVERIFY(window.actionManager()->saveAction()->isEnabled());
        QVERIFY(!window.actionManager()->action("edit.rename")->isEnabled());
        QVERIFY(!window.actionManager()->action("edit.format")->isEnabled());
        QVERIFY(!window.actionManager()->action("edit.openGeneratedOrigin")->isEnabled());
    }

    void dqmodFilesOpenInDedicatedModuleEditorAndSaveSchemaV2()
    {
        MainWindow window;
        auto *editor = window.findChild<CodeEditorWidget *>();
        QVERIFY(editor != nullptr);

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QJsonObject moduleJson;
        moduleJson["id"] = "legacy-increment";
        moduleJson["name"] = "legacy_increment";
        moduleJson["version"] = "1.0.0";
        moduleJson["language"] = "c";
        moduleJson["description"] = "Legacy module";
        moduleJson["origin"] = "local";
        moduleJson["source"] = "src/legacy_increment.c";
        moduleJson["source_code"] =
            "int dq_legacy_increment(int value) {\n"
            "    return value + 1;\n"
            "}\n";

        QJsonObject ports;
        ports["input"] = QJsonArray{QJsonObject{{"name", "value"}, {"type", "int"}}};
        ports["output"] = QJsonArray{QJsonObject{{"name", "result"}, {"type", "int"}}};
        moduleJson["ports"] = ports;

        QJsonObject verification;
        verification["compile_status"] = "passed";
        verification["test_status"] = "passed";
        moduleJson["verification"] = verification;

        const QString modulePath = writeFile(
            tmpDir.path(),
            "legacy_increment.dqmod",
            QString::fromUtf8(QJsonDocument(moduleJson).toJson(QJsonDocument::Indented)));
        QVERIFY(!modulePath.isEmpty());

        QVERIFY(QMetaObject::invokeMethod(&window, "onFileActivated", Q_ARG(QString, modulePath)));
        QVERIFY2(waitForCondition([editor]() {
                     return qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget()) != nullptr;
                 }),
                 "Module editor tab was not opened");

        auto *moduleEditor = qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget());
        QVERIFY(moduleEditor != nullptr);
        QVERIFY(window.actionManager()->saveAction()->isEnabled());
        QVERIFY(window.actionManager()->action("edit.rename")->isEnabled());
        QVERIFY(window.actionManager()->action("edit.format")->isEnabled());

        auto *descriptionEdit = moduleEditor->findChild<QLineEdit *>(QStringLiteral("moduleStudioDescriptionEdit"));
        auto *trustStateCombo = moduleEditor->findChild<QComboBox *>(QStringLiteral("moduleStudioTrustStateCombo"));
        auto *provenanceLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioProvenanceLabel"));
        auto *contractStatus = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioContractStatusLabel"));
        auto *codeEdit = moduleEditor->findChild<QPlainTextEdit *>(QStringLiteral("moduleStudioCodeEdit"));
        auto *portTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioPortTable"));
        auto *tabWidget = editor->findChild<QTabWidget *>();
        QVERIFY(descriptionEdit != nullptr);
        QVERIFY(trustStateCombo != nullptr);
        QVERIFY(provenanceLabel != nullptr);
        QVERIFY(contractStatus != nullptr);
        QVERIFY(codeEdit != nullptr);
        QVERIFY(portTable != nullptr);
        QVERIFY(tabWidget != nullptr);

        QVERIFY(moduleEditor->lspDocumentPath().endsWith("/src/legacy_increment.c"));
        QCOMPARE(trustStateCombo->currentText(), QString("smoke_passed"));
        QVERIFY(provenanceLabel->text().contains(QString("kind=manual")));
        QVERIFY(contractStatus->text().contains(QString("matches current")));
        QCOMPARE(portTable->rowCount(), 2);

        descriptionEdit->setText("Dedicated module editor path");
        codeEdit->setFocus();
        codeEdit->moveCursor(QTextCursor::End);
        QTest::keyClick(codeEdit, Qt::Key_Return);
        QTest::keyClicks(codeEdit, "// note");
        processUi();

        QVERIFY(contractStatus->text().contains(QString("matches current")));
        QCOMPARE(portTable->rowCount(), 2);
        QVERIFY(tabWidget->tabText(tabWidget->currentIndex()).endsWith(" *"));
        QVERIFY(codeEdit->toPlainText().contains(QString("// note")));

        window.actionManager()->undoAction()->trigger();
        processUi();
        QVERIFY(!codeEdit->toPlainText().contains(QString("// note")));
        QCOMPARE(portTable->rowCount(), 2);

        window.actionManager()->redoAction()->trigger();
        processUi();
        QVERIFY(codeEdit->toPlainText().contains(QString("// note")));
        QCOMPARE(portTable->rowCount(), 2);

        window.actionManager()->saveAction()->trigger();
        processUi();

        const QJsonObject saved = loadJsonObject(modulePath);
        QCOMPARE(saved["schema_version"].toInt(), Module::SchemaVersion);
        QCOMPARE(saved["trust_state"].toString(), QString("smoke_passed"));
        QCOMPARE(saved["description"].toString(), QString("Dedicated module editor path"));
        QCOMPARE(saved["provenance"].toObject()["source_kind"].toString(), QString("manual"));
        QCOMPARE(saved["verification"].toObject()["compile_status"].toString(), QString("modified"));
        QCOMPARE(saved["verification"].toObject()["test_status"].toString(), QString("modified"));
        QCOMPARE(saved["ports"].toObject()["input"].toArray().size(), 1);
        QCOMPARE(saved["ports"].toObject()["output"].toArray().size(), 1);
        QVERIFY(!tabWidget->tabText(tabWidget->currentIndex()).endsWith(" *"));
    }

    void dqmodEditorReceivesDiagnosticsOnEmbeddedSourceSurface()
    {
        MainWindow window;
        auto *editor = window.findChild<CodeEditorWidget *>();
        QVERIFY(editor != nullptr);

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QJsonObject moduleJson;
        moduleJson["id"] = "diagnostics-module";
        moduleJson["name"] = "diagnostics_module";
        moduleJson["version"] = "1.0.0";
        moduleJson["language"] = "c";
        moduleJson["description"] = "Diagnostics bridge";
        moduleJson["origin"] = "local";
        moduleJson["source"] = "src/diagnostics_module.c";
        moduleJson["source_code"] =
            "int dq_diagnostics_module(int value) {\n"
            "    return value + missing_symbol;\n"
            "}\n";

        QJsonObject ports;
        ports["input"] = QJsonArray{QJsonObject{{"name", "value"}, {"type", "int"}}};
        ports["output"] = QJsonArray{QJsonObject{{"name", "result"}, {"type", "int"}}};
        moduleJson["ports"] = ports;

        const QString modulePath = writeFile(
            tmpDir.path(),
            "diagnostics_module.dqmod",
            QString::fromUtf8(QJsonDocument(moduleJson).toJson(QJsonDocument::Indented)));
        QVERIFY(!modulePath.isEmpty());

        QVERIFY(QMetaObject::invokeMethod(&window, "onFileActivated", Q_ARG(QString, modulePath)));
        QVERIFY2(waitForCondition([editor]() {
                     return qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget()) != nullptr;
                 }),
                 "Module editor tab was not opened");

        auto *moduleEditor = qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget());
        auto *codeEdit = moduleEditor ? moduleEditor->findChild<QPlainTextEdit *>(QStringLiteral("moduleStudioCodeEdit")) : nullptr;
        QVERIFY(moduleEditor != nullptr);
        QVERIFY(codeEdit != nullptr);
        QVERIFY(window.actionManager()->action("edit.rename")->isEnabled());
        QVERIFY(window.actionManager()->action("edit.format")->isEnabled());

        LSPDiagnostic diagnostic;
        diagnostic.range.start.line = 1;
        diagnostic.range.start.character = 19;
        diagnostic.range.end.line = 1;
        diagnostic.range.end.character = 33;
        diagnostic.severity = DiagnosticSeverity::Error;
        diagnostic.message = "use of undeclared identifier 'missing_symbol'";

        editor->onDiagnosticsReceived(
            LSPClient::pathToUri(moduleEditor->lspDocumentPath()),
            QVector<LSPDiagnostic>{diagnostic});
        processUi();

        QCOMPARE(moduleEditor->diagnostics().size(), 1);
        bool foundTooltip = false;
        for (const auto &selection : codeEdit->extraSelections()) {
            if (selection.format.toolTip() == diagnostic.message) {
                foundTooltip = true;
                break;
            }
        }
        QVERIFY(foundTooltip);
    }

    // Доказывает, что `.dqmod` surface участвует в полном live LSP lifecycle, а не только в локальном bridge.
    void dqmodEditorParticipatesInLiveLspLifecycle()
    {
        const QString pythonPath = findFirstExecutable({"python3", "python"});
        if (pythonPath.isEmpty())
            QSKIP("python3/python is required for fake LSP server test");

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString logPath = tmpDir.path() + "/fake_lsp_log.jsonl";
        const QString serverScript = writeFakeLspServerScript(tmpDir.path());
        QVERIFY(!serverScript.isEmpty());

        CommandBus commandBus;
        ModuleRegistry registry;
        CodeEditorWidget editor(&commandBus, &registry);
        editor.resize(1000, 700);
        editor.show();
        processUi();

        LSPClient client;
        QSignalSpy initializedSpy(&client, &LSPClient::initialized);
        QSignalSpy errorSpy(&client, &LSPClient::serverError);

        client.start(pythonPath, {serverScript, logPath});
        QVERIFY2(waitForCondition([&client]() { return client.isRunning(); }),
                 "Fake LSP server did not start");
        client.initialize(LSPClient::pathToUri(tmpDir.path()));
        QVERIFY2(waitForCondition([&initializedSpy]() { return initializedSpy.count() > 0; }),
                 "Fake LSP server did not initialize");
        QCOMPARE(errorSpy.count(), 0);

        editor.setLSPClient(&client);

        QJsonObject moduleJson;
        moduleJson["id"] = "live-lsp";
        moduleJson["name"] = "live_lsp";
        moduleJson["version"] = "1.0.0";
        moduleJson["language"] = "c";
        moduleJson["description"] = "Live LSP";
        moduleJson["origin"] = "local";
        moduleJson["source"] = "src/live_lsp.c";
        moduleJson["source_code"] =
            "int dq_live_lsp(int value) {\n"
            "    return value + missing_symbol;\n"
            "}\n";

        QJsonObject ports;
        ports["input"] = QJsonArray{QJsonObject{{"name", "value"}, {"type", "int"}}};
        ports["output"] = QJsonArray{QJsonObject{{"name", "result"}, {"type", "int"}}};
        moduleJson["ports"] = ports;

        const QString modulePath = writeFile(
            tmpDir.path(),
            "live_lsp.dqmod",
            QString::fromUtf8(QJsonDocument(moduleJson).toJson(QJsonDocument::Indented)));
        QVERIFY(!modulePath.isEmpty());

        auto *moduleEditor = new ModuleEditorWidget(&registry, &editor);
        QVERIFY(moduleEditor->loadFromFile(modulePath));
        const QString lspDocumentPath = moduleEditor->lspDocumentPath();
        const QString lspDocumentUri = LSPClient::pathToUri(lspDocumentPath);
        editor.openCustomTab(moduleEditor, "live_lsp.dqmod", modulePath);
        processUi();

        const bool openedWithDiagnostics = waitForCondition([&]() -> bool {
            return moduleEditor->diagnostics().size() == 1
                && logContainsEvent(logPath, "textDocument/didOpen", lspDocumentUri);
        });
        QVERIFY2(openedWithDiagnostics,
                 "Module editor did not participate in didOpen/diagnostics flow");

        QVERIFY(moduleEditor->editor()->toPlainText().contains("missing_symbol"));
        editor.formatDocument();
        const bool formattingApplied = waitForCondition([&]() -> bool {
            return moduleEditor->editor()->toPlainText().startsWith("/* formatted */\n")
                && logContainsEvent(logPath, "textDocument/formatting", lspDocumentUri)
                && logContainsEvent(logPath, "textDocument/didChange", lspDocumentUri);
        });
        QVERIFY2(formattingApplied,
                 "Module editor did not receive formatting edits through LSP");

        QVERIFY(moduleEditor->saveModule());
        const bool savedThroughLsp = waitForCondition([&]() -> bool {
            return logContainsEvent(logPath, "textDocument/didSave", lspDocumentUri);
        });
        QVERIFY2(savedThroughLsp, "Module editor save did not emit didSave");

        editor.closeAllTabs();
        const bool closedThroughLsp = waitForCondition([&]() -> bool {
            return logContainsEvent(logPath, "textDocument/didClose", lspDocumentUri);
        });
        QVERIFY2(closedThroughLsp, "Module editor close did not emit didClose");

        client.stop();
        processUi();
        QCOMPARE(errorSpy.count(), 0);
    }

    // Проверяет saved scenarios и rerun-path verification workspace прямо внутри module editor.
    void dqmodVerificationWorkspaceCanSaveAndRerunScenarios()
    {
        MainWindow window;
        auto *editor = window.findChild<CodeEditorWidget *>();
        QVERIFY(editor != nullptr);

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QJsonObject moduleJson;
        moduleJson["id"] = "verify-increment";
        moduleJson["name"] = "verify_increment";
        moduleJson["version"] = "1.0.0";
        moduleJson["language"] = "c";
        moduleJson["description"] = "Verification workspace";
        moduleJson["origin"] = "local";
        moduleJson["source"] = "src/verify_increment.c";
        moduleJson["source_code"] =
            "int dq_verify_increment(int value) {\n"
            "    return value + 1;\n"
            "}\n";

        QJsonObject ports;
        ports["input"] = QJsonArray{QJsonObject{{"name", "value"}, {"type", "int"}, {"default", "1"}}};
        ports["output"] = QJsonArray{QJsonObject{{"name", "result"}, {"type", "int"}}};
        moduleJson["ports"] = ports;

        const QString modulePath = writeFile(
            tmpDir.path(),
            "verify_increment.dqmod",
            QString::fromUtf8(QJsonDocument(moduleJson).toJson(QJsonDocument::Indented)));
        QVERIFY(!modulePath.isEmpty());

        QVERIFY(QMetaObject::invokeMethod(&window, "onFileActivated", Q_ARG(QString, modulePath)));
        QVERIFY2(waitForCondition([editor]() {
                     return qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget()) != nullptr;
                 }),
                 "Module editor tab was not opened");

        auto *moduleEditor = qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget());
        QVERIFY(moduleEditor != nullptr);

        auto *scenarioCombo = moduleEditor->findChild<QComboBox *>(QStringLiteral("moduleStudioVerificationScenarioCombo"));
        auto *scenarioNameEdit = moduleEditor->findChild<QLineEdit *>(QStringLiteral("moduleStudioScenarioNameEdit"));
        auto *addScenarioButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioAddScenarioButton"));
        auto *compileButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioCompileVerificationButton"));
        auto *runButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioRunVerificationButton"));
        auto *inputsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioVerificationInputsTable"));
        auto *expectedTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioVerificationExpectedTable"));
        auto *resultsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioVerificationResultsTable"));
        auto *statusLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioVerificationStatusLabel"));
        auto *logEdit = moduleEditor->findChild<QTextEdit *>(QStringLiteral("moduleStudioVerificationLogEdit"));
        auto *verificationSummaryLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioVerificationLabel"));
        auto *traceStatusLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioTraceStatusLabel"));
        auto *traceCurrentStepLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioTraceCurrentStepLabel"));
        auto *traceCurrentFlowLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioTraceCurrentFlowLabel"));
        auto *traceSourceLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioTraceSourceLabel"));
        auto *traceEventsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioTraceEventsTable"));
        auto *traceInputsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioTraceInputsTable"));
        auto *traceLocalsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioTraceLocalsTable"));
        auto *traceOutputsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioTraceOutputsTable"));
        auto *traceRestartButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioTraceRestartButton"));
        auto *traceStepButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioTraceStepButton"));
        auto *traceStepOverButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioTraceStepOverButton"));
        auto *traceRunToEndButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioTraceRunToEndButton"));
        auto *trustStateCombo = moduleEditor->findChild<QComboBox *>(QStringLiteral("moduleStudioTrustStateCombo"));
        QVERIFY(scenarioCombo != nullptr);
        QVERIFY(scenarioNameEdit != nullptr);
        QVERIFY(addScenarioButton != nullptr);
        QVERIFY(compileButton != nullptr);
        QVERIFY(runButton != nullptr);
        QVERIFY(inputsTable != nullptr);
        QVERIFY(expectedTable != nullptr);
        QVERIFY(resultsTable != nullptr);
        QVERIFY(statusLabel != nullptr);
        QVERIFY(logEdit != nullptr);
        QVERIFY(verificationSummaryLabel != nullptr);
        QVERIFY(traceStatusLabel != nullptr);
        QVERIFY(traceCurrentStepLabel != nullptr);
        QVERIFY(traceCurrentFlowLabel != nullptr);
        QVERIFY(traceSourceLabel != nullptr);
        QVERIFY(traceEventsTable != nullptr);
        QVERIFY(traceInputsTable != nullptr);
        QVERIFY(traceLocalsTable != nullptr);
        QVERIFY(traceOutputsTable != nullptr);
        QVERIFY(traceRestartButton != nullptr);
        QVERIFY(traceStepButton != nullptr);
        QVERIFY(traceStepOverButton != nullptr);
        QVERIFY(traceRunToEndButton != nullptr);
        QVERIFY(trustStateCombo != nullptr);

        QCOMPARE(scenarioCombo->count(), 0);
        addScenarioButton->click();
        processUi();

        QCOMPARE(scenarioCombo->count(), 1);
        QCOMPARE(inputsTable->rowCount(), 1);
        QCOMPARE(expectedTable->rowCount(), 1);

        scenarioNameEdit->setText("basic");
        inputsTable->item(0, 1)->setText("2");
        expectedTable->item(0, 1)->setText("3");
        processUi();

        compileButton->click();
        QVERIFY2(waitForCondition([statusLabel]() -> bool {
                     return statusLabel->text().contains("Compile check passed");
                 }),
                 "Compile-only verification did not pass");

        runButton->click();
        QVERIFY2(waitForCondition([statusLabel, resultsTable, trustStateCombo, verificationSummaryLabel, traceStatusLabel, traceEventsTable, traceOutputsTable]() -> bool {
                     return statusLabel->text().contains("passed")
                         && resultsTable->rowCount() == 1
                         && resultsTable->item(0, 2) != nullptr
                         && resultsTable->item(0, 2)->text() == "3"
                         && resultsTable->item(0, 3) != nullptr
                         && resultsTable->item(0, 3)->text() == "Match"
                         && verificationSummaryLabel->text().contains("Traces: 1")
                         && verificationSummaryLabel->text().contains("Trace status: captured")
                         && traceStatusLabel->text().contains("Trace captured")
                         && traceEventsTable->rowCount() == 2
                         && traceOutputsTable->rowCount() == 1
                         && traceOutputsTable->item(0, 1) != nullptr
                         && traceOutputsTable->item(0, 1)->text() == "3"
                         && trustStateCombo->currentText() == "smoke_passed";
                 }),
                 "Scenario verification did not produce expected result");

        QVERIFY(logEdit->toPlainText().contains("Verification started"));
        QVERIFY(logEdit->toPlainText().contains("Trace captured"));
        QCOMPARE(traceInputsTable->rowCount(), 1);
        QCOMPARE(traceInputsTable->item(0, 1)->text(), QString("2"));
        QCOMPARE(traceLocalsTable->rowCount(), 1);
        QCOMPARE(traceLocalsTable->item(0, 1)->text(), QString("2"));
        QCOMPARE(traceCurrentStepLabel->text(), QString("Step 2 of 2 (return)"));
        QVERIFY(traceCurrentFlowLabel->text().contains("return"));
        QVERIFY(traceSourceLabel->text().contains("return value + 1;"));
        QVERIFY(traceRestartButton->isEnabled());
        QVERIFY(!traceStepButton->isEnabled());
        QVERIFY(!traceStepOverButton->isEnabled());
        QVERIFY(!traceRunToEndButton->isEnabled());

        traceRestartButton->click();
        processUi();
        QVERIFY(traceCurrentStepLabel->text().contains("Step 1 of 2"));
        QVERIFY(traceCurrentFlowLabel->text().contains("enter"));
        QCOMPARE(traceOutputsTable->rowCount(), 0);
        QVERIFY(!traceRestartButton->isEnabled());
        QVERIFY(traceStepButton->isEnabled());
        QVERIFY(traceStepOverButton->isEnabled());
        QVERIFY(traceRunToEndButton->isEnabled());

        traceStepButton->click();
        processUi();
        QVERIFY(traceCurrentStepLabel->text().contains("Step 2 of 2"));
        QCOMPARE(traceOutputsTable->rowCount(), 1);
        QCOMPARE(traceOutputsTable->item(0, 1)->text(), QString("3"));

        traceRestartButton->click();
        processUi();
        QVERIFY(traceCurrentStepLabel->text().contains("Step 1 of 2"));

        traceStepOverButton->click();
        processUi();
        QVERIFY(traceCurrentStepLabel->text().contains("Step 2 of 2"));

        traceRestartButton->click();
        processUi();
        traceRunToEndButton->click();
        processUi();
        QVERIFY(traceCurrentStepLabel->text().contains("Step 2 of 2"));

        window.actionManager()->saveAction()->trigger();
        processUi();

        const QJsonObject saved = loadJsonObject(modulePath);
        const QJsonObject verification = saved["verification"].toObject();
        QCOMPARE(verification["compile_status"].toString(), QString("passed"));
        QCOMPARE(verification["test_status"].toString(), QString("passed"));
        QVERIFY(!verification["last_verified_at"].toString().isEmpty());
        QCOMPARE(verification["scenario_refs"].toArray().size(), 1);
        QCOMPARE(verification["scenario_refs"].toArray().at(0).toString(), QString("basic"));
        QCOMPARE(verification["scenarios"].toArray().size(), 1);
        const QJsonObject scenarioJson = verification["scenarios"].toArray().at(0).toObject();
        QCOMPARE(scenarioJson["name"].toString(), QString("basic"));
        QCOMPARE(scenarioJson["inputs"].toObject()["value"].toString(), QString("2"));
        QCOMPARE(scenarioJson["expected_outputs"].toObject()["result"].toString(), QString("3"));
        QCOMPARE(verification["trace_artifact_refs"].toArray().size(), 1);
        QCOMPARE(verification["trace_artifact_refs"].toArray().at(0).toString(),
                 QString("trace://verify-increment/scenario_1"));
        QCOMPARE(verification["trace_artifacts"].toArray().size(), 1);
        const QJsonObject traceArtifactJson = verification["trace_artifacts"].toArray().at(0).toObject();
        QCOMPARE(traceArtifactJson["scenario_id"].toString(), QString("scenario_1"));
        QCOMPARE(traceArtifactJson["status"].toString(), QString("captured"));
        QCOMPARE(traceArtifactJson["events"].toArray().size(), 2);
        QCOMPARE(traceArtifactJson["events"].toArray().at(0).toObject()["event_kind"].toString(),
                 QString("enter"));
        QCOMPARE(traceArtifactJson["events"].toArray().at(0).toObject()["variable_delta"].toObject()["value"].toString(),
                 QString("2"));
        QCOMPARE(traceArtifactJson["events"].toArray().at(1).toObject()["event_kind"].toString(),
                 QString("return"));
        QCOMPARE(traceArtifactJson["events"].toArray().at(1).toObject()["output_snapshot"].toObject()["result"].toString(),
                 QString("3"));

        editor->closeAllTabs();
        processUi();

        QVERIFY(QMetaObject::invokeMethod(&window, "onFileActivated", Q_ARG(QString, modulePath)));
        QVERIFY2(waitForCondition([editor]() {
                     return qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget()) != nullptr;
                 }),
                 "Module editor tab was not reopened");

        moduleEditor = qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget());
        QVERIFY(moduleEditor != nullptr);
        scenarioCombo = moduleEditor->findChild<QComboBox *>(QStringLiteral("moduleStudioVerificationScenarioCombo"));
        scenarioNameEdit = moduleEditor->findChild<QLineEdit *>(QStringLiteral("moduleStudioScenarioNameEdit"));
        runButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioRunVerificationButton"));
        expectedTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioVerificationExpectedTable"));
        statusLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioVerificationStatusLabel"));
        traceStatusLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioTraceStatusLabel"));
        traceCurrentStepLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioTraceCurrentStepLabel"));
        traceEventsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioTraceEventsTable"));
        traceOutputsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioTraceOutputsTable"));
        traceRestartButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioTraceRestartButton"));
        traceStepButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioTraceStepButton"));
        traceStepOverButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioTraceStepOverButton"));
        traceRunToEndButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioTraceRunToEndButton"));
        QVERIFY(scenarioCombo != nullptr);
        QVERIFY(scenarioNameEdit != nullptr);
        QVERIFY(runButton != nullptr);
        QVERIFY(expectedTable != nullptr);
        QVERIFY(statusLabel != nullptr);
        QVERIFY(traceStatusLabel != nullptr);
        QVERIFY(traceCurrentStepLabel != nullptr);
        QVERIFY(traceEventsTable != nullptr);
        QVERIFY(traceOutputsTable != nullptr);
        QVERIFY(traceRestartButton != nullptr);
        QVERIFY(traceStepButton != nullptr);
        QVERIFY(traceStepOverButton != nullptr);
        QVERIFY(traceRunToEndButton != nullptr);

        QCOMPARE(scenarioCombo->count(), 1);
        QCOMPARE(scenarioNameEdit->text(), QString("basic"));
        QCOMPARE(expectedTable->item(0, 1)->text(), QString("3"));
        QVERIFY(traceStatusLabel->text().contains("Trace captured"));
        QCOMPARE(traceEventsTable->rowCount(), 2);
        QCOMPARE(traceCurrentStepLabel->text(), QString("Step 2 of 2 (return)"));
        QCOMPARE(traceOutputsTable->rowCount(), 1);
        QCOMPARE(traceOutputsTable->item(0, 1)->text(), QString("3"));
        QVERIFY(traceRestartButton->isEnabled());

        runButton->click();
        QVERIFY2(waitForCondition([statusLabel]() -> bool {
                     return statusLabel->text().contains("passed");
                 }),
                 "Saved verification scenario did not rerun after reopen");
    }

    // Проверяет, что unsupported control-flow не симулируется молча и сохраняется как trace_unavailable.
    void dqmodVerificationWorkspaceMarksUnsupportedTraceAsUnavailable()
    {
        MainWindow window;
        auto *editor = window.findChild<CodeEditorWidget *>();
        QVERIFY(editor != nullptr);

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QJsonObject moduleJson;
        moduleJson["id"] = "verify-branch";
        moduleJson["name"] = "verify_branch";
        moduleJson["version"] = "1.0.0";
        moduleJson["language"] = "c";
        moduleJson["description"] = "Unsupported trace branch";
        moduleJson["origin"] = "local";
        moduleJson["source"] = "src/verify_branch.c";
        moduleJson["source_code"] =
            "int dq_verify_branch(int value) {\n"
            "    if (value > 0)\n"
            "        return value + 1;\n"
            "    return value;\n"
            "}\n";

        QJsonObject ports;
        ports["input"] = QJsonArray{QJsonObject{{"name", "value"}, {"type", "int"}, {"default", "1"}}};
        ports["output"] = QJsonArray{QJsonObject{{"name", "result"}, {"type", "int"}}};
        moduleJson["ports"] = ports;

        const QString modulePath = writeFile(
            tmpDir.path(),
            "verify_branch.dqmod",
            QString::fromUtf8(QJsonDocument(moduleJson).toJson(QJsonDocument::Indented)));
        QVERIFY(!modulePath.isEmpty());

        QVERIFY(QMetaObject::invokeMethod(&window, "onFileActivated", Q_ARG(QString, modulePath)));
        QVERIFY2(waitForCondition([editor]() {
                     return qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget()) != nullptr;
                 }),
                 "Module editor tab was not opened");

        auto *moduleEditor = qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget());
        QVERIFY(moduleEditor != nullptr);

        auto *scenarioNameEdit = moduleEditor->findChild<QLineEdit *>(QStringLiteral("moduleStudioScenarioNameEdit"));
        auto *addScenarioButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioAddScenarioButton"));
        auto *runButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioRunVerificationButton"));
        auto *inputsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioVerificationInputsTable"));
        auto *expectedTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioVerificationExpectedTable"));
        auto *verificationSummaryLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioVerificationLabel"));
        auto *logEdit = moduleEditor->findChild<QTextEdit *>(QStringLiteral("moduleStudioVerificationLogEdit"));
        auto *traceStatusLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioTraceStatusLabel"));
        auto *traceCurrentFlowLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioTraceCurrentFlowLabel"));
        auto *traceEventsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioTraceEventsTable"));
        auto *traceRestartButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioTraceRestartButton"));
        auto *traceStepButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioTraceStepButton"));
        auto *traceStepOverButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioTraceStepOverButton"));
        auto *traceRunToEndButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioTraceRunToEndButton"));
        QVERIFY(scenarioNameEdit != nullptr);
        QVERIFY(addScenarioButton != nullptr);
        QVERIFY(runButton != nullptr);
        QVERIFY(inputsTable != nullptr);
        QVERIFY(expectedTable != nullptr);
        QVERIFY(verificationSummaryLabel != nullptr);
        QVERIFY(logEdit != nullptr);
        QVERIFY(traceStatusLabel != nullptr);
        QVERIFY(traceCurrentFlowLabel != nullptr);
        QVERIFY(traceEventsTable != nullptr);
        QVERIFY(traceRestartButton != nullptr);
        QVERIFY(traceStepButton != nullptr);
        QVERIFY(traceStepOverButton != nullptr);
        QVERIFY(traceRunToEndButton != nullptr);

        addScenarioButton->click();
        processUi();
        scenarioNameEdit->setText("branch");
        inputsTable->item(0, 1)->setText("2");
        expectedTable->item(0, 1)->setText("3");
        processUi();

        runButton->click();
        QVERIFY2(waitForCondition([verificationSummaryLabel, logEdit, traceStatusLabel, traceCurrentFlowLabel, traceEventsTable]() -> bool {
                     return verificationSummaryLabel->text().contains("Trace status: trace_unavailable")
                         && logEdit->toPlainText().contains("Trace unavailable")
                         && traceStatusLabel->text().contains("Trace unavailable")
                         && traceCurrentFlowLabel->text().contains("if")
                         && traceEventsTable->rowCount() == 0;
                 }),
                 "Unsupported trace case was not reported as trace_unavailable");
        QVERIFY(!traceRestartButton->isEnabled());
        QVERIFY(!traceStepButton->isEnabled());
        QVERIFY(!traceStepOverButton->isEnabled());
        QVERIFY(!traceRunToEndButton->isEnabled());

        window.actionManager()->saveAction()->trigger();
        processUi();

        const QJsonObject saved = loadJsonObject(modulePath);
        const QJsonObject verification = saved["verification"].toObject();
        QCOMPARE(verification["trace_artifacts"].toArray().size(), 1);
        const QJsonObject traceArtifactJson = verification["trace_artifacts"].toArray().at(0).toObject();
        QCOMPARE(traceArtifactJson["status"].toString(), QString("trace_unavailable"));
        QVERIFY(traceArtifactJson["unavailable_reason"].toString().contains("if"));
        QVERIFY(traceArtifactJson["events"].toArray().isEmpty());
    }

    void dqmodContractEditingCanRewriteSourceSignature()
    {
        MainWindow window;
        auto *editor = window.findChild<CodeEditorWidget *>();
        QVERIFY(editor != nullptr);

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QJsonObject moduleJson;
        moduleJson["id"] = "contract-edit";
        moduleJson["name"] = "contract_edit";
        moduleJson["version"] = "1.0.0";
        moduleJson["language"] = "c";
        moduleJson["description"] = "Contract editor";
        moduleJson["origin"] = "local";
        moduleJson["source"] = "src/contract_edit.c";
        moduleJson["source_code"] =
            "int dq_contract_edit(int value) {\n"
            "    return value + 1;\n"
            "}\n";

        QJsonObject ports;
        ports["input"] = QJsonArray{QJsonObject{{"name", "value"}, {"type", "int"}}};
        ports["output"] = QJsonArray{QJsonObject{{"name", "result"}, {"type", "int"}}};
        moduleJson["ports"] = ports;

        const QString modulePath = writeFile(
            tmpDir.path(),
            "contract_edit.dqmod",
            QString::fromUtf8(QJsonDocument(moduleJson).toJson(QJsonDocument::Indented)));
        QVERIFY(!modulePath.isEmpty());

        QVERIFY(QMetaObject::invokeMethod(&window, "onFileActivated", Q_ARG(QString, modulePath)));
        QVERIFY2(waitForCondition([editor]() {
                     return qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget()) != nullptr;
                 }),
                 "Module editor tab was not opened");

        auto *moduleEditor = qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget());
        QVERIFY(moduleEditor != nullptr);

        auto *portTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioPortTable"));
        auto *codeEdit = moduleEditor->findChild<QPlainTextEdit *>(QStringLiteral("moduleStudioCodeEdit"));
        auto *contractStatus = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioContractStatusLabel"));
        auto *addInputButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioAddInputButton"));
        auto *applyContractButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioApplyContractButton"));
        QVERIFY(portTable != nullptr);
        QVERIFY(codeEdit != nullptr);
        QVERIFY(contractStatus != nullptr);
        QVERIFY(addInputButton != nullptr);
        QVERIFY(applyContractButton != nullptr);

        QCOMPARE(portTable->rowCount(), 2);
        addInputButton->click();
        processUi();
        QCOMPARE(portTable->rowCount(), 3);

        QVERIFY(portTable->item(2, 0) != nullptr);
        QVERIFY(portTable->item(2, 1) != nullptr);
        QVERIFY(portTable->item(2, 2) != nullptr);
        portTable->item(2, 0)->setText("step");
        portTable->item(2, 1)->setText("int");
        portTable->item(2, 2)->setText("Input");
        processUi();

        QVERIFY(contractStatus->text().contains(QString("differs from current source")));
        applyContractButton->click();
        processUi();

        QVERIFY(contractStatus->text().contains(QString("matches current")));
        QVERIFY(codeEdit->toPlainText().contains(QString("int dq_contract_edit(int value, int step)")));

        window.actionManager()->saveAction()->trigger();
        processUi();

        const QJsonObject saved = loadJsonObject(modulePath);
        QCOMPARE(saved["ports"].toObject()["input"].toArray().size(), 2);
        QCOMPARE(saved["ports"].toObject()["output"].toArray().size(), 1);
        QVERIFY(saved["source_code"].toString().contains(
            QString("int dq_contract_edit(int value, int step)")));
    }

    void linuxExportActionIsRegisteredAndWired()
    {
        MainWindow window;
        QAction *exportAction = window.actionManager()->action("build.exportLinuxBundle");
        QVERIFY(exportAction != nullptr);
        QCOMPARE(exportAction->text(), "Export Linux Bundle");

        exportAction->trigger();
        processUi();

        QCOMPARE(window.statusBar()->currentMessage(), "No project open");
    }

    void projectActionsAreRegisteredAndWired()
    {
        MainWindow window;

        QAction *propertiesAction = window.actionManager()->action("project.properties");
        QAction *rescanAction = window.actionManager()->action("project.rescanToolchains");
        QAction *openBuildDirAction = window.actionManager()->action("project.openBuildDirectory");
        QAction *openDistDirAction = window.actionManager()->action("project.openDistDirectory");
        QVERIFY(propertiesAction != nullptr);
        QVERIFY(rescanAction != nullptr);
        QVERIFY(openBuildDirAction != nullptr);
        QVERIFY(openDistDirAction != nullptr);
        QCOMPARE(propertiesAction->text(), "Project Properties...");
        QCOMPARE(rescanAction->text(), "Rescan Toolchains");
        QCOMPARE(openBuildDirAction->text(), "Open Build Directory");
        QCOMPARE(openDistDirAction->text(), "Open Dist Directory");

        propertiesAction->trigger();
        processUi();
        QCOMPARE(window.statusBar()->currentMessage(), "No project open");

        rescanAction->trigger();
        processUi();
        QVERIFY(window.statusBar()->currentMessage().startsWith("Toolchain scan completed: "));

        openBuildDirAction->trigger();
        processUi();
        QCOMPARE(window.statusBar()->currentMessage(), "No project open");

        openDistDirAction->trigger();
        processUi();
        QCOMPARE(window.statusBar()->currentMessage(), "No project open");
    }

    void toolbarUsesIconsAndHoverDescriptions()
    {
        MainWindow window;

        auto *toolBar = window.findChild<QToolBar *>("mainToolBar");
        QVERIFY(toolBar != nullptr);
        QCOMPARE(toolBar->toolButtonStyle(), Qt::ToolButtonIconOnly);

        QAction *propertiesAction = window.actionManager()->action("project.properties");
        QAction *buildAction = window.actionManager()->action("build.build");
        QVERIFY(propertiesAction != nullptr);
        QVERIFY(buildAction != nullptr);
        QVERIFY(!propertiesAction->icon().isNull());
        QVERIFY(!propertiesAction->toolTip().isEmpty());
        QVERIFY(!propertiesAction->statusTip().isEmpty());
        QVERIFY(!buildAction->icon().isNull());
        QVERIFY(!buildAction->toolTip().isEmpty());
        QVERIFY(!buildAction->statusTip().isEmpty());
    }

    void desktopBaselineProjectSupportsOpenSaveReopenBuildRun()
    {
        const ManualToolchainConfig toolchain = localDesktopToolchain();
        QVERIFY2(toolchain.isValid(), "Desktop MainWindow integration test requires cmake, C/C++ compilers, and ninja/make");

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString projectDir = tmpDir.path() + "/Stage1DesktopBaseline";
        QString error;
        QVERIFY2(ProjectTemplates::generate("desktop", projectDir, "Stage1DesktopBaseline", &error),
                 qPrintable(error));

        MainWindow window;
        window.show();
        processUi();

        const QString projectFilePath = projectDir + "/Stage1DesktopBaseline.dqproj";
        window.startStartupAutomation(projectFilePath, false, false, false, false);
        QVERIFY2(waitForCondition([&window]() {
                     return window.projectManager()->isProjectOpen();
                 }),
                 "Project was not opened by startup automation");
        QCOMPARE(window.projectManager()->currentProject().name, QString("Stage1DesktopBaseline"));

        configureManualToolchain(window.projectManager(), toolchain);

        auto *editor = window.findChild<CodeEditorWidget *>();
        QVERIFY(editor != nullptr);

        const QString layoutPath = projectDir + "/ui/window1.dqui";
        QVERIFY(QMetaObject::invokeMethod(&window, "onFileActivated", Q_ARG(QString, layoutPath)));
        QVERIFY2(waitForCondition([editor]() {
                     return qobject_cast<UIDesignerWidget *>(editor->currentCustomTabWidget()) != nullptr;
                 }),
                 "UI designer tab was not opened");

        auto *designer = qobject_cast<UIDesignerWidget *>(editor->currentCustomTabWidget());
        QVERIFY(designer != nullptr);
        auto *propertyEditor = designer->findChild<PropertyEditor *>();
        QVERIFY(propertyEditor != nullptr);

        designer->scene()->selectWindow();
        propertyEditor->setWindowProperties(designer->scene());
        processUi();

        auto *titleEdit = propertyEditor->findChild<QLineEdit *>(QStringLiteral("windowTitleEdit"));
        auto *widthSpin = propertyEditor->findChild<QDoubleSpinBox *>(QStringLiteral("windowWidthSpinBox"));
        auto *heightSpin = propertyEditor->findChild<QDoubleSpinBox *>(QStringLiteral("windowHeightSpinBox"));
        auto *minWidthSpin = propertyEditor->findChild<QDoubleSpinBox *>(QStringLiteral("windowMinWidthSpinBox"));
        auto *minHeightSpin = propertyEditor->findChild<QDoubleSpinBox *>(QStringLiteral("windowMinHeightSpinBox"));
        auto *resizableCheck = propertyEditor->findChild<QCheckBox *>(QStringLiteral("windowResizableCheckBox"));
        QVERIFY(titleEdit != nullptr);
        QVERIFY(widthSpin != nullptr);
        QVERIFY(heightSpin != nullptr);
        QVERIFY(minWidthSpin != nullptr);
        QVERIFY(minHeightSpin != nullptr);
        QVERIFY(resizableCheck != nullptr);

        const QString title = "Stage 1 Baseline Window";
        const int minWidth = 520;
        const int minHeight = 400;
        const int width = 700;
        const int height = 520;

        titleEdit->setText(title);
        QVERIFY(QMetaObject::invokeMethod(titleEdit, "editingFinished"));
        minWidthSpin->setValue(minWidth);
        minHeightSpin->setValue(minHeight);
        widthSpin->setValue(width);
        heightSpin->setValue(height);
        resizableCheck->setChecked(false);
        processUi();

        QCOMPARE(designer->scene()->windowTitle(), title);
        QCOMPARE(designer->scene()->windowMinimumSize(), QSizeF(minWidth, minHeight));
        QCOMPARE(designer->scene()->windowRect(), QRectF(0, 0, width, height));
        QCOMPARE(designer->scene()->windowResizable(), false);

        window.actionManager()->saveAction()->trigger();
        processUi();

        const QJsonObject layout = loadJsonObject(layoutPath);
        const QJsonObject layoutWindow = layout.value("window").toObject();
        const QJsonObject properties = layoutWindow.value("properties").toObject();
        QCOMPARE(layoutWindow.value("width").toDouble(), static_cast<double>(width));
        QCOMPARE(layoutWindow.value("height").toDouble(), static_cast<double>(height));
        QCOMPARE(properties.value("title").toString(), title);
        QCOMPARE(properties.value("min_width").toInt(), minWidth);
        QCOMPARE(properties.value("min_height").toInt(), minHeight);
        QCOMPARE(properties.value("resizable").toBool(), false);

        editor->closeAllTabs();
        processUi();
        QVERIFY(QMetaObject::invokeMethod(&window, "onFileActivated", Q_ARG(QString, layoutPath)));
        QVERIFY2(waitForCondition([editor]() {
                     return qobject_cast<UIDesignerWidget *>(editor->currentCustomTabWidget()) != nullptr;
                 }),
                 "UI designer tab did not reopen after save");

        auto *reopenedDesigner = qobject_cast<UIDesignerWidget *>(editor->currentCustomTabWidget());
        QVERIFY(reopenedDesigner != nullptr);
        QCOMPARE(reopenedDesigner->scene()->windowTitle(), title);
        QCOMPARE(reopenedDesigner->scene()->windowMinimumSize(), QSizeF(minWidth, minHeight));
        QCOMPARE(reopenedDesigner->scene()->windowRect(), QRectF(0, 0, width, height));
        QCOMPARE(reopenedDesigner->scene()->windowResizable(), false);

        auto *buildPipeline = window.findChild<BuildPipeline *>();
        auto *buildOutput = window.findChild<QTextEdit *>(QStringLiteral("buildOutputView"));
        QVERIFY(buildPipeline != nullptr);
        QVERIFY(buildOutput != nullptr);

        QSignalSpy buildFinished(buildPipeline, &BuildPipeline::pipelineFinished);
        window.actionManager()->buildAction()->trigger();
        QVERIFY2(buildFinished.wait(120000), qPrintable(buildOutput->toPlainText()));
        QCOMPARE(buildFinished.count(), 1);
        QVERIFY2(buildFinished.takeFirst().value(0).toBool(), qPrintable(buildOutput->toPlainText()));

        QFile mainFile(projectDir + "/src/main.c");
        QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString mainCode = QString::fromUtf8(mainFile.readAll());
        QVERIFY(mainCode.contains(expectedInlineDesktopInitCall(title,
                                                                width, height,
                                                                minWidth, minHeight,
                                                                false)));

        auto *outputDock = window.findChild<QDockWidget *>(QStringLiteral("outputDock"));
        QVERIFY(outputDock != nullptr);
        auto *outputTabs = outputDock->findChild<QTabWidget *>();
        QVERIFY(outputTabs != nullptr);
        auto *appOutput = qobject_cast<QTextEdit *>(outputTabs->widget(2));
        QVERIFY(appOutput != nullptr);

        const ScopedEnvVar videoDriver("SDL_VIDEODRIVER", "dummy");
        const ScopedEnvVar renderDriver("SDL_RENDER_DRIVER", "software");
        const ScopedEnvVar autoclose("DQ_DESKTOP_TEMPLATE_AUTOCLOSE_MS", "1500");

        window.actionManager()->runAction()->trigger();
        QVERIFY2(waitForCondition([appOutput]() {
                     return appOutput->toPlainText().contains("=== Process exited (code: 0) ===");
                 }, 20000),
                 qPrintable(appOutput->toPlainText()));
    }

    void desktopBaselineMouseResizePathSurvivesSaveBuildRun()
    {
        const ManualToolchainConfig toolchain = localDesktopToolchain();
        QVERIFY2(toolchain.isValid(), "Desktop MainWindow integration test requires cmake, C/C++ compilers, and ninja/make");

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString projectDir = tmpDir.path() + "/Stage1DesktopMouseResize";
        QString error;
        QVERIFY2(ProjectTemplates::generate("desktop", projectDir, "Stage1DesktopMouseResize", &error),
                 qPrintable(error));

        MainWindow window;
        window.show();
        processUi();

        const QString projectFilePath = projectDir + "/Stage1DesktopMouseResize.dqproj";
        window.startStartupAutomation(projectFilePath, false, false, false, false);
        QVERIFY2(waitForCondition([&window]() {
                     return window.projectManager()->isProjectOpen();
                 }),
                 "Project was not opened by startup automation");

        configureManualToolchain(window.projectManager(), toolchain);

        auto *editor = window.findChild<CodeEditorWidget *>();
        QVERIFY(editor != nullptr);

        const QString layoutPath = projectDir + "/ui/window1.dqui";
        QVERIFY(QMetaObject::invokeMethod(&window, "onFileActivated", Q_ARG(QString, layoutPath)));
        QVERIFY2(waitForCondition([editor]() {
                     return qobject_cast<UIDesignerWidget *>(editor->currentCustomTabWidget()) != nullptr;
                 }),
                 "UI designer tab was not opened");

        auto *designer = qobject_cast<UIDesignerWidget *>(editor->currentCustomTabWidget());
        QVERIFY(designer != nullptr);

        const QRectF initialRect = designer->scene()->windowRect();
        const QPointF pressPos = designer->scene()->windowFrameRect().bottomRight();
        const QPointF releasePos = pressPos + QPointF(80, 60);
        dragWindowResizeHandle(designer->scene(), pressPos, releasePos);
        processUi();

        const QRectF expectedRect(initialRect.x(), initialRect.y(),
                                  initialRect.width() + 80.0,
                                  initialRect.height() + 60.0);
        QCOMPARE(designer->scene()->windowRect(), expectedRect);

        window.actionManager()->saveAction()->trigger();
        processUi();

        const QJsonObject layout = loadJsonObject(layoutPath);
        const QJsonObject layoutWindow = layout.value("window").toObject();
        const QJsonObject properties = layoutWindow.value("properties").toObject();
        QCOMPARE(layoutWindow.value("width").toDouble(), expectedRect.width());
        QCOMPARE(layoutWindow.value("height").toDouble(), expectedRect.height());
        QCOMPARE(properties.value("min_width").toInt(), 480);
        QCOMPARE(properties.value("min_height").toInt(), 360);
        QCOMPARE(properties.value("resizable").toBool(), true);

        editor->closeAllTabs();
        processUi();
        QVERIFY(QMetaObject::invokeMethod(&window, "onFileActivated", Q_ARG(QString, layoutPath)));
        QVERIFY2(waitForCondition([editor]() {
                     return qobject_cast<UIDesignerWidget *>(editor->currentCustomTabWidget()) != nullptr;
                 }),
                 "UI designer tab did not reopen after save");

        auto *reopenedDesigner = qobject_cast<UIDesignerWidget *>(editor->currentCustomTabWidget());
        QVERIFY(reopenedDesigner != nullptr);
        QCOMPARE(reopenedDesigner->scene()->windowRect(), expectedRect);
        QCOMPARE(reopenedDesigner->scene()->windowMinimumSize(), QSizeF(480, 360));
        QCOMPARE(reopenedDesigner->scene()->windowResizable(), true);

        auto *buildPipeline = window.findChild<BuildPipeline *>();
        auto *buildOutput = window.findChild<QTextEdit *>(QStringLiteral("buildOutputView"));
        QVERIFY(buildPipeline != nullptr);
        QVERIFY(buildOutput != nullptr);

        QSignalSpy buildFinished(buildPipeline, &BuildPipeline::pipelineFinished);
        window.actionManager()->buildAction()->trigger();
        QVERIFY2(buildFinished.wait(120000), qPrintable(buildOutput->toPlainText()));
        QCOMPARE(buildFinished.count(), 1);
        QVERIFY2(buildFinished.takeFirst().value(0).toBool(), qPrintable(buildOutput->toPlainText()));

        QFile mainFile(projectDir + "/src/main.c");
        QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString mainCode = QString::fromUtf8(mainFile.readAll());
        QVERIFY(mainCode.contains(expectedInlineDesktopInitCall("Desktop UI Baseline",
                                                                static_cast<int>(expectedRect.width()),
                                                                static_cast<int>(expectedRect.height()),
                                                                480, 360,
                                                                true)));

        auto *outputDock = window.findChild<QDockWidget *>(QStringLiteral("outputDock"));
        QVERIFY(outputDock != nullptr);
        auto *outputTabs = outputDock->findChild<QTabWidget *>();
        QVERIFY(outputTabs != nullptr);
        auto *appOutput = qobject_cast<QTextEdit *>(outputTabs->widget(2));
        QVERIFY(appOutput != nullptr);

        const ScopedEnvVar videoDriver("SDL_VIDEODRIVER", "dummy");
        const ScopedEnvVar renderDriver("SDL_RENDER_DRIVER", "software");
        const ScopedEnvVar autoclose("DQ_DESKTOP_TEMPLATE_AUTOCLOSE_MS", "1500");

        window.actionManager()->runAction()->trigger();
        QVERIFY2(waitForCondition([appOutput]() {
                     return appOutput->toPlainText().contains("=== Process exited (code: 0) ===");
                 }, 20000),
                 qPrintable(appOutput->toPlainText()));
    }

    // Проверяет, что Module Studio реально умеет прогонять saved scenario для imported SQLite adapter-а.
    void fragmentScratchpadCanVerifyTraceAndPromoteToModule()
    {
        const ScopedEnvVar disableLsp("DQ_DISABLE_AUTOSTART_LSP", "1");
        MainWindow window;
        auto *editor = window.findChild<CodeEditorWidget *>();
        QVERIFY(editor != nullptr);

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString projectDir = tmpDir.path() + "/ScratchpadProject";
        QVERIFY(window.projectManager()->createProject("ScratchpadProject", projectDir));

        const QString promotedModulePath = projectDir + "/dqmods/fragment_playground.dqmod";
        QVERIFY(!QFile::exists(promotedModulePath));
        QVERIFY(window.moduleRegistry()->findModuleByName("fragment_playground") == nullptr);

        QVERIFY(QMetaObject::invokeMethod(&window, "openModuleScratchpad", Q_ARG(QString, QString("fragment_playground"))));
        QVERIFY2(waitForCondition([editor]() {
                     auto *moduleEditor = qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget());
                     return moduleEditor != nullptr && moduleEditor->isScratchpadMode();
                 }),
                 "Fragment scratchpad tab was not opened");

        auto *moduleEditor = qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget());
        QVERIFY(moduleEditor != nullptr);
        QVERIFY(moduleEditor->isScratchpadMode());
        QVERIFY(moduleEditor->tabTitle().contains("[scratch]"));
        QCOMPARE(moduleEditor->filePath(), promotedModulePath);

        auto *scenarioCombo = moduleEditor->findChild<QComboBox *>(QStringLiteral("moduleStudioVerificationScenarioCombo"));
        auto *runButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioRunVerificationButton"));
        auto *resultsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioVerificationResultsTable"));
        auto *statusLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioVerificationStatusLabel"));
        auto *traceStatusLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioTraceStatusLabel"));
        auto *traceEventsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioTraceEventsTable"));
        auto *traceOutputsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioTraceOutputsTable"));
        QVERIFY(scenarioCombo != nullptr);
        QVERIFY(runButton != nullptr);
        QVERIFY(resultsTable != nullptr);
        QVERIFY(statusLabel != nullptr);
        QVERIFY(traceStatusLabel != nullptr);
        QVERIFY(traceEventsTable != nullptr);
        QVERIFY(traceOutputsTable != nullptr);

        QCOMPARE(scenarioCombo->count(), 1);
        QCOMPARE(scenarioCombo->currentText(), QString("quick_check"));

        runButton->click();
        QVERIFY2(waitForCondition([statusLabel, traceStatusLabel, traceEventsTable, traceOutputsTable, resultsTable]() {
                     const int resultRow = findTableRowByPortName(resultsTable, QStringLiteral("result"));
                     return statusLabel->text().contains("passed")
                         && traceStatusLabel->text().contains("Trace captured")
                         && traceEventsTable->rowCount() == 2
                         && traceOutputsTable->rowCount() == 1
                         && traceOutputsTable->item(0, 1) != nullptr
                         && traceOutputsTable->item(0, 1)->text() == QStringLiteral("3")
                         && resultRow >= 0
                         && resultsTable->item(resultRow, 3) != nullptr
                         && resultsTable->item(resultRow, 3)->text() == QStringLiteral("Match");
                 }),
                 "Fragment scratchpad did not pass verification and trace");

        QVERIFY(window.moduleRegistry()->findModuleByName("fragment_playground") == nullptr);

        window.actionManager()->saveAction()->trigger();
        processUi();

        QVERIFY(QFile::exists(promotedModulePath));
        QVERIFY(window.moduleRegistry()->findModuleByName("fragment_playground") != nullptr);
        QVERIFY(!moduleEditor->isScratchpadMode());
        QVERIFY(!moduleEditor->tabTitle().contains("[scratch]"));

        auto *tabWidget = editor->findChild<QTabWidget *>();
        QVERIFY(tabWidget != nullptr);
        editor->closeTab(tabWidget->currentIndex());
        processUi();

        QVERIFY(QMetaObject::invokeMethod(&window, "onFileActivated", Q_ARG(QString, promotedModulePath)));
        QVERIFY2(waitForCondition([editor]() {
                     return qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget()) != nullptr;
                 }),
                 "Promoted scratchpad module tab was not reopened");

        moduleEditor = qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget());
        QVERIFY(moduleEditor != nullptr);
        QVERIFY(!moduleEditor->isScratchpadMode());
        scenarioCombo = moduleEditor->findChild<QComboBox *>(QStringLiteral("moduleStudioVerificationScenarioCombo"));
        traceStatusLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioTraceStatusLabel"));
        traceEventsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioTraceEventsTable"));
        QVERIFY(scenarioCombo != nullptr);
        QVERIFY(traceStatusLabel != nullptr);
        QVERIFY(traceEventsTable != nullptr);
        QCOMPARE(scenarioCombo->count(), 1);
        QCOMPARE(scenarioCombo->currentText(), QString("quick_check"));
        QVERIFY(traceStatusLabel->text().contains("Trace captured"));
        QCOMPARE(traceEventsTable->rowCount(), 2);

        QVERIFY(window.projectManager()->closeProject());
        processUi();
    }

    // Проверяет, что Module Studio реально умеет прогонять saved scenario для imported SQLite adapter-а.
    void sqliteImportedModuleCanRunSavedVerificationScenario()
    {
        MainWindow window;
        auto *editor = window.findChild<CodeEditorWidget *>();
        QVERIFY(editor != nullptr);

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString packManifestPath = findTestDataPath("../../modules/sqlite_curated/pack.json");
        QVERIFY2(!packManifestPath.isEmpty(), "sqlite_curated pack.json was not found via QFINDTESTDATA");
        const QString sourcePackRoot = QFileInfo(packManifestPath).absolutePath();
        const QString copiedPackRoot = tmpDir.path() + "/sqlite_curated";
        QVERIFY(copyDirectory(sourcePackRoot, copiedPackRoot));

        const QString modulePath = copiedPackRoot + "/curated/sqlite_exec_path.dqmod";
        QVERIFY(QFile::exists(modulePath));

        QVERIFY(QMetaObject::invokeMethod(&window, "onFileActivated", Q_ARG(QString, modulePath)));
        QVERIFY2(waitForCondition([editor]() {
                     return qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget()) != nullptr;
                 }),
                 "SQLite module editor tab was not opened");

        auto *moduleEditor = qobject_cast<ModuleEditorWidget *>(editor->currentCustomTabWidget());
        QVERIFY(moduleEditor != nullptr);

        auto *scenarioCombo = moduleEditor->findChild<QComboBox *>(QStringLiteral("moduleStudioVerificationScenarioCombo"));
        auto *compileButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioCompileVerificationButton"));
        auto *runButton = moduleEditor->findChild<QPushButton *>(QStringLiteral("moduleStudioRunVerificationButton"));
        auto *resultsTable = moduleEditor->findChild<QTableWidget *>(QStringLiteral("moduleStudioVerificationResultsTable"));
        auto *statusLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioVerificationStatusLabel"));
        auto *summaryLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioVerificationLabel"));
        auto *traceStatusLabel = moduleEditor->findChild<QLabel *>(QStringLiteral("moduleStudioTraceStatusLabel"));
        auto *trustStateCombo = moduleEditor->findChild<QComboBox *>(QStringLiteral("moduleStudioTrustStateCombo"));
        QVERIFY(scenarioCombo != nullptr);
        QVERIFY(compileButton != nullptr);
        QVERIFY(runButton != nullptr);
        QVERIFY(resultsTable != nullptr);
        QVERIFY(statusLabel != nullptr);
        QVERIFY(summaryLabel != nullptr);
        QVERIFY(traceStatusLabel != nullptr);
        QVERIFY(trustStateCombo != nullptr);

        QCOMPARE(scenarioCombo->count(), 1);
        QCOMPARE(scenarioCombo->currentText(), QString("create_table_smoke"));

        compileButton->click();
        QVERIFY2(waitForCondition([statusLabel]() {
                     return statusLabel->text().contains("Compile check passed");
                 }),
                 "SQLite imported module compile-only verification did not pass");

        runButton->click();
        QVERIFY2(waitForCondition([statusLabel, resultsTable]() {
                     const int statusRow = findTableRowByPortName(resultsTable, QStringLiteral("status"));
                     return statusLabel->text().contains("passed")
                         && statusRow >= 0
                         && resultsTable->item(statusRow, 2) != nullptr
                         && resultsTable->item(statusRow, 2)->text() == QStringLiteral("0")
                         && resultsTable->item(statusRow, 3) != nullptr
                         && resultsTable->item(statusRow, 3)->text() == QStringLiteral("Match");
                 }),
                 "SQLite imported module scenario did not pass inside Module Studio");

        QVERIFY(summaryLabel->text().contains("Compile: passed"));
        QVERIFY(summaryLabel->text().contains("Test: passed"));
        QVERIFY(traceStatusLabel->text().contains("Trace unavailable"));
        QCOMPARE(trustStateCombo->currentText(), QString("curated"));

        window.actionManager()->saveAction()->trigger();
        processUi();

        const QJsonObject saved = loadJsonObject(modulePath);
        const QJsonObject verification = saved["verification"].toObject();
        QVERIFY(!verification["last_verified_at"].toString().isEmpty());
        QCOMPARE(verification["scenarios"].toArray().size(), 1);
        const QJsonObject savedScenario = verification["scenarios"].toArray().at(0).toObject();
        QCOMPARE(savedScenario["name"].toString(), QString("create_table_smoke"));
        QVERIFY(!savedScenario["inputs"].toObject().contains("flow_in"));
        QVERIFY(!savedScenario["expected_outputs"].toObject().contains("flow_out"));
        QCOMPARE(verification["test_status"].toString(), QString("passed"));
        const QString savedSource = saved["source_code"].toString();
        QVERIFY(savedSource.contains("int dq_sqlite_exec_path("));
        QVERIFY(savedSource.contains("db_path"));
        QVERIFY(savedSource.contains("sql"));
        QVERIFY(!savedSource.contains("flow_in"));
        QVERIFY(!savedSource.contains("flow_out"));

        const QJsonObject ports = saved["ports"].toObject();
        const QJsonArray inputPorts = ports["input"].toArray();
        const QJsonArray outputPorts = ports["output"].toArray();
        QVERIFY(!inputPorts.isEmpty());
        QVERIFY(!outputPorts.isEmpty());
        QCOMPARE(inputPorts.at(0).toObject()["name"].toString(), QString("flow_in"));
        QCOMPARE(inputPorts.at(0).toObject()["kind"].toString(), QString("execution"));
        QCOMPARE(outputPorts.at(0).toObject()["name"].toString(), QString("flow_out"));
        QCOMPARE(outputPorts.at(0).toObject()["kind"].toString(), QString("execution"));
    }
};

QTEST_MAIN(TestMainWindowEditorActions)
#include "test_MainWindowEditorActions.moc"
