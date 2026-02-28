#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QDockWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QLabel>
#include <QTextEdit>
#include <memory>

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
class UIDesignerWidget;
class LibProcessorWidget;
class ProjectTreeView;
class BuildManager;
class LSPClient;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    CommandBus *commandBus() const { return m_commandBus; }
    ModuleRegistry *moduleRegistry() const { return m_moduleRegistry; }
    ProjectManager *projectManager() const { return m_projectManager; }
    ActionManager *actionManager() const { return m_actionManager; }

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveFile();
    void onBuild();
    void onClean();
    void onRun();
    void switchToCodeEditor();
    void switchToBlockEditor();
    void switchToUIDesigner();
    void switchToLibProcessor();
    void updateTitle();
    void updateStatusBar(const QString &message);
    void updateCursorPosition();

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

    // UI
    QStackedWidget *m_centralStack = nullptr;
    CodeEditorWidget *m_codeEditor = nullptr;
    BlockEditorWidget *m_blockEditor = nullptr;
    UIDesignerWidget *m_uiDesigner = nullptr;
    LibProcessorWidget *m_libProcessor = nullptr;

    // Docks
    QDockWidget *m_projectDock = nullptr;
    ProjectTreeView *m_projectTree = nullptr;
    QDockWidget *m_outputDock = nullptr;
    QTabWidget *m_outputTabs = nullptr;

    // Toolbar
    QToolBar *m_mainToolBar = nullptr;

    // StatusBar widgets
    QLabel *m_cursorPosLabel = nullptr;

    // Output widgets (ссылки для подключения BuildManager)
    QTextEdit *m_buildOutput = nullptr;
    QTextEdit *m_appOutput = nullptr;
};

} // namespace DeltaQ
