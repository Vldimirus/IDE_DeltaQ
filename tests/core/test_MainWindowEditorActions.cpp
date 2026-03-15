#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QGraphicsSceneMouseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QSignalSpy>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QToolBar>
#include <QTextEdit>
#include <QStandardPaths>

#include "../../src/blockEditor/BlockEditorWidget.h"
#include "../../src/codegen/BuildPipeline.h"
#include "../../src/core/ActionManager.h"
#include "../../src/core/MainWindow.h"
#include "../../src/core/ProjectManager.h"
#include "../../src/core/ProjectTemplates.h"
#include "../../src/editor/CodeEditorWidget.h"
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
};

QTEST_MAIN(TestMainWindowEditorActions)
#include "test_MainWindowEditorActions.moc"
