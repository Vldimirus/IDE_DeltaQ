#include "CodeEditorWidget.h"
#include "CodeEditorTab.h"
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
