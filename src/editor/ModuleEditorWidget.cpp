#include "ModuleEditorWidget.h"

#include "CodeEditorTab.h"
#include "CompletionPopup.h"
#include "ModuleTestRunner.h"
#include "ModuleSignatureUtils.h"
#include "SyntaxHighlighter.h"
#include "../core/ModuleRegistry.h"

#include <deltaq/Module.h>

#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QHeaderView>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextEdit>
#include <QToolTip>
#include <QVBoxLayout>

#include <algorithm>

namespace DeltaQ {

namespace {

// Возвращает сохранённые или вычисленные scenario refs для summary и save-preview.
QStringList resolvedScenarioRefs(const Module &module)
{
    QStringList refs = module.verificationScenarioRefs;
    if (!refs.isEmpty())
        return refs;

    for (const auto &scenario : module.verificationScenarios) {
        const QString displayName = scenario.displayName();
        if (!displayName.isEmpty())
            refs.append(displayName);
    }
    return refs;
}

// Возвращает сохранённые или вычисленные trace refs без зависимости от ad-hoc UI-state.
QStringList resolvedTraceArtifactRefs(const Module &module)
{
    QStringList refs = module.verificationTraceArtifactRefs;
    if (!refs.isEmpty())
        return refs;

    for (const auto &artifact : module.verificationTraceArtifacts) {
        const QString ref = artifact.summaryRef();
        if (!ref.isEmpty())
            refs.append(ref);
    }
    return refs;
}

// Собирает краткое summary текущего verification-state для правой панели.
QString verificationSummaryText(const Module &module)
{
    const QStringList scenarioRefs = resolvedScenarioRefs(module);
    const QStringList traceRefs = resolvedTraceArtifactRefs(module);
    QString summary = QObject::tr("Compile: %1 | Test: %2")
        .arg(module.compileStatus)
        .arg(module.testStatus);
    if (!module.lastVerifiedAt.isEmpty())
        summary += QObject::tr(" | Last verified: %1").arg(module.lastVerifiedAt);
    if (!scenarioRefs.isEmpty()) {
        summary += QObject::tr(" | Scenarios: %1")
            .arg(scenarioRefs.join(", "));
    }
    if (!traceRefs.isEmpty()) {
        summary += QObject::tr(" | Traces: %1").arg(traceRefs.size());
        if (!module.verificationTraceArtifacts.isEmpty()) {
            const ModuleTraceArtifact &latestArtifact = module.verificationTraceArtifacts.last();
            if (!latestArtifact.status.trimmed().isEmpty())
                summary += QObject::tr(" | Trace status: %1").arg(latestArtifact.status);
        }
    }
    return summary;
}

// Формирует компактное текстовое представление provenance модуля.
QString provenanceSummaryText(const Module &module)
{
    const ModuleProvenance provenance = module.effectiveProvenance();
    QStringList parts;
    parts.append(QObject::tr("kind=%1").arg(provenance.sourceKind));
    if (!provenance.sourceRef.isEmpty())
        parts.append(QObject::tr("ref=%1").arg(provenance.sourceRef));
    if (!provenance.symbol.isEmpty())
        parts.append(QObject::tr("symbol=%1").arg(provenance.symbol));
    if (!provenance.manifestId.isEmpty())
        parts.append(QObject::tr("manifest=%1").arg(provenance.manifestId));
    return parts.join(" | ");
}

// Возвращает стартовый набор категорий для authoring surface.
QStringList defaultCategories()
{
    return {
        "custom",
        "math",
        "logic",
        "io",
        "string",
        "filesystem",
        "process",
        "timers",
        "config_json",
        "tcp_udp",
        "serial"
    };
}

// Возвращает цвет подчёркивания для конкретной LSP-диагностики.
QColor diagnosticUnderlineColor(DiagnosticSeverity severity)
{
    switch (severity) {
    case DiagnosticSeverity::Error:
        return QColor(255, 0, 0);
    case DiagnosticSeverity::Warning:
        return QColor(255, 165, 0);
    case DiagnosticSeverity::Information:
        return QColor(0, 120, 255);
    case DiagnosticSeverity::Hint:
        return QColor(150, 150, 150);
    }
    return QColor(255, 0, 0);
}

// Создаёт не редактируемую ячейку для колонок с именами портов и статусов.
QTableWidgetItem *lockedTableItem(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

// Стабилизирует токен для trace-id, чтобы его можно было сохранять в schema без пробелов и мусора.
QString stableTraceToken(const QString &value)
{
    QString token = value.trimmed();
    token.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]+")), QStringLiteral("_"));
    token = token.trimmed();
    if (token.isEmpty())
        token = QStringLiteral("trace");
    return token;
}

// Строит стабильный trace-id на основе модуля и verification scenario.
QString makeTraceArtifactId(const Module &module, const QString &scenarioId)
{
    const QString moduleToken = stableTraceToken(
        module.id.trimmed().isEmpty() ? module.name : module.id);
    const QString scenarioToken = stableTraceToken(scenarioId);
    return QStringLiteral("trace://%1/%2").arg(moduleToken, scenarioToken);
}

// Ищет первую осмысленную строку исходника для входного trace event.
int firstMeaningfulSourceLineNumber(const QStringList &sourceLines)
{
    for (int i = 0; i < sourceLines.size(); ++i) {
        if (!sourceLines.at(i).trimmed().isEmpty())
            return i + 1;
    }
    return -1;
}

// Ищет первую строку, содержащую указанное слово как отдельный токен.
int lineNumberContainingWord(const QStringList &sourceLines, const QString &word)
{
    const QRegularExpression pattern(QStringLiteral("\\b%1\\b")
                                         .arg(QRegularExpression::escape(word)));
    for (int i = 0; i < sourceLines.size(); ++i) {
        if (pattern.match(sourceLines.at(i)).hasMatch())
            return i + 1;
    }
    return -1;
}

// Возвращает строку исходника для конкретного one-based номера строки trace event-а.
QString sourceSnippetAtLine(const QStringList &sourceLines, int oneBasedLine)
{
    if (oneBasedLine < 1 || oneBasedLine > sourceLines.size())
        return {};
    return sourceLines.at(oneBasedLine - 1).trimmed();
}

// Определяет, можно ли для текущего модуля честно снять упрощённый A4.0 boundary-trace.
QString simplifiedTraceUnavailableReason(const Module &module)
{
    const QString language = module.language.trimmed().toLower();
    if (language != QStringLiteral("c") && language != QStringLiteral("cpp")) {
        return QObject::tr("Trace unavailable because simplified A4.0 capture currently supports only C/C++ modules.");
    }

    const QStringList sourceLines = module.sourceCode.split('\n');
    const QRegularExpression controlFlowPattern(
        QStringLiteral("\\b(if|else|for|while|switch|goto|break|continue|do)\\b"));
    const int signatureLineIndex = firstMeaningfulSourceLineNumber(sourceLines) - 1;

    int returnCount = 0;
    for (int i = 0; i < sourceLines.size(); ++i) {
        const QString trimmedLine = sourceLines.at(i).trimmed();
        if (trimmedLine.isEmpty() || trimmedLine == QStringLiteral("{") || trimmedLine == QStringLiteral("}"))
            continue;
        if (i == signatureLineIndex)
            continue;

        const QRegularExpressionMatch controlFlowMatch = controlFlowPattern.match(trimmedLine);
        if (controlFlowMatch.hasMatch()) {
            return QObject::tr("Trace unavailable for control-flow construct '%1' in simplified A4.0 capture.")
                .arg(controlFlowMatch.captured(1));
        }

        if (trimmedLine.startsWith(QStringLiteral("return "))) {
            ++returnCount;
            continue;
        }

        if (trimmedLine == QStringLiteral("return;")) {
            ++returnCount;
            continue;
        }

        return QObject::tr("Trace unavailable because simplified A4.0 capture currently supports only single-return module bodies.");
    }

    if (returnCount != 1) {
        return QObject::tr("Trace unavailable because simplified A4.0 capture requires exactly one return statement.");
    }

    return {};
}

} // namespace

// Инициализирует widget и строит весь UI редактора модуля.
ModuleEditorWidget::ModuleEditorWidget(ModuleRegistry *registry, QWidget *parent)
    : QWidget(parent)
    , m_registry(registry)
    , m_testRunner(new ModuleTestRunner(this))
{
    setObjectName("moduleEditorWidget");
    setupUi();
}

// Загружает `.dqmod` с диска и разворачивает его в поля редактора.
bool ModuleEditorWidget::loadFromFile(const QString &path)
{
    if (path.isEmpty())
        return false;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    // Editor работает поверх реального `.dqmod`, а не над временной копией в памяти.
    m_module = Module::fromJson(doc.object());
    m_module.storagePath = path;
    m_filePath = path;
    m_scratchpadMode = false;
    populateForm();
    setModified(false);
    return true;
}

// Запускает transient fragment scratchpad с будущим target-path, но без немедленной записи `.dqmod`.
void ModuleEditorWidget::startScratchpad(const Module &module, const QString &targetPath)
{
    m_module = module;
    m_module.storagePath.clear();
    m_filePath = QDir::cleanPath(targetPath);
    m_scratchpadMode = true;
    populateForm();
    setModified(true);
    appendVerificationLogLine(
        tr("Fragment scratchpad ready. Verify and trace first, then promote with Save."));
}

// Сохраняет текущий модуль, предварительно сводя форму и исходник в одну консистентную модель.
bool ModuleEditorWidget::saveModule()
{
    if (m_filePath.isEmpty())
        return false;

    if (!isEditable()) {
        QMessageBox::information(this,
                                 tr("Read-only module"),
                                 tr("Core and UI contract modules are read-only."));
        return false;
    }

    // Перед сохранением сводим UI-правки обратно в единый согласованный модуль и source.
    QString errorMessage;
    if (!applyFormToModule(&errorMessage)) {
        QMessageBox::warning(this,
                             tr("Cannot save module"),
                             errorMessage.isEmpty()
                                 ? tr("Module contract could not be applied to source.")
                                 : errorMessage);
        return false;
    }
    m_module.storagePath = m_filePath;

    if (m_registry) {
        QDir().mkpath(QFileInfo(m_filePath).absolutePath());
        if (!m_registry->saveModuleFile(m_module, m_filePath))
            return false;
        m_registry->registerModule(m_module);
    } else {
        return false;
    }

    // После первой успешной записи fragment перестаёт быть scratchpad и становится обычным `.dqmod`.
    m_scratchpadMode = false;
    setModified(false);
    emit moduleSaved(m_filePath, m_module.id);
    return true;
}

// Определяет, можно ли редактировать модуль в текущем authoring surface.
bool ModuleEditorWidget::isEditable() const
{
    return m_module.origin != "core" && m_module.origin != "ui";
}

// Формирует заголовок вкладки редактора с индикатором несохранённых правок.
QString ModuleEditorWidget::tabTitle() const
{
    const QString baseName = m_filePath.isEmpty()
        ? tr("module.dqmod")
        : QFileInfo(m_filePath).fileName();
    QString title = baseName;
    if (m_scratchpadMode)
        title += tr(" [scratch]");
    if (m_modified)
        title += " *";
    return title;
}

// Обрабатывает любые пользовательские правки и переводит verification-state в устаревший.
void ModuleEditorWidget::onFieldEdited()
{
    if (m_loading || !isEditable())
        return;

    // Любая правка authoring surface делает прежний verification-результат устаревшим.
    if (!m_modified)
        invalidateVerificationState();
    if (!m_modified)
        setModified(true);
    else {
        refreshSummary();
        refreshContractPreview();
    }
}

// Перечитывает модуль с диска, если пользователь согласен отбросить локальные правки.
void ModuleEditorWidget::reloadFromDisk()
{
    if (m_filePath.isEmpty())
        return;

    if (m_modified) {
        const auto result = QMessageBox::question(
            this,
            tr("Discard changes"),
            tr("Reload module from disk and discard unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (result != QMessageBox::Yes)
            return;
    }

    loadFromFile(m_filePath);
}

// Создаёт визуальную структуру Module Studio и соединяет все действия редактора.
void ModuleEditorWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    auto *headerLayout = new QHBoxLayout;
    auto *title = new QLabel(tr("Module Editor"), this);
    title->setStyleSheet("font-size: 16px; font-weight: 600;");
    m_statusLabel = new QLabel(tr("No module loaded"), this);
    m_statusLabel->setObjectName("moduleStudioStatusLabel");
    m_statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(m_statusLabel, 1);
    layout->addLayout(headerLayout);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    layout->addWidget(splitter, 1);

    auto *topPanel = new QWidget(this);
    auto *topLayout = new QHBoxLayout(topPanel);

    auto *identityBox = new QGroupBox(tr("Identity"), topPanel);
    auto *identityForm = new QFormLayout(identityBox);
    m_nameEdit = new QLineEdit(identityBox);
    m_nameEdit->setObjectName("moduleStudioNameEdit");
    m_descriptionEdit = new QLineEdit(identityBox);
    m_descriptionEdit->setObjectName("moduleStudioDescriptionEdit");
    m_categoryCombo = new QComboBox(identityBox);
    m_categoryCombo->setObjectName("moduleStudioCategoryCombo");
    m_categoryCombo->setEditable(true);
    m_categoryCombo->addItems(defaultCategories());
    m_languageCombo = new QComboBox(identityBox);
    m_languageCombo->setObjectName("moduleStudioLanguageCombo");
    m_languageCombo->addItems(QStringList{"c", "cpp"});
    m_trustStateCombo = new QComboBox(identityBox);
    m_trustStateCombo->setObjectName("moduleStudioTrustStateCombo");
    m_trustStateCombo->addItems(QStringList{
        "draft",
        "generated_raw",
        "generated_unverified",
        "smoke_passed",
        "curated",
        "verified"
    });
    identityForm->addRow(tr("Name"), m_nameEdit);
    identityForm->addRow(tr("Description"), m_descriptionEdit);
    identityForm->addRow(tr("Category"), m_categoryCombo);
    identityForm->addRow(tr("Language"), m_languageCombo);
    identityForm->addRow(tr("Trust state"), m_trustStateCombo);

    auto *summaryBox = new QGroupBox(tr("Lifecycle"), topPanel);
    auto *summaryForm = new QFormLayout(summaryBox);
    m_originValueLabel = new QLabel(summaryBox);
    m_originValueLabel->setObjectName("moduleStudioOriginLabel");
    m_originValueLabel->setWordWrap(true);
    m_provenanceValueLabel = new QLabel(summaryBox);
    m_provenanceValueLabel->setObjectName("moduleStudioProvenanceLabel");
    m_provenanceValueLabel->setWordWrap(true);
    m_verificationValueLabel = new QLabel(summaryBox);
    m_verificationValueLabel->setObjectName("moduleStudioVerificationLabel");
    m_verificationValueLabel->setWordWrap(true);
    summaryForm->addRow(tr("Origin"), m_originValueLabel);
    summaryForm->addRow(tr("Provenance"), m_provenanceValueLabel);
    summaryForm->addRow(tr("Verification"), m_verificationValueLabel);

    auto *contractBox = new QGroupBox(tr("Contract"), topPanel);
    auto *contractLayout = new QVBoxLayout(contractBox);
    m_contractStatusLabel = new QLabel(contractBox);
    m_contractStatusLabel->setObjectName("moduleStudioContractStatusLabel");
    m_contractStatusLabel->setWordWrap(true);
    m_portTable = new QTableWidget(contractBox);
    m_portTable->setObjectName("moduleStudioPortTable");
    m_portTable->setColumnCount(4);
    m_portTable->setHorizontalHeaderLabels({tr("Port"), tr("Type"), tr("Direction"), tr("Default")});
    m_portTable->verticalHeader()->setVisible(false);
    // Таблица контракта теперь не только preview, а и главный editable surface для ports.
    m_portTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_portTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_portTable->setAlternatingRowColors(true);
    m_portTable->horizontalHeader()->setStretchLastSection(true);
    auto *contractButtons = new QHBoxLayout;
    m_addInputButton = new QPushButton(tr("Add Input"), contractBox);
    m_addInputButton->setObjectName("moduleStudioAddInputButton");
    m_addOutputButton = new QPushButton(tr("Add Output"), contractBox);
    m_addOutputButton->setObjectName("moduleStudioAddOutputButton");
    m_removePortButton = new QPushButton(tr("Remove Port"), contractBox);
    m_removePortButton->setObjectName("moduleStudioRemovePortButton");
    m_syncContractButton = new QPushButton(tr("Sync From Source"), contractBox);
    m_syncContractButton->setObjectName("moduleStudioSyncContractButton");
    m_applyContractButton = new QPushButton(tr("Apply To Source"), contractBox);
    m_applyContractButton->setObjectName("moduleStudioApplyContractButton");
    contractButtons->addWidget(m_addInputButton);
    contractButtons->addWidget(m_addOutputButton);
    contractButtons->addWidget(m_removePortButton);
    contractButtons->addStretch();
    contractButtons->addWidget(m_syncContractButton);
    contractButtons->addWidget(m_applyContractButton);
    contractLayout->addWidget(m_contractStatusLabel);
    contractLayout->addLayout(contractButtons);
    contractLayout->addWidget(m_portTable, 1);

    topLayout->addWidget(identityBox, 3);
    topLayout->addWidget(summaryBox, 2);
    topLayout->addWidget(contractBox, 3);
    splitter->addWidget(topPanel);

    auto *bottomPanel = new QWidget(this);
    auto *bottomLayout = new QVBoxLayout(bottomPanel);
    auto *codeHeader = new QHBoxLayout;
    auto *codeTitle = new QLabel(tr("Implementation"), bottomPanel);
    codeTitle->setStyleSheet("font-weight: 600;");
    m_saveButton = new QPushButton(tr("Save Module"), bottomPanel);
    m_saveButton->setObjectName("moduleStudioSaveButton");
    m_reloadButton = new QPushButton(tr("Reload"), bottomPanel);
    m_reloadButton->setObjectName("moduleStudioReloadButton");
    codeHeader->addWidget(codeTitle);
    codeHeader->addStretch();
    codeHeader->addWidget(m_reloadButton);
    codeHeader->addWidget(m_saveButton);
    bottomLayout->addLayout(codeHeader);

    auto *includesLabel = new QLabel(tr("Includes"), bottomPanel);
    m_includesEdit = new QTextEdit(bottomPanel);
    m_includesEdit->setObjectName("moduleStudioIncludesEdit");
    m_includesEdit->setMaximumHeight(96);
    auto *codeLabel = new QLabel(tr("Source code"), bottomPanel);
    auto *codeEdit = new CodePlainTextEdit(bottomPanel);
    m_codeEdit = codeEdit;
    m_codeEdit->setObjectName("moduleStudioCodeEdit");
    m_codeEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_codeEdit->setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    m_highlighter = new SyntaxHighlighter(m_codeEdit->document());
    m_completionPopup = new CompletionPopup(codeEdit);

    bottomLayout->addWidget(includesLabel);
    bottomLayout->addWidget(m_includesEdit);
    bottomLayout->addWidget(codeLabel);
    bottomLayout->addWidget(m_codeEdit, 1);

    auto *verificationBox = new QGroupBox(tr("Verification Workspace"), bottomPanel);
    auto *verificationLayout = new QVBoxLayout(verificationBox);
    auto *scenarioRow = new QHBoxLayout;
    auto *scenarioLabel = new QLabel(tr("Scenario"), verificationBox);
    m_verificationScenarioCombo = new QComboBox(verificationBox);
    m_verificationScenarioCombo->setObjectName("moduleStudioVerificationScenarioCombo");
    m_addVerificationScenarioButton = new QPushButton(tr("Add Scenario"), verificationBox);
    m_addVerificationScenarioButton->setObjectName("moduleStudioAddScenarioButton");
    m_removeVerificationScenarioButton = new QPushButton(tr("Remove Scenario"), verificationBox);
    m_removeVerificationScenarioButton->setObjectName("moduleStudioRemoveScenarioButton");
    m_compileVerificationButton = new QPushButton(tr("Compile"), verificationBox);
    m_compileVerificationButton->setObjectName("moduleStudioCompileVerificationButton");
    m_runVerificationButton = new QPushButton(tr("Verify Scenario"), verificationBox);
    m_runVerificationButton->setObjectName("moduleStudioRunVerificationButton");
    scenarioRow->addWidget(scenarioLabel);
    scenarioRow->addWidget(m_verificationScenarioCombo, 1);
    scenarioRow->addWidget(m_addVerificationScenarioButton);
    scenarioRow->addWidget(m_removeVerificationScenarioButton);
    scenarioRow->addWidget(m_compileVerificationButton);
    scenarioRow->addWidget(m_runVerificationButton);

    auto *scenarioNameRow = new QHBoxLayout;
    auto *scenarioNameLabel = new QLabel(tr("Scenario name"), verificationBox);
    m_verificationScenarioNameEdit = new QLineEdit(verificationBox);
    m_verificationScenarioNameEdit->setObjectName("moduleStudioScenarioNameEdit");
    scenarioNameRow->addWidget(scenarioNameLabel);
    scenarioNameRow->addWidget(m_verificationScenarioNameEdit, 1);

    auto *tablesRow = new QHBoxLayout;
    auto *inputBox = new QGroupBox(tr("Inputs"), verificationBox);
    auto *inputLayout = new QVBoxLayout(inputBox);
    m_verificationInputsTable = new QTableWidget(inputBox);
    m_verificationInputsTable->setObjectName("moduleStudioVerificationInputsTable");
    m_verificationInputsTable->setColumnCount(2);
    m_verificationInputsTable->setHorizontalHeaderLabels({tr("Input"), tr("Value")});
    m_verificationInputsTable->verticalHeader()->setVisible(false);
    m_verificationInputsTable->horizontalHeader()->setStretchLastSection(true);
    inputLayout->addWidget(m_verificationInputsTable);

    auto *expectedBox = new QGroupBox(tr("Expected Outputs"), verificationBox);
    auto *expectedLayout = new QVBoxLayout(expectedBox);
    m_verificationExpectedTable = new QTableWidget(expectedBox);
    m_verificationExpectedTable->setObjectName("moduleStudioVerificationExpectedTable");
    m_verificationExpectedTable->setColumnCount(2);
    m_verificationExpectedTable->setHorizontalHeaderLabels({tr("Output"), tr("Expected")});
    m_verificationExpectedTable->verticalHeader()->setVisible(false);
    m_verificationExpectedTable->horizontalHeader()->setStretchLastSection(true);
    expectedLayout->addWidget(m_verificationExpectedTable);

    tablesRow->addWidget(inputBox, 1);
    tablesRow->addWidget(expectedBox, 1);

    m_verificationRunStatusLabel = new QLabel(tr("Verification workspace ready"), verificationBox);
    m_verificationRunStatusLabel->setObjectName("moduleStudioVerificationStatusLabel");
    m_verificationRunStatusLabel->setWordWrap(true);

    m_verificationResultsTable = new QTableWidget(verificationBox);
    m_verificationResultsTable->setObjectName("moduleStudioVerificationResultsTable");
    m_verificationResultsTable->setColumnCount(4);
    m_verificationResultsTable->setHorizontalHeaderLabels(
        {tr("Output"), tr("Expected"), tr("Actual"), tr("Status")});
    m_verificationResultsTable->verticalHeader()->setVisible(false);
    m_verificationResultsTable->horizontalHeader()->setStretchLastSection(true);
    m_verificationResultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_verificationResultsTable->setMaximumHeight(120);

    m_verificationLogEdit = new QTextEdit(verificationBox);
    m_verificationLogEdit->setObjectName("moduleStudioVerificationLogEdit");
    m_verificationLogEdit->setReadOnly(true);
    m_verificationLogEdit->setMaximumHeight(96);

    auto *traceBox = new QGroupBox(tr("Trace Viewer"), verificationBox);
    auto *traceLayout = new QVBoxLayout(traceBox);
    m_traceStatusLabel = new QLabel(tr("No saved trace for current scenario"), traceBox);
    m_traceStatusLabel->setObjectName("moduleStudioTraceStatusLabel");
    m_traceStatusLabel->setWordWrap(true);

    auto *traceControlsRow = new QHBoxLayout;
    m_traceRestartButton = new QPushButton(tr("Restart"), traceBox);
    m_traceRestartButton->setObjectName("moduleStudioTraceRestartButton");
    m_traceStepButton = new QPushButton(tr("Step"), traceBox);
    m_traceStepButton->setObjectName("moduleStudioTraceStepButton");
    m_traceStepOverButton = new QPushButton(tr("Step Over"), traceBox);
    m_traceStepOverButton->setObjectName("moduleStudioTraceStepOverButton");
    m_traceRunToEndButton = new QPushButton(tr("Run To End"), traceBox);
    m_traceRunToEndButton->setObjectName("moduleStudioTraceRunToEndButton");
    traceControlsRow->addWidget(m_traceRestartButton);
    traceControlsRow->addWidget(m_traceStepButton);
    traceControlsRow->addWidget(m_traceStepOverButton);
    traceControlsRow->addWidget(m_traceRunToEndButton);
    traceControlsRow->addStretch();

    auto *traceContentRow = new QHBoxLayout;
    m_traceEventsTable = new QTableWidget(traceBox);
    m_traceEventsTable->setObjectName("moduleStudioTraceEventsTable");
    m_traceEventsTable->setColumnCount(4);
    m_traceEventsTable->setHorizontalHeaderLabels({tr("Step"), tr("Kind"), tr("Line"), tr("Source")});
    m_traceEventsTable->verticalHeader()->setVisible(false);
    m_traceEventsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_traceEventsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_traceEventsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_traceEventsTable->horizontalHeader()->setStretchLastSection(true);
    m_traceEventsTable->setMaximumHeight(180);

    auto *traceDetailsBox = new QGroupBox(tr("Current Step"), traceBox);
    auto *traceDetailsLayout = new QFormLayout(traceDetailsBox);
    m_traceCurrentStepLabel = new QLabel(tr("No step selected"), traceDetailsBox);
    m_traceCurrentStepLabel->setObjectName("moduleStudioTraceCurrentStepLabel");
    m_traceCurrentStepLabel->setWordWrap(true);
    m_traceCurrentFlowLabel = new QLabel(tr("Current branch: n/a"), traceDetailsBox);
    m_traceCurrentFlowLabel->setObjectName("moduleStudioTraceCurrentFlowLabel");
    m_traceCurrentFlowLabel->setWordWrap(true);
    m_traceSourceLabel = new QLabel(tr("Source: n/a"), traceDetailsBox);
    m_traceSourceLabel->setObjectName("moduleStudioTraceSourceLabel");
    m_traceSourceLabel->setWordWrap(true);
    traceDetailsLayout->addRow(tr("Step"), m_traceCurrentStepLabel);
    traceDetailsLayout->addRow(tr("Flow"), m_traceCurrentFlowLabel);
    traceDetailsLayout->addRow(tr("Source"), m_traceSourceLabel);

    traceContentRow->addWidget(m_traceEventsTable, 2);
    traceContentRow->addWidget(traceDetailsBox, 1);

    auto *traceValuesRow = new QHBoxLayout;
    auto *traceInputsBox = new QGroupBox(tr("Trace Inputs"), traceBox);
    auto *traceInputsLayout = new QVBoxLayout(traceInputsBox);
    m_traceInputsTable = new QTableWidget(traceInputsBox);
    m_traceInputsTable->setObjectName("moduleStudioTraceInputsTable");
    m_traceInputsTable->setColumnCount(2);
    m_traceInputsTable->setHorizontalHeaderLabels({tr("Input"), tr("Value")});
    m_traceInputsTable->verticalHeader()->setVisible(false);
    m_traceInputsTable->horizontalHeader()->setStretchLastSection(true);
    m_traceInputsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    traceInputsLayout->addWidget(m_traceInputsTable);

    auto *traceLocalsBox = new QGroupBox(tr("Locals"), traceBox);
    auto *traceLocalsLayout = new QVBoxLayout(traceLocalsBox);
    m_traceLocalsTable = new QTableWidget(traceLocalsBox);
    m_traceLocalsTable->setObjectName("moduleStudioTraceLocalsTable");
    m_traceLocalsTable->setColumnCount(2);
    m_traceLocalsTable->setHorizontalHeaderLabels({tr("Variable"), tr("Value")});
    m_traceLocalsTable->verticalHeader()->setVisible(false);
    m_traceLocalsTable->horizontalHeader()->setStretchLastSection(true);
    m_traceLocalsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    traceLocalsLayout->addWidget(m_traceLocalsTable);

    auto *traceOutputsBox = new QGroupBox(tr("Outputs / Return"), traceBox);
    auto *traceOutputsLayout = new QVBoxLayout(traceOutputsBox);
    m_traceOutputsTable = new QTableWidget(traceOutputsBox);
    m_traceOutputsTable->setObjectName("moduleStudioTraceOutputsTable");
    m_traceOutputsTable->setColumnCount(2);
    m_traceOutputsTable->setHorizontalHeaderLabels({tr("Output"), tr("Value")});
    m_traceOutputsTable->verticalHeader()->setVisible(false);
    m_traceOutputsTable->horizontalHeader()->setStretchLastSection(true);
    m_traceOutputsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    traceOutputsLayout->addWidget(m_traceOutputsTable);

    traceValuesRow->addWidget(traceInputsBox, 1);
    traceValuesRow->addWidget(traceLocalsBox, 1);
    traceValuesRow->addWidget(traceOutputsBox, 1);

    verificationLayout->addLayout(scenarioRow);
    verificationLayout->addLayout(scenarioNameRow);
    verificationLayout->addLayout(tablesRow);
    verificationLayout->addWidget(m_verificationRunStatusLabel);
    verificationLayout->addWidget(m_verificationResultsTable);
    verificationLayout->addWidget(m_verificationLogEdit);
    verificationLayout->addWidget(traceBox);
    traceLayout->addWidget(m_traceStatusLabel);
    traceLayout->addLayout(traceControlsRow);
    traceLayout->addLayout(traceContentRow);
    traceLayout->addLayout(traceValuesRow);
    bottomLayout->addWidget(verificationBox);
    splitter->addWidget(bottomPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({240, 560});

    connect(m_nameEdit, &QLineEdit::textChanged, this, &ModuleEditorWidget::onFieldEdited);
    connect(m_descriptionEdit, &QLineEdit::textChanged, this, &ModuleEditorWidget::onFieldEdited);
    connect(m_categoryCombo, &QComboBox::currentTextChanged, this, &ModuleEditorWidget::onFieldEdited);
    connect(m_languageCombo, &QComboBox::currentTextChanged, this, &ModuleEditorWidget::onFieldEdited);
    connect(m_trustStateCombo, &QComboBox::currentTextChanged, this, &ModuleEditorWidget::onFieldEdited);
    connect(m_includesEdit, &QTextEdit::textChanged, this, &ModuleEditorWidget::onFieldEdited);
    connect(m_codeEdit, &QPlainTextEdit::textChanged, this, &ModuleEditorWidget::onFieldEdited);
    connect(m_codeEdit, &QPlainTextEdit::cursorPositionChanged, this, &ModuleEditorWidget::refreshCodeSelections);
    connect(m_portTable, &QTableWidget::itemChanged, this, &ModuleEditorWidget::onContractTableItemChanged);
    connect(m_saveButton, &QPushButton::clicked, this, &ModuleEditorWidget::saveModule);
    connect(m_reloadButton, &QPushButton::clicked, this, &ModuleEditorWidget::reloadFromDisk);
    connect(m_addInputButton, &QPushButton::clicked, this, &ModuleEditorWidget::onAddInputPort);
    connect(m_addOutputButton, &QPushButton::clicked, this, &ModuleEditorWidget::onAddOutputPort);
    connect(m_removePortButton, &QPushButton::clicked, this, &ModuleEditorWidget::onRemoveSelectedPort);
    connect(m_syncContractButton, &QPushButton::clicked, this, &ModuleEditorWidget::onSyncContractFromSource);
    connect(m_applyContractButton, &QPushButton::clicked, this, &ModuleEditorWidget::onApplyContractToSource);
    connect(m_verificationScenarioCombo,
            &QComboBox::currentIndexChanged,
            this,
            &ModuleEditorWidget::onVerificationScenarioChanged);
    connect(m_verificationScenarioNameEdit,
            &QLineEdit::textChanged,
            this,
            &ModuleEditorWidget::onVerificationScenarioNameEdited);
    connect(m_verificationInputsTable,
            &QTableWidget::itemChanged,
            this,
            &ModuleEditorWidget::onVerificationTableItemChanged);
    connect(m_verificationExpectedTable,
            &QTableWidget::itemChanged,
            this,
            &ModuleEditorWidget::onVerificationTableItemChanged);
    connect(m_addVerificationScenarioButton,
            &QPushButton::clicked,
            this,
            &ModuleEditorWidget::onAddVerificationScenario);
    connect(m_removeVerificationScenarioButton,
            &QPushButton::clicked,
            this,
            &ModuleEditorWidget::onRemoveVerificationScenario);
    connect(m_compileVerificationButton,
            &QPushButton::clicked,
            this,
            &ModuleEditorWidget::onCompileVerification);
    connect(m_runVerificationButton,
            &QPushButton::clicked,
            this,
            &ModuleEditorWidget::onRunVerificationScenario);
    connect(m_traceEventsTable,
            &QTableWidget::currentCellChanged,
            this,
            &ModuleEditorWidget::onTraceEventSelectionChanged);
    connect(m_traceRestartButton,
            &QPushButton::clicked,
            this,
            &ModuleEditorWidget::onTraceRestart);
    connect(m_traceStepButton,
            &QPushButton::clicked,
            this,
            &ModuleEditorWidget::onTraceStep);
    connect(m_traceStepOverButton,
            &QPushButton::clicked,
            this,
            &ModuleEditorWidget::onTraceStepOver);
    connect(m_traceRunToEndButton,
            &QPushButton::clicked,
            this,
            &ModuleEditorWidget::onTraceRunToEnd);
    connect(codeEdit, &CodePlainTextEdit::hoverRequested, this, &ModuleEditorWidget::onCodeHoverRequested);
    connect(codeEdit, &CodePlainTextEdit::completionRequested, this, &ModuleEditorWidget::onCodeCompletionRequested);
}

// Заполняет все поля редактора значениями текущего загруженного модуля.
void ModuleEditorWidget::populateForm()
{
    m_loading = true;

    if (m_registry) {
        for (const QString &category : m_registry->categories()) {
            if (m_categoryCombo->findText(category) < 0)
                m_categoryCombo->addItem(category);
        }
    }

    m_nameEdit->setText(m_module.name);
    m_descriptionEdit->setText(m_module.description);

    int categoryIndex = m_categoryCombo->findText(m_module.category);
    if (categoryIndex < 0)
        m_categoryCombo->setCurrentText(m_module.category);
    else
        m_categoryCombo->setCurrentIndex(categoryIndex);

    int languageIndex = m_languageCombo->findText(m_module.language);
    if (languageIndex >= 0)
        m_languageCombo->setCurrentIndex(languageIndex);
    else
        m_languageCombo->setCurrentText(m_module.language);

    const QString trustState = m_module.effectiveTrustState();
    int trustIndex = m_trustStateCombo->findText(trustState);
    if (trustIndex < 0) {
        m_trustStateCombo->addItem(trustState);
        trustIndex = m_trustStateCombo->findText(trustState);
    }
    m_trustStateCombo->setCurrentIndex(trustIndex);

    m_includesEdit->setPlainText(m_module.includes.join("\n"));
    m_codeEdit->setPlainText(m_module.sourceCode);
    loadContractTableFromModule(m_module);
    normalizeVerificationScenarios(&m_module);
    normalizeVerificationTraceArtifacts(&m_module);
    loadVerificationWorkspaceFromModule(m_module);

    const bool editable = isEditable();
    const bool canVerify = !m_module.sourceCode.trimmed().isEmpty() && !m_module.isComposite();
    const bool canInspectScenarios = canVerify || !m_module.verificationScenarios.isEmpty();
    m_nameEdit->setReadOnly(!editable);
    m_descriptionEdit->setReadOnly(!editable);
    m_categoryCombo->setEnabled(editable);
    m_languageCombo->setEnabled(editable);
    m_trustStateCombo->setEnabled(editable);
    m_includesEdit->setReadOnly(!editable);
    m_codeEdit->setReadOnly(!editable);
    m_portTable->setEditTriggers(editable
        ? QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
        : QAbstractItemView::NoEditTriggers);
    m_saveButton->setEnabled(editable);
    m_addInputButton->setEnabled(editable);
    m_addOutputButton->setEnabled(editable);
    m_removePortButton->setEnabled(editable);
    m_syncContractButton->setEnabled(editable);
    m_applyContractButton->setEnabled(editable);
    m_verificationScenarioCombo->setEnabled(canInspectScenarios);
    m_verificationScenarioNameEdit->setReadOnly(!editable || !canInspectScenarios);
    m_verificationInputsTable->setEditTriggers((editable && canVerify)
        ? QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
        : QAbstractItemView::NoEditTriggers);
    m_verificationExpectedTable->setEditTriggers((editable && canVerify)
        ? QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
        : QAbstractItemView::NoEditTriggers);
    m_addVerificationScenarioButton->setEnabled(editable && canVerify);
    m_removeVerificationScenarioButton->setEnabled(editable && canVerify);
    m_compileVerificationButton->setEnabled(canVerify);
    m_runVerificationButton->setEnabled(canVerify);
    m_traceEventsTable->setEnabled(!m_module.verificationTraceArtifacts.isEmpty());
    m_reloadButton->setEnabled(!m_scratchpadMode);

    refreshSummary();
    refreshContractPreview();
    refreshCodeSelections();
    m_loading = false;
}

// Считывает форму в модель, затем синхронизирует контракт и сигнатуру исходника перед сохранением.
bool ModuleEditorWidget::applyFormToModule(QString *errorMessage)
{
    m_module.name = m_nameEdit->text().trimmed();
    m_module.description = m_descriptionEdit->text().trimmed();
    m_module.category = m_categoryCombo->currentText().trimmed();
    m_module.language = m_languageCombo->currentText().trimmed();
    m_module.trustState = m_trustStateCombo->currentText().trimmed();

    m_module.includes.clear();
    for (const auto &line : m_includesEdit->toPlainText().split('\n')) {
        const QString includeLine = line.trimmed();
        if (!includeLine.isEmpty())
            m_module.includes.append(includeLine);
    }

    m_module.sourceCode = m_codeEdit->toPlainText();
    // Сначала читаем явный contract из таблицы, затем синхронизируем сигнатуру в source.
    if (!readContractTableIntoModule(&m_module, errorMessage))
        return false;
    if (!rewriteModuleSignatureFromContract(&m_module.sourceCode, m_module, errorMessage))
        return false;
    syncVerificationScenarioFromUi();
    normalizeVerificationScenarios(&m_module);
    normalizeVerificationTraceArtifacts(&m_module);

    if (errorMessage)
        errorMessage->clear();
    refreshSummary();
    refreshContractPreview();
    refreshCodeSelections();
    return true;
}

// Обновляет верхний summary-блок редактора по текущему состоянию модуля.
void ModuleEditorWidget::refreshSummary()
{
    const bool editable = isEditable();
    const QString fileName = m_filePath.isEmpty()
        ? tr("unsaved")
        : QFileInfo(m_filePath).fileName();
    const QString state = m_modified ? tr("modified") : tr("saved");
    const QString surfaceKind = m_scratchpadMode
        ? tr("scratchpad")
        : (editable ? tr("editable") : tr("read-only"));
    m_statusLabel->setText(tr("%1 | %2 | %3")
                               .arg(fileName,
                                    surfaceKind,
                                    state));

    m_originValueLabel->setText(m_scratchpadMode ? tr("scratchpad") : m_module.origin);
    m_provenanceValueLabel->setText(provenanceSummaryText(m_module));
    m_verificationValueLabel->setText(verificationSummaryText(m_module));
    if (m_saveButton)
        m_saveButton->setText(m_scratchpadMode ? tr("Promote To Module") : tr("Save Module"));
    if (m_reloadButton) {
        m_reloadButton->setEnabled(!m_scratchpadMode);
        m_reloadButton->setToolTip(
            m_scratchpadMode
                ? tr("Scratchpad has no saved source-of-truth on disk until first promotion.")
                : QString());
    }
}

// Загружает verification workspace из current module state и восстанавливает active scenario.
void ModuleEditorWidget::loadVerificationWorkspaceFromModule(const Module &module)
{
    if (!m_verificationScenarioCombo
        || !m_verificationScenarioNameEdit
        || !m_verificationInputsTable
        || !m_verificationExpectedTable) {
        return;
    }

    m_loadingVerification = true;
    populateVerificationScenarioSelector();

    if (module.verificationScenarios.isEmpty()) {
        m_currentVerificationScenarioIndex = -1;
        m_verificationScenarioNameEdit->clear();
        m_verificationInputsTable->setRowCount(0);
        m_verificationExpectedTable->setRowCount(0);
        m_verificationLogEdit->clear();
        m_removeVerificationScenarioButton->setEnabled(false);
        m_runVerificationButton->setEnabled(false);
        updateVerificationResultViews(module, nullptr, nullptr);
        loadTraceViewerFromModule(module);
        m_loadingVerification = false;
        return;
    }

    int index = m_currentVerificationScenarioIndex;
    if (index < 0 || index >= module.verificationScenarios.size())
        index = 0;
    m_currentVerificationScenarioIndex = index;
    m_verificationScenarioCombo->setCurrentIndex(index);
    m_verificationScenarioNameEdit->setText(module.verificationScenarios.at(index).name);
    m_removeVerificationScenarioButton->setEnabled(isEditable());
    m_runVerificationButton->setEnabled(true);
    populateVerificationTablesFromCurrentScenario(module);
    loadTraceViewerFromModule(module);
    m_loadingVerification = false;
}

// Перестраивает selector сохранённых verification scenarios по данным модуля.
void ModuleEditorWidget::populateVerificationScenarioSelector()
{
    if (!m_verificationScenarioCombo)
        return;

    const QSignalBlocker blocker(m_verificationScenarioCombo);
    m_verificationScenarioCombo->clear();
    for (const auto &scenario : m_module.verificationScenarios) {
        const QString displayName = scenario.displayName().isEmpty()
            ? tr("Scenario")
            : scenario.displayName();
        m_verificationScenarioCombo->addItem(displayName);
    }
}

// Загружает в verification workspace только data-порты:
// execution-flow не требует пользовательских значений и не сравнивается как output-value.
void ModuleEditorWidget::populateVerificationTablesFromCurrentScenario(const Module &module)
{
    if (m_currentVerificationScenarioIndex < 0
        || m_currentVerificationScenarioIndex >= module.verificationScenarios.size()) {
        m_verificationInputsTable->setRowCount(0);
        m_verificationExpectedTable->setRowCount(0);
        return;
    }

    const ModuleVerificationScenario &scenario = module.verificationScenarios.at(m_currentVerificationScenarioIndex);
    populatePortValueTable(m_verificationInputsTable, module.dataInputs(), scenario.inputValues);
    populatePortValueTable(m_verificationExpectedTable, module.dataOutputs(), scenario.expectedOutputValues);
}

// Возвращает trace artifact, который соответствует активному verification scenario.
const ModuleTraceArtifact *ModuleEditorWidget::currentTraceArtifact(const Module &module) const
{
    if (m_currentVerificationScenarioIndex < 0
        || m_currentVerificationScenarioIndex >= module.verificationScenarios.size()) {
        return nullptr;
    }

    const QString scenarioId = module.verificationScenarios.at(m_currentVerificationScenarioIndex).id.trimmed();
    if (scenarioId.isEmpty())
        return nullptr;

    for (const auto &artifact : module.verificationTraceArtifacts) {
        if (artifact.scenarioId.trimmed() == scenarioId)
            return &artifact;
    }
    return nullptr;
}

// Загружает trace viewer для активного scenario из уже сохранённых trace artifacts.
void ModuleEditorWidget::loadTraceViewerFromModule(const Module &module)
{
    if (!m_traceStatusLabel
        || !m_traceEventsTable
        || !m_traceInputsTable
        || !m_traceLocalsTable
        || !m_traceOutputsTable
        || !m_traceCurrentStepLabel
        || !m_traceCurrentFlowLabel
        || !m_traceSourceLabel) {
        return;
    }

    if (m_currentVerificationScenarioIndex < 0
        || m_currentVerificationScenarioIndex >= module.verificationScenarios.size()) {
        m_traceStatusLabel->setText(tr("No scenario selected for trace review"));
        m_traceCurrentStepLabel->setText(tr("No step selected"));
        m_traceCurrentFlowLabel->setText(tr("Current branch: n/a"));
        m_traceSourceLabel->setText(tr("Source: n/a"));
        populateTraceEventTable(nullptr);
        populateTraceValueTable(m_traceInputsTable, {});
        populateTraceValueTable(m_traceLocalsTable, {});
        populateTraceValueTable(m_traceOutputsTable, {});
        m_traceEventsTable->setEnabled(false);
        refreshTraceControls(module);
        return;
    }

    const ModuleVerificationScenario &scenario = module.verificationScenarios.at(m_currentVerificationScenarioIndex);
    populateTraceValueTable(m_traceInputsTable, scenario.inputValues);

    const ModuleTraceArtifact *artifact = currentTraceArtifact(module);
    populateTraceEventTable(artifact);

    if (!artifact) {
        m_traceStatusLabel->setText(
            tr("No saved trace for scenario '%1'").arg(scenario.displayName()));
        m_traceCurrentStepLabel->setText(tr("No step selected"));
        m_traceCurrentFlowLabel->setText(tr("Current branch: n/a"));
        m_traceSourceLabel->setText(tr("Source: n/a"));
        populateTraceValueTable(m_traceLocalsTable, {});
        populateTraceValueTable(m_traceOutputsTable, {});
        m_traceEventsTable->setEnabled(false);
        return;
    }

    if (artifact->status.trimmed() == QStringLiteral("trace_unavailable")) {
        m_traceStatusLabel->setText(
            tr("Trace unavailable for scenario '%1'").arg(scenario.displayName()));
        m_traceCurrentStepLabel->setText(tr("No step selected"));
        m_traceCurrentFlowLabel->setText(
            artifact->unavailableReason.trimmed().isEmpty()
                ? tr("Current branch: trace unavailable")
                : artifact->unavailableReason.trimmed());
        m_traceSourceLabel->setText(tr("Source: n/a"));
        populateTraceValueTable(m_traceLocalsTable, {});
        populateTraceValueTable(m_traceOutputsTable, {});
        m_traceEventsTable->setEnabled(false);
        refreshTraceControls(module);
        return;
    }

    m_traceStatusLabel->setText(
        tr("Trace captured for scenario '%1' (%2 step(s))")
            .arg(scenario.displayName())
            .arg(artifact->events.size()));
    m_traceEventsTable->setEnabled(!artifact->events.isEmpty());
    updateTraceViewerDetails(module);
    refreshTraceControls(module);
}

// Перестраивает таблицу trace events и по умолчанию выбирает последний шаг.
void ModuleEditorWidget::populateTraceEventTable(const ModuleTraceArtifact *artifact)
{
    if (!m_traceEventsTable)
        return;

    const QSignalBlocker blocker(m_traceEventsTable);
    m_traceEventsTable->setRowCount(0);
    if (!artifact || artifact->events.isEmpty())
        return;

    for (int row = 0; row < artifact->events.size(); ++row) {
        const ModuleTraceEvent &event = artifact->events.at(row);
        m_traceEventsTable->insertRow(row);
        m_traceEventsTable->setItem(row, 0, lockedTableItem(QString::number(event.stepIndex)));
        m_traceEventsTable->setItem(row, 1, lockedTableItem(event.eventKind));
        m_traceEventsTable->setItem(
            row,
            2,
            lockedTableItem(event.sourceLine >= 0 ? QString::number(event.sourceLine) : QStringLiteral("-")));
        m_traceEventsTable->setItem(row, 3, lockedTableItem(event.sourceSnippet));
    }
    m_traceEventsTable->resizeColumnsToContents();
    m_traceEventsTable->setCurrentCell(artifact->events.size() - 1, 0);
}

// Возвращает индекс выбранного шага в trace events table.
int ModuleEditorWidget::currentTraceEventIndex() const
{
    if (!m_traceEventsTable)
        return -1;
    return m_traceEventsTable->currentRow();
}

// Переводит selection viewer на нужный row и не выходит за пределы trace table.
void ModuleEditorWidget::selectTraceEventRow(int row)
{
    if (!m_traceEventsTable || m_traceEventsTable->rowCount() <= 0)
        return;

    const int clampedRow = std::clamp(row, 0, m_traceEventsTable->rowCount() - 1);
    m_traceEventsTable->setCurrentCell(clampedRow, 0);
    updateTraceViewerDetails(m_module);
    refreshTraceControls(m_module);
}

// Освежает правую панель trace viewer по текущему выбранному step event.
void ModuleEditorWidget::updateTraceViewerDetails(const Module &module)
{
    if (!m_traceCurrentStepLabel
        || !m_traceCurrentFlowLabel
        || !m_traceSourceLabel
        || !m_traceLocalsTable
        || !m_traceOutputsTable) {
        return;
    }

    const ModuleTraceArtifact *artifact = currentTraceArtifact(module);
    if (!artifact || artifact->status.trimmed() != QStringLiteral("captured") || artifact->events.isEmpty()) {
        m_traceCurrentStepLabel->setText(tr("No step selected"));
        if (!artifact || artifact->unavailableReason.trimmed().isEmpty())
            m_traceCurrentFlowLabel->setText(tr("Current branch: n/a"));
        else
            m_traceCurrentFlowLabel->setText(artifact->unavailableReason.trimmed());
        m_traceSourceLabel->setText(tr("Source: n/a"));
        populateTraceValueTable(m_traceLocalsTable, {});
        populateTraceValueTable(m_traceOutputsTable, {});
        return;
    }

    int eventIndex = currentTraceEventIndex();
    if (eventIndex < 0 || eventIndex >= artifact->events.size())
        eventIndex = artifact->events.size() - 1;

    const ModuleTraceEvent &event = artifact->events.at(eventIndex);
    m_traceCurrentStepLabel->setText(
        tr("Step %1 of %2 (%3)")
            .arg(eventIndex + 1)
            .arg(artifact->events.size())
            .arg(event.eventKind));

    QString flowText = tr("Current branch: %1").arg(event.eventKind);
    if (event.sourceLine >= 0)
        flowText += tr(" | line %1").arg(event.sourceLine);
    if (!event.note.trimmed().isEmpty())
        flowText += tr(" | %1").arg(event.note.trimmed());
    m_traceCurrentFlowLabel->setText(flowText);
    m_traceSourceLabel->setText(
        event.sourceSnippet.trimmed().isEmpty()
            ? tr("Source: n/a")
            : tr("Source: %1").arg(event.sourceSnippet.trimmed()));

    populateTraceValueTable(
        m_traceLocalsTable,
        accumulatedTraceVariableState(*artifact, eventIndex));
    populateTraceValueTable(
        m_traceOutputsTable,
        accumulatedTraceOutputState(*artifact, eventIndex));
    refreshTraceControls(module);
}

// Освежает enabled-state stepping controls для текущего captured/unavailable trace.
void ModuleEditorWidget::refreshTraceControls(const Module &module)
{
    const ModuleTraceArtifact *artifact = currentTraceArtifact(module);
    const bool hasCapturedTrace = artifact
        && artifact->status.trimmed() == QStringLiteral("captured")
        && !artifact->events.isEmpty();
    const int currentRow = currentTraceEventIndex();
    const int lastRow = hasCapturedTrace ? artifact->events.size() - 1 : -1;

    if (m_traceRestartButton)
        m_traceRestartButton->setEnabled(hasCapturedTrace && currentRow > 0);
    if (m_traceStepButton)
        m_traceStepButton->setEnabled(hasCapturedTrace && currentRow >= 0 && currentRow < lastRow);
    if (m_traceStepOverButton)
        m_traceStepOverButton->setEnabled(hasCapturedTrace && currentRow >= 0 && currentRow < lastRow);
    if (m_traceRunToEndButton)
        m_traceRunToEndButton->setEnabled(hasCapturedTrace && currentRow >= 0 && currentRow < lastRow);
}

// Собирает накопленное состояние переменных по sequence trace events до выбранного шага.
QMap<QString, QString> ModuleEditorWidget::accumulatedTraceVariableState(const ModuleTraceArtifact &artifact,
                                                                         int eventIndex) const
{
    QMap<QString, QString> values;
    if (eventIndex < 0)
        return values;

    const int cappedIndex = std::min(eventIndex, static_cast<int>(artifact.events.size()) - 1);
    for (int i = 0; i <= cappedIndex; ++i) {
        const ModuleTraceEvent &event = artifact.events.at(i);
        for (auto it = event.variableSnapshotDelta.begin(); it != event.variableSnapshotDelta.end(); ++it)
            values[it.key()] = it.value();
    }
    return values;
}

// Собирает накопленное output-state по sequence trace events до выбранного шага.
QMap<QString, QString> ModuleEditorWidget::accumulatedTraceOutputState(const ModuleTraceArtifact &artifact,
                                                                       int eventIndex) const
{
    QMap<QString, QString> values;
    if (eventIndex < 0)
        return values;

    const int cappedIndex = std::min(eventIndex, static_cast<int>(artifact.events.size()) - 1);
    for (int i = 0; i <= cappedIndex; ++i) {
        const ModuleTraceEvent &event = artifact.events.at(i);
        for (auto it = event.outputSnapshot.begin(); it != event.outputSnapshot.end(); ++it)
            values[it.key()] = it.value();
    }
    return values;
}

// Заполняет компактную trace table значениями одного logical scope-а.
void ModuleEditorWidget::populateTraceValueTable(QTableWidget *table,
                                                 const QMap<QString, QString> &values) const
{
    if (!table)
        return;

    const QSignalBlocker blocker(table);
    table->setRowCount(0);
    int row = 0;
    for (auto it = values.begin(); it != values.end(); ++it) {
        table->insertRow(row);
        table->setItem(row, 0, lockedTableItem(it.key()));
        table->setItem(row, 1, lockedTableItem(it.value()));
        ++row;
    }
    table->resizeColumnsToContents();
}

// Сохраняет текущее имя, data-inputs и data-outputs обратно в активный verification scenario.
void ModuleEditorWidget::syncVerificationScenarioFromUi()
{
    if (m_loadingVerification)
        return;
    if (m_currentVerificationScenarioIndex < 0
        || m_currentVerificationScenarioIndex >= m_module.verificationScenarios.size()) {
        return;
    }

    Module currentContract = m_module;
    QString contractError;
    if (!readContractTableIntoModule(&currentContract, &contractError))
        currentContract = m_module;

    ModuleVerificationScenario &scenario = m_module.verificationScenarios[m_currentVerificationScenarioIndex];
    scenario.name = m_verificationScenarioNameEdit
        ? m_verificationScenarioNameEdit->text().trimmed()
        : scenario.name;
    if (scenario.id.trimmed().isEmpty())
        scenario.id = QStringLiteral("scenario_%1").arg(m_currentVerificationScenarioIndex + 1);

    const QMap<QString, QString> inputValues = readVerificationTableValues(m_verificationInputsTable);
    scenario.inputValues.clear();
    for (const auto &port : currentContract.dataInputs())
        scenario.inputValues[port.name] = inputValues.value(port.name, port.defaultValue);

    const QMap<QString, QString> expectedValues = readVerificationTableValues(m_verificationExpectedTable);
    scenario.expectedOutputValues.clear();
    for (const auto &port : currentContract.dataOutputs())
        scenario.expectedOutputValues[port.name] = expectedValues.value(port.name);
}

// Нормализует verification scenarios под текущий набор data-портов и обновляет summary refs.
void ModuleEditorWidget::normalizeVerificationScenarios(Module *module) const
{
    if (!module)
        return;

    QStringList scenarioRefs;
    for (int i = 0; i < module->verificationScenarios.size(); ++i) {
        ModuleVerificationScenario &scenario = module->verificationScenarios[i];
        if (scenario.id.trimmed().isEmpty())
            scenario.id = QStringLiteral("scenario_%1").arg(i + 1);
        if (scenario.name.trimmed().isEmpty())
            scenario.name = tr("Scenario %1").arg(i + 1);

        QMap<QString, QString> normalizedInputs;
        for (const auto &port : module->dataInputs())
            normalizedInputs[port.name] = scenario.inputValues.value(port.name, port.defaultValue);
        scenario.inputValues = normalizedInputs;

        QMap<QString, QString> normalizedExpectedOutputs;
        for (const auto &port : module->dataOutputs())
            normalizedExpectedOutputs[port.name] = scenario.expectedOutputValues.value(port.name);
        scenario.expectedOutputValues = normalizedExpectedOutputs;

        const QString displayName = scenario.displayName();
        if (!displayName.isEmpty())
            scenarioRefs.append(displayName);
    }

    module->verificationScenarioRefs = scenarioRefs;
}

// Удерживает trace artifacts в явной связи с существующими verification scenarios.
void ModuleEditorWidget::normalizeVerificationTraceArtifacts(Module *module) const
{
    if (!module)
        return;

    if (module->verificationScenarios.isEmpty()) {
        module->verificationTraceArtifacts.clear();
        module->verificationTraceArtifactRefs.clear();
        return;
    }

    QStringList scenarioIds;
    for (const auto &scenario : module->verificationScenarios)
        scenarioIds.append(scenario.id.trimmed());

    QVector<ModuleTraceArtifact> normalizedArtifacts;
    QStringList traceRefs;
    for (const auto &existingArtifact : module->verificationTraceArtifacts) {
        ModuleTraceArtifact artifact = existingArtifact;
        if (artifact.scenarioId.trimmed().isEmpty()
            || !scenarioIds.contains(artifact.scenarioId.trimmed())) {
            continue;
        }

        if (artifact.id.trimmed().isEmpty())
            artifact.id = makeTraceArtifactId(*module, artifact.scenarioId.trimmed());
        if (artifact.createdAt.trimmed().isEmpty() && !module->lastVerifiedAt.trimmed().isEmpty())
            artifact.createdAt = module->lastVerifiedAt.trimmed();
        if (artifact.status.trimmed().isEmpty())
            artifact.status = artifact.events.isEmpty() ? "trace_unavailable" : "captured";

        normalizedArtifacts.append(artifact);
        const QString ref = artifact.summaryRef();
        if (!ref.isEmpty())
            traceRefs.append(ref);
    }

    module->verificationTraceArtifacts = normalizedArtifacts;
    module->verificationTraceArtifactRefs = traceRefs;
}

// Создаёт стартовый verification scenario по текущему data-контракту и default values входов.
ModuleVerificationScenario ModuleEditorWidget::makeDefaultVerificationScenario(const Module &module,
                                                                               int index) const
{
    ModuleVerificationScenario scenario;
    scenario.id = QStringLiteral("scenario_%1").arg(index + 1);
    scenario.name = tr("Scenario %1").arg(index + 1);

    for (const auto &port : module.dataInputs())
        scenario.inputValues[port.name] = port.defaultValue;
    for (const auto &port : module.dataOutputs())
        scenario.expectedOutputValues[port.name] = QString();
    return scenario;
}

// Создаёт trace artifact для verification-run без претензии на полный интерпретатор языка.
ModuleTraceArtifact ModuleEditorWidget::buildVerificationTraceArtifact(
    const Module &module,
    const ModuleVerificationScenario &scenario,
    const TestResult &result) const
{
    ModuleTraceArtifact artifact;
    const QString scenarioId = scenario.id.trimmed().isEmpty()
        ? stableTraceToken(scenario.displayName())
        : scenario.id.trimmed();
    artifact.id = makeTraceArtifactId(module, scenarioId);
    artifact.scenarioId = scenarioId;
    artifact.createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    if (!result.compiled) {
        artifact.status = "trace_unavailable";
        artifact.unavailableReason = tr("Trace unavailable because compile step failed.");
        return artifact;
    }

    if (!result.ran) {
        artifact.status = "trace_unavailable";
        artifact.unavailableReason = tr("Trace unavailable because module run did not complete.");
        return artifact;
    }

    const QString unsupportedReason = simplifiedTraceUnavailableReason(module);
    if (!unsupportedReason.isEmpty()) {
        artifact.status = "trace_unavailable";
        artifact.unavailableReason = unsupportedReason;
        return artifact;
    }

    artifact.status = "captured";
    const QStringList sourceLines = module.sourceCode.split('\n');
    const int entryLine = firstMeaningfulSourceLineNumber(sourceLines);
    const int returnLine = lineNumberContainingWord(sourceLines, QStringLiteral("return"));

    ModuleTraceEvent enterEvent;
    enterEvent.stepIndex = 0;
    enterEvent.eventKind = "enter";
    enterEvent.sourceLine = entryLine;
    enterEvent.sourceColumn = 1;
    enterEvent.sourceSnippet = sourceSnippetAtLine(sourceLines, entryLine);
    enterEvent.variableSnapshotDelta = scenario.inputValues;
    enterEvent.note = tr("A4.0 boundary trace captured scenario inputs.");
    artifact.events.append(enterEvent);

    ModuleTraceEvent returnEvent;
    returnEvent.stepIndex = 1;
    returnEvent.eventKind = "return";
    returnEvent.sourceLine = (returnLine >= 0) ? returnLine : entryLine;
    returnEvent.sourceColumn = 1;
    returnEvent.sourceSnippet = sourceSnippetAtLine(sourceLines, returnEvent.sourceLine);
    returnEvent.outputSnapshot = result.outputValues;
    returnEvent.note = result.passed
        ? tr("A4.0 boundary trace captured outputs that matched expectations.")
        : tr("A4.0 boundary trace captured actual outputs for comparison.");
    artifact.events.append(returnEvent);
    return artifact;
}

// Заменяет trace artifact для сценария или добавляет новый, затем обновляет summary refs.
void ModuleEditorWidget::storeVerificationTraceArtifact(Module *module,
                                                        const ModuleTraceArtifact &artifact) const
{
    if (!module)
        return;

    bool replaced = false;
    for (int i = 0; i < module->verificationTraceArtifacts.size(); ++i) {
        const ModuleTraceArtifact &existingArtifact = module->verificationTraceArtifacts.at(i);
        if (existingArtifact.scenarioId == artifact.scenarioId || existingArtifact.id == artifact.id) {
            module->verificationTraceArtifacts[i] = artifact;
            replaced = true;
            break;
        }
    }

    if (!replaced)
        module->verificationTraceArtifacts.append(artifact);

    normalizeVerificationTraceArtifacts(module);
}

// Считывает значения второй колонки verification table в key-value map по имени порта.
QMap<QString, QString> ModuleEditorWidget::readVerificationTableValues(const QTableWidget *table) const
{
    QMap<QString, QString> values;
    if (!table)
        return values;

    for (int row = 0; row < table->rowCount(); ++row) {
        const auto *nameItem = table->item(row, 0);
        const auto *valueItem = table->item(row, 1);
        if (!nameItem)
            continue;
        values[nameItem->text()] = valueItem ? valueItem->text() : QString();
    }
    return values;
}

// Заполняет verification table текущим набором портов и значениями выбранного сценария.
void ModuleEditorWidget::populatePortValueTable(QTableWidget *table,
                                                const QVector<Port> &ports,
                                                const QMap<QString, QString> &values) const
{
    if (!table)
        return;

    const QSignalBlocker blocker(table);
    table->setRowCount(0);
    for (int row = 0; row < ports.size(); ++row) {
        table->insertRow(row);
        table->setItem(row, 0, lockedTableItem(ports.at(row).name));
        table->setItem(row, 1, new QTableWidgetItem(values.value(ports.at(row).name)));
    }
    table->resizeColumnsToContents();
}

// Обновляет expected-vs-actual таблицу и итоговый статус последнего verification run
// только для data-outputs, а не для execution-flow.
void ModuleEditorWidget::updateVerificationResultViews(const Module &module,
                                                       const ModuleVerificationScenario *scenario,
                                                       const TestResult *result)
{
    if (!m_verificationRunStatusLabel || !m_verificationResultsTable)
        return;

    if (!scenario || !result) {
        m_verificationRunStatusLabel->setText(tr("Verification workspace ready"));
        m_verificationRunStatusLabel->setStyleSheet("color: #607d8b;");
        m_verificationResultsTable->setRowCount(0);
        return;
    }

    if (!result->compiled) {
        m_verificationRunStatusLabel->setText(tr("Compile failed for scenario '%1'").arg(scenario->displayName()));
        m_verificationRunStatusLabel->setStyleSheet("color: #b71c1c; font-weight: 600;");
        m_verificationResultsTable->setRowCount(0);
        return;
    }

    m_verificationRunStatusLabel->setText(
        result->passed
            ? tr("Scenario '%1' passed").arg(scenario->displayName())
            : tr("Scenario '%1' failed").arg(scenario->displayName()));
    m_verificationRunStatusLabel->setStyleSheet(
        result->passed
            ? "color: #2e7d32; font-weight: 600;"
            : "color: #b71c1c; font-weight: 600;");

    const QVector<Port> dataOutputs = module.dataOutputs();
    m_verificationResultsTable->setRowCount(0);
    for (int row = 0; row < dataOutputs.size(); ++row) {
        const Port &port = dataOutputs.at(row);
        const QString expectedValue = scenario->expectedOutputValues.value(port.name);
        const QString actualValue = result->outputValues.value(port.name);
        QString status = tr("Not checked");
        if (!expectedValue.trimmed().isEmpty())
            status = (expectedValue.trimmed() == actualValue.trimmed()) ? tr("Match") : tr("Mismatch");
        else if (result->outputValues.contains(port.name))
            status = tr("Produced");

        m_verificationResultsTable->insertRow(row);
        m_verificationResultsTable->setItem(row, 0, lockedTableItem(port.name));
        m_verificationResultsTable->setItem(row, 1, lockedTableItem(expectedValue));
        m_verificationResultsTable->setItem(row, 2, lockedTableItem(actualValue));
        m_verificationResultsTable->setItem(row, 3, lockedTableItem(status));
    }
    m_verificationResultsTable->resizeColumnsToContents();
}

// Добавляет строку в verification log без переключения пользователя в другой surface.
void ModuleEditorWidget::appendVerificationLogLine(const QString &text)
{
    if (!m_verificationLogEdit)
        return;
    m_verificationLogEdit->append(text);
}

// Освежает trust_state после проверки, не понижая curated/verified ручным образом.
void ModuleEditorWidget::refreshTrustStateAfterVerification(bool verificationPassed)
{
    const QString currentTrustState = m_module.trustState.trimmed();
    if (currentTrustState == "curated" || currentTrustState == "verified")
        return;

    if (verificationPassed) {
        m_module.trustState = "smoke_passed";
    } else if (currentTrustState.isEmpty()
               || currentTrustState == "draft"
               || currentTrustState == "smoke_passed") {
        m_module.trustState = "draft";
    }

    if (m_trustStateCombo) {
        const QSignalBlocker blocker(m_trustStateCombo);
        if (m_trustStateCombo->findText(m_module.trustState) < 0)
            m_trustStateCombo->addItem(m_module.trustState);
        m_trustStateCombo->setCurrentText(m_module.trustState);
    }
}

// Синхронизирует verification workspace с текущим contract table после смены портов.
void ModuleEditorWidget::refreshVerificationWorkspaceFromCurrentContract()
{
    syncVerificationScenarioFromUi();

    Module staged = m_module;
    QString contractError;
    if (!readContractTableIntoModule(&staged, &contractError))
        return;

    m_module.inputs = staged.inputs;
    m_module.outputs = staged.outputs;
    staged.verificationScenarios = m_module.verificationScenarios;
    staged.verificationScenarioRefs = m_module.verificationScenarioRefs;
    staged.verificationTraceArtifacts = m_module.verificationTraceArtifacts;
    staged.verificationTraceArtifactRefs = m_module.verificationTraceArtifactRefs;
    normalizeVerificationScenarios(&staged);
    normalizeVerificationTraceArtifacts(&staged);
    m_module.verificationScenarios = staged.verificationScenarios;
    m_module.verificationScenarioRefs = staged.verificationScenarioRefs;
    m_module.verificationTraceArtifacts = staged.verificationTraceArtifacts;
    m_module.verificationTraceArtifactRefs = staged.verificationTraceArtifactRefs;
    loadVerificationWorkspaceFromModule(staged);
}

// Переносит входы и выходы из модели модуля в editable contract table.
void ModuleEditorWidget::loadContractTableFromModule(const Module &module)
{
    if (!m_portTable)
        return;

    // При загрузке и sync-from-source таблица становится визуальным source-of-truth для контракта.
    const QSignalBlocker blocker(m_portTable);
    m_portTable->setRowCount(0);

    int row = 0;
    const auto appendPort = [this, &row](const Port &port, const QString &direction) {
        m_portTable->insertRow(row);
        m_portTable->setItem(row, 0, new QTableWidgetItem(port.name));
        m_portTable->setItem(row, 1, new QTableWidgetItem(port.type));
        m_portTable->setItem(row, 2, new QTableWidgetItem(direction));
        m_portTable->setItem(row, 3, new QTableWidgetItem(port.defaultValue));
        ++row;
    };

    for (const auto &port : module.inputs)
        appendPort(port, tr("Input"));
    for (const auto &port : module.outputs)
        appendPort(port, tr("Output"));

    m_portTable->resizeColumnsToContents();
}

// Считывает contract table обратно в модель, сохраняя execution/data kind и проверяя
// ограничения только для data-return части atomic-source формата.
bool ModuleEditorWidget::readContractTableIntoModule(Module *module, QString *errorMessage) const
{
    if (!module || !m_portTable)
        return false;

    // Считываем contract из editable таблицы и валидируем то, что можем честно выразить в C-source.
    module->inputs.clear();
    module->outputs.clear();

    for (int row = 0; row < m_portTable->rowCount(); ++row) {
        auto *nameItem = m_portTable->item(row, 0);
        auto *typeItem = m_portTable->item(row, 1);
        auto *directionItem = m_portTable->item(row, 2);
        auto *defaultItem = m_portTable->item(row, 3);

        const QString name = nameItem ? nameItem->text().trimmed() : QString();
        const QString type = typeItem ? typeItem->text().trimmed() : QString();
        const QString direction = directionItem ? directionItem->text().trimmed().toLower() : QString();

        if (name.isEmpty() || type.isEmpty()) {
            if (errorMessage) {
                *errorMessage = tr("Contract row %1 must contain both port name and type.")
                    .arg(row + 1);
            }
            return false;
        }

        Port port;
        port.name = name;
        port.type = type.compare(QStringLiteral("exec"), Qt::CaseInsensitive) == 0
            ? QStringLiteral("exec")
            : type;
        port.kind = port.type == QStringLiteral("exec") ? PortKind::Execution : PortKind::Data;
        port.defaultValue = (port.kind == PortKind::Execution || !defaultItem)
            ? QString()
            : defaultItem->text().trimmed();

        if (direction == tr("Input").toLower() || direction == "input") {
            module->inputs.append(port);
        } else if (direction == tr("Output").toLower() || direction == "output") {
            module->outputs.append(port);
        } else {
            if (errorMessage) {
                *errorMessage = tr("Contract row %1 has invalid direction '%2'. Use Input or Output.")
                    .arg(row + 1)
                    .arg(directionItem ? directionItem->text() : QString());
            }
            return false;
        }
    }

    if (module->dataOutputs().size() > 1) {
        if (errorMessage) {
            *errorMessage = tr(
                "Atomic source-backed module contract currently supports at most one data output.");
        }
        return false;
    }

    if (errorMessage)
        errorMessage->clear();
    return true;
}

// Собирает временную версию модуля из текущей формы без записи на диск.
Module ModuleEditorWidget::stagedModuleFromForm(bool *contractValid, QString *errorMessage) const
{
    Module staged = m_module;
    staged.name = m_nameEdit ? m_nameEdit->text().trimmed() : staged.name;
    staged.description = m_descriptionEdit ? m_descriptionEdit->text().trimmed() : staged.description;
    staged.category = m_categoryCombo ? m_categoryCombo->currentText().trimmed() : staged.category;
    staged.language = m_languageCombo ? m_languageCombo->currentText().trimmed() : staged.language;
    staged.trustState = m_trustStateCombo ? m_trustStateCombo->currentText().trimmed() : staged.trustState;
    staged.includes.clear();
    if (m_includesEdit) {
        for (const auto &line : m_includesEdit->toPlainText().split('\n')) {
            const QString includeLine = line.trimmed();
            if (!includeLine.isEmpty())
                staged.includes.append(includeLine);
        }
    }
    staged.sourceCode = m_codeEdit ? m_codeEdit->toPlainText() : staged.sourceCode;
    QString localError;
    const bool ok = readContractTableIntoModule(&staged, &localError);
    if (contractValid)
        *contractValid = ok;
    if (errorMessage)
        *errorMessage = localError;
    return staged;
}

// Возвращает путь документа, через который embedded source должен разговаривать с LSP.
QString ModuleEditorWidget::lspDocumentPath() const
{
    const QString sourcePath = m_module.sourcePath.trimmed();
    if (!sourcePath.isEmpty()) {
        QFileInfo sourceInfo(sourcePath);
        if (sourceInfo.isAbsolute())
            return QDir::cleanPath(sourceInfo.absoluteFilePath());
        if (!m_filePath.isEmpty())
            return QDir(QFileInfo(m_filePath).absolutePath()).absoluteFilePath(sourcePath);
        return QDir::cleanPath(sourcePath);
    }

    // Если реального sourcePath нет, используем стабильный виртуальный путь рядом с `.dqmod`.
    if (!m_filePath.isEmpty()) {
        const QFileInfo fileInfo(m_filePath);
        const QString suffix = (m_module.language.trimmed() == "cpp") ? "cpp" : "c";
        return QDir(fileInfo.absolutePath()).absoluteFilePath(
            fileInfo.completeBaseName() + QStringLiteral("_embedded_module.") + suffix);
    }

    return {};
}

// Возвращает LSP language id для текущего embedded source.
QString ModuleEditorWidget::lspLanguageId() const
{
    return m_module.language.trimmed() == "cpp" ? QStringLiteral("cpp") : QStringLiteral("c");
}

// Возвращает актуальный текст исходника из code editor панели.
QString ModuleEditorWidget::sourceText() const
{
    return m_codeEdit ? m_codeEdit->toPlainText() : QString();
}

// Сохраняет LSP-диагностику и обновляет подсветку в code editor панели.
void ModuleEditorWidget::setDiagnostics(const QVector<LSPDiagnostic> &diagnostics)
{
    m_diagnostics = diagnostics;
    refreshCodeSelections();
}

// Показывает hover-текст LSP прямо поверх embedded source.
void ModuleEditorWidget::showHoverTooltip(const QString &text, const QPoint &globalPos)
{
    if (text.isEmpty()) {
        QToolTip::hideText();
        return;
    }
    QToolTip::showText(globalPos, text, m_codeEdit);
}

// Показывает popup автодополнения рядом с курсором embedded source.
void ModuleEditorWidget::showCompletion(const QVector<CompletionItem> &items)
{
    if (!m_completionPopup)
        return;

    if (items.isEmpty()) {
        m_completionPopup->hide();
        return;
    }

    m_completionPopup->setItems(items);
    const QTextCursor cursor = m_codeEdit->textCursor();
    const QRect cursorRect = m_codeEdit->cursorRect(cursor);
    m_completionPopup->popup(m_codeEdit->mapToGlobal(cursorRect.bottomLeft()));
}

// Пересобирает визуальные выделения исходника: текущую строку и волнистые LSP-диагностики.
void ModuleEditorWidget::refreshCodeSelections()
{
    if (!m_codeEdit)
        return;

    QList<QTextEdit::ExtraSelection> selections;

    if (!m_codeEdit->isReadOnly()) {
        QTextEdit::ExtraSelection currentLine;
        currentLine.format.setBackground(QColor(255, 255, 220));
        currentLine.format.setProperty(QTextFormat::FullWidthSelection, true);
        currentLine.cursor = m_codeEdit->textCursor();
        currentLine.cursor.clearSelection();
        selections.append(currentLine);
    }

    QTextDocument *document = m_codeEdit->document();
    for (const auto &diag : m_diagnostics) {
        QTextBlock startBlock = document->findBlockByNumber(diag.range.start.line);
        QTextBlock endBlock = document->findBlockByNumber(diag.range.end.line);
        if (!startBlock.isValid())
            continue;
        if (!endBlock.isValid())
            endBlock = startBlock;

        const int startPos = startBlock.position()
            + qMin(diag.range.start.character, qMax(0, startBlock.length() - 1));
        const int endPos = endBlock.position()
            + qMin(diag.range.end.character, qMax(0, endBlock.length() - 1));

        QTextEdit::ExtraSelection diagnosticSelection;
        diagnosticSelection.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        diagnosticSelection.format.setUnderlineColor(diagnosticUnderlineColor(diag.severity));
        diagnosticSelection.format.setToolTip(diag.message);

        diagnosticSelection.cursor = QTextCursor(document);
        diagnosticSelection.cursor.setPosition(startPos);
        if (startPos == endPos)
            diagnosticSelection.cursor.select(QTextCursor::WordUnderCursor);
        else
            diagnosticSelection.cursor.setPosition(endPos, QTextCursor::KeepAnchor);
        selections.append(diagnosticSelection);
    }

    m_codeEdit->setExtraSelections(selections);
}

// Сравнивает editable contract table с тем, что реально выражено в текущей data-сигнатуре исходника.
void ModuleEditorWidget::refreshContractPreview()
{
    if (!m_portTable || !m_contractStatusLabel)
        return;

    // Сравниваем editable contract table с тем, что реально выражено в текущей сигнатуре source.
    bool contractValid = false;
    QString errorMessage;
    const Module staged = stagedModuleFromForm(&contractValid, &errorMessage);
    Module sourceDerived = staged;
    const bool parsed = parseModuleSignatureIntoContract(&sourceDerived, staged.sourceCode);
    const bool matchesSource = parsed
        && contractValid
        && sourceDerived.dataInputs() == staged.dataInputs()
        && sourceDerived.dataOutputs() == staged.dataOutputs();
    const int execInputCount = staged.inputs.size() - staged.dataInputs().size();
    const int execOutputCount = staged.outputs.size() - staged.dataOutputs().size();

    if (!contractValid) {
        m_contractStatusLabel->setText(
            tr("Contract table is invalid: %1").arg(errorMessage));
        m_contractStatusLabel->setStyleSheet("color: #b71c1c;");
    } else if (matchesSource) {
        m_contractStatusLabel->setText(
            tr("Contract table matches current `dq_*` data signature. Data inputs: %1 | Data outputs: %2 | Exec inputs: %3 | Exec outputs: %4")
                .arg(staged.dataInputs().size())
                .arg(staged.dataOutputs().size())
                .arg(execInputCount)
                .arg(execOutputCount));
        m_contractStatusLabel->setStyleSheet("color: #2e7d32;");
    } else if (parsed) {
        m_contractStatusLabel->setText(
            tr("Contract table differs from current source data signature. Use `Apply To Source` or `Sync From Source`."));
        m_contractStatusLabel->setStyleSheet("color: #b26a00;");
    } else {
        m_contractStatusLabel->setText(
            tr("Current source does not expose a parseable `dq_*` data signature. Use `Apply To Source` to regenerate one."));
        m_contractStatusLabel->setStyleSheet("color: #b26a00;");
    }
}

// Меняет dirty-state редактора и синхронно обновляет видимые summary/preview панели.
void ModuleEditorWidget::setModified(bool modified)
{
    if (m_modified == modified) {
        refreshSummary();
        return;
    }

    m_modified = modified;
    refreshSummary();
    refreshContractPreview();
    refreshCodeSelections();
    emit modificationChanged(modified);
}

// Помечает compile/test status как устаревшие после любой ручной правки модуля.
void ModuleEditorWidget::invalidateVerificationState()
{
    if (m_module.compileStatus == "passed" || m_module.compileStatus == "failed")
        m_module.compileStatus = "modified";
    if (m_module.testStatus == "passed" || m_module.testStatus == "failed")
        m_module.testStatus = "modified";
}

// Добавляет новый входной порт в editable contract table.
void ModuleEditorWidget::onAddInputPort()
{
    if (!m_portTable || !isEditable())
        return;

    // Новый input сразу попадает в contract table и затем может быть протолкнут в source.
    const int row = m_portTable->rowCount();
    m_portTable->insertRow(row);
    m_portTable->setItem(row, 0, new QTableWidgetItem(QStringLiteral("arg_%1").arg(row + 1)));
    m_portTable->setItem(row, 1, new QTableWidgetItem("int"));
    m_portTable->setItem(row, 2, new QTableWidgetItem(tr("Input")));
    m_portTable->setItem(row, 3, new QTableWidgetItem(QString()));
    m_portTable->setCurrentCell(row, 0);
    refreshVerificationWorkspaceFromCurrentContract();
    onFieldEdited();
}

// Добавляет единственный допустимый data-output для atomic source-backed модуля.
void ModuleEditorWidget::onAddOutputPort()
{
    if (!m_portTable || !isEditable())
        return;

    // Пока поддерживаем только один return-based data-output для atomic source-backed module.
    for (int row = 0; row < m_portTable->rowCount(); ++row) {
        const auto *typeItem = m_portTable->item(row, 1);
        const auto *directionItem = m_portTable->item(row, 2);
        const bool isOutput = directionItem
            && directionItem->text().trimmed().compare(tr("Output"), Qt::CaseInsensitive) == 0;
        const bool isExec = typeItem
            && typeItem->text().trimmed().compare(QStringLiteral("exec"), Qt::CaseInsensitive) == 0;
        if (isOutput && !isExec) {
            QMessageBox::information(this,
                                     tr("Single-output limit"),
                                     tr("Atomic source-backed modules currently support only one data output."));
            return;
        }
    }

    const int row = m_portTable->rowCount();
    m_portTable->insertRow(row);
    m_portTable->setItem(row, 0, new QTableWidgetItem("result"));
    m_portTable->setItem(row, 1, new QTableWidgetItem("int"));
    m_portTable->setItem(row, 2, new QTableWidgetItem(tr("Output")));
    m_portTable->setItem(row, 3, new QTableWidgetItem(QString()));
    m_portTable->setCurrentCell(row, 0);
    refreshVerificationWorkspaceFromCurrentContract();
    onFieldEdited();
}

// Удаляет выбранный порт из contract table и помечает модуль как изменённый.
void ModuleEditorWidget::onRemoveSelectedPort()
{
    if (!m_portTable || !isEditable())
        return;

    const int row = m_portTable->currentRow();
    if (row < 0)
        return;

    m_portTable->removeRow(row);
    refreshVerificationWorkspaceFromCurrentContract();
    onFieldEdited();
}

// Считывает контракт из исходника и заменяет им текущую contract table.
void ModuleEditorWidget::onSyncContractFromSource()
{
    if (!isEditable())
        return;

    // Явно перетягивает contract из текущего source, если пользователь правит сигнатуру вручную.
    Module sourceDerived = stagedModuleFromForm();
    if (!parseModuleSignatureIntoContract(&sourceDerived, sourceDerived.sourceCode)) {
        QMessageBox::warning(this,
                             tr("Cannot sync contract"),
                             tr("Current source does not expose a parseable `dq_*` signature."));
        return;
    }

    loadContractTableFromModule(sourceDerived);
    refreshVerificationWorkspaceFromCurrentContract();
    onFieldEdited();
}

// Переписывает сигнатуру исходника так, чтобы она совпала с editable contract table.
void ModuleEditorWidget::onApplyContractToSource()
{
    if (!isEditable())
        return;

    // Обратное направление: пользователь редактирует contract table, а мы переписываем сигнатуру.
    bool contractValid = false;
    QString errorMessage;
    Module staged = stagedModuleFromForm(&contractValid, &errorMessage);
    if (!contractValid) {
        QMessageBox::warning(this,
                             tr("Cannot apply contract"),
                             errorMessage);
        return;
    }

    QString rewrittenSource = staged.sourceCode;
    if (!rewriteModuleSignatureFromContract(&rewrittenSource, staged, &errorMessage)) {
        QMessageBox::warning(this,
                             tr("Cannot apply contract"),
                             errorMessage);
        return;
    }

    m_codeEdit->setPlainText(rewrittenSource);
    refreshVerificationWorkspaceFromCurrentContract();
    refreshContractPreview();
}

// Относит правки таблицы контракта к тем же modified/verification правилам, что и source editor.
void ModuleEditorWidget::onContractTableItemChanged(QTableWidgetItem *item)
{
    Q_UNUSED(item)
    // Таблица контракта участвует в той же modified/verification модели, что и source editor.
    refreshVerificationWorkspaceFromCurrentContract();
    onFieldEdited();
}

// Переключает active verification scenario и подгружает его значения в workspace.
void ModuleEditorWidget::onVerificationScenarioChanged(int index)
{
    if (m_loadingVerification)
        return;

    syncVerificationScenarioFromUi();
    m_currentVerificationScenarioIndex = index;

    m_loadingVerification = true;
    if (index >= 0 && index < m_module.verificationScenarios.size()) {
        m_verificationScenarioNameEdit->setText(m_module.verificationScenarios.at(index).name);
        populateVerificationTablesFromCurrentScenario(m_module);
    } else {
        m_verificationScenarioNameEdit->clear();
        m_verificationInputsTable->setRowCount(0);
        m_verificationExpectedTable->setRowCount(0);
    }
    loadTraceViewerFromModule(m_module);
    m_loadingVerification = false;
}

// Сохраняет новое имя текущего verification scenario и помечает модуль изменённым.
void ModuleEditorWidget::onVerificationScenarioNameEdited(const QString &text)
{
    if (m_loadingVerification)
        return;
    if (m_currentVerificationScenarioIndex < 0
        || m_currentVerificationScenarioIndex >= m_module.verificationScenarios.size()) {
        return;
    }

    m_module.verificationScenarios[m_currentVerificationScenarioIndex].name = text.trimmed();
    normalizeVerificationScenarios(&m_module);
    normalizeVerificationTraceArtifacts(&m_module);
    populateVerificationScenarioSelector();
    const QSignalBlocker blocker(m_verificationScenarioCombo);
    m_verificationScenarioCombo->setCurrentIndex(m_currentVerificationScenarioIndex);
    onFieldEdited();
}

// Добавляет новый verification scenario с default inputs по текущему контракту.
void ModuleEditorWidget::onAddVerificationScenario()
{
    if (!isEditable())
        return;

    syncVerificationScenarioFromUi();
    Module staged = stagedModuleFromForm();
    ModuleVerificationScenario scenario = makeDefaultVerificationScenario(
        staged,
        m_module.verificationScenarios.size());
    m_module.verificationScenarios.append(scenario);
    normalizeVerificationScenarios(&m_module);
    normalizeVerificationTraceArtifacts(&m_module);
    m_currentVerificationScenarioIndex = m_module.verificationScenarios.size() - 1;
    loadVerificationWorkspaceFromModule(m_module);
    onFieldEdited();
}

// Удаляет выбранный verification scenario и оставляет workspace в консистентном состоянии.
void ModuleEditorWidget::onRemoveVerificationScenario()
{
    if (!isEditable())
        return;
    if (m_currentVerificationScenarioIndex < 0
        || m_currentVerificationScenarioIndex >= m_module.verificationScenarios.size()) {
        return;
    }

    m_module.verificationScenarios.removeAt(m_currentVerificationScenarioIndex);
    normalizeVerificationScenarios(&m_module);
    normalizeVerificationTraceArtifacts(&m_module);
    if (m_currentVerificationScenarioIndex >= m_module.verificationScenarios.size())
        m_currentVerificationScenarioIndex = m_module.verificationScenarios.size() - 1;
    loadVerificationWorkspaceFromModule(m_module);
    onFieldEdited();
}

// Относит правки verification inputs/expected tables к текущему saved scenario.
void ModuleEditorWidget::onVerificationTableItemChanged(QTableWidgetItem *item)
{
    Q_UNUSED(item)
    if (m_loadingVerification)
        return;
    syncVerificationScenarioFromUi();
    onFieldEdited();
}

// Переключает детали trace viewer на новый выбранный шаг.
void ModuleEditorWidget::onTraceEventSelectionChanged()
{
    updateTraceViewerDetails(m_module);
}

// Возвращает trace playback к первому шагу текущего artifact-а.
void ModuleEditorWidget::onTraceRestart()
{
    selectTraceEventRow(0);
}

// Переходит к следующему шагу в linear A4.2 stepping sequence.
void ModuleEditorWidget::onTraceStep()
{
    const int currentRow = currentTraceEventIndex();
    if (currentRow < 0)
        return;
    selectTraceEventRow(currentRow + 1);
}

// В current v1 trace model `step over` идёт по той же linear sequence, что и `step`.
void ModuleEditorWidget::onTraceStepOver()
{
    onTraceStep();
}

// Переводит playback сразу на последний доступный trace event.
void ModuleEditorWidget::onTraceRunToEnd()
{
    if (!m_traceEventsTable || m_traceEventsTable->rowCount() <= 0)
        return;
    selectTraceEventRow(m_traceEventsTable->rowCount() - 1);
}

// Выполняет compile-only check для текущего staged module прямо из editor workspace.
void ModuleEditorWidget::onCompileVerification()
{
    bool contractValid = false;
    QString errorMessage;
    syncVerificationScenarioFromUi();
    Module staged = stagedModuleFromForm(&contractValid, &errorMessage);
    if (!contractValid) {
        QMessageBox::warning(this, tr("Cannot compile module"), errorMessage);
        return;
    }

    normalizeVerificationScenarios(&staged);
    m_verificationLogEdit->clear();
    appendVerificationLogLine(tr("Compile check started for '%1'").arg(staged.name));

    const TestResult result = m_testRunner->compile(staged);
    m_module = staged;
    m_module.compileStatus = result.compiled ? "passed" : "failed";
    if (!result.compiled && m_module.testStatus == "passed")
        m_module.testStatus = "modified";

    appendVerificationLogLine(result.compiled ? tr("Compile check passed") : tr("Compile check failed"));
    if (!result.compilerOutput.trimmed().isEmpty())
        appendVerificationLogLine(result.compilerOutput.trimmed());
    updateVerificationResultViews(m_module, nullptr, nullptr);
    m_verificationRunStatusLabel->setText(
        result.compiled ? tr("Compile check passed") : tr("Compile check failed"));
    m_verificationRunStatusLabel->setStyleSheet(
        result.compiled ? "color: #2e7d32; font-weight: 600;" : "color: #b71c1c; font-weight: 600;");
    refreshSummary();
}

// Выполняет compile+run verify path по выбранному saved scenario и сравнивает expected vs actual.
void ModuleEditorWidget::onRunVerificationScenario()
{
    if (m_currentVerificationScenarioIndex < 0
        || m_currentVerificationScenarioIndex >= m_module.verificationScenarios.size()) {
        QMessageBox::information(this,
                                 tr("No scenario selected"),
                                 tr("Add a verification scenario before running module verification."));
        return;
    }

    bool contractValid = false;
    QString errorMessage;
    syncVerificationScenarioFromUi();
    Module staged = stagedModuleFromForm(&contractValid, &errorMessage);
    if (!contractValid) {
        QMessageBox::warning(this, tr("Cannot verify module"), errorMessage);
        return;
    }

    normalizeVerificationScenarios(&staged);
    const ModuleVerificationScenario scenario = staged.verificationScenarios.at(m_currentVerificationScenarioIndex);

    m_verificationLogEdit->clear();
    appendVerificationLogLine(tr("Verification started for scenario '%1'").arg(scenario.displayName()));

    const TestResult result = m_testRunner->runTest(
        staged,
        scenario.inputValues,
        scenario.expectedOutputValues);

    m_module = staged;
    m_module.lastVerifiedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    m_module.compileStatus = result.compiled ? "passed" : "failed";
    if (!result.compiled) {
        m_module.testStatus = "modified";
    } else if (result.passed) {
        m_module.testStatus = "passed";
    } else if (result.ran) {
        m_module.testStatus = "failed";
    }

    refreshTrustStateAfterVerification(result.compiled && result.ran && result.passed);
    const ModuleTraceArtifact traceArtifact = buildVerificationTraceArtifact(m_module, scenario, result);
    storeVerificationTraceArtifact(&m_module, traceArtifact);

    if (!result.compilerOutput.trimmed().isEmpty())
        appendVerificationLogLine(result.compilerOutput.trimmed());
    if (!result.runOutput.trimmed().isEmpty())
        appendVerificationLogLine(result.runOutput.trimmed());
    for (const QString &error : result.errors)
        appendVerificationLogLine(error);
    if (traceArtifact.status == "captured") {
        appendVerificationLogLine(
            tr("Trace captured: %1 event(s) saved for scenario '%2'.")
                .arg(traceArtifact.events.size())
                .arg(scenario.displayName()));
    } else {
        appendVerificationLogLine(tr("Trace unavailable: %1").arg(traceArtifact.unavailableReason));
    }

    const ModuleVerificationScenario *scenarioPtr =
        (m_currentVerificationScenarioIndex >= 0
         && m_currentVerificationScenarioIndex < m_module.verificationScenarios.size())
        ? &m_module.verificationScenarios.at(m_currentVerificationScenarioIndex)
        : nullptr;
    updateVerificationResultViews(m_module, scenarioPtr, &result);
    loadTraceViewerFromModule(m_module);
    refreshSummary();
}

// Преобразует локальный hover исходника в запрос к внешнему LSP bridge.
void ModuleEditorWidget::onCodeHoverRequested(int line, int character)
{
    const QString documentPath = lspDocumentPath();
    if (documentPath.isEmpty())
        return;
    emit hoverRequested(documentPath, line, character);
}

// Преобразует локальный completion trigger исходника в запрос к внешнему LSP bridge.
void ModuleEditorWidget::onCodeCompletionRequested(int line, int character)
{
    const QString documentPath = lspDocumentPath();
    if (documentPath.isEmpty())
        return;
    emit completionRequested(documentPath, line, character);
}

} // namespace DeltaQ
