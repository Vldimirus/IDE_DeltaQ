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
#include "../editor/CodeEditorTab.h"
#include "../editor/ProjectTreeView.h"
#include "../editor/BuildManager.h"
#include "../lsp/LSPClient.h"
#include "../lsp/LSPTypes.h"
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
#include <QTextCursor>

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

    m_buildManager = new BuildManager(this);

    // LSP-клиент
    m_lspClient = new LSPClient(this);

    m_actionManager->setupStandardActions();
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
    editMenu->addAction(m_actionManager->action("edit.replace"));
    editMenu->addSeparator();
    editMenu->addAction(m_actionManager->action("edit.goToLine"));
    editMenu->addAction(m_actionManager->action("edit.goToDefinition"));
    editMenu->addAction(m_actionManager->action("edit.findReferences"));

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
    m_buildOutput->setReadOnly(true);
    m_buildOutput->setFont(QFont("Monospace", 10));
    m_outputTabs->addTab(m_buildOutput, tr("Build Output"));

    m_appOutput = new QTextEdit(this);
    m_appOutput->setReadOnly(true);
    m_appOutput->setFont(QFont("Monospace", 10));
    m_outputTabs->addTab(m_appOutput, tr("Application Output"));

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

    connect(am->action("edit.find"), &QAction::triggered, m_codeEditor, &CodeEditorWidget::showFind);
    connect(am->action("edit.replace"), &QAction::triggered, m_codeEditor, &CodeEditorWidget::showReplace);
    connect(am->action("edit.goToLine"), &QAction::triggered, m_codeEditor, &CodeEditorWidget::goToLine);
    connect(am->action("edit.goToDefinition"), &QAction::triggered, m_codeEditor, &CodeEditorWidget::goToDefinition);
    connect(am->action("edit.findReferences"), &QAction::triggered, m_codeEditor, &CodeEditorWidget::findReferences);

    connect(am->buildAction(), &QAction::triggered, this, &MainWindow::onBuild);
    connect(am->action("build.clean"), &QAction::triggered, this, &MainWindow::onClean);
    connect(am->runAction(), &QAction::triggered, this, &MainWindow::onRun);

    connect(am->action("view.codeEditor"), &QAction::triggered, this, &MainWindow::switchToCodeEditor);
    connect(am->action("view.blockEditor"), &QAction::triggered, this, &MainWindow::switchToBlockEditor);
    connect(am->action("view.uiDesigner"), &QAction::triggered, this, &MainWindow::switchToUIDesigner);
    connect(am->action("view.libProcessor"), &QAction::triggered, this, &MainWindow::switchToLibProcessor);

    connect(m_projectManager, &ProjectManager::projectOpened, this, &MainWindow::updateTitle);
    connect(m_projectManager, &ProjectManager::projectClosed, this, &MainWindow::updateTitle);

    connect(m_commandBus, &CommandBus::commandExecuted, this, &MainWindow::updateStatusBar);

    // Вывод сборки в Output dock
    connect(m_buildManager, &BuildManager::buildOutput, this, [this](const QString &text) {
        m_buildOutput->append(text);
        m_outputTabs->setCurrentWidget(m_buildOutput);
        m_outputDock->show();
    });
    connect(m_buildManager, &BuildManager::buildStarted, this, [this]() {
        m_buildOutput->clear();
        m_actionManager->buildAction()->setEnabled(false);
        statusBar()->showMessage(tr("Building..."));
    });
    connect(m_buildManager, &BuildManager::buildFinished, this, [this](bool success, int errors, int warnings) {
        m_actionManager->buildAction()->setEnabled(true);
        if (success)
            statusBar()->showMessage(tr("Build succeeded"), 5000);
        else
            statusBar()->showMessage(tr("Build failed: %1 error(s), %2 warning(s)")
                .arg(errors).arg(warnings), 5000);
    });

    // Навигация к ошибке: клик по строке вывода сборки → переход в редактор
    connect(m_buildManager, &BuildManager::buildError, this,
            [this](const QString &file, int line, int /*column*/,
                   const QString &/*severity*/, const QString &/*message*/) {
        Q_UNUSED(file)
        Q_UNUSED(line)
        // Ошибки собираются в CompilerOutputParser и доступны через buildManager
    });

    // Двойной клик по строке в Build Output → навигация к ошибке
    connect(m_buildOutput, &QTextEdit::cursorPositionChanged, this, []() {
        // Обработка через контекстное меню или double-click (см. ниже)
    });

    // Дерево проекта: открытие файлов в редакторе
    connect(m_projectTree, &ProjectTreeView::fileSelected, m_codeEditor, &CodeEditorWidget::openFile);

    // Позиция курсора в редакторе → статус-бар
    connect(m_codeEditor, &CodeEditorWidget::currentTabChanged, this, &MainWindow::updateCursorPosition);

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
    connect(m_lspClient, &LSPClient::diagnosticsReceived, this,
            [this](const QString &uri, const QVector<LSPDiagnostic> &diagnostics) {
        Q_UNUSED(uri)
        Q_UNUSED(diagnostics)
        // TODO: реализовать подчёркивание ошибок через ExtraSelections
        // Пока просто показываем количество в статус-баре
        int errors = 0, warnings = 0;
        for (const auto &d : diagnostics) {
            if (d.severity == DiagnosticSeverity::Error) ++errors;
            else if (d.severity == DiagnosticSeverity::Warning) ++warnings;
        }
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
    m_buildManager->build(m_projectManager->projectDir(),
                          m_projectManager->currentProject().name,
                          m_projectManager->currentProject().build.standard);
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

void MainWindow::updateCursorPosition()
{
    auto *tab = m_codeEditor->currentTab();
    if (!tab || !tab->editor()) {
        m_cursorPosLabel->setText("Ln 1, Col 1");
        return;
    }

    auto *editor = tab->editor();
    // Подключаем обновление при перемещении курсора (один раз на таб)
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, [this]() {
        auto *t = m_codeEditor->currentTab();
        if (!t || !t->editor()) return;
        auto cursor = t->editor()->textCursor();
        int line = cursor.blockNumber() + 1;
        int col = cursor.columnNumber() + 1;
        m_cursorPosLabel->setText(QString("Ln %1, Col %2").arg(line).arg(col));
    }, Qt::UniqueConnection);

    // Обновляем сразу
    auto cursor = editor->textCursor();
    int line = cursor.blockNumber() + 1;
    int col = cursor.columnNumber() + 1;
    m_cursorPosLabel->setText(QString("Ln %1, Col %2").arg(line).arg(col));
}

void MainWindow::restoreSession()
{
    auto geom = m_sessionManager->windowGeometry();
    if (!geom.isEmpty())
        restoreGeometry(geom);
    auto state = m_sessionManager->windowState();
    if (!state.isEmpty())
        restoreState(state);

    // Восстановление последнего проекта
    QString lastProject = m_sessionManager->lastOpenedProject();
    if (!lastProject.isEmpty() && QFile::exists(lastProject)) {
        if (m_projectManager->openProject(lastProject))
            m_projectTree->setRootPath(m_projectManager->projectDir());
    }

    // Восстановление открытых вкладок
    for (const auto &path : m_sessionManager->openTabs()) {
        if (QFile::exists(path))
            m_codeEditor->openFile(path);
    }
}

void MainWindow::saveSession()
{
    m_sessionManager->setWindowGeometry(saveGeometry());
    m_sessionManager->setWindowState(saveState());

    // Сохранение открытых вкладок
    m_sessionManager->setOpenTabs(m_codeEditor->openFilePaths());

    // Сохранение текущего проекта
    if (m_projectManager->isProjectOpen())
        m_sessionManager->setLastOpenedProject(m_projectManager->currentProject().projectFilePath);
}

} // namespace DeltaQ
