#include "MainWindow.h"
#include "CommandBus.h"
#include "ModuleRegistry.h"
#include "GraphStore.h"
#include "UILayoutStore.h"
#include "ProjectManager.h"
#include "SessionManager.h"
#include "ActionManager.h"
#include "UndoManager.h"
#include "NewProjectWizard.h"
#include "NewFileDialog.h"
#include "SettingsDialog.h"
#include "ProjectTemplates.h"

#include "../editor/CodeEditorWidget.h"
#include "../editor/CodeEditorTab.h"
#include "../editor/ProjectTreeView.h"
#include "../editor/BuildManager.h"
#include "../editor/CompilerOutputParser.h"
#include "../lsp/LSPClient.h"
#include "../lsp/LSPTypes.h"
#include "../debug/DebugManager.h"
#include "../debug/DebugTypes.h"
#include "../blockEditor/BlockEditorWidget.h"
#include "../blockEditor/BlockScene.h"
#include "../blockEditor/ModulePalette.h"
#include "../blockEditor/GraphCommands.h"
#include "../blockEditor/NodeItem.h"
#include "../blockEditor/GraphDebugger.h"
#include "../codegen/GraphCompiler.h"
#include "../codegen/PreBuildProcessor.h"
#include "../codegen/BuildPipeline.h"
#include "../codegen/CompilerDetector.h"
#include "../uiDesigner/UIDesignerWidget.h"
#include "../uiDesigner/UIModuleFactory.h"
// StandardLibrary модули загружаются из <app_dir>/modules/ (копируются CMake при сборке)
#include "../uiDesigner/DesignScene.h"
#include "../uiDesigner/WidgetItem.h"
#include "../uiDesigner/UICommands.h"
#include "../uiDesigner/SDL2CodeGenerator.h"
#include "../uiDesigner/UIPreview.h"
#include "../libProcessor/LibProcessorWidget.h"
#include "../editor/ModuleManagerWidget.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QInputDialog>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QActionGroup>
#include <QMouseEvent>
#include <QTextBlock>
#include <QTextCursor>
#include <QTreeWidget>
#include <QHeaderView>

namespace DeltaQ {

namespace {

enum BuildDiagnosticRole {
    ResolvedPathRole = Qt::UserRole + 1,
    OriginPathRole,
    LineRole,
    ColumnRole
};

// Разбирает одну строку build output и возвращает структурированную ошибку компилятора.
bool parseBuildOutputLine(const QString &lineText, CompilerError *error)
{
    CompilerOutputParser parser;
    const QString trimmedLine = lineText.trimmed();
    if (!parser.parseLine(trimmedLine) || parser.errors().isEmpty())
        return false;

    if (error)
        *error = parser.errors().first();
    return true;
}

// Возвращает формат для строк build output, по которым можно перейти к ошибке.
QTextCharFormat buildOutputFormatForLine(const QString &lineText)
{
    QTextCharFormat format;
    if (!parseBuildOutputLine(lineText, nullptr))
        return format;

    format.setForeground(QColor(0, 102, 204));
    format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    return format;
}

// Формирует короткое описание позиции ошибки для списка diagnostics.
QString buildDiagnosticLocationText(const QString &filePath, int line, int column)
{
    QString text = QFileInfo(filePath).fileName();
    if (line > 0)
        text += QObject::tr(":%1").arg(line);
    if (column > 0)
        text += QObject::tr(":%1").arg(column);
    return text;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupCoreServices();
    setupUI();
    setupMenus();
    setupToolBar();
    setupStatusBar();
    setupDocks();
    setupConnections();
    restoreSession();

    updateTitle();
    resize(1280, 800);
}

MainWindow::~MainWindow()
{
    saveSession();
}

void MainWindow::setupCoreServices()
{
    m_commandBus = new CommandBus(this);
    m_moduleRegistry = new ModuleRegistry(this);
    m_graphStore = new GraphStore(this);
    // Реестр модулей должен видеть графы, чтобы корректно оценивать составные модули.
    m_moduleRegistry->setGraphStore(m_graphStore);
    m_uiLayoutStore = new UILayoutStore(this);
    m_projectManager = new ProjectManager(m_moduleRegistry, m_graphStore, m_uiLayoutStore, this);
    m_sessionManager = new SessionManager(this);
    m_actionManager = new ActionManager(this);
    m_undoManager = new UndoManager(m_commandBus, this);

    m_buildManager = new BuildManager(this);
    m_buildManager->setModuleRegistry(m_moduleRegistry);
    m_buildManager->setGraphStore(m_graphStore);

    // Pre-build процессор и pipeline сборки
    m_preBuildProcessor = new PreBuildProcessor(m_moduleRegistry, m_graphStore, m_uiLayoutStore, this);
    m_buildPipeline = new BuildPipeline(m_preBuildProcessor, m_buildManager, this);

    // Автоопределение компилятора
    m_compilerDetector = new CompilerDetector(this);
    m_compilerDetector->detect();

    // LSP-клиент
    m_lspClient = new LSPClient(this);

    // Отладчик
    m_debugManager = new DebugManager(this);

    m_actionManager->setupStandardActions();

    // GraphDebugger создаётся после setupUI, инициализируем nullptr
    m_graphDebugger = nullptr;

    // 1. Убедиться, что директория модулей существует
    m_sessionManager->ensureGlobalDirs();

    // 2. Регистрация UI-модулей (фиксированные виджеты)
    UIModuleFactory::registerAll(m_moduleRegistry);

    // 3. Загрузка модулей из <app_dir>/modules/ (core + расширения)
    m_moduleRegistry->loadGlobalModules(m_sessionManager->globalModulesDir());
}

void MainWindow::setupUI()
{
    m_centralStack = new QStackedWidget(this);
    setCentralWidget(m_centralStack);

    // Module 1: Code Editor
    m_codeEditor = new CodeEditorWidget(m_commandBus, m_moduleRegistry, this);
    m_codeEditor->setLSPClient(m_lspClient);
    m_centralStack->addWidget(m_codeEditor);

    // Module 2: Block Editor
    m_blockEditor = new BlockEditorWidget(m_moduleRegistry, m_commandBus, this);
    m_blockEditor->setGraphStore(m_graphStore);
    m_blockEditor->scene()->setGraphStore(m_graphStore);
    m_modulePalette = new ModulePalette(m_moduleRegistry, this);
    m_blockEditor->setPalette(m_modulePalette);
    m_centralStack->addWidget(m_blockEditor);

    // Module 3: UI Designer
    m_uiDesigner = new UIDesignerWidget(m_moduleRegistry, m_commandBus, this);
    m_centralStack->addWidget(m_uiDesigner);

    // Module 4: Library Processor
    m_libProcessor = new LibProcessorWidget(m_moduleRegistry, this);
    m_libProcessor->setGraphStore(m_graphStore);
    m_centralStack->addWidget(m_libProcessor);

    // Module 5: Module Manager
    m_moduleManager = new ModuleManagerWidget(m_moduleRegistry, this);
    m_moduleManager->setGlobalModulesDir(m_sessionManager->globalModulesDir());
    m_centralStack->addWidget(m_moduleManager);

    m_centralStack->setCurrentIndex(0); // Start with code editor
}

void MainWindow::setupMenus()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_actionManager->newProjectAction());

    auto *newFileAction = fileMenu->addAction(tr("New File..."));
    newFileAction->setShortcut(QKeySequence(tr("Ctrl+N")));
    connect(newFileAction, &QAction::triggered, this, &MainWindow::onNewFile);

    fileMenu->addAction(m_actionManager->openProjectAction());
    m_recentProjectsMenu = fileMenu->addMenu(tr("Recent Projects"));
    updateRecentProjectsMenu();
    fileMenu->addSeparator();
    fileMenu->addAction(m_actionManager->saveAction());
    fileMenu->addAction(m_actionManager->saveAllAction());
    fileMenu->addSeparator();
    fileMenu->addAction(m_actionManager->action("file.close"));
    fileMenu->addAction(m_actionManager->action("file.quit"));

    auto *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(m_actionManager->undoAction());
    editMenu->addAction(m_actionManager->redoAction());
    editMenu->addSeparator();
    editMenu->addAction(m_actionManager->action("edit.cut"));
    editMenu->addAction(m_actionManager->action("edit.copy"));
    editMenu->addAction(m_actionManager->action("edit.paste"));
    editMenu->addSeparator();
    editMenu->addAction(m_actionManager->action("edit.find"));
    editMenu->addAction(m_actionManager->action("edit.replace"));
    editMenu->addSeparator();
    editMenu->addAction(m_actionManager->action("edit.goToLine"));
    editMenu->addAction(m_actionManager->action("edit.goToDefinition"));
    editMenu->addAction(m_actionManager->action("edit.findReferences"));
    editMenu->addAction(m_actionManager->action("edit.openGeneratedOrigin"));
    editMenu->addSeparator();
    editMenu->addAction(m_actionManager->action("edit.rename"));
    editMenu->addAction(m_actionManager->action("edit.format"));
    editMenu->addSeparator();
    auto *settingsAction = editMenu->addAction(tr("Settings..."));
    settingsAction->setShortcut(QKeySequence(tr("Ctrl+,")));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);

    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_actionManager->action("view.codeEditor"));
    viewMenu->addAction(m_actionManager->action("view.blockEditor"));
    viewMenu->addAction(m_actionManager->action("view.uiDesigner"));
    viewMenu->addAction(m_actionManager->action("view.libProcessor"));
    viewMenu->addSeparator();
    auto *moduleManagerAct = viewMenu->addAction(tr("Module Manager"));
    moduleManagerAct->setShortcut(QKeySequence(tr("Ctrl+M")));
    connect(moduleManagerAct, &QAction::triggered, this, &MainWindow::switchToModuleManager);

    auto *buildMenu = menuBar()->addMenu(tr("&Build"));
    buildMenu->addAction(m_actionManager->buildAction());
    buildMenu->addAction(m_actionManager->runAction());
    buildMenu->addAction(m_actionManager->action("build.clean"));

    auto *debugMenu = menuBar()->addMenu(tr("&Debug"));
    debugMenu->addAction(m_actionManager->action("debug.start"));
    debugMenu->addAction(m_actionManager->action("debug.stop"));
    debugMenu->addSeparator();
    debugMenu->addAction(m_actionManager->action("debug.continue"));
    debugMenu->addAction(m_actionManager->action("debug.stepOver"));
    debugMenu->addAction(m_actionManager->action("debug.stepInto"));
    debugMenu->addAction(m_actionManager->action("debug.stepOut"));
    debugMenu->addSeparator();
    debugMenu->addAction(m_actionManager->action("debug.toggleBreakpoint"));

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("About DeltaQ"), this, [this]() {
        QMessageBox::about(this, tr("About DeltaQ"),
            tr("DeltaQ IDE v%1\n\n"
               "Modular C/C++ Development Environment\n\n"
               "Modules:\n"
               "1. Code Editor\n"
               "2. Visual Block Editor\n"
               "3. UI Designer (SDL2)\n"
               "4. Library Processor")
            .arg(QApplication::applicationVersion()));
    });
}

void MainWindow::setupToolBar()
{
    m_mainToolBar = addToolBar(tr("Main"));
    m_mainToolBar->setObjectName("mainToolBar");

    m_mainToolBar->addAction(m_actionManager->newProjectAction());
    m_mainToolBar->addAction(m_actionManager->openProjectAction());
    m_mainToolBar->addAction(m_actionManager->saveAction());
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_actionManager->undoAction());
    m_mainToolBar->addAction(m_actionManager->redoAction());
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_actionManager->buildAction());
    m_mainToolBar->addAction(m_actionManager->runAction());
    m_mainToolBar->addSeparator();

    // Module switch buttons
    m_mainToolBar->addAction(m_actionManager->action("view.codeEditor"));
    m_mainToolBar->addAction(m_actionManager->action("view.blockEditor"));
    m_mainToolBar->addAction(m_actionManager->action("view.uiDesigner"));
    m_mainToolBar->addAction(m_actionManager->action("view.libProcessor"));
}

void MainWindow::setupStatusBar()
{
    m_cursorPosLabel = new QLabel("Ln 1, Col 1", this);
    m_cursorPosLabel->setMinimumWidth(120);
    m_cursorPosLabel->setAlignment(Qt::AlignCenter);
    statusBar()->addPermanentWidget(m_cursorPosLabel);
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::setupDocks()
{
    // Project tree dock
    m_projectDock = new QDockWidget(tr("Project"), this);
    m_projectDock->setObjectName("projectDock");
    m_projectTree = new ProjectTreeView(this);
    m_projectDock->setWidget(m_projectTree);
    addDockWidget(Qt::LeftDockWidgetArea, m_projectDock);

    // Output dock
    m_outputDock = new QDockWidget(tr("Output"), this);
    m_outputDock->setObjectName("outputDock");
    m_outputTabs = new QTabWidget(this);

    m_buildOutput = new QTextEdit(this);
    m_buildOutput->setObjectName("buildOutputView");
    m_buildOutput->setReadOnly(true);
    m_buildOutput->setFont(QFont("Monospace", 10));
    m_buildOutput->viewport()->installEventFilter(this);
    m_outputTabs->addTab(m_buildOutput, tr("Build Output"));

    m_buildDiagnostics = new QTreeWidget(this);
    m_buildDiagnostics->setObjectName("buildDiagnosticsView");
    m_buildDiagnostics->setRootIsDecorated(false);
    m_buildDiagnostics->setAlternatingRowColors(true);
    m_buildDiagnostics->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_buildDiagnostics->setSelectionMode(QAbstractItemView::SingleSelection);
    m_buildDiagnostics->setUniformRowHeights(true);
    m_buildDiagnostics->setColumnCount(4);
    m_buildDiagnostics->setHeaderLabels({tr("Severity"), tr("Location"), tr("Source"), tr("Message")});
    m_buildDiagnostics->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_buildDiagnostics->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_buildDiagnostics->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_buildDiagnostics->header()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_buildDiagnostics->header()->setStretchLastSection(true);
    m_outputTabs->addTab(m_buildDiagnostics, tr("Build Diagnostics"));

    m_appOutput = new QTextEdit(this);
    m_appOutput->setReadOnly(true);
    m_appOutput->setFont(QFont("Monospace", 10));
    m_outputTabs->addTab(m_appOutput, tr("Application Output"));

    m_debugConsole = new QTextEdit(this);
    m_debugConsole->setReadOnly(true);
    m_debugConsole->setFont(QFont("Monospace", 10));
    m_outputTabs->addTab(m_debugConsole, tr("Debug Console"));

    m_outputDock->setWidget(m_outputTabs);
    addDockWidget(Qt::BottomDockWidgetArea, m_outputDock);

    // Панель переменных
    m_variablesDock = new QDockWidget(tr("Variables"), this);
    m_variablesDock->setObjectName("variablesDock");
    auto *varsTree = new QTreeWidget(this);
    varsTree->setHeaderLabels({tr("Name"), tr("Type"), tr("Value")});
    varsTree->setColumnCount(3);
    varsTree->header()->setStretchLastSection(true);
    m_variablesDock->setWidget(varsTree);
    addDockWidget(Qt::RightDockWidgetArea, m_variablesDock);
    m_variablesDock->hide();

    // Панель стека вызовов
    m_callStackDock = new QDockWidget(tr("Call Stack"), this);
    m_callStackDock->setObjectName("callStackDock");
    auto *stackTree = new QTreeWidget(this);
    stackTree->setHeaderLabels({tr("#"), tr("Function"), tr("File"), tr("Line")});
    stackTree->setColumnCount(4);
    stackTree->header()->setStretchLastSection(true);
    stackTree->setRootIsDecorated(false);
    m_callStackDock->setWidget(stackTree);
    addDockWidget(Qt::RightDockWidgetArea, m_callStackDock);
    m_callStackDock->hide();
}

void MainWindow::setupConnections()
{
    auto *am = m_actionManager;

    connect(am->newProjectAction(), &QAction::triggered, this, &MainWindow::onNewProject);
    connect(am->openProjectAction(), &QAction::triggered, this, &MainWindow::onOpenProject);
    connect(am->saveAction(), &QAction::triggered, this, &MainWindow::onSaveFile);
    connect(am->action("file.close"), &QAction::triggered, this, &MainWindow::onCloseProject);
    connect(am->action("file.quit"), &QAction::triggered, qApp, &QApplication::quit);

    // Undo/redo: если фокус в редакторе кода — делегируем QPlainTextEdit, иначе — UndoManager
    connect(am->undoAction(), &QAction::triggered, this, [this]() {
        if (m_centralStack->currentWidget() == m_codeEditor) {
            auto *tab = m_codeEditor->currentTab();
            if (tab && tab->editor()) {
                tab->editor()->undo();
                return;
            }
        }
        m_undoManager->undo();
    });
    connect(am->redoAction(), &QAction::triggered, this, [this]() {
        if (m_centralStack->currentWidget() == m_codeEditor) {
            auto *tab = m_codeEditor->currentTab();
            if (tab && tab->editor()) {
                tab->editor()->redo();
                return;
            }
        }
        m_undoManager->redo();
    });

    connect(am->action("edit.find"), &QAction::triggered, m_codeEditor, &CodeEditorWidget::showFind);
    connect(am->action("edit.replace"), &QAction::triggered, m_codeEditor, &CodeEditorWidget::showReplace);
    connect(am->action("edit.goToLine"), &QAction::triggered, m_codeEditor, &CodeEditorWidget::goToLine);
    connect(am->action("edit.goToDefinition"), &QAction::triggered, m_codeEditor, &CodeEditorWidget::goToDefinition);
    connect(am->action("edit.findReferences"), &QAction::triggered, m_codeEditor, &CodeEditorWidget::findReferences);
    connect(am->action("edit.openGeneratedOrigin"), &QAction::triggered,
            this, &MainWindow::onOpenGeneratedOrigin);
    connect(m_codeEditor, &CodeEditorWidget::openGeneratedOriginRequested,
            this, &MainWindow::onOpenGeneratedOrigin);
    connect(am->action("edit.rename"), &QAction::triggered, m_codeEditor, &CodeEditorWidget::renameSymbol);
    connect(am->action("edit.format"), &QAction::triggered, m_codeEditor, &CodeEditorWidget::formatDocument);

    connect(am->buildAction(), &QAction::triggered, this, &MainWindow::onBuild);
    connect(am->action("build.clean"), &QAction::triggered, this, &MainWindow::onClean);
    connect(am->runAction(), &QAction::triggered, this, &MainWindow::onRun);

    // Действия отладки
    connect(am->action("debug.start"), &QAction::triggered, this, &MainWindow::onDebugStart);
    connect(am->action("debug.stop"), &QAction::triggered, this, &MainWindow::onDebugStop);
    connect(am->action("debug.continue"), &QAction::triggered, m_debugManager, &DebugManager::continueExecution);
    connect(am->action("debug.stepOver"), &QAction::triggered, m_debugManager, &DebugManager::stepOver);
    connect(am->action("debug.stepInto"), &QAction::triggered, m_debugManager, &DebugManager::stepInto);
    connect(am->action("debug.stepOut"), &QAction::triggered, m_debugManager, &DebugManager::stepOut);
    connect(am->action("debug.toggleBreakpoint"), &QAction::triggered, this, &MainWindow::onToggleBreakpoint);

    connect(am->action("view.codeEditor"), &QAction::triggered, this, &MainWindow::switchToCodeEditor);
    connect(am->action("view.blockEditor"), &QAction::triggered, this, &MainWindow::switchToBlockEditor);
    connect(am->action("view.uiDesigner"), &QAction::triggered, this, &MainWindow::switchToUIDesigner);
    connect(am->action("view.libProcessor"), &QAction::triggered, this, &MainWindow::switchToLibProcessor);

    connect(m_projectManager, &ProjectManager::projectOpened, this, &MainWindow::updateTitle);
    connect(m_projectManager, &ProjectManager::projectClosed, this, &MainWindow::updateTitle);

    connect(m_commandBus, &CommandBus::commandExecuted, this, &MainWindow::updateStatusBar);

    // Вывод сборки в Output dock
    connect(m_buildManager, &BuildManager::buildOutput, this, [this](const QString &text) {
        appendBuildOutputChunk(text);
        m_outputDock->show();
    });
    connect(m_buildManager, &BuildManager::buildStarted, this, [this]() {
        m_buildOutput->clear();
        m_buildDiagnostics->clear();
        m_outputTabs->setCurrentWidget(m_buildOutput);
        m_outputDock->show();
        m_actionManager->buildAction()->setEnabled(false);
        statusBar()->showMessage(tr("Building..."));
    });
    connect(m_buildManager, &BuildManager::buildError,
            this, &MainWindow::addBuildDiagnosticEntry);
    connect(m_buildManager, &BuildManager::buildFinished, this, [this](bool success, int errors, int warnings) {
        m_actionManager->buildAction()->setEnabled(true);
        if (success)
            statusBar()->showMessage(tr("Build succeeded"), 5000);
        else
            statusBar()->showMessage(tr("Build failed: %1 error(s), %2 warning(s)")
                .arg(errors).arg(warnings), 5000);
    });
    connect(m_buildDiagnostics, &QTreeWidget::itemDoubleClicked,
            this, &MainWindow::onBuildDiagnosticActivated);

    // Дерево проекта: открытие файлов в редакторе
    connect(m_projectTree, &ProjectTreeView::fileSelected, this, &MainWindow::onFileActivated);

    // Дерево проекта: создание файла через NewFileDialog
    connect(m_projectTree, &ProjectTreeView::newFileRequested, this, [this](const QString &) {
        onNewFile();
    });
    connect(m_preBuildProcessor, &PreBuildProcessor::fileGenerated, this,
            [this](const PreBuildArtifact &artifact) {
        m_projectTree->markGeneratedFile(artifact.path);
    });

    // Позиция курсора в редакторе → статус-бар
    connect(m_codeEditor, &CodeEditorWidget::currentTabChanged, this, &MainWindow::updateCursorPosition);
    connect(m_codeEditor, &CodeEditorWidget::currentTabChanged, this, &MainWindow::updateEditorActions);
    connect(m_centralStack, &QStackedWidget::currentChanged, this,
            [this](int) { updateEditorActions(); });

    // Сигналы отладчика
    connect(m_debugManager, &DebugManager::debugStarted, this, [this]() {
        m_variablesDock->show();
        m_callStackDock->show();
        m_outputTabs->setCurrentWidget(m_debugConsole);
        m_outputDock->show();
        statusBar()->showMessage(tr("Debugging started"), 3000);
    });
    connect(m_debugManager, &DebugManager::debugStopped, this, [this]() {
        m_variablesDock->hide();
        m_callStackDock->hide();
        m_codeEditor->clearDebugLineInAllTabs();
        statusBar()->showMessage(tr("Debugging stopped"), 3000);
    });
    connect(m_debugManager, &DebugManager::debugOutput, this, [this](const QString &text) {
        m_debugConsole->append(text);
    });
    connect(m_debugManager, &DebugManager::targetOutput, this, [this](const QString &text) {
        m_appOutput->append(text);
    });
    connect(m_debugManager, &DebugManager::errorOccurred, this, [this](const QString &msg) {
        statusBar()->showMessage(tr("Debug error: %1").arg(msg), 5000);
    });
    connect(m_debugManager, &DebugManager::breakpointHit, this, [this](const QString &file, int line) {
        m_codeEditor->clearDebugLineInAllTabs();
        m_codeEditor->openFile(file);
        auto *tab = m_codeEditor->currentTab();
        if (tab) {
            tab->setDebugCurrentLine(line);
            QTextCursor cursor(tab->editor()->document()->findBlockByNumber(line - 1));
            tab->editor()->setTextCursor(cursor);
            tab->editor()->centerCursor();
        }
        statusBar()->showMessage(tr("Breakpoint hit: %1:%2").arg(QFileInfo(file).fileName()).arg(line), 5000);
    });
    connect(m_debugManager, &DebugManager::stepped, this, [this](const QString &file, int line) {
        m_codeEditor->clearDebugLineInAllTabs();
        if (!file.isEmpty()) {
            m_codeEditor->openFile(file);
            auto *tab = m_codeEditor->currentTab();
            if (tab) {
                tab->setDebugCurrentLine(line);
                QTextCursor cursor(tab->editor()->document()->findBlockByNumber(line - 1));
                tab->editor()->setTextCursor(cursor);
                tab->editor()->centerCursor();
            }
        }
    });
    connect(m_debugManager, &DebugManager::breakpointAdded, this,
            [this](const Breakpoint &bp) {
        auto *tab = m_codeEditor->findTabForFile(bp.file);
        if (tab)
            tab->addBreakpointMarker(bp.line);
    });
    connect(m_debugManager, &DebugManager::breakpointRemoved, this,
            [this](const QString &file, int line) {
        auto *tab = m_codeEditor->findTabForFile(file);
        if (tab)
            tab->removeBreakpointMarker(line);
    });
    // Breakpoint toggle из редактора → DebugManager
    connect(m_codeEditor, &CodeEditorWidget::breakpointToggleRequested,
            m_debugManager, &DebugManager::toggleBreakpoint);

    // Обновление панели переменных
    connect(m_debugManager, &DebugManager::variablesUpdated, this,
            [this](const QVector<Variable> &vars) {
        auto *tree = qobject_cast<QTreeWidget *>(m_variablesDock->widget());
        if (!tree) return;
        tree->clear();
        for (const auto &var : vars) {
            auto *item = new QTreeWidgetItem(tree, {var.name, var.type, var.value});
            Q_UNUSED(item)
        }
    });

    // Обновление стека вызовов
    connect(m_debugManager, &DebugManager::callStackUpdated, this,
            [this](const QVector<StackFrame> &frames) {
        auto *tree = qobject_cast<QTreeWidget *>(m_callStackDock->widget());
        if (!tree) return;
        tree->clear();
        for (const auto &f : frames) {
            auto *item = new QTreeWidgetItem(tree, {
                QString::number(f.level), f.function,
                QFileInfo(f.file).fileName(), QString::number(f.line)
            });
            item->setData(0, Qt::UserRole, f.file);
            item->setData(0, Qt::UserRole + 1, f.line);
        }
    });

    // Двойной клик по кадру стека → навигация
    auto *stackTree = qobject_cast<QTreeWidget *>(m_callStackDock->widget());
    if (stackTree) {
        connect(stackTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
            QString file = item->data(0, Qt::UserRole).toString();
            int line = item->data(0, Qt::UserRole + 1).toInt();
            if (!file.isEmpty()) {
                m_codeEditor->openFile(file);
                auto *tab = m_codeEditor->currentTab();
                if (tab && tab->editor()) {
                    QTextCursor cursor(tab->editor()->document()->findBlockByNumber(line - 1));
                    tab->editor()->setTextCursor(cursor);
                    tab->editor()->centerCursor();
                }
            }
            // Выбираем кадр в GDB
            m_debugManager->selectFrame(item->text(0).toInt());
        });
    }

    // UI Designer: подключаем сигналы основного экземпляра
    connectUIDesignerSignals(m_uiDesigner);

    // Блочный редактор: визуальная отладка
    auto *blockScene = m_blockEditor->scene();
    m_graphDebugger = new GraphDebugger(blockScene, m_debugManager, this);

    // Блочный редактор: соединение через CommandBus
    connectBlockEditorSignals(m_blockEditor);

    // Менеджер модулей: обновление палитры при изменении модуля
    connect(m_moduleManager, &ModuleManagerWidget::moduleChanged, this, [this]() {
        m_modulePalette->rebuildTree();
    });

    // Загрузка реестра модулей при открытии проекта
    connect(m_projectManager, &ProjectManager::projectOpened, this, [this]() {
        m_moduleRegistry->loadRegistry(m_projectManager->projectDir());
        // Загрузка локальных модулей проекта из dqmods/
        m_moduleRegistry->loadLocalModules(m_projectManager->projectDir() + "/dqmods");
        m_moduleManager->setProjectDir(m_projectManager->projectDir());
        int count = m_moduleRegistry->count();
        if (count > 0)
            statusBar()->showMessage(tr("Loaded %1 module(s)").arg(count), 3000);
    });
    connect(m_projectManager, &ProjectManager::projectClosed, this, [this]() {
        // Очищаем реестр и перезагружаем глобальные модули
        m_moduleRegistry->clear();
        UIModuleFactory::registerAll(m_moduleRegistry);
        m_moduleRegistry->loadGlobalModules(m_sessionManager->globalModulesDir());
        m_moduleManager->setProjectDir(QString());
    });

    // Уведомление о регистрации модуля из аннотаций + обновление палитры и менеджера
    connect(m_moduleRegistry, &ModuleRegistry::moduleRegistered, this, [this](const QString &id) {
        auto *mod = m_moduleRegistry->findModule(id);
        if (mod)
            statusBar()->showMessage(tr("Module '%1' registered").arg(mod->name), 3000);
        m_modulePalette->rebuildTree();
        m_moduleManager->rebuildTree();
    });
    connect(m_moduleRegistry, &ModuleRegistry::moduleUpdated, this, [this](const QString &id) {
        auto *mod = m_moduleRegistry->findModule(id);
        if (mod)
            statusBar()->showMessage(tr("Module '%1' updated").arg(mod->name), 3000);
        m_modulePalette->rebuildTree();
        m_moduleManager->rebuildTree();
    });
    connect(m_moduleRegistry, &ModuleRegistry::moduleUnregistered, this, [this]() {
        m_modulePalette->rebuildTree();
        m_moduleManager->rebuildTree();
    });

    // LSP: запуск при открытии проекта
    connect(m_projectManager, &ProjectManager::projectOpened, this, [this]() {
        // Ищем clangd в PATH
        QString clangd = "clangd";
        m_lspClient->start(clangd, {"--background-index"});
        if (m_lspClient->isRunning()) {
            QString rootUri = LSPClient::pathToUri(m_projectManager->projectDir());
            m_lspClient->initialize(rootUri);
        }
    });
    connect(m_projectManager, &ProjectManager::projectClosed, this, [this]() {
        m_lspClient->stop();
    });

    // LSP: переход к определению → открыть файл и перейти к позиции
    connect(m_lspClient, &LSPClient::definitionResult, this, [this](const QVector<LSPLocation> &locations) {
        if (locations.isEmpty()) {
            statusBar()->showMessage(tr("Definition not found"), 3000);
            return;
        }
        const auto &loc = locations.first();
        QString path = LSPClient::uriToPath(loc.uri);
        m_codeEditor->openFile(path);
        // Перейти к позиции
        auto *tab = m_codeEditor->currentTab();
        if (tab && tab->editor()) {
            QTextCursor cursor(tab->editor()->document()->findBlockByNumber(loc.range.start.line));
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, loc.range.start.character);
            tab->editor()->setTextCursor(cursor);
            tab->editor()->centerCursor();
        }
    });

    // LSP: диагностика → подчёркивание ошибок в редакторе
    connect(m_lspClient, &LSPClient::diagnosticsReceived,
            m_codeEditor, &CodeEditorWidget::onDiagnosticsReceived);
    connect(m_codeEditor, &CodeEditorWidget::diagnosticsUpdated, this,
            [this](const QString &path, int errors, int warnings) {
        // Обновляем бейджи в дереве проекта
        m_projectTree->updateDiagnosticCounts(path, errors, warnings);
        if (errors > 0 || warnings > 0) {
            statusBar()->showMessage(tr("Diagnostics: %1 errors, %2 warnings")
                .arg(errors).arg(warnings), 5000);
        }
    });

    // LSP: ошибка сервера
    connect(m_lspClient, &LSPClient::serverError, this, [this](const QString &msg) {
        statusBar()->showMessage(tr("LSP: %1").arg(msg), 5000);
    });
    connect(m_lspClient, &LSPClient::initialized, this, [this]() {
        statusBar()->showMessage(tr("LSP server initialized"), 3000);
    });

    updateEditorActions();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSession();
    event->accept();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_buildOutput->viewport() && event->type() == QEvent::MouseMove) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QTextCursor cursor = m_buildOutput->cursorForPosition(mouseEvent->position().toPoint());
        const QString lineText = cursor.block().text();
        const bool navigable = parseBuildOutputLine(lineText, nullptr);

        m_buildOutput->viewport()->setCursor(navigable ? Qt::PointingHandCursor : Qt::IBeamCursor);
        m_buildOutput->setToolTip(navigable
            ? tr("Double-click to open error source")
            : QString());
        return QMainWindow::eventFilter(watched, event);
    }

    if (watched == m_buildOutput->viewport() && event->type() == QEvent::Leave) {
        m_buildOutput->viewport()->unsetCursor();
        m_buildOutput->setToolTip(QString());
        return QMainWindow::eventFilter(watched, event);
    }

    // Двойной щелчок по строке Build Output открывает место ошибки или её source-of-truth.
    if (watched == m_buildOutput->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() != Qt::LeftButton)
            return QMainWindow::eventFilter(watched, event);

        const QTextCursor cursor = m_buildOutput->cursorForPosition(mouseEvent->position().toPoint());
        const QString lineText = cursor.block().text();
        CompilerError error;
        if (!parseBuildOutputLine(lineText, &error))
            return QMainWindow::eventFilter(watched, event);

        navigateBuildOutputLine(lineText);
        return true;
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::appendBuildOutputChunk(const QString &text)
{
    // Вставляет build output построчно, чтобы навигационные строки можно было выделить отдельно.
    QTextCursor cursor(m_buildOutput->document());
    cursor.movePosition(QTextCursor::End);

    const QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines.at(i);
        cursor.insertText(line, buildOutputFormatForLine(line));
        if (i != lines.size() - 1)
            cursor.insertBlock();
    }

    m_buildOutput->setTextCursor(cursor);
    m_buildOutput->ensureCursorVisible();
}

void MainWindow::addBuildDiagnosticEntry(const QString &file, int line, int column,
                                         const QString &severity, const QString &message)
{
    if (!m_buildDiagnostics)
        return;

    // Привязывает строку diagnostics к реальному файлу сборки и, при наличии, к source-of-truth.
    const QString resolvedPath = normalizeBuildErrorPath(file);
    const QString displayPath = resolvedPath.isEmpty() ? file : resolvedPath;
    const QString originPath = (m_projectTree && !resolvedPath.isEmpty())
        ? m_projectTree->generatedOriginPath(resolvedPath)
        : QString();

    auto *item = new QTreeWidgetItem();
    const QString severityKey = severity.trimmed().toLower();
    const QString sourceLabel = !originPath.isEmpty()
        ? QFileInfo(originPath).fileName()
        : QFileInfo(displayPath).fileName();

    item->setText(0, severityKey.isEmpty() ? tr("info") : severityKey);
    item->setText(1, buildDiagnosticLocationText(displayPath, line, column));
    item->setText(2, sourceLabel);
    item->setText(3, message);
    item->setToolTip(1, displayPath);
    if (!originPath.isEmpty()) {
        item->setToolTip(2, tr("Source-of-truth: %1").arg(originPath));
        item->setToolTip(3, tr("%1\n\nGenerated file: %2").arg(message, displayPath));
    }

    item->setData(0, ResolvedPathRole, resolvedPath);
    item->setData(0, OriginPathRole, originPath);
    item->setData(0, LineRole, line);
    item->setData(0, ColumnRole, column);

    // Цветом отделяет ошибки от предупреждений и подсказок.
    QColor severityColor(80, 80, 80);
    if (severityKey == "error")
        severityColor = QColor(180, 40, 40);
    else if (severityKey == "warning")
        severityColor = QColor(180, 120, 0);
    else if (severityKey == "note")
        severityColor = QColor(0, 102, 204);
    item->setForeground(0, severityColor);

    m_buildDiagnostics->addTopLevelItem(item);
    m_buildDiagnostics->scrollToItem(item);
    m_buildDiagnostics->setCurrentItem(item);
    m_outputTabs->setCurrentWidget(m_buildDiagnostics);
    m_outputDock->show();
}

void MainWindow::onBuildDiagnosticActivated(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)

    if (!item)
        return;

    // По клику в diagnostics открывает либо source-of-truth, либо точную позицию в текстовом файле.
    const QString originPath = item->data(0, OriginPathRole).toString();
    const QString resolvedPath = item->data(0, ResolvedPathRole).toString();
    const int line = item->data(0, LineRole).toInt();
    const int columnValue = item->data(0, ColumnRole).toInt();

    if (!originPath.isEmpty()) {
        switchToCodeEditor();
        m_codeEditor->openFile(originPath);
        statusBar()->showMessage(
            tr("Opened source-of-truth for diagnostic: %1")
                .arg(QFileInfo(originPath).fileName()),
            3000);
        return;
    }

    if (resolvedPath.isEmpty() || !QFile::exists(resolvedPath)) {
        statusBar()->showMessage(tr("Diagnostic file not found"), 3000);
        return;
    }

    openTextFileAtLocation(resolvedPath, line, columnValue);
    statusBar()->showMessage(
        tr("Opened diagnostic location: %1:%2")
            .arg(QFileInfo(resolvedPath).fileName())
            .arg(line),
        3000);
}

void MainWindow::onNewProject()
{
    NewProjectWizard wizard(this);
    wizard.setDefaultDir(m_sessionManager->defaultProjectDir());
    if (wizard.exec() != QDialog::Accepted)
        return;

    QString name = wizard.projectName();
    QString dir = wizard.projectDir() + "/" + name;
    QString type = wizard.projectType();

    if (m_projectManager->createProject(name, dir, type)) {
        // Генерация шаблонных файлов по типу проекта
        ProjectTemplates::generate(type, dir, name);

        // Загружаем модули и графы в stores
        m_moduleRegistry->loadRegistry(dir);
        m_graphStore->loadFromDirectory(dir);

        // Для desktop-проекта загружаем layout в UILayoutStore
        if (type == "desktop")
            m_uiLayoutStore->loadFromDirectory(dir);

        m_projectTree->setRootPath(dir);
        m_sessionManager->addRecentProject(m_projectManager->currentProject().projectFilePath);
        m_sessionManager->setDefaultProjectDir(wizard.projectDir());
        updateRecentProjectsMenu();
        statusBar()->showMessage(tr("Project '%1' created").arg(name), 3000);

        // Автооткрытие main.c в редакторе
        QString mainPath = dir + "/src/main.c";
        if (QFile::exists(mainPath))
            m_codeEditor->openFile(mainPath);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to create project"));
    }
}

void MainWindow::onOpenProject()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open Project"),
                                                 QString(), tr("DeltaQ Projects (*.dqproj)"));
    if (path.isEmpty())
        return;

    if (m_projectManager->openProject(path)) {
        m_projectTree->setRootPath(m_projectManager->projectDir());
        m_sessionManager->addRecentProject(path);
        updateRecentProjectsMenu();
        statusBar()->showMessage(tr("Project opened"), 3000);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to open project"));
    }
}

void MainWindow::onSaveFile()
{
    // Проверяем, является ли текущая вкладка кастомной (UIDesigner или BlockEditor)
    QWidget *currentWidget = m_codeEditor->currentCustomTabWidget();
    if (auto *uiDesigner = qobject_cast<UIDesignerWidget *>(currentWidget)) {
        uiDesigner->saveLayout(m_uiLayoutStore);
        // Записать .dqui файл на диск
        if (m_uiLayoutStore && m_projectManager->isProjectOpen()) {
            m_uiLayoutStore->saveAll(m_projectManager->projectDir());
        }
        statusBar()->showMessage(tr("UI layout saved"), 3000);
        return;
    }
    if (auto *blockEditor = qobject_cast<BlockEditorWidget *>(currentWidget)) {
        blockEditor->saveGraph(m_graphStore);
        if (m_graphStore && m_projectManager->isProjectOpen()) {
            m_graphStore->saveAll(m_projectManager->projectDir());
        }
        statusBar()->showMessage(tr("Graph saved"), 3000);
        return;
    }
    // Обычный текстовый файл
    m_codeEditor->saveCurrentFile();
}

void MainWindow::onBuild()
{
    if (!m_projectManager->isProjectOpen()) {
        statusBar()->showMessage(tr("No project open"), 3000);
        return;
    }

    m_buildOutput->clear();
    m_buildDiagnostics->clear();
    m_outputTabs->setCurrentWidget(m_buildOutput);
    m_outputDock->show();

    // Подключаем вывод pipeline к buildOutput (однократно через lambda)
    auto conn = connect(m_buildPipeline, &BuildPipeline::pipelineOutput,
                        this, [this](const QString &text) {
        appendBuildOutputChunk(text);
    });

    // По завершению pipeline отключаем соединение
    connect(m_buildPipeline, &BuildPipeline::pipelineFinished,
            this, [this, conn](bool success) {
        disconnect(conn);
        if (success)
            statusBar()->showMessage(tr("Build completed"), 3000);
        else
            statusBar()->showMessage(tr("Build failed"), 3000);
    });

    // Запускаем двухфазную сборку
    m_buildPipeline->run(m_projectManager->projectDir(),
                         m_projectManager->currentProject().name,
                         m_projectManager->currentProject().build.standard,
                         "20",
                         m_projectManager->currentProject().projectType);
}

void MainWindow::onClean()
{
    if (!m_projectManager->isProjectOpen()) {
        statusBar()->showMessage(tr("No project open"), 3000);
        return;
    }
    m_buildManager->clean(m_projectManager->projectDir());
}

void MainWindow::onRun()
{
    if (!m_projectManager->isProjectOpen()) {
        statusBar()->showMessage(tr("No project open"), 3000);
        return;
    }

    // Ищем исполняемый файл в build/
    QString buildDir = m_projectManager->projectDir() + "/build";
    QString projectName = m_projectManager->currentProject().name;

    // Пробуем найти бинарник с именем проекта
    QStringList candidates = {
        buildDir + "/" + projectName,
        buildDir + "/src/" + projectName,
        buildDir + "/" + projectName.toLower(),
    };

    QString executable;
    for (const auto &path : candidates) {
        if (QFile::exists(path)) {
            executable = path;
            break;
        }
    }

    if (executable.isEmpty()) {
        statusBar()->showMessage(tr("Executable not found — build first"), 3000);
        return;
    }

    m_appOutput->clear();
    m_outputTabs->setCurrentWidget(m_appOutput);
    m_outputDock->show();

    auto *process = new QProcess(this);
    process->setWorkingDirectory(m_projectManager->projectDir());
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
        m_appOutput->append(process->readAllStandardOutput());
    });
    connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
        m_appOutput->append(process->readAllStandardError());
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus) {
        m_appOutput->append(tr("\n=== Process exited (code: %1) ===").arg(exitCode));
        process->deleteLater();
    });
    process->start(executable, {});
    statusBar()->showMessage(tr("Running %1").arg(projectName), 3000);
}

void MainWindow::onDebugStart()
{
    if (!m_projectManager->isProjectOpen()) {
        statusBar()->showMessage(tr("No project open"), 3000);
        return;
    }

    // Ищем исполняемый файл в build/
    QString buildDir = m_projectManager->projectDir() + "/build";
    QString projectName = m_projectManager->currentProject().name;
    QStringList candidates = {
        buildDir + "/" + projectName,
        buildDir + "/src/" + projectName,
        buildDir + "/" + projectName.toLower(),
    };

    QString executable;
    for (const auto &path : candidates) {
        if (QFile::exists(path)) {
            executable = path;
            break;
        }
    }

    if (executable.isEmpty()) {
        statusBar()->showMessage(tr("Executable not found — build first"), 3000);
        return;
    }

    m_debugConsole->clear();
    m_debugManager->startDebug(executable);
}

void MainWindow::onDebugStop()
{
    m_debugManager->stopDebug();
}

void MainWindow::onToggleBreakpoint()
{
    auto *tab = m_codeEditor->currentTab();
    if (!tab || !tab->editor())
        return;

    QString file = tab->filePath();
    int line = tab->editor()->textCursor().blockNumber() + 1;
    m_debugManager->toggleBreakpoint(file, line);
}

void MainWindow::switchToCodeEditor() { m_centralStack->setCurrentWidget(m_codeEditor); }
void MainWindow::switchToBlockEditor() { m_centralStack->setCurrentWidget(m_blockEditor); }
void MainWindow::switchToUIDesigner() { m_centralStack->setCurrentWidget(m_uiDesigner); }
void MainWindow::switchToLibProcessor() { m_centralStack->setCurrentWidget(m_libProcessor); }
void MainWindow::switchToModuleManager() { m_centralStack->setCurrentWidget(m_moduleManager); }

void MainWindow::onFileActivated(const QString &path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    const QString name = QFileInfo(path).fileName();

    // Всегда показываем CodeEditorWidget (единый таб-бар)
    switchToCodeEditor();

    // Исходные файлы и CMakeLists → текстовый редактор
    if (ext == "c" || ext == "cpp" || ext == "h" || ext == "hpp" ||
        ext == "txt" || name == "CMakeLists.txt") {
        m_codeEditor->openFile(path);
        return;
    }

    // Граф блочного редактора — открываем как вкладку
    if (ext == "dqgraph") {
        if (m_codeEditor->findCustomTabWidget(path)) {
            m_codeEditor->openCustomTab(nullptr, {}, path); // переключение
            return;
        }
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            QString id = QJsonDocument::fromJson(f.readAll()).object()["id"].toString();
            if (!id.isEmpty()) {
                auto *editor = new BlockEditorWidget(m_moduleRegistry, m_commandBus);
                editor->setGraphStore(m_graphStore);
                editor->scene()->setGraphStore(m_graphStore);
                auto *palette = new ModulePalette(m_moduleRegistry, editor);
                editor->setPalette(palette);
                editor->loadGraph(id, m_graphStore);
                connectBlockEditorSignals(editor);
                m_codeEditor->openCustomTab(editor, name, path);
            }
        }
        return;
    }

    // UI-макет — открываем как вкладку
    if (ext == "dqui") {
        if (m_codeEditor->findCustomTabWidget(path)) {
            m_codeEditor->openCustomTab(nullptr, {}, path);
            return;
        }
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            QString id = QJsonDocument::fromJson(f.readAll()).object()["id"].toString();
            if (!id.isEmpty()) {
                auto *editor = new UIDesignerWidget(m_moduleRegistry, m_commandBus);
                editor->setLayoutStore(m_uiLayoutStore);
                editor->setFilePath(path);
                editor->loadLayout(id, m_uiLayoutStore);
                connectUIDesignerSignals(editor);
                m_codeEditor->openCustomTab(editor, name, path);
            }
        }
        return;
    }

    // Модуль — ищем граф с этим модулем и открываем как вкладку
    if (ext == "dqmod") {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            QString moduleId = QJsonDocument::fromJson(f.readAll()).object()["id"].toString();
            if (!moduleId.isEmpty() && m_graphStore) {
                for (auto *graph : m_graphStore->allGraphs()) {
                    for (const auto &node : graph->nodes) {
                        if (node.moduleId == moduleId) {
                            // Ищем файл графа, чтобы использовать как ключ вкладки
                            QString graphPath = m_projectManager->currentProject().projectDir
                                + "/graphs/" + graph->name + ".dqgraph";
                            if (m_codeEditor->findCustomTabWidget(graphPath)) {
                                m_codeEditor->openCustomTab(nullptr, {}, graphPath);
                            } else {
                                auto *editor = new BlockEditorWidget(m_moduleRegistry, m_commandBus);
                                editor->loadGraph(graph->id, m_graphStore);
                                connectBlockEditorSignals(editor);
                                m_codeEditor->openCustomTab(editor,
                                    graph->name + ".dqgraph", graphPath);
                            }
                            return;
                        }
                    }
                }
            }
        }
        return;
    }

    // .dqproj — игнорируем
    if (ext == "dqproj")
        return;

    // Прочие файлы — открываем как текст
    m_codeEditor->openFile(path);
}

void MainWindow::navigateBuildOutputLine(const QString &lineText)
{
    // Разбирает строку build output и ведёт пользователя либо к файлу, либо к origin.
    CompilerError error;
    if (!parseBuildOutputLine(lineText, &error))
        return;

    const QString resolvedPath = normalizeBuildErrorPath(error.file);
    if (resolvedPath.isEmpty()) {
        statusBar()->showMessage(tr("Build error path is empty"), 3000);
        return;
    }

    if (m_projectTree) {
        const QString originPath = m_projectTree->generatedOriginPath(resolvedPath);
        if (!originPath.isEmpty()) {
            // Из build output открываем origin как текстовый файл, чтобы трассировка ошибок
            // оставалась надёжной даже для .dqgraph/.dqui source-of-truth.
            switchToCodeEditor();
            m_codeEditor->openFile(originPath);
            statusBar()->showMessage(
                tr("Opened source-of-truth for build error: %1")
                    .arg(QFileInfo(originPath).fileName()),
                3000);
            return;
        }
    }

    if (!QFile::exists(resolvedPath)) {
        statusBar()->showMessage(
            tr("Build error file not found: %1").arg(error.file),
            3000);
        return;
    }

    openTextFileAtLocation(resolvedPath, error.line, error.column);
    statusBar()->showMessage(
        tr("Opened build error location: %1:%2")
            .arg(QFileInfo(resolvedPath).fileName())
            .arg(error.line),
        3000);
}

void MainWindow::updateEditorActions()
{
    // Пересчитывает доступность editor-actions по активной вкладке и её роли.
    const bool codeEditorActive = (m_centralStack->currentWidget() == m_codeEditor);

    bool canSave = false;
    bool canEditText = false;

    if (codeEditorActive) {
        if (auto *customTab = m_codeEditor->currentCustomTabWidget()) {
            canSave = qobject_cast<UIDesignerWidget *>(customTab)
                || qobject_cast<BlockEditorWidget *>(customTab);
        } else if (auto *tab = m_codeEditor->currentTab()) {
            canEditText = tab->editor() && !tab->editor()->isReadOnly();
            canSave = canEditText;
        }
    }

    if (auto *saveAction = m_actionManager->saveAction())
        saveAction->setEnabled(canSave);
    if (auto *renameAction = m_actionManager->action("edit.rename"))
        renameAction->setEnabled(canEditText);
    if (auto *formatAction = m_actionManager->action("edit.format"))
        formatAction->setEnabled(canEditText);

    updateGeneratedOriginAction();
}

void MainWindow::updateGeneratedOriginAction()
{
    // Разрешает переход к origin только для generated-файлов с распознанным источником.
    QAction *action = m_actionManager->action("edit.openGeneratedOrigin");
    if (!action)
        return;

    bool enabled = false;
    if (m_centralStack->currentWidget() == m_codeEditor) {
        if (auto *tab = m_codeEditor->currentTab()) {
            enabled = tab->property("deltaqGenerated").toBool()
                && !tab->property("deltaqGeneratedSourceKind").toString().isEmpty()
                && !tab->property("deltaqGeneratedSourceName").toString().isEmpty();
        }
    }

    action->setEnabled(enabled);
}

QString MainWindow::normalizeBuildErrorPath(const QString &filePath) const
{
    // Приводит путь из compiler output к реальному пути в проекте или build-директории.
    if (filePath.isEmpty())
        return {};

    QFileInfo info(filePath);
    if (info.isAbsolute())
        return QDir::cleanPath(info.absoluteFilePath());

    if (m_projectManager && m_projectManager->isProjectOpen()) {
        const QString projectDir = m_projectManager->projectDir();
        const QString buildCandidate = QDir::cleanPath(projectDir + "/build/" + filePath);
        if (QFile::exists(buildCandidate))
            return buildCandidate;

        const QString projectCandidate = QDir::cleanPath(projectDir + "/" + filePath);
        if (QFile::exists(projectCandidate))
            return projectCandidate;

        return buildCandidate;
    }

    return QDir::cleanPath(QDir::current().absoluteFilePath(filePath));
}

void MainWindow::openTextFileAtLocation(const QString &path, int line, int column)
{
    // Открывает текстовый файл и ставит курсор в строку/колонку из сообщения компилятора.
    onFileActivated(path);

    auto *tab = m_codeEditor->findTabForFile(path);
    if (!tab || !tab->editor())
        return;

    QTextBlock block = tab->editor()->document()->findBlockByNumber(qMax(0, line - 1));
    if (!block.isValid())
        return;

    const int blockOffset = qMin(qMax(0, column - 1), qMax(0, block.length() - 1));
    QTextCursor cursor(tab->editor()->document());
    cursor.setPosition(block.position() + blockOffset);
    tab->editor()->setTextCursor(cursor);
    tab->editor()->centerCursor();
    tab->editor()->setFocus();
}

void MainWindow::onOpenGeneratedOrigin()
{
    // Открывает source-of-truth для текущего generated-файла из редактора.
    auto *tab = m_codeEditor->currentTab();
    if (!tab) {
        statusBar()->showMessage(tr("No generated file selected"), 3000);
        return;
    }

    if (!tab->property("deltaqGenerated").toBool()) {
        statusBar()->showMessage(tr("Current file is not generated by DeltaQ"), 3000);
        return;
    }

    const QString sourceKind = tab->property("deltaqGeneratedSourceKind").toString();
    const QString sourceName = tab->property("deltaqGeneratedSourceName").toString();
    if (sourceKind.isEmpty() || sourceName.isEmpty()) {
        statusBar()->showMessage(tr("Generated file origin is unknown"), 3000);
        return;
    }

    if (!m_projectManager->isProjectOpen()) {
        statusBar()->showMessage(tr("No project open"), 3000);
        return;
    }

    QString sourcePath;
    if (sourceKind == "graph")
        sourcePath = m_projectManager->projectDir() + "/graphs/" + sourceName + ".dqgraph";
    else if (sourceKind == "UI layout")
        sourcePath = m_projectManager->projectDir() + "/ui/" + sourceName + ".dqui";
    else {
        statusBar()->showMessage(tr("Unsupported generated source kind: %1").arg(sourceKind), 3000);
        return;
    }

    if (!QFile::exists(sourcePath)) {
        QMessageBox::warning(this, tr("Generated Origin Not Found"),
                             tr("Could not find source-of-truth file:\n%1").arg(sourcePath));
        return;
    }

    onFileActivated(sourcePath);
    statusBar()->showMessage(tr("Opened source of generated file: %1").arg(sourceName), 3000);
}

void MainWindow::connectBlockEditorSignals(BlockEditorWidget *editor)
{
    if (!editor) return;
    BlockScene *scene = editor->scene();
    if (!scene) return;

    // Перетаскивание модуля из палитры → создание узла через AddNodeCommand
    connect(scene, &BlockScene::nodeDropped, this,
            [this, scene](const QString &moduleId, const QPointF &scenePos) {
        Graph *graph = m_graphStore->findGraph(scene->currentGraphId());
        if (!graph) return;
        auto node = GraphNode::create(moduleId, scenePos);
        m_commandBus->execute(std::make_unique<AddNodeCommand>(scene, graph, node));
    });

    // Соединение портов через ConnectCommand
    connect(scene, &BlockScene::connectionRequested, this,
            [this, scene](const QString &fromNodeId, const QString &fromPort,
                           const QString &toNodeId, const QString &toPort) {
        Graph *graph = m_graphStore->findGraph(scene->currentGraphId());
        if (!graph) return;
        GraphConnection conn;
        conn.from = {fromNodeId, fromPort};
        conn.to = {toNodeId, toPort};
        m_commandBus->execute(std::make_unique<ConnectCommand>(scene, graph, conn));
    });

    // Перемещение узла → MoveNodeCommand
    connect(scene, &BlockScene::nodeMovedByUser, this,
            [this, scene](const QString &nodeId, const QPointF &oldPos, const QPointF &newPos) {
        Graph *graph = m_graphStore->findGraph(scene->currentGraphId());
        if (!graph) return;
        m_commandBus->execute(std::make_unique<MoveNodeCommand>(scene, graph, nodeId, oldPos, newPos));
    });

    connect(editor, &BlockEditorWidget::modulePreviewRequested, this,
            &MainWindow::showModuleSourcePreview, Qt::UniqueConnection);
}

void MainWindow::showModuleSourcePreview(const QString &moduleId)
{
    if (!m_moduleRegistry)
        return;

    const Module *mod = m_moduleRegistry->findModule(moduleId);
    if (!mod)
        return;

    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Модуль: %1").arg(mod->name));
    dialog->resize(820, 560);

    auto *layout = new QVBoxLayout(dialog);
    auto *title = new QLabel(
        tr("<b>%1</b><br><span style='color:#999'>%2</span>")
            .arg(mod->name, mod->description.isEmpty() ? tr("Описание отсутствует") : mod->description),
        dialog);
    title->setTextFormat(Qt::RichText);
    title->setWordWrap(true);
    layout->addWidget(title);

    auto *codeView = new QPlainTextEdit(dialog);
    codeView->setReadOnly(true);
    codeView->setFont(QFont("Monospace", 10));
    codeView->setPlainText(
        mod->sourceCode.isEmpty()
            ? tr("// Исходный текст модуля недоступен")
            : mod->sourceCode);
    layout->addWidget(codeView, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    auto *editBtn = buttons->addButton(tr("Редактировать"), QDialogButtonBox::ActionRole);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    connect(editBtn, &QPushButton::clicked, this, [this, dialog, moduleId]() {
        switchToModuleManager();
        if (m_moduleManager)
            m_moduleManager->openModule(moduleId);
        dialog->close();
    });
    layout->addWidget(buttons);

    dialog->show();
}

void MainWindow::connectUIDesignerSignals(UIDesignerWidget *designer)
{
    // Generate Code — с именованием файлов по имени .dqui
    connect(designer, &UIDesignerWidget::generateCodeRequested, this, [this, designer]() {
        if (!m_projectManager->isProjectOpen()) {
            statusBar()->showMessage(tr("No project open"), 3000);
            return;
        }

        // Определяем baseName из пути к .dqui файлу
        QString baseName = "ui";
        QString filePath = designer->filePath();
        if (!filePath.isEmpty()) {
            baseName = QFileInfo(filePath).baseName();
        }

        UILayout layout = designer->scene()->toLayout(baseName);
        GeneratedCode code = SDL2CodeGenerator::generate(layout, baseName);

        // Генерируем UI-файлы в папку ui/ (не в src/)
        QString uiDir = m_projectManager->projectDir() + "/ui";
        QDir().mkpath(uiDir);

        auto writeFile = [&](const QString &dir, const QString &name, const QString &content) {
            QFile f(dir + "/" + name);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(content.toUtf8());
                f.close();
            }
        };

        writeFile(uiDir, baseName + ".h", code.uiHeader);
        writeFile(uiDir, baseName + ".c", code.uiSource);
        writeFile(uiDir, baseName + "_events.h", code.eventsHeader);
        writeFile(uiDir, baseName + "_events.c", code.eventsSource);

        m_buildOutput->clear();
        appendBuildOutputChunk(tr("=== SDL2 code generated in %1 ===\n").arg(uiDir));
        appendBuildOutputChunk(tr("Files: %1.h, %1.c, %1_events.h, %1_events.c")
                                   .arg(baseName));
        m_outputTabs->setCurrentWidget(m_buildOutput);
        m_outputDock->show();
        statusBar()->showMessage(tr("SDL2 code generated"), 3000);
    });

    // Двойной клик по виджету → создать/открыть граф-обработчик события
    connect(designer, &UIDesignerWidget::openEventHandler, this,
        [this, designer](const QString &widgetId, const QString &widgetName, const QString &eventName) {

        // Формируем имя графа (латиницей для совместимости файловых систем)
        QString graphName = eventName + "_" + widgetName;

        // Ищем существующий граф с таким именем
        Graph *existing = m_graphStore->findGraphByName(graphName);

        if (!existing) {
            // Создаём новый граф
            Graph newGraph = Graph::create(graphName);
            m_graphStore->registerGraph(newGraph);

            // Сохраняем .dqgraph файл
            if (m_projectManager->isProjectOpen()) {
                m_graphStore->saveAll(m_projectManager->projectDir());
            }

            existing = m_graphStore->findGraphByName(graphName);
        }

        if (!existing) return;

        // Привязываем событие к виджету
        auto *item = designer->scene()->widgetItem(widgetId);
        if (item) {
            item->setEvent(eventName, "graph:" + existing->id);
        }

        // Открываем граф в редакторе
        QString graphPath = m_projectManager->projectDir() + "/graphs/" + graphName + ".dqgraph";

        if (m_codeEditor->findCustomTabWidget(graphPath)) {
            m_codeEditor->openCustomTab(nullptr, {}, graphPath);
        } else {
            auto *editor = new BlockEditorWidget(m_moduleRegistry, m_commandBus);
            editor->setGraphStore(m_graphStore);
            editor->scene()->setGraphStore(m_graphStore);
            auto *palette = new ModulePalette(m_moduleRegistry, editor);
            editor->setPalette(palette);
            editor->loadGraph(existing->id, m_graphStore);
            connectBlockEditorSignals(editor);
            m_codeEditor->openCustomTab(editor, graphName + ".dqgraph", graphPath);
        }

        // Обновить дерево проекта
        if (m_projectTree && m_projectManager->isProjectOpen())
            m_projectTree->setRootPath(m_projectManager->projectDir());
    });

    // Preview
    connect(designer, &UIDesignerWidget::previewRequested, this, [this, designer]() {
        UILayout layout = designer->scene()->toLayout("preview");
        auto *preview = new UIPreview(this);
        connect(preview, &UIPreview::previewError, this, [this](const QString &err) {
            appendBuildOutputChunk(err);
            m_outputTabs->setCurrentWidget(m_buildOutput);
            m_outputDock->show();
        });
        connect(preview, &UIPreview::buildOutput, this, [this](const QString &text) {
            appendBuildOutputChunk(text);
        });
        connect(preview, &UIPreview::previewStarted, this, [this]() {
            statusBar()->showMessage(tr("Preview started"), 3000);
        });
        preview->startPreview(layout);
    });
}

void MainWindow::updateTitle()
{
    QString title = "DeltaQ IDE";
    if (m_projectManager->isProjectOpen())
        title += " — " + m_projectManager->currentProject().name;
    setWindowTitle(title);
}

void MainWindow::updateStatusBar(const QString &message)
{
    statusBar()->showMessage(message, 3000);
}

void MainWindow::updateCursorPosition()
{
    // Следит только за активным редактором, чтобы статус-бар не копил лишние подключения.
    auto *tab = m_codeEditor->currentTab();
    auto *editor = tab ? tab->editor() : nullptr;

    if (m_trackedCursorEditor != editor) {
        if (m_cursorPositionConnection)
            disconnect(m_cursorPositionConnection);
        m_trackedCursorEditor = editor;
        if (editor) {
            m_cursorPositionConnection = connect(editor, &QPlainTextEdit::cursorPositionChanged,
                                                 this, &MainWindow::updateCursorPosition);
        } else {
            m_cursorPositionConnection = {};
        }
    }

    if (!editor) {
        m_cursorPosLabel->setText("Ln 1, Col 1");
        return;
    }

    auto cursor = editor->textCursor();
    int line = cursor.blockNumber() + 1;
    int col = cursor.columnNumber() + 1;
    m_cursorPosLabel->setText(QString("Ln %1, Col %2").arg(line).arg(col));
}

void MainWindow::onCloseProject()
{
    if (!m_projectManager->isProjectOpen()) {
        statusBar()->showMessage(tr("No project open"), 3000);
        return;
    }

    // Сохраняем проект перед закрытием (UILayoutStore, GraphStore → диск)
    m_projectManager->saveProject();

    m_projectManager->closeProject();
    m_codeEditor->closeAllTabs();
    m_projectTree->setRootPath("");
    updateTitle();
    statusBar()->showMessage(tr("Project closed"), 3000);
}

void MainWindow::onSettings()
{
    SettingsDialog dlg(m_sessionManager, this);
    dlg.setModuleRegistry(m_moduleRegistry);
    dlg.exec();
}

void MainWindow::onNewFile()
{
    if (!m_projectManager->isProjectOpen()) {
        statusBar()->showMessage(tr("No project open"), 3000);
        return;
    }

    NewFileDialog dlg(m_projectManager->projectDir(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QString type = dlg.fileType();
    QString name = dlg.fileName();
    QString path = dlg.fullPath();
    if (name.isEmpty() || path.isEmpty()) return;

    // Создаём директорию
    QDir().mkpath(QFileInfo(path).absolutePath());

    if (type == "dqui") {
        // Создаём UILayout и сохраняем
        UILayout layout = UILayout::create(name);
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(QJsonDocument(layout.toJson()).toJson(QJsonDocument::Indented));
            f.close();
        }
        // Регистрируем в UILayoutStore
        m_uiLayoutStore->registerLayout(layout);
    } else if (type == "dqgraph") {
        // Создаём Graph и сохраняем
        Graph graph = Graph::create(name);
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(QJsonDocument(graph.toJson()).toJson(QJsonDocument::Indented));
            f.close();
        }
        m_graphStore->registerGraph(graph);
    } else if (type == "dqmod") {
        // Создаём Module и сохраняем
        Module mod = Module::create(name);
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(QJsonDocument(mod.toJson()).toJson(QJsonDocument::Indented));
            f.close();
        }
        m_moduleRegistry->registerModule(mod);
    } else {
        // Обычный C/H файл — создаём пустой
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            if (type == "h") {
                // Добавить include guard
                QString guard = name.toUpper() + "_H";
                f.write(QString("#ifndef %1\n#define %1\n\n\n\n#endif // %1\n")
                    .arg(guard).toUtf8());
            }
            f.close();
        }
    }

    // Обновить дерево проекта
    m_projectTree->setRootPath(m_projectManager->projectDir());

    // Открыть файл во вкладке
    onFileActivated(path);
    statusBar()->showMessage(tr("File '%1' created").arg(QFileInfo(path).fileName()), 3000);
}

void MainWindow::updateRecentProjectsMenu()
{
    if (!m_recentProjectsMenu)
        return;

    m_recentProjectsMenu->clear();

    QStringList recent = m_sessionManager->recentProjects();

    if (recent.isEmpty()) {
        auto *emptyAction = m_recentProjectsMenu->addAction(tr("No recent projects"));
        emptyAction->setEnabled(false);
        return;
    }

    for (const QString &path : recent) {
        QFileInfo fi(path);
        QString label = fi.dir().dirName();
        auto *action = m_recentProjectsMenu->addAction(label);
        action->setToolTip(path);
        action->setData(path);
        connect(action, &QAction::triggered, this, [this, path]() {
            if (m_projectManager->openProject(path)) {
                m_projectTree->setRootPath(m_projectManager->projectDir());
                m_sessionManager->addRecentProject(path);
                updateRecentProjectsMenu();
                statusBar()->showMessage(tr("Project opened"), 3000);
            } else {
                QMessageBox::warning(this, tr("Error"), tr("Failed to open project: %1").arg(path));
            }
        });
    }

    m_recentProjectsMenu->addSeparator();
    auto *clearAction = m_recentProjectsMenu->addAction(tr("Clear History"));
    connect(clearAction, &QAction::triggered, this, [this]() {
        m_sessionManager->clearRecentProjects();
        updateRecentProjectsMenu();
    });
}

void MainWindow::restoreSession()
{
    auto geom = m_sessionManager->windowGeometry();
    if (!geom.isEmpty())
        restoreGeometry(geom);
    auto state = m_sessionManager->windowState();
    if (!state.isEmpty())
        restoreState(state);

    // Пустой запуск — проект и вкладки не восстанавливаются
}

void MainWindow::saveSession()
{
    m_sessionManager->setWindowGeometry(saveGeometry());
    m_sessionManager->setWindowState(saveState());
}

} // namespace DeltaQ
