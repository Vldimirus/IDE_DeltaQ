#include "CodeEditorWidget.h"
#include "CodeEditorTab.h"
#include "BuildManager.h"

#include <QFileInfo>
#include <QMessageBox>

namespace DeltaQ {

CodeEditorWidget::CodeEditorWidget(CommandBus *bus, ModuleRegistry *registry,
                                     QWidget *parent)
    : QWidget(parent)
    , m_commandBus(bus)
    , m_moduleRegistry(registry)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    layout->addWidget(m_tabWidget);

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
        emit fileSaved(tab->filePath());
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
    // Can be used for status bar updates
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
