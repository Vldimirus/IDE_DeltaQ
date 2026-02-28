#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QMap>

namespace DeltaQ {

class CommandBus;
class ModuleRegistry;
class CodeEditorTab;

class CodeEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit CodeEditorWidget(CommandBus *bus, ModuleRegistry *registry,
                               QWidget *parent = nullptr);

    void openFile(const QString &path);
    void saveCurrentFile();
    bool hasUnsavedChanges() const;
    void buildProject(const QString &projectDir);

    CodeEditorTab *currentTab() const;
    QString currentFilePath() const;
    QStringList openFilePaths() const;

signals:
    void fileSaved(const QString &path);
    void buildRequested(const QString &projectDir);
    void buildOutput(const QString &text);
    void currentTabChanged();

public slots:
    void closeTab(int index);
    void onTabChanged(int index);

private:
    CodeEditorTab *findTabForFile(const QString &path) const;

    QTabWidget *m_tabWidget;
    CommandBus *m_commandBus;
    ModuleRegistry *m_moduleRegistry;
    QMap<QString, int> m_openFiles; // path -> tab index
};

} // namespace DeltaQ
