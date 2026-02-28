#include "MainWindow.h"
#include "CommandBus.h"
#include "ModuleRegistry.h"
#include "GraphStore.h"
#include "UILayoutStore.h"
#include "ProjectManager.h"
#include "SessionManager.h"
#include "ActionManager.h"
#include "UndoManager.h"

#include "../editor/CodeEditorWidget.h"
#include "../editor/ProjectTreeView.h"
#include "../blockEditor/BlockEditorWidget.h"
#include "../uiDesigner/UIDesignerWidget.h"
#include "../libProcessor/LibProcessorWidget.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QActionGroup>

namespace DeltaQ {

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
    m_uiLayoutStore = new UILayoutStore(this);
    m_projectManager = new ProjectManager(m_moduleRegistry, m_graphStore, m_uiLayoutStore, this);
    m_sessionManager = new SessionManager(this);
    m_actionManager = new ActionManager(this);
    m_undoManager = new UndoManager(m_commandBus, this);

    m_actionManager->setupStandardActions();
}

void MainWindow::setupUI()
{
    m_centralStack = new QStackedWidget(this);
    setCentralWidget(m_centralStack);

    // Module 1: Code Editor
    m_codeEditor = new CodeEditorWidget(m_commandBus, m_moduleRegistry, this);
    m_centralStack->addWidget(m_codeEditor);

    // Module 2: Block Editor
    m_blockEditor = new BlockEditorWidget(m_moduleRegistry, m_commandBus, this);
    m_centralStack->addWidget(m_blockEditor);

    // Module 3: UI Designer
    m_uiDesigner = new UIDesignerWidget(m_moduleRegistry, m_commandBus, this);
    m_centralStack->addWidget(m_uiDesigner);

    // Module 4: Library Processor
    m_libProcessor = new LibProcessorWidget(m_moduleRegistry, this);
    m_centralStack->addWidget(m_libProcessor);

    m_centralStack->setCurrentIndex(0); // Start with code editor
}

void MainWindow::setupMenus()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_actionManager->newProjectAction());
    fileMenu->addAction(m_actionManager->openProjectAction());
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

    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_actionManager->action("view.codeEditor"));
    viewMenu->addAction(m_actionManager->action("view.blockEditor"));
    viewMenu->addAction(m_actionManager->action("view.uiDesigner"));
    viewMenu->addAction(m_actionManager->action("view.libProcessor"));

    auto *buildMenu = menuBar()->addMenu(tr("&Build"));
    buildMenu->addAction(m_actionManager->buildAction());
    buildMenu->addAction(m_actionManager->runAction());
    buildMenu->addAction(m_actionManager->action("build.clean"));

    auto *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    auto *langMenu = settingsMenu->addMenu(tr("Language"));

    auto *langGroup = new QActionGroup(this);
    langGroup->setExclusive(true);

    // Названия языков — всегда на родном языке, без tr()
    auto *langEn = langMenu->addAction("English");
    langEn->setCheckable(true);
    langEn->setData("en");
    langGroup->addAction(langEn);

    auto *langRu = langMenu->addAction(QString::fromUtf8("Русский"));
    langRu->setCheckable(true);
    langRu->setData("ru");
    langGroup->addAction(langRu);

    // Отмечаем текущий язык
    QString currentLang = m_sessionManager->language();
    if (currentLang == "ru")
        langRu->setChecked(true);
    else
        langEn->setChecked(true);

    connect(langGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        QString newLang = action->data().toString();
        if (newLang == m_sessionManager->language())
            return;
        m_sessionManager->setLanguage(newLang);
        QMessageBox::information(this, tr("Language Changed"),
            tr("The language will be changed after restarting the application."));
    });

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

    auto *buildOutput = new QTextEdit(this);
    buildOutput->setReadOnly(true);
    buildOutput->setFont(QFont("Monospace", 10));
    m_outputTabs->addTab(buildOutput, tr("Build Output"));

    auto *appOutput = new QTextEdit(this);
    appOutput->setReadOnly(true);
    appOutput->setFont(QFont("Monospace", 10));
    m_outputTabs->addTab(appOutput, tr("Application Output"));

    m_outputDock->setWidget(m_outputTabs);
    addDockWidget(Qt::BottomDockWidgetArea, m_outputDock);
}

void MainWindow::setupConnections()
{
    auto *am = m_actionManager;

    connect(am->newProjectAction(), &QAction::triggered, this, &MainWindow::onNewProject);
    connect(am->openProjectAction(), &QAction::triggered, this, &MainWindow::onOpenProject);
    connect(am->saveAction(), &QAction::triggered, this, &MainWindow::onSaveFile);
    connect(am->action("file.quit"), &QAction::triggered, qApp, &QApplication::quit);

    connect(am->undoAction(), &QAction::triggered, m_undoManager, &UndoManager::undo);
    connect(am->redoAction(), &QAction::triggered, m_undoManager, &UndoManager::redo);

    connect(am->buildAction(), &QAction::triggered, this, &MainWindow::onBuild);
    connect(am->runAction(), &QAction::triggered, this, &MainWindow::onRun);

    connect(am->action("view.codeEditor"), &QAction::triggered, this, &MainWindow::switchToCodeEditor);
    connect(am->action("view.blockEditor"), &QAction::triggered, this, &MainWindow::switchToBlockEditor);
    connect(am->action("view.uiDesigner"), &QAction::triggered, this, &MainWindow::switchToUIDesigner);
    connect(am->action("view.libProcessor"), &QAction::triggered, this, &MainWindow::switchToLibProcessor);

    connect(m_projectManager, &ProjectManager::projectOpened, this, &MainWindow::updateTitle);
    connect(m_projectManager, &ProjectManager::projectClosed, this, &MainWindow::updateTitle);

    connect(m_commandBus, &CommandBus::commandExecuted, this, &MainWindow::updateStatusBar);

    // Project tree: open files in editor
    connect(m_projectTree, &ProjectTreeView::fileSelected, m_codeEditor, &CodeEditorWidget::openFile);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSession();
    event->accept();
}

void MainWindow::onNewProject()
{
    QString name = QInputDialog::getText(this, tr("New Project"), tr("Project name:"));
    if (name.isEmpty())
        return;

    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Project Directory"));
    if (dir.isEmpty())
        return;

    dir = dir + "/" + name;

    if (m_projectManager->createProject(name, dir)) {
        m_projectTree->setRootPath(dir);
        m_sessionManager->addRecentProject(m_projectManager->currentProject().projectFilePath);
        statusBar()->showMessage(tr("Project '%1' created").arg(name), 3000);
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
        statusBar()->showMessage(tr("Project opened"), 3000);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to open project"));
    }
}

void MainWindow::onSaveFile()
{
    m_codeEditor->saveCurrentFile();
}

void MainWindow::onBuild()
{
    if (!m_projectManager->isProjectOpen()) {
        statusBar()->showMessage(tr("No project open"), 3000);
        return;
    }
    m_codeEditor->buildProject(m_projectManager->projectDir());
}

void MainWindow::onRun()
{
    statusBar()->showMessage(tr("Run — not yet implemented"), 3000);
}

void MainWindow::switchToCodeEditor() { m_centralStack->setCurrentWidget(m_codeEditor); }
void MainWindow::switchToBlockEditor() { m_centralStack->setCurrentWidget(m_blockEditor); }
void MainWindow::switchToUIDesigner() { m_centralStack->setCurrentWidget(m_uiDesigner); }
void MainWindow::switchToLibProcessor() { m_centralStack->setCurrentWidget(m_libProcessor); }

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

void MainWindow::restoreSession()
{
    auto geom = m_sessionManager->windowGeometry();
    if (!geom.isEmpty())
        restoreGeometry(geom);
    auto state = m_sessionManager->windowState();
    if (!state.isEmpty())
        restoreState(state);
}

void MainWindow::saveSession()
{
    m_sessionManager->setWindowGeometry(saveGeometry());
    m_sessionManager->setWindowState(saveState());
}

} // namespace DeltaQ
