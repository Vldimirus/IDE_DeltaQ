#include "CodeEditorWidget.h"
#include "CodeEditorTab.h"
#include "CompletionPopup.h"
#include "FindReplaceBar.h"
#include "BuildManager.h"
#include "AnnotationParser.h"
#include "../lsp/LSPClient.h"
#include "../core/ModuleRegistry.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QFile>
#include <QJsonDocument>
#include <QToolTip>
#include <QPlainTextEdit>
#include <QTextCursor>

namespace DeltaQ {

CodeEditorWidget::CodeEditorWidget(CommandBus *bus, ModuleRegistry *registry,
                                     QWidget *parent)
    : QWidget(parent)
    , m_commandBus(bus)
    , m_moduleRegistry(registry)
    , m_annotationParser(new AnnotationParser(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    layout->addWidget(m_tabWidget);

    // Панель поиска и замены (под вкладками)
    m_findBar = new FindReplaceBar(this);
    layout->addWidget(m_findBar);

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &CodeEditorWidget::closeTab);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &CodeEditorWidget::onTabChanged);
}

void CodeEditorWidget::closeAllTabs()
{
    while (m_tabWidget->count() > 0)
        closeTab(0);
}

void CodeEditorWidget::openFile(const QString &path)
{
    // Check if already open
    auto *existing = findTabForFile(path);
    if (existing) {
        m_tabWidget->setCurrentWidget(existing);
        return;
    }

    auto *tab = new CodeEditorTab(path, m_commandBus, this);
    if (!tab->loadFile(path)) {
        delete tab;
        QMessageBox::warning(this, tr("Error"),
                             tr("Cannot open file: %1").arg(path));
        return;
    }

    QFileInfo fi(path);
    int index = m_tabWidget->addTab(tab, fi.fileName());
    m_tabWidget->setTabToolTip(index, path);
    m_tabWidget->setCurrentIndex(index);
    m_openFiles[path] = index;

    // Подключаем сигналы вкладки
    connectTabSignals(tab);

    // LSP: уведомляем сервер об открытии файла
    if (m_lspClient && m_lspClient->isRunning()) {
        QString uri = LSPClient::pathToUri(path);
        QString langId = path.endsWith(".h") || path.endsWith(".hpp") ? "cpp" :
                          path.endsWith(".c") ? "c" : "cpp";
        m_documentVersions[uri] = 1;
        m_lspClient->didOpen(uri, langId, tab->editor()->toPlainText());

        // Подключаем отправку изменений при редактировании
        connect(tab->editor()->document(), &QTextDocument::contentsChanged, this, [this, tab]() {
            if (!m_lspClient || !m_lspClient->isRunning())
                return;
            QString uri = LSPClient::pathToUri(tab->filePath());
            int &ver = m_documentVersions[uri];
            ++ver;
            m_lspClient->didChange(uri, ver, tab->editor()->toPlainText());
        });
    }

    connect(tab, &CodeEditorTab::modificationChanged, this, [this, tab](bool modified) {
        int idx = m_tabWidget->indexOf(tab);
        if (idx >= 0) {
            QString title = QFileInfo(tab->filePath()).fileName();
            if (modified)
                title += " *";
            m_tabWidget->setTabText(idx, title);
        }
    });
}

void CodeEditorWidget::connectTabSignals(CodeEditorTab *tab)
{
    // Hover: tab → LSPClient → tab
    connect(tab, &CodeEditorTab::hoverRequested,
            this, [this](const QString &filePath, int line, int character) {
        if (!m_lspClient || !m_lspClient->isRunning()) return;
        QString uri = LSPClient::pathToUri(filePath);
        m_lspClient->hover(uri, line, character);
    });

    // Completion: tab → LSPClient
    connect(tab, &CodeEditorTab::completionRequested,
            this, [this](const QString &filePath, int line, int character) {
        if (!m_lspClient || !m_lspClient->isRunning()) return;
        QString uri = LSPClient::pathToUri(filePath);
        m_lspClient->completion(uri, line, character);
    });

    // Breakpoint: tab → MainWindow (через сигнал)
    connect(tab, &CodeEditorTab::breakpointToggleRequested,
            this, &CodeEditorWidget::breakpointToggleRequested);

    // Вставка текста из popup автодополнения
    if (tab->completionPopup()) {
        connect(tab->completionPopup(), &CompletionPopup::itemSelected,
                this, [tab](const QString &text) {
            QTextCursor cursor = tab->editor()->textCursor();
            // Удаляем слово перед курсором и вставляем выбранное
            cursor.select(QTextCursor::WordUnderCursor);
            cursor.insertText(text);
        });
    }
}

void CodeEditorWidget::saveCurrentFile()
{
    auto *tab = currentTab();
    if (tab) {
        tab->saveFile();
        // LSP: уведомляем о сохранении
        if (m_lspClient && m_lspClient->isRunning())
            m_lspClient->didSave(LSPClient::pathToUri(tab->filePath()));

        // Автогенерация .dqmod из аннотаций
        parseAndSaveModules(tab->filePath());

        emit fileSaved(tab->filePath());
    }
}

void CodeEditorWidget::parseAndSaveModules(const QString &filePath)
{
    // Парсим только C/C++ файлы
    if (!filePath.endsWith(".c") && !filePath.endsWith(".cpp") &&
        !filePath.endsWith(".cxx") && !filePath.endsWith(".cc") &&
        !filePath.endsWith(".h") && !filePath.endsWith(".hpp"))
        return;

    auto modules = m_annotationParser->parseFile(filePath);
    if (modules.isEmpty())
        return;

    // Сохраняем каждый модуль как .dqmod и регистрируем в реестре
    for (const auto &mod : modules) {
        // Сохраняем .dqmod рядом с исходником
        QString dqmodPath = AnnotationParser::dqmodPathForSource(filePath);
        if (modules.size() > 1) {
            // Несколько модулей в одном файле — добавляем имя модуля в путь
            QFileInfo fi(filePath);
            dqmodPath = fi.absolutePath() + "/" + mod.name + ".dqmod";
        }

        QFile file(dqmodPath);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(mod.toJson());
            file.write(doc.toJson(QJsonDocument::Indented));
        }

        // Регистрируем/обновляем в реестре
        if (m_moduleRegistry)
            m_moduleRegistry->registerModule(mod);
    }
}

bool CodeEditorWidget::hasUnsavedChanges() const
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *tab = qobject_cast<CodeEditorTab *>(m_tabWidget->widget(i));
        if (tab && tab->isModified())
            return true;
    }
    return false;
}

void CodeEditorWidget::buildProject(const QString &projectDir)
{
    emit buildRequested(projectDir);
}

CodeEditorTab *CodeEditorWidget::currentTab() const
{
    return qobject_cast<CodeEditorTab *>(m_tabWidget->currentWidget());
}

QString CodeEditorWidget::currentFilePath() const
{
    auto *tab = currentTab();
    return tab ? tab->filePath() : QString();
}

QStringList CodeEditorWidget::openFilePaths() const
{
    QStringList paths;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *tab = qobject_cast<CodeEditorTab *>(m_tabWidget->widget(i));
        if (tab)
            paths.append(tab->filePath());
    }
    return paths;
}

void CodeEditorWidget::closeTab(int index)
{
    auto *tab = qobject_cast<CodeEditorTab *>(m_tabWidget->widget(index));
    if (!tab) return;

    if (tab->isModified()) {
        auto result = QMessageBox::question(this, tr("Save Changes"),
            tr("Save changes to %1?").arg(QFileInfo(tab->filePath()).fileName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (result == QMessageBox::Cancel)
            return;
        if (result == QMessageBox::Save)
            tab->saveFile();
    }

    // LSP: уведомляем о закрытии документа
    if (m_lspClient && m_lspClient->isRunning()) {
        QString uri = LSPClient::pathToUri(tab->filePath());
        m_lspClient->didClose(uri);
        m_documentVersions.remove(uri);
    }

    m_openFiles.remove(tab->filePath());
    m_tabWidget->removeTab(index);
    delete tab;

    // Update indices
    m_openFiles.clear();
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *t = qobject_cast<CodeEditorTab *>(m_tabWidget->widget(i));
        if (t)
            m_openFiles[t->filePath()] = i;
    }
}

void CodeEditorWidget::onTabChanged(int /*index*/)
{
    // Обновляем редактор в панели поиска при смене вкладки
    auto *tab = currentTab();
    m_findBar->setEditor(tab ? tab->editor() : nullptr);
    emit currentTabChanged();
}

void CodeEditorWidget::setLSPClient(LSPClient *client)
{
    m_lspClient = client;

    if (!m_lspClient) return;

    // Hover-результат → показываем подсказку
    connect(m_lspClient, &LSPClient::hoverResult, this, [this](const HoverInfo &info) {
        auto *tab = currentTab();
        if (!tab) return;
        if (info.contents.isEmpty()) return;

        // Показываем tooltip в позиции мыши
        QPoint globalPos = QCursor::pos();
        tab->showHoverTooltip(info.contents, globalPos);
    });

    // Completion-результат → показываем popup
    connect(m_lspClient, &LSPClient::completionResult, this,
            [this](const QVector<CompletionItem> &items) {
        auto *tab = currentTab();
        if (!tab) return;
        tab->showCompletion(items);
    });

    // Rename-результат → применяем правки
    connect(m_lspClient, &LSPClient::renameResult, this,
            [this](const WorkspaceEdit &edits) {
        for (auto it = edits.changes.begin(); it != edits.changes.end(); ++it) {
            QString path = LSPClient::uriToPath(it.key());
            auto *tab = findTabForFile(path);
            if (!tab) {
                openFile(path);
                tab = findTabForFile(path);
            }
            if (!tab) continue;

            auto *editor = tab->editor();
            QTextCursor cursor(editor->document());
            cursor.beginEditBlock();

            // Применяем правки в обратном порядке (от конца к началу)
            QVector<LSPTextEdit> sortedEdits = it.value();
            std::sort(sortedEdits.begin(), sortedEdits.end(),
                      [](const LSPTextEdit &a, const LSPTextEdit &b) {
                if (a.range.start.line != b.range.start.line)
                    return a.range.start.line > b.range.start.line;
                return a.range.start.character > b.range.start.character;
            });

            for (const auto &edit : sortedEdits) {
                QTextBlock startBlock = editor->document()->findBlockByNumber(edit.range.start.line);
                QTextBlock endBlock = editor->document()->findBlockByNumber(edit.range.end.line);
                if (!startBlock.isValid()) continue;
                if (!endBlock.isValid()) endBlock = startBlock;

                int startPos = startBlock.position() + qMin(edit.range.start.character, startBlock.length() - 1);
                int endPos = endBlock.position() + qMin(edit.range.end.character, endBlock.length() - 1);

                cursor.setPosition(startPos);
                cursor.setPosition(endPos, QTextCursor::KeepAnchor);
                cursor.insertText(edit.newText);
            }

            cursor.endEditBlock();
        }
    });

    // Formatting-результат → применяем правки
    connect(m_lspClient, &LSPClient::formattingResult, this,
            [this](const QVector<LSPTextEdit> &edits) {
        auto *tab = currentTab();
        if (!tab) return;

        auto *editor = tab->editor();
        QTextCursor cursor(editor->document());
        cursor.beginEditBlock();

        // Применяем правки в обратном порядке
        QVector<LSPTextEdit> sortedEdits = edits;
        std::sort(sortedEdits.begin(), sortedEdits.end(),
                  [](const LSPTextEdit &a, const LSPTextEdit &b) {
            if (a.range.start.line != b.range.start.line)
                return a.range.start.line > b.range.start.line;
            return a.range.start.character > b.range.start.character;
        });

        for (const auto &edit : sortedEdits) {
            QTextBlock startBlock = editor->document()->findBlockByNumber(edit.range.start.line);
            QTextBlock endBlock = editor->document()->findBlockByNumber(edit.range.end.line);
            if (!startBlock.isValid()) continue;
            if (!endBlock.isValid()) endBlock = startBlock;

            int startPos = startBlock.position() + qMin(edit.range.start.character, startBlock.length() - 1);
            int endPos = endBlock.position() + qMin(edit.range.end.character, endBlock.length() - 1);

            cursor.setPosition(startPos);
            cursor.setPosition(endPos, QTextCursor::KeepAnchor);
            cursor.insertText(edit.newText);
        }

        cursor.endEditBlock();
    });
}

void CodeEditorWidget::goToDefinition()
{
    if (!m_lspClient || !m_lspClient->isRunning())
        return;
    auto *tab = currentTab();
    if (!tab) return;

    auto cursor = tab->editor()->textCursor();
    QString uri = LSPClient::pathToUri(tab->filePath());
    m_lspClient->definition(uri, cursor.blockNumber(), cursor.columnNumber());
}

void CodeEditorWidget::findReferences()
{
    if (!m_lspClient || !m_lspClient->isRunning())
        return;
    auto *tab = currentTab();
    if (!tab) return;

    auto cursor = tab->editor()->textCursor();
    QString uri = LSPClient::pathToUri(tab->filePath());
    m_lspClient->references(uri, cursor.blockNumber(), cursor.columnNumber());
}

void CodeEditorWidget::renameSymbol()
{
    if (!m_lspClient || !m_lspClient->isRunning())
        return;
    auto *tab = currentTab();
    if (!tab) return;

    auto cursor = tab->editor()->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    QString oldName = cursor.selectedText();

    bool ok = false;
    QString newName = QInputDialog::getText(this, tr("Rename Symbol"),
        tr("New name:"), QLineEdit::Normal, oldName, &ok);

    if (!ok || newName.isEmpty() || newName == oldName)
        return;

    QString uri = LSPClient::pathToUri(tab->filePath());
    m_lspClient->rename(uri, cursor.blockNumber(),
                        tab->editor()->textCursor().columnNumber(), newName);
}

void CodeEditorWidget::formatDocument()
{
    if (!m_lspClient || !m_lspClient->isRunning())
        return;
    auto *tab = currentTab();
    if (!tab) return;

    QString uri = LSPClient::pathToUri(tab->filePath());
    m_lspClient->formatting(uri);
}

void CodeEditorWidget::onDiagnosticsReceived(const QString &uri,
                                              const QVector<LSPDiagnostic> &diagnostics)
{
    QString path = LSPClient::uriToPath(uri);
    auto *tab = findTabForFile(path);
    if (tab) {
        tab->setDiagnostics(diagnostics);
    }

    // Подсчитываем ошибки/предупреждения для ProjectTreeView
    int errors = 0, warnings = 0;
    for (const auto &d : diagnostics) {
        if (d.severity == DiagnosticSeverity::Error) ++errors;
        else if (d.severity == DiagnosticSeverity::Warning) ++warnings;
    }
    emit diagnosticsUpdated(path, errors, warnings);
}

void CodeEditorWidget::clearDebugLineInAllTabs()
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *tab = qobject_cast<CodeEditorTab *>(m_tabWidget->widget(i));
        if (tab)
            tab->clearDebugCurrentLine();
    }
}

void CodeEditorWidget::showFind()
{
    auto *tab = currentTab();
    if (tab) {
        m_findBar->setEditor(tab->editor());
        m_findBar->showFind();
    }
}

void CodeEditorWidget::showReplace()
{
    auto *tab = currentTab();
    if (tab) {
        m_findBar->setEditor(tab->editor());
        m_findBar->showReplace();
    }
}

void CodeEditorWidget::goToLine()
{
    auto *tab = currentTab();
    if (!tab) return;

    auto *editor = tab->editor();
    int maxLine = editor->blockCount();
    bool ok = false;
    int line = QInputDialog::getInt(this, tr("Go to Line"),
        tr("Line number (1–%1):").arg(maxLine),
        editor->textCursor().blockNumber() + 1,
        1, maxLine, 1, &ok);

    if (ok) {
        QTextCursor cursor(editor->document()->findBlockByNumber(line - 1));
        editor->setTextCursor(cursor);
        editor->centerCursor();
        editor->setFocus();
    }
}

CodeEditorTab *CodeEditorWidget::findTabForFile(const QString &path) const
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *tab = qobject_cast<CodeEditorTab *>(m_tabWidget->widget(i));
        if (tab && tab->filePath() == path)
            return tab;
    }
    return nullptr;
}

} // namespace DeltaQ
