#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QMap>

namespace DeltaQ {

class CommandBus;
class ModuleRegistry;
class CodeEditorTab;
class FindReplaceBar;
class LSPClient;
class AnnotationParser;

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

    // LSP
    void setLSPClient(LSPClient *client);

    // Поиск и замена
    void showFind();
    void showReplace();
    void goToLine();

    // Навигация LSP
    void goToDefinition();
    void findReferences();

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
    void parseAndSaveModules(const QString &filePath);

    QTabWidget *m_tabWidget;
    FindReplaceBar *m_findBar;
    CommandBus *m_commandBus;
    ModuleRegistry *m_moduleRegistry;
    AnnotationParser *m_annotationParser;
    LSPClient *m_lspClient = nullptr;
    QMap<QString, int> m_openFiles; // path -> tab index
    QMap<QString, int> m_documentVersions; // uri -> version (для LSP)
};

} // namespace DeltaQ
