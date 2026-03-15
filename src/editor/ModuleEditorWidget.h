#pragma once

#include <deltaq/Module.h>
#include "../lsp/LSPTypes.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QPlainTextEdit;
class QTableWidget;
class QTableWidgetItem;
class QTextEdit;

namespace DeltaQ {

class ModuleRegistry;
class ModuleTestRunner;
class SyntaxHighlighter;
class CompletionPopup;
struct TestResult;

class ModuleEditorWidget : public QWidget {
    Q_OBJECT

public:
    // Создаёт отдельную authoring-поверхность для редактирования `.dqmod`.
    explicit ModuleEditorWidget(ModuleRegistry *registry, QWidget *parent = nullptr);

    // Загружает модуль из `.dqmod` и разворачивает его в форму редактора.
    bool loadFromFile(const QString &path);
    // Запускает transient fragment scratchpad без немедленного создания `.dqmod` на диске.
    void startScratchpad(const Module &module, const QString &targetPath);
    // Сводит правки формы обратно в `.dqmod` и сохраняет файл на диск.
    bool saveModule();
    // Возвращает признак несохранённых изменений во вкладке редактора модуля.
    bool hasUnsavedChanges() const { return m_modified; }
    // Определяет, можно ли редактировать текущий модуль или он только для чтения.
    bool isEditable() const;
    // Показывает, работает ли surface в transient scratchpad-режиме.
    bool isScratchpadMode() const { return m_scratchpadMode; }
    // Возвращает путь к открытому `.dqmod`.
    QString filePath() const { return m_filePath; }
    // Даёт доступ к основному текстовому редактору исходника модуля.
    QPlainTextEdit *editor() const { return m_codeEdit; }
    // Формирует заголовок вкладки с учётом dirty-state.
    QString tabTitle() const;
    // Возвращает путь документа, который должен использоваться для LSP/diagnostics поверх embedded source.
    QString lspDocumentPath() const;
    // Возвращает language id для embedded source модуля.
    QString lspLanguageId() const;
    // Возвращает текущий текст исходника из code editor панели.
    QString sourceText() const;
    // Сохраняет LSP-диагностику и обновляет подсветку исходника.
    void setDiagnostics(const QVector<LSPDiagnostic> &diagnostics);
    // Возвращает последнее полученное состояние LSP-диагностики.
    const QVector<LSPDiagnostic> &diagnostics() const { return m_diagnostics; }
    // Показывает hover-tooltip поверх embedded source.
    void showHoverTooltip(const QString &text, const QPoint &globalPos);
    // Показывает popup автодополнения поверх embedded source.
    void showCompletion(const QVector<CompletionItem> &items);

signals:
    void modificationChanged(bool modified);
    void moduleSaved(const QString &path, const QString &moduleId);
    // Проксирует hover-запрос исходника модуля наружу в LSP bridge.
    void hoverRequested(const QString &documentPath, int line, int character);
    // Проксирует completion-запрос исходника модуля наружу в LSP bridge.
    void completionRequested(const QString &documentPath, int line, int character);

private slots:
    // Реагирует на любую правку формы и переводит модуль в modified-state.
    void onFieldEdited();
    // Перезагружает `.dqmod` с диска с защитой от потери несохранённых правок.
    void reloadFromDisk();
    // Добавляет новый входной порт в editable contract table.
    void onAddInputPort();
    // Добавляет выходной порт в editable contract table.
    void onAddOutputPort();
    // Удаляет выбранный порт из editable contract table.
    void onRemoveSelectedPort();
    // Синхронизирует contract table из текущей сигнатуры исходника.
    void onSyncContractFromSource();
    // Проталкивает contract table обратно в сигнатуру исходника.
    void onApplyContractToSource();
    // Реагирует на правки таблицы контракта как на обычное редактирование модуля.
    void onContractTableItemChanged(QTableWidgetItem *item);
    // Переключает active verification scenario и загружает его значения в workspace.
    void onVerificationScenarioChanged(int index);
    // Сохраняет новое имя текущего verification scenario.
    void onVerificationScenarioNameEdited(const QString &text);
    // Добавляет новый verification scenario для текущего модуля.
    void onAddVerificationScenario();
    // Удаляет выбранный verification scenario.
    void onRemoveVerificationScenario();
    // Реагирует на правки input/expected таблиц verification workspace.
    void onVerificationTableItemChanged(QTableWidgetItem *item);
    // Выполняет compile-only check для текущего состояния модуля.
    void onCompileVerification();
    // Запускает compile+run verify path для текущего сохранённого сценария.
    void onRunVerificationScenario();
    // Обновляет детали trace viewer после выбора другого шага в trace events table.
    void onTraceEventSelectionChanged();
    // Переводит trace viewer на первый доступный шаг текущего trace artifact-а.
    void onTraceRestart();
    // Переходит к следующему шагу в linear trace sequence.
    void onTraceStep();
    // Выполняет `step over` в current v1 trace sequence.
    void onTraceStepOver();
    // Переводит trace viewer сразу на последний доступный шаг.
    void onTraceRunToEnd();

private:
    // Строит все визуальные панели редактора модуля и соединяет сигналы.
    void setupUi();
    // Заполняет форму значениями из уже загруженного модуля.
    void populateForm();
    // Считывает значения формы в `m_module` и синхронизирует контракт с source code.
    bool applyFormToModule(QString *errorMessage = nullptr);
    // Обновляет summary-блок жизненного цикла и верхний статус.
    void refreshSummary();
    // Показывает, совпадает ли editable contract table с текущей сигнатурой source code.
    void refreshContractPreview();
    // Загружает входы и выходы модуля в editable contract table.
    void loadContractTableFromModule(const Module &module);
    // Считывает contract table обратно в модель модуля с базовой валидацией.
    bool readContractTableIntoModule(Module *module, QString *errorMessage = nullptr) const;
    // Собирает временную версию модуля из формы без немедленного сохранения на диск.
    Module stagedModuleFromForm(bool *contractValid = nullptr,
                                QString *errorMessage = nullptr) const;
    // Меняет dirty-state редактора и уведомляет вкладку о смене состояния.
    void setModified(bool modified);
    // Помечает результаты compile/test как устаревшие после ручной правки модуля.
    void invalidateVerificationState();
    // Пересобирает визуальные выделения исходника: текущая строка и LSP-диагностика.
    void refreshCodeSelections();
    // Загружает verification workspace из current module state.
    void loadVerificationWorkspaceFromModule(const Module &module);
    // Перестраивает selector сохранённых verification scenarios.
    void populateVerificationScenarioSelector();
    // Загружает значения текущего verification scenario в таблицы inputs/expected outputs.
    void populateVerificationTablesFromCurrentScenario(const Module &module);
    // Считывает правки verification workspace обратно в текущий scenario.
    void syncVerificationScenarioFromUi();
    // Нормализует verification scenarios под текущий набор input/output ports.
    void normalizeVerificationScenarios(Module *module) const;
    // Нормализует trace artifacts и удерживает их привязанными к существующим сценариям проверки.
    void normalizeVerificationTraceArtifacts(Module *module) const;
    // Создаёт стартовый verification scenario по текущему контракту модуля.
    ModuleVerificationScenario makeDefaultVerificationScenario(const Module &module, int index) const;
    // Строит минимальный trace artifact для текущего verification-run или explicit unavailable-status.
    ModuleTraceArtifact buildVerificationTraceArtifact(const Module &module,
                                                      const ModuleVerificationScenario &scenario,
                                                      const TestResult &result) const;
    // Сохраняет или заменяет trace artifact для текущего verification scenario.
    void storeVerificationTraceArtifact(Module *module, const ModuleTraceArtifact &artifact) const;
    // Считывает значения из verification table в key-value map.
    QMap<QString, QString> readVerificationTableValues(const QTableWidget *table) const;
    // Заполняет verification table значениями портов и сценария.
    void populatePortValueTable(QTableWidget *table,
                                const QVector<Port> &ports,
                                const QMap<QString, QString> &values) const;
    // Показывает expected-vs-actual результат последнего verification run.
    void updateVerificationResultViews(const Module &module,
                                       const ModuleVerificationScenario *scenario,
                                       const TestResult *result);
    // Добавляет строку в verification log внутри editor workspace.
    void appendVerificationLogLine(const QString &text);
    // Обновляет trust_state после успешной или неуспешной проверки сценария.
    void refreshTrustStateAfterVerification(bool verificationPassed);
    // Синхронизирует verification tables после изменения контракта модуля.
    void refreshVerificationWorkspaceFromCurrentContract();
    // Возвращает trace artifact, привязанный к текущему verification scenario.
    const ModuleTraceArtifact *currentTraceArtifact(const Module &module) const;
    // Загружает trace viewer из текущего набора сохранённых verification artifacts.
    void loadTraceViewerFromModule(const Module &module);
    // Перестраивает таблицу событий текущего trace artifact-а.
    void populateTraceEventTable(const ModuleTraceArtifact *artifact);
    // Возвращает индекс выбранного шага в trace viewer.
    int currentTraceEventIndex() const;
    // Переводит selection trace viewer на указанный row и обновляет детали.
    void selectTraceEventRow(int row);
    // Обновляет правую часть trace viewer по выбранному step event.
    void updateTraceViewerDetails(const Module &module);
    // Освежает enabled-state stepping controls под текущий trace artifact и selection.
    void refreshTraceControls(const Module &module);
    // Накопительно собирает видимое состояние локальных переменных до выбранного шага.
    QMap<QString, QString> accumulatedTraceVariableState(const ModuleTraceArtifact &artifact,
                                                        int eventIndex) const;
    // Накопительно собирает видимое состояние outputs/return до выбранного шага.
    QMap<QString, QString> accumulatedTraceOutputState(const ModuleTraceArtifact &artifact,
                                                      int eventIndex) const;
    // Заполняет компактную key-value таблицу для trace inputs/locals/outputs.
    void populateTraceValueTable(QTableWidget *table,
                                 const QMap<QString, QString> &values) const;
    // Преобразует локальный hover от code editor в сигнал для внешнего LSP bridge.
    void onCodeHoverRequested(int line, int character);
    // Преобразует локальный completion trigger в сигнал для внешнего LSP bridge.
    void onCodeCompletionRequested(int line, int character);

    ModuleRegistry *m_registry = nullptr;
    Module m_module;
    QString m_filePath;
    bool m_loading = false;
    bool m_modified = false;
    bool m_scratchpadMode = false;

    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_descriptionEdit = nullptr;
    QComboBox *m_categoryCombo = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QComboBox *m_trustStateCombo = nullptr;
    QLabel *m_originValueLabel = nullptr;
    QLabel *m_provenanceValueLabel = nullptr;
    QLabel *m_verificationValueLabel = nullptr;
    QLabel *m_contractStatusLabel = nullptr;
    QTextEdit *m_includesEdit = nullptr;
    QPlainTextEdit *m_codeEdit = nullptr;
    QTableWidget *m_portTable = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_reloadButton = nullptr;
    QPushButton *m_addInputButton = nullptr;
    QPushButton *m_addOutputButton = nullptr;
    QPushButton *m_removePortButton = nullptr;
    QPushButton *m_syncContractButton = nullptr;
    QPushButton *m_applyContractButton = nullptr;
    QComboBox *m_verificationScenarioCombo = nullptr;
    QLineEdit *m_verificationScenarioNameEdit = nullptr;
    QTableWidget *m_verificationInputsTable = nullptr;
    QTableWidget *m_verificationExpectedTable = nullptr;
    QTableWidget *m_verificationResultsTable = nullptr;
    QLabel *m_verificationRunStatusLabel = nullptr;
    QTextEdit *m_verificationLogEdit = nullptr;
    QLabel *m_traceStatusLabel = nullptr;
    QLabel *m_traceCurrentStepLabel = nullptr;
    QLabel *m_traceCurrentFlowLabel = nullptr;
    QLabel *m_traceSourceLabel = nullptr;
    QTableWidget *m_traceEventsTable = nullptr;
    QTableWidget *m_traceInputsTable = nullptr;
    QTableWidget *m_traceLocalsTable = nullptr;
    QTableWidget *m_traceOutputsTable = nullptr;
    QPushButton *m_addVerificationScenarioButton = nullptr;
    QPushButton *m_removeVerificationScenarioButton = nullptr;
    QPushButton *m_compileVerificationButton = nullptr;
    QPushButton *m_runVerificationButton = nullptr;
    QPushButton *m_traceRestartButton = nullptr;
    QPushButton *m_traceStepButton = nullptr;
    QPushButton *m_traceStepOverButton = nullptr;
    QPushButton *m_traceRunToEndButton = nullptr;
    SyntaxHighlighter *m_highlighter = nullptr;
    CompletionPopup *m_completionPopup = nullptr;
    ModuleTestRunner *m_testRunner = nullptr;
    QVector<LSPDiagnostic> m_diagnostics;
    int m_currentVerificationScenarioIndex = -1;
    bool m_loadingVerification = false;
};

} // namespace DeltaQ
