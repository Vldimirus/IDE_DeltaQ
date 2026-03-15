#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QDockWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QLabel>
#include <QPointer>
#include <QTextEdit>
#include <functional>
#include <memory>

class QPlainTextEdit;
class QProcess;
class QTreeWidget;
class QTreeWidgetItem;

namespace DeltaQ {

class CommandBus;
class ModuleRegistry;
class GraphStore;
class UILayoutStore;
class ProjectManager;
class SessionManager;
class ActionManager;
class UndoManager;
class CodeEditorWidget;
class BlockEditorWidget;
class ModulePalette;
class UIDesignerWidget;
class LibProcessorWidget;
class ModuleManagerWidget;
class ProjectTreeView;
class BuildManager;
class LSPClient;
class DebugManager;
class GraphDebugger;
class PreBuildProcessor;
class BuildPipeline;
class CompilerDetector;
struct ProjectBuildRequest;
struct ResolvedToolchainConfig;
struct BuildGuidance;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    CommandBus *commandBus() const { return m_commandBus; }
    ModuleRegistry *moduleRegistry() const { return m_moduleRegistry; }
    ProjectManager *projectManager() const { return m_projectManager; }
    ActionManager *actionManager() const { return m_actionManager; }
    void startStartupAutomation(const QString &projectFilePath,
                                bool buildProject,
                                bool exportProject,
                                bool runProject,
                                bool quitWhenDone,
                                const QString &runStdin = {},
                                const QString &expectedRunOutput = {});

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void appendBuildOutputChunk(const QString &text);
    void addBuildDiagnosticEntry(const QString &file, int line, int column,
                                 const QString &severity, const QString &message);
    void onBuildDiagnosticActivated(QTreeWidgetItem *item, int column);
    void onNewProject();
    void onOpenProject();
    void onCloseProject();
    void onSaveFile();
    void onSettings();
    void onProjectProperties();
    void onRescanToolchains();
    void onOpenBuildDirectory();
    void onOpenDistDirectory();
    void onNewFile();
    void onBuild();
    void onExportLinuxBundle();
    void onClean();
    void onRun();
    void onDebugStart();
    void onDebugStop();
    void onToggleBreakpoint();
    void switchToCodeEditor();
    void switchToBlockEditor();
    void switchToUIDesigner();
    void switchToLibProcessor();
    void switchToModuleManager();
    void updateTitle();
    void updateStatusBar(const QString &message);
    void updateCursorPosition();
    void onFileActivated(const QString &path);
    void openModuleScratchpad(const QString &name);
    void navigateBuildOutputLine(const QString &lineText);
    void onOpenGeneratedOrigin();
    void connectBlockEditorSignals(BlockEditorWidget *editor);
    void connectUIDesignerSignals(UIDesignerWidget *designer);
    void showModuleSourcePreview(const QString &moduleId);

private:
    void setupCoreServices();
    void setupUI();
    void setupMenus();
    void setupToolBar();
    void setupStatusBar();
    void setupDocks();
    void setupConnections();
    void restoreSession();
    void saveSession();
    void updateRecentProjectsMenu();
    void updateEditorActions();
    void updateGeneratedOriginAction();
    QString normalizeBuildErrorPath(const QString &filePath) const;
    void openTextFileAtLocation(const QString &path, int line, int column);
    bool openProjectPath(const QString &path, QString *errorMessage = nullptr);
    void setBuildActionsEnabled(bool enabled);
    void runBuildPipeline(const QString &successMessage,
                          const QString &failureMessage,
                          std::function<void(bool)> completionHandler = {});
    QString rescanToolchainsAndDescribe() const;
    void presentBuildGuidance(const BuildGuidance &guidance);
    bool openProjectSubdirectory(const QString &relativePath,
                                 const QString &missingMessage);
    bool resolveCurrentProjectBuildRequest(ProjectBuildRequest *request,
                                           QString *errorMessage = nullptr,
                                           ResolvedToolchainConfig *resolvedToolchain = nullptr);
    bool exportCurrentProjectLinuxBundle(QString *errorMessage = nullptr);
    bool startProjectRun(const QString &stdinText = {}, QString *errorMessage = nullptr);
    QString resolveProjectExecutable() const;
    void finishStartupAutomation(bool success, const QString &message);

    struct StartupAutomationState {
        bool active = false;
        bool buildProject = false;
        bool exportProject = false;
        bool runProject = false;
        bool quitWhenDone = false;
        bool awaitingBuild = false;
        bool awaitingRun = false;
        QString stdinText;
        QString expectedRunOutput;
    };

    // Core services
    CommandBus *m_commandBus = nullptr;
    ModuleRegistry *m_moduleRegistry = nullptr;
    GraphStore *m_graphStore = nullptr;
    UILayoutStore *m_uiLayoutStore = nullptr;
    ProjectManager *m_projectManager = nullptr;
    SessionManager *m_sessionManager = nullptr;
    ActionManager *m_actionManager = nullptr;
    UndoManager *m_undoManager = nullptr;
    BuildManager *m_buildManager = nullptr;
    LSPClient *m_lspClient = nullptr;
    DebugManager *m_debugManager = nullptr;
    GraphDebugger *m_graphDebugger = nullptr;
    PreBuildProcessor *m_preBuildProcessor = nullptr;
    BuildPipeline *m_buildPipeline = nullptr;
    CompilerDetector *m_compilerDetector = nullptr;

    // UI
    QStackedWidget *m_centralStack = nullptr;
    CodeEditorWidget *m_codeEditor = nullptr;
    BlockEditorWidget *m_blockEditor = nullptr;
    ModulePalette *m_modulePalette = nullptr;
    UIDesignerWidget *m_uiDesigner = nullptr;
    LibProcessorWidget *m_libProcessor = nullptr;
    ModuleManagerWidget *m_moduleManager = nullptr;

    // Docks
    QDockWidget *m_projectDock = nullptr;
    ProjectTreeView *m_projectTree = nullptr;
    QDockWidget *m_outputDock = nullptr;
    QTabWidget *m_outputTabs = nullptr;

    // Menus
    QMenu *m_recentProjectsMenu = nullptr;

    // Toolbar
    QToolBar *m_mainToolBar = nullptr;

    // StatusBar widgets
    QLabel *m_cursorPosLabel = nullptr;
    QMetaObject::Connection m_cursorPositionConnection;
    QPointer<QPlainTextEdit> m_trackedCursorEditor;

    // Output widgets (ссылки для подключения BuildManager)
    QTextEdit *m_buildOutput = nullptr;
    QTreeWidget *m_buildDiagnostics = nullptr;
    QTextEdit *m_appOutput = nullptr;
    QTextEdit *m_debugConsole = nullptr;
    QPointer<QProcess> m_runProcess;
    QString m_lastRunStdout;
    QString m_lastRunStderr;
    StartupAutomationState m_startupAutomation;

    // Debug panels
    QDockWidget *m_variablesDock = nullptr;
    QDockWidget *m_callStackDock = nullptr;
};

} // namespace DeltaQ
