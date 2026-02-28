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
struct LSPDiagnostic;
struct LSPTextEdit;
struct WorkspaceEdit;

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

    // Rename + Formatting
    void renameSymbol();
    void formatDocument();

    // Диагностика
    void onDiagnosticsReceived(const QString &uri, const QVector<LSPDiagnostic> &diagnostics);

    // Отладка — очистить маркер текущей строки во всех вкладках
    void clearDebugLineInAllTabs();

    // Доступ к вкладке по пути файла
    CodeEditorTab *findTabForFile(const QString &path) const;

signals:
    void fileSaved(const QString &path);
    void buildRequested(const QString &projectDir);
    void buildOutput(const QString &text);
    void currentTabChanged();
    void diagnosticsUpdated(const QString &path, int errors, int warnings);
    void breakpointToggleRequested(const QString &filePath, int line);

public slots:
    void closeTab(int index);
    void onTabChanged(int index);

private:
    void parseAndSaveModules(const QString &filePath);
    void connectTabSignals(CodeEditorTab *tab);

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
