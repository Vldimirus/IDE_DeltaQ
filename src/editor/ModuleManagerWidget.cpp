// Менеджер модулей — реализация
#include "ModuleManagerWidget.h"
#include "ModuleTestRunner.h"
#include "../core/ModuleRegistry.h"
#include "../core/StandardLibrary.h"
#include <deltaq/Module.h>

#include "SyntaxHighlighter.h"
#include "../blockEditor/NodeItem.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QSignalBlocker>
#include <QTabWidget>
#include <functional>

namespace DeltaQ {

namespace {

struct ModuleStateSummary {
    QString badgeText;
    QString badgeStyle;
    QString detailText;
    QString iconStatus;
};

struct ModuleEcosystemSummary {
    QString layerText;
    QString roleText;
    QString qualityText;
};

// Человекочитаемое название роли imported-модуля внутри v1 curation flow.
QString importedCurationRoleTitle(const Module &module)
{
    const QString role = module.importedCurationRole();
    if (role == "curated_entry")
        return QObject::tr("curated");
    if (role == "adapter")
        return QObject::tr("adapter");
    if (role == "hidden")
        return QObject::tr("hidden");
    return QObject::tr("raw");
}

// Возвращает видимый label модуля с учётом display name и curation-role imported pack-а.
QString moduleDisplayLabel(const Module &module, bool forPalette)
{
    QString label = module.name;

    if (module.isUIContractModule()) {
        // UI contract хранит человекочитаемое имя отдельно от внутреннего module.name.
        label = module.metadataString("deltaq.ui.display_name", module.name);
        const QString contractType = module.uiWidgetType();
        if (!contractType.isEmpty())
            label += QString(" [%1]").arg(contractType);
    } else if (module.isImportedPackModule()) {
        label = module.importedDisplayName();
        const QString role = module.importedCurationRole();
        if (role == "raw_wrapper")
            label += QObject::tr(" [raw]");
        else if (role == "adapter")
            label += QObject::tr(" [adapter]");
        else if (!forPalette && role == "hidden")
            label += QObject::tr(" [hidden]");
    } else if (module.origin == "core" || module.id.startsWith("core.")) {
        // В менеджере модулей legacy core-модули тоже помечаются явно,
        // иначе audit standard library не читается пользователю.
        const StandardLibraryCurationInfo curation = StandardLibrary::curationForModule(module);
        if (curation.tier == "legacy")
            label += QObject::tr(" [legacy]");
    }

    return label;
}

// Преобразует статус compile check в понятный русскоязычный текст для UI.
QString moduleCompileStatusText(const QString &status)
{
    if (status == "passed")
        return QObject::tr("пройдена");
    if (status == "failed")
        return QObject::tr("ошибка");
    if (status == "modified")
        return QObject::tr("изменён после компиляции");
    return QObject::tr("не запускалась");
}

// Преобразует статус test check в понятный русскоязычный текст для UI.
QString moduleTestStatusText(const QString &status)
{
    if (status == "passed")
        return QObject::tr("пройден");
    if (status == "failed")
        return QObject::tr("не пройден");
    if (status == "modified")
        return QObject::tr("изменён после проверки");
    return QObject::tr("не запускался");
}

// Добавляет к tooltip/панели единый documentation block: summary, use-case и ограничения.
void appendDocumentationTooltip(QString &tooltip, const Module &module)
{
    if (!module.hasDocumentationDetails())
        return;

    const QString summary = module.documentationSummary();
    const QString whenToUse = module.documentationWhenToUse();
    const QString limitations = module.documentationLimitations();

    if (!summary.isEmpty())
        tooltip += QObject::tr("\nНазначение: %1").arg(summary);
    if (!whenToUse.isEmpty())
        tooltip += QObject::tr("\nКогда использовать: %1").arg(whenToUse);
    if (!limitations.isEmpty())
        tooltip += QObject::tr("\nОграничения: %1").arg(limitations);
}

// Собирает сводку о готовности модуля: валидность, реализация, compile/test и режим редактирования.
ModuleStateSummary buildModuleStateSummary(const ModuleRegistry *registry, const Module &module)
{
    const bool isReadOnly = (module.origin == "ui" || module.origin == "core");
    const bool contractValid = registry ? registry->validateModule(module) : module.isValid();
    const bool hasImplementation = registry
        ? registry->moduleHasImplementation(module)
        : (!module.sourceCode.trimmed().isEmpty()
           || !module.graphId.trimmed().isEmpty()
           || !module.sourcePath.trimmed().isEmpty());
    QString admissionReason;
    const bool admitted = registry
        ? registry->isModuleAdmittedForComposition(module, &admissionReason)
        : (contractValid && hasImplementation);
    const QString moduleKind = module.isComposite()
        ? QObject::tr("составной")
        : QObject::tr("атомарный");
    const QString implementationText = module.isComposite()
        ? (hasImplementation ? QObject::tr("внутренний граф") : QObject::tr("отсутствует"))
        : (hasImplementation ? QObject::tr("есть") : QObject::tr("отсутствует"));
    const QString admissionText = admitted ? QObject::tr("есть") : QObject::tr("нет");

    ModuleStateSummary summary;
    if (!contractValid || !hasImplementation || (module.isComposite() && !admitted)) {
        summary.badgeText = QObject::tr("Неготов");
        summary.iconStatus = "failed";
        summary.badgeStyle =
            "QLabel { background:#5a1f1f; color:#ffd9d9; border-radius:10px; "
            "padding:3px 10px; font-weight:bold; }";
    } else if (isReadOnly) {
        summary.badgeText = QObject::tr("Библиотечный");
        summary.iconStatus = "passed";
        summary.badgeStyle =
            "QLabel { background:#1f3f5a; color:#d9efff; border-radius:10px; "
            "padding:3px 10px; font-weight:bold; }";
    } else if (module.isComposite()) {
        summary.badgeText = QObject::tr("Составной");
        summary.iconStatus = "passed";
        summary.badgeStyle =
            "QLabel { background:#1f3f5a; color:#d9efff; border-radius:10px; "
            "padding:3px 10px; font-weight:bold; }";
    } else if (module.compileStatus == "failed") {
        summary.badgeText = QObject::tr("Ошибка компиляции");
        summary.iconStatus = "failed";
        summary.badgeStyle =
            "QLabel { background:#5a1f1f; color:#ffd9d9; border-radius:10px; "
            "padding:3px 10px; font-weight:bold; }";
    } else if (module.testStatus == "failed") {
        summary.badgeText = QObject::tr("Ошибка проверки");
        summary.iconStatus = "failed";
        summary.badgeStyle =
            "QLabel { background:#5a1f1f; color:#ffd9d9; border-radius:10px; "
            "padding:3px 10px; font-weight:bold; }";
    } else if (module.compileStatus == "passed" && module.testStatus == "passed") {
        summary.badgeText = QObject::tr("Проверен");
        summary.iconStatus = "passed";
        summary.badgeStyle =
            "QLabel { background:#214a2f; color:#d9ffe3; border-radius:10px; "
            "padding:3px 10px; font-weight:bold; }";
    } else if (module.compileStatus == "modified" || module.testStatus == "modified") {
        summary.badgeText = QObject::tr("Изменён");
        summary.iconStatus = "modified";
        summary.badgeStyle =
            "QLabel { background:#5d3d11; color:#ffe7c2; border-radius:10px; "
            "padding:3px 10px; font-weight:bold; }";
    } else if (module.compileStatus != "passed") {
        summary.badgeText = QObject::tr("Требует компиляции");
        summary.iconStatus = "untested";
        summary.badgeStyle =
            "QLabel { background:#5d3d11; color:#ffe7c2; border-radius:10px; "
            "padding:3px 10px; font-weight:bold; }";
    } else {
        summary.badgeText = QObject::tr("Требует проверки");
        summary.iconStatus = "untested";
        summary.badgeStyle =
            "QLabel { background:#5d3d11; color:#ffe7c2; border-radius:10px; "
            "padding:3px 10px; font-weight:bold; }";
    }

    if (module.isComposite()) {
        summary.detailText = QObject::tr(
            "Тип: %1 | Контракт: %2 | Реализация: %3 | Допуск: %4 | Режим: %5")
            .arg(moduleKind)
            .arg(contractValid ? QObject::tr("корректен") : QObject::tr("ошибка"))
            .arg(implementationText)
            .arg(admissionText)
            .arg(isReadOnly ? QObject::tr("только чтение") : QObject::tr("редактируемый"));
    } else if (isReadOnly) {
        summary.detailText = QObject::tr(
            "Тип: %1 | Контракт: %2 | Реализация: %3 | Допуск: %4 | Режим: %5")
            .arg(moduleKind)
            .arg(contractValid ? QObject::tr("корректен") : QObject::tr("ошибка"))
            .arg(implementationText)
            .arg(admissionText)
            .arg(QObject::tr("только чтение"));
    } else {
        summary.detailText = QObject::tr(
            "Тип: %1 | Контракт: %2 | Реализация: %3 | Компиляция: %4 | Тест: %5 | Допуск: %6 | Режим: %7")
            .arg(moduleKind)
            .arg(contractValid ? QObject::tr("корректен") : QObject::tr("ошибка"))
            .arg(implementationText)
            .arg(moduleCompileStatusText(module.compileStatus))
            .arg(moduleTestStatusText(module.testStatus))
            .arg(admissionText)
            .arg(isReadOnly ? QObject::tr("только чтение") : QObject::tr("редактируемый"));
    }

    if (module.isImportedPackModule()) {
        summary.detailText += QObject::tr(" | Pack: %1 | Роль: %2 | Символ: %3")
            .arg(module.importedPackName(),
                 importedCurationRoleTitle(module),
                 module.importedOriginalSymbol());
    }

    if (!admitted && !admissionReason.isEmpty())
        summary.detailText += QObject::tr(" | Причина: %1").arg(admissionReason);
    return summary;
}

// Явная сводка по слою экосистемы и quality bar, чтобы пользователю не приходилось
// вычитывать origin/capability только из tooltip и стратегии.
ModuleEcosystemSummary buildModuleEcosystemSummary(const ModuleRegistry *registry,
                                                   const Module &module)
{
    const bool contractValid = registry ? registry->validateModule(module) : module.isValid();
    const bool hasImplementation = registry
        ? registry->moduleHasImplementation(module)
        : (!module.sourceCode.trimmed().isEmpty()
           || !module.graphId.trimmed().isEmpty()
           || !module.sourcePath.trimmed().isEmpty());
    QString admissionReason;
    const bool admitted = registry
        ? registry->isModuleAdmittedForComposition(module, &admissionReason)
        : (contractValid && hasImplementation);

    ModuleEcosystemSummary summary;

    if (module.origin == "core" || module.id.startsWith("core.")) {
        const StandardLibraryCurationInfo curation = StandardLibrary::curationForModule(module);
        summary.layerText = QObject::tr("Стандартная библиотека");
        summary.roleText = curation.isKnown()
            ? QObject::tr("%1 (%2)").arg(curation.title, curation.tier)
            : QObject::tr("core без explicit curation review");
        summary.qualityText = curation.isKnown()
            ? QObject::tr("Quality bar: explicit curation review для checked-in core.")
            : QObject::tr("Quality bar: checked-in core ещё не прошёл explicit curation review.");
        return summary;
    }

    if (module.origin == "ui") {
        summary.layerText = QObject::tr("UI contract");
        summary.roleText = QObject::tr("backend-agnostic contract");
        summary.qualityText = QObject::tr(
            "Quality bar: read-only UI contract layer, runtime приходит из backend codegen.");
        return summary;
    }

    if (module.isImportedPackModule()) {
        summary.layerText = QObject::tr("Imported pack");
        summary.roleText = QObject::tr("%1 | pack: %2")
            .arg(importedCurationRoleTitle(module), module.importedPackName());
        summary.qualityText = QObject::tr("Symbol: %1 | compile: %2 | test: %3")
            .arg(module.importedOriginalSymbol(),
                 moduleCompileStatusText(module.compileStatus),
                 moduleTestStatusText(module.testStatus));
        return summary;
    }

    if (module.origin == "graph") {
        summary.layerText = QObject::tr("Проектный модуль");
        summary.roleText = QObject::tr("составной reusable module");
        summary.qualityText = QObject::tr("Quality bar: inner graph admission внутри текущего проекта.");
        return summary;
    }

    if (module.origin == "local") {
        summary.layerText = QObject::tr("Проектный модуль");
        summary.roleText = QObject::tr("локальный атомарный модуль");
        summary.qualityText = QObject::tr("Quality bar: staged verification внутри текущего проекта.");
        return summary;
    }

    summary.layerText = QObject::tr("Расширение");
    summary.roleText = QObject::tr("внешний модуль");
    summary.qualityText = QObject::tr("Контракт: %1 | Реализация: %2 | Допуск: %3")
        .arg(contractValid ? QObject::tr("корректен") : QObject::tr("ошибка"))
        .arg(hasImplementation ? QObject::tr("есть") : QObject::tr("отсутствует"))
        .arg(admitted ? QObject::tr("есть") : QObject::tr("нет"));
    return summary;
}

// Сводка imported pack-а в дереве менеджера модулей.
QString importedPackTreeTooltip(const QString &packName, const QVector<const Module *> &modules)
{
    int rawCount = 0;
    int curatedCount = 0;
    int adapterCount = 0;
    int hiddenCount = 0;

    for (const auto *module : modules) {
        const QString role = module->importedCurationRole();
        if (role == "raw_wrapper")
            ++rawCount;
        else if (role == "adapter")
            ++adapterCount;
        else if (role == "hidden")
            ++hiddenCount;
        else
            ++curatedCount;
    }

    return QObject::tr("Imported pack: %1\nCurated: %2 | Raw: %3 | Adapter: %4 | Hidden: %5")
        .arg(packName)
        .arg(curatedCount)
        .arg(rawCount)
        .arg(adapterCount)
        .arg(hiddenCount);
}

// Рекурсивный фильтр дерева поддерживает arbitrary depth, включая imported pack groups.
bool filterTreeItemRecursive(QTreeWidgetItem *item, const QString &text, bool ancestorMatched = false)
{
    if (!item)
        return false;

    const bool selfMatch = ancestorMatched
        || text.isEmpty()
        || item->text(0).contains(text, Qt::CaseInsensitive);

    if (item->childCount() == 0) {
        item->setHidden(!selfMatch);
        return selfMatch;
    }

    bool childVisible = false;
    for (int i = 0; i < item->childCount(); ++i) {
        if (filterTreeItemRecursive(item->child(i), text, selfMatch))
            childVisible = true;
    }

    const bool visible = selfMatch || childVisible;
    item->setHidden(!visible);
    return visible;
}

} // namespace

ModuleManagerWidget::ModuleManagerWidget(ModuleRegistry *registry, QWidget *parent)
    : QWidget(parent)
    , m_registry(registry)
{
    m_testRunner = new ModuleTestRunner(this);
    setupUI();
    buildTree();

    // Сигналы тестирования
    connect(m_testRunner, &ModuleTestRunner::compilationFinished, this,
            [this](bool success, const QString &output) {
        m_logView->append(success ? tr("Компиляция успешна") : tr("Ошибка компиляции"));
        if (!output.isEmpty())
            m_logView->append(output);

        if (!m_currentModuleId.isEmpty()) {
            Module *mod = m_registry->findModule(m_currentModuleId);
            if (mod) {
                mod->compileStatus = success ? "passed" : "failed";
                if (success && mod->testStatus == "failed")
                    mod->testStatus = "modified";
                updateVerificationPanel(*mod);
                updateModuleStateSummary(*mod);
                updateModuleEcosystemSummary(*mod);
                updateModuleTreeItemState(m_tree->currentItem(), *mod);
            }
        }
    });

    connect(m_testRunner, &ModuleTestRunner::testFinished, this,
            [this](const TestResult &result) {
        if (!m_currentModuleId.isEmpty()) {
            Module *mod = m_registry->findModule(m_currentModuleId);
            if (mod) {
                if (result.compiled)
                    mod->compileStatus = "passed";

                if (!result.compiled) {
                    m_logView->append(tr("Тест не выполнен: compile check не пройден"));
                } else if (result.passed) {
                    mod->testStatus = "passed";
                } else if (result.ran) {
                    mod->testStatus = "failed";
                }

                updateVerificationPanel(*mod);
                updateModuleTreeItemState(m_tree->currentItem(), *mod);
                updateModuleStateSummary(*mod);
                updateModuleEcosystemSummary(*mod);
            }
        }

        if (result.passed) {
            m_logView->append(tr("Тест пройден"));
            m_testOutputLabel->setText(tr("Результат: PASSED"));
            m_testOutputLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
        } else {
            m_logView->append(tr("Тест не пройден"));
            if (result.compiled && result.ran)
                m_testOutputLabel->setText(tr("Результат: FAILED"));
            else
                m_testOutputLabel->setText(tr("Результат: НЕ ВЫПОЛНЕН"));
            m_testOutputLabel->setStyleSheet("color: #F44336; font-weight: bold;");
        }

        // Показываем выходные значения
        for (auto it = result.outputValues.begin(); it != result.outputValues.end(); ++it) {
            m_logView->append(QString("  %1 = %2").arg(it.key(), it.value()));
        }

        if (!result.runOutput.isEmpty())
            m_logView->append(result.runOutput);
        for (const auto &err : result.errors)
            m_logView->append(tr("Ошибка: ") + err);

        if (!m_currentModuleId.isEmpty()) {
            const Module *mod = m_registry->findModule(m_currentModuleId);
            if (mod)
                updateVerificationPanel(*mod);
        }
    });
}

void ModuleManagerWidget::setupUI()
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // === Левая панель: библиотека модулей ===
    auto *leftPanel = new QWidget(this);
    leftPanel->setMaximumWidth(260);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(4, 4, 4, 4);

    // Поиск
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Поиск модулей..."));
    leftLayout->addWidget(m_searchEdit);

    // Фильтр языка
    auto *filterLayout = new QHBoxLayout;
    filterLayout->addWidget(new QLabel(tr("Язык:"), this));
    m_langFilter = new QComboBox(this);
    m_langFilter->addItems({"Все", "C", "C++"});
    filterLayout->addWidget(m_langFilter);
    leftLayout->addLayout(filterLayout);

    m_showSpecializedCheck = new QCheckBox(tr("Показывать specialized"), this);
    m_showSpecializedCheck->setObjectName("moduleManagerShowSpecializedCheck");
    m_showSpecializedCheck->setChecked(false);
    leftLayout->addWidget(m_showSpecializedCheck);

    m_showLegacyCheck = new QCheckBox(tr("Показывать legacy"), this);
    m_showLegacyCheck->setObjectName("moduleManagerShowLegacyCheck");
    m_showLegacyCheck->setChecked(false);
    leftLayout->addWidget(m_showLegacyCheck);

    // Дерево модулей
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    leftLayout->addWidget(m_tree);

    // Кнопки
    auto *btnLayout = new QHBoxLayout;
    m_newBtn = new QPushButton(tr("Новый"), this);
    m_newPackBtn = new QPushButton(tr("Пакет"), this);
    m_deleteBtn = new QPushButton(tr("Удалить"), this);
    m_deleteBtn->setEnabled(false);
    btnLayout->addWidget(m_newBtn);
    btnLayout->addWidget(m_newPackBtn);
    btnLayout->addWidget(m_deleteBtn);
    leftLayout->addLayout(btnLayout);

    // === Правая панель: редактор модуля ===
    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 4, 4, 4);
    rightLayout->setSpacing(4);

    // --- Верхняя метаполоса (компактная горизонтальная) ---
    auto *metaBar = new QHBoxLayout;
    metaBar->setSpacing(6);

    // 1. Свойства (QGridLayout 2x4)
    auto *propsGroup = new QGroupBox(tr("Свойства"), this);
    auto *propsGrid = new QGridLayout(propsGroup);
    propsGrid->setContentsMargins(4, 4, 4, 4);
    propsGrid->setSpacing(2);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("имя_модуля"));
    propsGrid->addWidget(new QLabel(tr("Имя:"), this), 0, 0);
    propsGrid->addWidget(m_nameEdit, 0, 1);

    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->setEditable(true);
    m_categoryCombo->addItems({"math", "logic", "io", "string", "ui", "custom"});
    propsGrid->addWidget(new QLabel(tr("Кат:"), this), 0, 2);
    propsGrid->addWidget(m_categoryCombo, 0, 3);

    m_descEdit = new QLineEdit(this);
    m_descEdit->setPlaceholderText(tr("Описание модуля"));
    propsGrid->addWidget(new QLabel(tr("Опис:"), this), 1, 0);
    propsGrid->addWidget(m_descEdit, 1, 1);

    m_langCombo = new QComboBox(this);
    m_langCombo->addItems({"c", "cpp"});
    propsGrid->addWidget(new QLabel(tr("Яз:"), this), 1, 2);
    propsGrid->addWidget(m_langCombo, 1, 3);

    metaBar->addWidget(propsGroup, 2);

    // 2. Imported pack curation (минимальный v1 flow для raw/curated/adapter/hidden).
    auto *importGroup = new QGroupBox(tr("Import/Curation"), this);
    auto *importLayout = new QGridLayout(importGroup);
    importLayout->setContentsMargins(4, 4, 4, 4);
    importLayout->setSpacing(2);

    m_importDisplayNameEdit = new QLineEdit(this);
    m_importDisplayNameEdit->setObjectName("importDisplayNameEdit");
    m_importDisplayNameEdit->setPlaceholderText(tr("Display name"));
    importLayout->addWidget(new QLabel(tr("Label:"), this), 0, 0);
    importLayout->addWidget(m_importDisplayNameEdit, 0, 1);

    m_importRoleCombo = new QComboBox(this);
    m_importRoleCombo->setObjectName("importRoleCombo");
    m_importRoleCombo->addItem(tr("Raw wrapper"), "raw_wrapper");
    m_importRoleCombo->addItem(tr("Curated entry"), "curated_entry");
    m_importRoleCombo->addItem(tr("Adapter"), "adapter");
    m_importRoleCombo->addItem(tr("Hidden"), "hidden");
    importLayout->addWidget(new QLabel(tr("Role:"), this), 1, 0);
    importLayout->addWidget(m_importRoleCombo, 1, 1);

    m_importSourceLabel = new QLabel(tr("Не imported module"), this);
    m_importSourceLabel->setObjectName("importSourceLabel");
    m_importSourceLabel->setWordWrap(true);
    m_importSourceLabel->setStyleSheet("QLabel { color:#bdbdbd; }");
    importLayout->addWidget(m_importSourceLabel, 2, 0, 1, 2);

    metaBar->addWidget(importGroup, 1);

    // 3. Зависимости (#include)
    auto *includesGroup = new QGroupBox(tr("Зависимости"), this);
    auto *includesLayout = new QVBoxLayout(includesGroup);
    includesLayout->setContentsMargins(4, 4, 4, 4);
    m_includesEdit = new QTextEdit(this);
    m_includesEdit->setMaximumHeight(80);
    m_includesEdit->setPlaceholderText(tr("stdlib.h\nmath.h"));
    m_includesEdit->setFont(QFont("Monospace", 9));
    includesLayout->addWidget(m_includesEdit);
    metaBar->addWidget(includesGroup, 1);

    // 4. Порты (компактная таблица)
    auto *portGroup = new QGroupBox(tr("Порты"), this);
    auto *portLayout = new QVBoxLayout(portGroup);
    portLayout->setContentsMargins(4, 4, 4, 4);
    m_portTable = new QTableWidget(0, 3, this);
    m_portTable->setHorizontalHeaderLabels({tr("Имя"), tr("Тип"), tr("Напр.")});
    m_portTable->horizontalHeader()->setStretchLastSection(true);
    m_portTable->horizontalHeader()->setDefaultSectionSize(55);
    m_portTable->setMaximumHeight(80);
    m_portTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_portTable->verticalHeader()->setDefaultSectionSize(18);
    m_portTable->verticalHeader()->hide();
    m_portTable->setFont(QFont("Monospace", 8));
    portLayout->addWidget(m_portTable);
    metaBar->addWidget(portGroup, 1);

    // 5. Превью блока на графе
    auto *previewFrame = new QFrame(this);
    previewFrame->setFixedWidth(200);
    previewFrame->setFrameShape(QFrame::StyledPanel);
    previewFrame->setStyleSheet("QFrame { background: #1e1e1e; border-radius: 4px; }");
    auto *previewLayout = new QVBoxLayout(previewFrame);
    previewLayout->setContentsMargins(2, 2, 2, 2);

    m_previewScene = new QGraphicsScene(this);
    m_previewView = new QGraphicsView(m_previewScene, this);
    m_previewView->setRenderHint(QPainter::Antialiasing);
    m_previewView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_previewView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_previewView->setStyleSheet("QGraphicsView { background: #1e1e1e; border: none; }");
    m_previewView->setInteractive(false);
    previewLayout->addWidget(m_previewView);

    metaBar->addWidget(previewFrame, 0);

    rightLayout->addLayout(metaBar);

    // Явная сводка состояния модуля: годность, тестовый статус и режим редактирования.
    auto *stateBar = new QHBoxLayout;
    stateBar->setSpacing(8);

    m_moduleStateBadge = new QLabel(tr("Модуль не выбран"), this);
    m_moduleStateBadge->setObjectName("moduleStateBadge");
    m_moduleStateBadge->setStyleSheet(
        "QLabel { background:#4a4a4a; color:#f0f0f0; border-radius:10px; "
        "padding:3px 10px; font-weight:bold; }");
    stateBar->addWidget(m_moduleStateBadge, 0, Qt::AlignLeft);

    m_moduleStateDetails = new QLabel(tr("Выберите модуль, чтобы увидеть его состояние."), this);
    m_moduleStateDetails->setObjectName("moduleStateDetails");
    m_moduleStateDetails->setWordWrap(true);
    m_moduleStateDetails->setStyleSheet("QLabel { color:#bdbdbd; }");
    stateBar->addWidget(m_moduleStateDetails, 1);

    rightLayout->addLayout(stateBar);

    auto *ecosystemGroup = new QGroupBox(tr("Экосистема"), this);
    auto *ecosystemLayout = new QGridLayout(ecosystemGroup);
    ecosystemLayout->setContentsMargins(4, 4, 4, 4);
    ecosystemLayout->setSpacing(4);

    m_moduleLayerLabel = new QLabel(tr("—"), this);
    m_moduleLayerLabel->setObjectName("moduleEcosystemLayerLabel");
    m_moduleLayerLabel->setWordWrap(true);
    ecosystemLayout->addWidget(new QLabel(tr("Слой:"), this), 0, 0);
    ecosystemLayout->addWidget(m_moduleLayerLabel, 0, 1);

    m_moduleRoleLabel = new QLabel(tr("—"), this);
    m_moduleRoleLabel->setObjectName("moduleEcosystemRoleLabel");
    m_moduleRoleLabel->setWordWrap(true);
    ecosystemLayout->addWidget(new QLabel(tr("Роль:"), this), 1, 0);
    ecosystemLayout->addWidget(m_moduleRoleLabel, 1, 1);

    m_moduleQualityLabel = new QLabel(tr("—"), this);
    m_moduleQualityLabel->setObjectName("moduleEcosystemQualityLabel");
    m_moduleQualityLabel->setWordWrap(true);
    m_moduleQualityLabel->setStyleSheet("QLabel { color:#bdbdbd; }");
    ecosystemLayout->addWidget(new QLabel(tr("Quality:"), this), 2, 0);
    ecosystemLayout->addWidget(m_moduleQualityLabel, 2, 1);

    rightLayout->addWidget(ecosystemGroup);

    // Явный documentation block делает модуль читаемым без открытия исходного кода.
    auto *docGroup = new QGroupBox(tr("Документация"), this);
    auto *docLayout = new QGridLayout(docGroup);
    docLayout->setContentsMargins(4, 4, 4, 4);
    docLayout->setSpacing(4);

    auto *docHint = new QLabel(tr("Назначение задаётся через поле 'Опис'."), this);
    docHint->setStyleSheet("QLabel { color:#9e9e9e; }");
    docLayout->addWidget(docHint, 0, 0, 1, 2);

    m_docWhenToUseEdit = new QTextEdit(this);
    m_docWhenToUseEdit->setObjectName("moduleDocWhenToUseEdit");
    m_docWhenToUseEdit->setMaximumHeight(58);
    m_docWhenToUseEdit->setPlaceholderText(tr("Когда использовать этот модуль"));
    docLayout->addWidget(new QLabel(tr("Когда использовать:"), this), 1, 0);
    docLayout->addWidget(m_docWhenToUseEdit, 1, 1);

    m_docLimitationsEdit = new QTextEdit(this);
    m_docLimitationsEdit->setObjectName("moduleDocLimitationsEdit");
    m_docLimitationsEdit->setMaximumHeight(58);
    m_docLimitationsEdit->setPlaceholderText(tr("Ограничения и caveats"));
    docLayout->addWidget(new QLabel(tr("Ограничения:"), this), 2, 0);
    docLayout->addWidget(m_docLimitationsEdit, 2, 1);

    rightLayout->addWidget(docGroup);

    // --- Центральный вертикальный сплиттер: код + табы внизу ---
    auto *centerSplitter = new QSplitter(Qt::Vertical, this);

    // Верх: редактор кода с подсветкой синтаксиса
    auto *codeWidget = new QWidget(this);
    auto *codeLayout = new QVBoxLayout(codeWidget);
    codeLayout->setContentsMargins(0, 0, 0, 0);
    codeLayout->setSpacing(4);

    m_codeEdit = new QPlainTextEdit(this);
    m_codeEdit->setObjectName("moduleCodeEditor");
    m_codeEdit->setFont(QFont("Monospace", 11));
    m_codeEdit->setPlaceholderText(
        tr("int dq_example(int a, int b) {\n    return a + b;\n}"));
    m_codeEdit->setTabStopDistance(QFontMetricsF(m_codeEdit->font()).horizontalAdvance(' ') * 4);

    // Подсветка синтаксиса C/C++
    m_highlighter = new SyntaxHighlighter(m_codeEdit->document());

    codeLayout->addWidget(m_codeEdit, 1);

    // Кнопки под кодом
    auto *actionLayout = new QHBoxLayout;
    m_saveBtn = new QPushButton(tr("Сохранить"), this);
    m_compileBtn = new QPushButton(tr("Компилировать"), this);
    m_testBtn = new QPushButton(tr("Тестировать"), this);
    m_saveBtn->setEnabled(false);
    m_compileBtn->setEnabled(false);
    m_testBtn->setEnabled(false);
    actionLayout->addWidget(m_saveBtn);
    actionLayout->addWidget(m_compileBtn);
    actionLayout->addWidget(m_testBtn);
    actionLayout->addStretch();
    codeLayout->addLayout(actionLayout);

    centerSplitter->addWidget(codeWidget);

    // Низ: QTabWidget (Журнал / Тестирование)
    m_bottomTabs = new QTabWidget(this);

    // Вкладка «Журнал»
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Monospace", 9));
    m_bottomTabs->addTab(m_logView, tr("Журнал"));

    // Вкладка «Тестирование»
    auto *testWidget = new QWidget(this);
    auto *testLayout = new QVBoxLayout(testWidget);
    testLayout->setContentsMargins(4, 4, 4, 4);
    m_testInputs = new QTableWidget(0, 2, this);
    m_testInputs->setHorizontalHeaderLabels({tr("Порт (вход)"), tr("Значение")});
    m_testInputs->horizontalHeader()->setStretchLastSection(true);
    testLayout->addWidget(m_testInputs);
    m_testOutputLabel = new QLabel(tr("Результат: —"), this);
    testLayout->addWidget(m_testOutputLabel);
    m_bottomTabs->addTab(testWidget, tr("Тестирование"));

    centerSplitter->addWidget(m_bottomTabs);

    // Пропорции: код ~70%, табы ~30%
    centerSplitter->setStretchFactor(0, 3);
    centerSplitter->setStretchFactor(1, 1);
    centerSplitter->setCollapsible(1, true);

    rightLayout->addWidget(centerSplitter, 1);

    // === Горизонтальный сплиттер: лево (библиотека) + право (редактор) ===
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setSizes({220, 700});
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // === Подключение сигналов ===
    connect(m_tree, &QTreeWidget::currentItemChanged, this, &ModuleManagerWidget::onModuleSelected);
    connect(m_newBtn, &QPushButton::clicked, this, &ModuleManagerWidget::onNewModule);
    connect(m_newPackBtn, &QPushButton::clicked, this, &ModuleManagerWidget::onNewPack);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ModuleManagerWidget::onDeleteModule);
    connect(m_saveBtn, &QPushButton::clicked, this, &ModuleManagerWidget::onSaveModule);
    connect(m_compileBtn, &QPushButton::clicked, this, &ModuleManagerWidget::onCompileModule);
    connect(m_testBtn, &QPushButton::clicked, this, &ModuleManagerWidget::onTestModule);
    connect(m_codeEdit, &QPlainTextEdit::textChanged, this, &ModuleManagerWidget::onCodeChanged);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &ModuleManagerWidget::onCodeChanged);
    connect(m_descEdit, &QLineEdit::textChanged, this, &ModuleManagerWidget::onCodeChanged);
    connect(m_includesEdit, &QTextEdit::textChanged, this, &ModuleManagerWidget::onCodeChanged);
    connect(m_docWhenToUseEdit, &QTextEdit::textChanged, this, &ModuleManagerWidget::onCodeChanged);
    connect(m_docLimitationsEdit, &QTextEdit::textChanged, this, &ModuleManagerWidget::onCodeChanged);
    connect(m_importDisplayNameEdit, &QLineEdit::textChanged, this, &ModuleManagerWidget::onCodeChanged);
    connect(m_importRoleCombo, &QComboBox::currentTextChanged, this, [this](const QString &) {
        onCodeChanged();
    });
    connect(m_categoryCombo, &QComboBox::currentTextChanged, this, [this](const QString &) {
        onCodeChanged();
    });
    connect(m_langCombo, &QComboBox::currentTextChanged, this, [this](const QString &) {
        onCodeChanged();
    });

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        // Поддерживает и обычные категории, и pack-группировку imported module-ов.
        for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
            filterTreeItemRecursive(m_tree->topLevelItem(i), text);
        }
    });

    connect(m_langFilter, &QComboBox::currentTextChanged, this, [this](const QString &lang) {
        setLanguageFilter(lang);
    });
    connect(m_showSpecializedCheck, &QCheckBox::toggled, this, [this] {
        buildTree();
    });
    connect(m_showLegacyCheck, &QCheckBox::toggled, this, [this] {
        buildTree();
    });
}

// Цвет иконки по категории
static QColor managerCategoryColor(const QString &cat)
{
    if (cat == "math")        return QColor(50, 100, 180);
    if (cat == "logic")       return QColor(50, 140, 80);
    if (cat == "io")          return QColor(200, 120, 40);
    if (cat == "string")      return QColor(140, 80, 180);
    if (cat == "conversion")  return QColor(100, 140, 200);
    if (cat == "control")     return QColor(180, 100, 50);
    if (cat == "ui")          return QColor(220, 80, 80);
    return QColor(100, 100, 100);
}

void ModuleManagerWidget::buildTree()
{
    m_tree->clear();
    if (!m_registry) return;

    QString langFilter;
    if (m_langFilter && m_langFilter->currentIndex() > 0) {
        langFilter = (m_langFilter->currentIndex() == 1) ? "c" : "cpp";
    }

    auto allModules = m_registry->allModules();
    const bool showSpecializedModules = m_showSpecializedCheck && m_showSpecializedCheck->isChecked();
    const bool showLegacyModules = m_showLegacyCheck && m_showLegacyCheck->isChecked();
    auto languageCompatible = [&](const Module *mod) {
        if (langFilter.isEmpty())
            return true;
        return (mod->language == langFilter)
            || (langFilter == "c" && mod->language == "cpp")
            || (langFilter == "cpp" && mod->language == "c")
            || mod->language.isEmpty();
    };
    auto addManagerModuleItem = [&](QTreeWidgetItem *parent, const Module *mod) {
        if (!languageCompatible(mod))
            return false;
        auto *item = new QTreeWidgetItem(parent, {moduleDisplayLabel(*mod, false)});
        item->setData(0, Qt::UserRole, mod->id);
        updateModuleTreeItemState(item, *mod);
        return true;
    };

    QFont boldFont = m_tree->font();
    boldFont.setBold(true);

    // === Секция 1: Стандартная библиотека (core) ===
    auto *coreRoot = new QTreeWidgetItem(m_tree, {tr("Стандартная библиотека")});
    coreRoot->setFont(0, boldFont);
    {
        QPixmap px(12, 12); px.fill(QColor(50, 120, 200));
        coreRoot->setIcon(0, QIcon(px));
    }

    QMap<QString, QVector<const Module *>> coreByCategory;
    for (const auto *mod : allModules) {
        if (mod->origin == "core")
            coreByCategory[mod->category].append(mod);
    }

    // Категории standard library показываем в product-defined v1 порядке,
    // а не просто по алфавиту, чтобы библиотека читалась как система.
    const QStringList coreCats = StandardLibrary::orderCategoriesForDisplay(coreByCategory.keys());
    bool coreHasChildren = false;

    for (const auto &cat : coreCats) {
        auto *catItem = new QTreeWidgetItem(coreRoot, {cat});
        QPixmap px(12, 12); px.fill(managerCategoryColor(cat));
        catItem->setIcon(0, QIcon(px));

        bool catHasChildren = false;
        const QVector<const Module *> orderedModules =
            StandardLibrary::orderModulesForDisplay(coreByCategory[cat]);
        for (const auto *mod : orderedModules) {
            const StandardLibraryCurationInfo curation = StandardLibrary::curationForModule(*mod);
            if (curation.tier == "specialized" && !showSpecializedModules)
                continue;
            if (curation.tier == "legacy" && !showLegacyModules)
                continue;
            if (addManagerModuleItem(catItem, mod))
                catHasChildren = true;
        }
        catItem->setHidden(!catHasChildren);
        if (catHasChildren) coreHasChildren = true;
    }
    coreRoot->setHidden(!coreHasChildren);
    coreRoot->setExpanded(true);

    // === Секция 2: UI-виджеты ===
    auto *uiRoot = new QTreeWidgetItem(m_tree, {tr("UI-виджеты")});
    uiRoot->setFont(0, boldFont);
    {
        QPixmap px(12, 12); px.fill(QColor(220, 80, 80));
        uiRoot->setIcon(0, QIcon(px));
    }

    bool uiHasChildren = false;
    for (const auto *mod : allModules) {
        if (mod->origin == "ui") {
            if (addManagerModuleItem(uiRoot, mod))
                uiHasChildren = true;
        }
    }
    uiRoot->setHidden(!uiHasChildren);
    uiRoot->setExpanded(true);

    // === Секция 3: Imported pack-и ===
    auto *importedRoot = new QTreeWidgetItem(m_tree, {tr("Импортированные пакеты")});
    importedRoot->setFont(0, boldFont);
    {
        QPixmap px(12, 12); px.fill(QColor(90, 170, 170));
        importedRoot->setIcon(0, QIcon(px));
    }

    QMap<QString, QVector<const Module *>> importedByPack;
    for (const auto *mod : allModules) {
        if (mod->isImportedPackModule())
            importedByPack[mod->importedPackName()].append(mod);
    }

    bool importedHasChildren = false;
    QStringList packNames = importedByPack.keys();
    packNames.sort();

    for (const auto &packName : packNames) {
        auto *packItem = new QTreeWidgetItem(importedRoot, {packName});
        packItem->setToolTip(0, importedPackTreeTooltip(packName, importedByPack[packName]));

        QMap<QString, QVector<const Module *>> packByCategory;
        for (const auto *mod : importedByPack[packName]) {
            const QString category = mod->category.isEmpty() ? QString("custom") : mod->category;
            packByCategory[category].append(mod);
        }

        bool packHasChildren = false;
        QStringList packCats = packByCategory.keys();
        packCats.sort();
        for (const auto &cat : packCats) {
            auto *catItem = new QTreeWidgetItem(packItem, {cat});
            QPixmap px(12, 12); px.fill(managerCategoryColor(cat));
            catItem->setIcon(0, QIcon(px));

            bool catHasChildren = false;
            for (const auto *mod : packByCategory[cat]) {
                if (addManagerModuleItem(catItem, mod))
                    catHasChildren = true;
            }
            catItem->setHidden(!catHasChildren);
            if (catHasChildren)
                packHasChildren = true;
        }
        packItem->setHidden(!packHasChildren);
        packItem->setExpanded(true);
        if (packHasChildren)
            importedHasChildren = true;
    }
    importedRoot->setHidden(!importedHasChildren);
    importedRoot->setExpanded(true);

    // === Секция 4: Расширения ===
    auto *extRoot = new QTreeWidgetItem(m_tree, {tr("Расширения")});
    extRoot->setFont(0, boldFont);
    {
        QPixmap px(12, 12); px.fill(QColor(80, 180, 80));
        extRoot->setIcon(0, QIcon(px));
    }

    // Группировка по категориям для расширений и пользовательских модулей
    QMap<QString, QVector<const Module *>> extByCategory;
    for (const auto *mod : allModules) {
        if (mod->origin != "core" && mod->origin != "ui"
            && mod->origin != "local" && mod->origin != "graph"
            && !mod->isImportedPackModule()) {
            QString cat = mod->category.isEmpty() ? "custom" : mod->category;
            extByCategory[cat].append(mod);
        }
    }

    bool extHasChildren = false;
    QStringList extCats = extByCategory.keys();
    extCats.sort();

    for (const auto &cat : extCats) {
        auto *catItem = new QTreeWidgetItem(extRoot, {cat});
        QPixmap px(12, 12); px.fill(managerCategoryColor(cat));
        catItem->setIcon(0, QIcon(px));

        bool catHasChildren = false;
        for (const auto *mod : extByCategory[cat]) {
            if (addManagerModuleItem(catItem, mod))
                catHasChildren = true;
        }
        catItem->setHidden(!catHasChildren);
        if (catHasChildren) extHasChildren = true;
    }
    extRoot->setHidden(!extHasChildren);
    extRoot->setExpanded(true);

    // === Секция 4: Проектные модули (local + graph) ===
    auto *localRoot = new QTreeWidgetItem(m_tree, {tr("Проектные модули")});
    localRoot->setFont(0, boldFont);
    {
        QPixmap px(12, 12); px.fill(QColor(180, 140, 50));
        localRoot->setIcon(0, QIcon(px));
    }

    bool localHasChildren = false;
    for (const auto *mod : allModules) {
        if (mod->origin == "local" || mod->origin == "graph") {
            if (addManagerModuleItem(localRoot, mod))
                localHasChildren = true;
        }
    }
    localRoot->setHidden(!localHasChildren);
    localRoot->setExpanded(true);
}

void ModuleManagerWidget::rebuildTree()
{
    buildTree();
}

void ModuleManagerWidget::setProjectDir(const QString &dir)
{
    m_projectDir = dir;
    buildTree();
}

void ModuleManagerWidget::setLanguageFilter(const QString &lang)
{
    Q_UNUSED(lang)
    buildTree();
}

void ModuleManagerWidget::setGlobalModulesDir(const QString &dir)
{
    m_globalModulesDir = dir;
}

bool ModuleManagerWidget::openModule(const QString &moduleId)
{
    if (moduleId.isEmpty())
        return false;

    std::function<QTreeWidgetItem *(QTreeWidgetItem *)> findItem;
    findItem = [&](QTreeWidgetItem *parent) -> QTreeWidgetItem * {
        for (int i = 0; i < parent->childCount(); ++i) {
            auto *child = parent->child(i);
            if (child->data(0, Qt::UserRole).toString() == moduleId)
                return child;
            if (auto *nested = findItem(child))
                return nested;
        }
        return nullptr;
    };

    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        if (auto *item = findItem(m_tree->topLevelItem(i))) {
            m_tree->setCurrentItem(item);
            m_tree->scrollToItem(item);
            return true;
        }
    }

    const Module *mod = m_registry ? m_registry->findModule(moduleId) : nullptr;
    if (mod) {
        loadModuleToEditor(*mod);
        return true;
    }

    return false;
}

QString ModuleManagerWidget::moduleFilePath(const QString &moduleId) const
{
    const Module *mod = m_registry->findModule(moduleId);
    if (!mod)
        return {};

    // Для уже загруженных imported/local модулей сначала сохраняем ровно туда, откуда они были открыты.
    if (!mod->storagePath.isEmpty())
        return mod->storagePath;

    // Проверяем, является ли модуль локальным (проектным)
    if (mod && (mod->origin == "local" || mod->origin == "graph")) {
        if (!m_projectDir.isEmpty())
            return m_projectDir + "/dqmods/" + moduleId + ".dqmod";
    }

    // Расширения сохраняются в глобальную папку модулей
    if (!m_globalModulesDir.isEmpty())
        return m_globalModulesDir + "/user_modules/" + moduleId + ".dqmod";
    if (!m_projectDir.isEmpty())
        return m_projectDir + "/modules/" + moduleId + ".dqmod";
    return {};
}

bool ModuleManagerWidget::saveToDisk(const Module &module)
{
    QString path = moduleFilePath(module.id);
    if (path.isEmpty()) return false;

    // Создаём директорию если нет
    QDir().mkpath(QFileInfo(path).absolutePath());

    return m_registry->saveModuleFile(module, path);
}

bool ModuleManagerWidget::deleteFromDisk(const QString &moduleId)
{
    QString path = moduleFilePath(moduleId);
    if (path.isEmpty()) return false;
    return QFile::remove(path);
}

void ModuleManagerWidget::onModuleSelected(QTreeWidgetItem *item, QTreeWidgetItem *)
{
    if (!item) {
        clearEditor();
        m_deleteBtn->setEnabled(false);
        return;
    }

    // Только листовые элементы с moduleId
    QString moduleId = item->data(0, Qt::UserRole).toString();
    if (moduleId.isEmpty()) {
        clearEditor();
        m_deleteBtn->setEnabled(false);
        return;
    }

    const Module *mod = m_registry->findModule(moduleId);
    if (!mod) {
        clearEditor();
        return;
    }

    loadModuleToEditor(*mod);
}

void ModuleManagerWidget::loadModuleToEditor(const Module &module)
{
    m_currentModuleId = module.id;
    m_modified = false;
    m_loadingModule = true;

    // Core и UI модули — только для просмотра, local и graph — полностью редактируемые
    bool isReadOnly = (module.origin == "ui" || module.origin == "core");

    m_nameEdit->setText(module.name);
    m_descEdit->setText(module.description);

    // Категория
    int catIdx = m_categoryCombo->findText(module.category);
    if (catIdx >= 0) m_categoryCombo->setCurrentIndex(catIdx);
    else m_categoryCombo->setCurrentText(module.category);

    // Язык
    int langIdx = m_langCombo->findText(module.language);
    if (langIdx >= 0) m_langCombo->setCurrentIndex(langIdx);

    const bool isImported = module.isImportedPackModule();
    m_importDisplayNameEdit->setText(isImported ? module.importedDisplayName() : QString());
    const QString role = isImported ? module.importedCurationRole() : QString("raw_wrapper");
    int roleIdx = m_importRoleCombo->findData(role);
    if (roleIdx < 0)
        roleIdx = 0;
    m_importRoleCombo->setCurrentIndex(roleIdx);
    m_importDisplayNameEdit->setEnabled(isImported && !isReadOnly);
    m_importRoleCombo->setEnabled(isImported && !isReadOnly);
    m_importSourceLabel->setText(isImported
        ? tr("Pack: %1\nSymbol: %2")
              .arg(module.importedPackName(), module.importedOriginalSymbol())
        : tr("Не imported module"));
    m_docWhenToUseEdit->setPlainText(module.documentationWhenToUse());
    m_docLimitationsEdit->setPlainText(module.documentationLimitations());

    // Includes
    m_includesEdit->setText(module.includes.join("\n"));

    // Код
    m_codeEdit->setPlainText(module.sourceCode);

    // Порты
    updatePortTable(module);

    // Превью блока на графе
    updatePreview(module);

    // Тестовые входы
    m_testInputs->setRowCount(module.inputs.size());
    for (int i = 0; i < module.inputs.size(); ++i) {
        m_testInputs->setItem(i, 0, new QTableWidgetItem(module.inputs[i].name));
        m_testInputs->setItem(i, 1, new QTableWidgetItem(module.inputs[i].defaultValue));
    }

    updateVerificationPanel(module);
    updateModuleEcosystemSummary(module);

    // Core и UI модули: только просмотр, блокируем редактирование
    m_nameEdit->setReadOnly(isReadOnly);
    m_descEdit->setReadOnly(isReadOnly);
    m_categoryCombo->setEnabled(!isReadOnly);
    m_langCombo->setEnabled(!isReadOnly);
    m_importDisplayNameEdit->setEnabled(isImported && !isReadOnly);
    m_importRoleCombo->setEnabled(isImported && !isReadOnly);
    m_docWhenToUseEdit->setReadOnly(isReadOnly);
    m_docLimitationsEdit->setReadOnly(isReadOnly);
    m_includesEdit->setReadOnly(isReadOnly);
    m_codeEdit->setReadOnly(isReadOnly);
    m_saveBtn->setEnabled(!isReadOnly);
    m_compileBtn->setEnabled(!isReadOnly);
    m_testBtn->setEnabled(!isReadOnly);
    m_deleteBtn->setEnabled(!isReadOnly);

    if (module.origin == "ui") {
        m_logView->append(tr("UI-модуль '%1' — только для просмотра").arg(module.name));
    } else if (module.origin == "core") {
        m_logView->append(tr("Стандартный модуль '%1' — только для просмотра").arg(module.name));
    }

    updateModuleStateSummary(module);
    m_loadingModule = false;
}

void ModuleManagerWidget::clearEditor()
{
    m_currentModuleId.clear();
    m_modified = false;
    m_loadingModule = true;
    m_nameEdit->clear();
    m_descEdit->clear();
    m_categoryCombo->setCurrentIndex(0);
    m_langCombo->setCurrentIndex(0);
    m_importDisplayNameEdit->clear();
    m_importRoleCombo->setCurrentIndex(0);
    m_importDisplayNameEdit->setEnabled(false);
    m_importRoleCombo->setEnabled(false);
    m_importSourceLabel->setText(tr("Не imported module"));
    m_docWhenToUseEdit->clear();
    m_docLimitationsEdit->clear();
    m_docWhenToUseEdit->setReadOnly(false);
    m_docLimitationsEdit->setReadOnly(false);
    m_includesEdit->clear();
    m_codeEdit->clear();
    m_portTable->setRowCount(0);
    m_testInputs->setRowCount(0);
    m_testOutputLabel->setText(tr("Компиляция: — | Тест: —"));
    m_testOutputLabel->setStyleSheet("");
    m_logView->clear();
    m_previewScene->clear();
    m_saveBtn->setEnabled(false);
    m_compileBtn->setEnabled(false);
    m_testBtn->setEnabled(false);
    m_moduleStateBadge->setText(tr("Модуль не выбран"));
    m_moduleStateBadge->setStyleSheet(
        "QLabel { background:#4a4a4a; color:#f0f0f0; border-radius:10px; "
        "padding:3px 10px; font-weight:bold; }");
    m_moduleStateDetails->setText(tr("Выберите модуль, чтобы увидеть его состояние."));
    m_moduleLayerLabel->setText(tr("—"));
    m_moduleRoleLabel->setText(tr("—"));
    m_moduleQualityLabel->setText(tr("Выберите модуль, чтобы увидеть его роль в экосистеме."));
    m_loadingModule = false;
}

void ModuleManagerWidget::updatePreview(const Module &module)
{
    m_previewScene->clear();
    if (module.name.isEmpty()) return;

    auto *node = new NodeItem("preview", module.name, module.category);
    node->setFlag(QGraphicsItem::ItemIsMovable, false);
    node->setFlag(QGraphicsItem::ItemIsSelectable, false);

    for (const auto &p : module.inputs)
        node->addInputPort(p.name, p.type);
    for (const auto &p : module.outputs)
        node->addOutputPort(p.name, p.type);

    m_previewScene->addItem(node);
    m_previewView->fitInView(node->boundingRect().adjusted(-10, -10, 10, 10),
                              Qt::KeepAspectRatio);
}

void ModuleManagerWidget::updatePortTable(const Module &module)
{
    m_portTable->setRowCount(0);
    int row = 0;

    for (const auto &p : module.inputs) {
        m_portTable->insertRow(row);
        m_portTable->setItem(row, 0, new QTableWidgetItem(p.name));
        m_portTable->setItem(row, 1, new QTableWidgetItem(p.type));
        m_portTable->setItem(row, 2, new QTableWidgetItem(tr("Вход")));
        row++;
    }

    for (const auto &p : module.outputs) {
        m_portTable->insertRow(row);
        m_portTable->setItem(row, 0, new QTableWidgetItem(p.name));
        m_portTable->setItem(row, 1, new QTableWidgetItem(p.type));
        m_portTable->setItem(row, 2, new QTableWidgetItem(tr("Выход")));
        row++;
    }
}

void ModuleManagerWidget::onNewModule()
{
    Module mod = Module::create("new_module", m_langCombo->currentText());
    mod.category = "custom";
    mod.description = tr("Новый модуль");
    // Если проект открыт — создаём локальный модуль, иначе — расширение
    mod.origin = m_projectDir.isEmpty() ? "extension" : "local";
    mod.compileStatus = "unknown";
    mod.testStatus = "untested";
    mod.sourceCode = QString("int dq_new_module(int a) {\n    return a;\n}");

    m_registry->registerModule(mod);
    saveToDisk(mod);
    buildTree();

    // Рекурсивно ищем и выбираем созданный модуль
    std::function<bool(QTreeWidgetItem *)> findAndSelect;
    findAndSelect = [&](QTreeWidgetItem *parent) -> bool {
        for (int i = 0; i < parent->childCount(); ++i) {
            auto *child = parent->child(i);
            if (child->data(0, Qt::UserRole).toString() == mod.id) {
                m_tree->setCurrentItem(child);
                return true;
            }
            if (findAndSelect(child)) return true;
        }
        return false;
    };
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        if (findAndSelect(m_tree->topLevelItem(i)))
            break;
    }
}

void ModuleManagerWidget::onNewPack()
{
    if (m_globalModulesDir.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"),
            tr("Глобальная папка модулей не настроена."));
        return;
    }

    bool ok = false;
    QString packName = QInputDialog::getText(this, tr("Новый пакет"),
        tr("Имя пакета (латиница, без пробелов):"),
        QLineEdit::Normal, "my_pack", &ok);

    if (!ok || packName.trimmed().isEmpty()) return;

    packName = packName.trimmed().replace(' ', '_').toLower();
    QString packDir = m_globalModulesDir + "/" + packName;

    if (QDir(packDir).exists()) {
        QMessageBox::warning(this, tr("Ошибка"),
            tr("Пакет '%1' уже существует.").arg(packName));
        return;
    }

    QDir().mkpath(packDir);

    // Создаём pack.json
    QJsonObject pack;
    pack["name"] = packName;
    pack["version"] = "1.0";
    pack["author"] = "User";
    pack["description"] = "";

    QFile packFile(packDir + "/pack.json");
    if (packFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(pack);
        packFile.write(doc.toJson(QJsonDocument::Indented));
    }

    m_logView->append(tr("Создан пакет '%1' в %2").arg(packName, packDir));
}

void ModuleManagerWidget::onDeleteModule()
{
    if (m_currentModuleId.isEmpty()) return;

    const Module *mod = m_registry->findModule(m_currentModuleId);
    if (!mod) return;

    // Предупреждение
    auto result = QMessageBox::warning(this, tr("Удаление модуля"),
        tr("Вы уверены, что хотите удалить модуль '%1'?\n\n"
           "Проверьте, не используется ли он в графах.").arg(mod->name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result != QMessageBox::Yes) return;

    deleteFromDisk(m_currentModuleId);
    m_registry->unregisterModule(m_currentModuleId);
    clearEditor();
    buildTree();
}

void ModuleManagerWidget::onSaveModule()
{
    if (m_currentModuleId.isEmpty()) return;

    Module *mod = m_registry->findModule(m_currentModuleId);
    if (!mod) return;

    // Core и UI модули нельзя редактировать
    if (mod->origin == "ui" || mod->origin == "core") {
        m_logView->append(tr("Стандартные и UI-модули фиксированы и не могут быть изменены."));
        return;
    }

    mod->name = m_nameEdit->text().trimmed();
    mod->description = m_descEdit->text().trimmed();
    mod->category = m_categoryCombo->currentText().trimmed();
    mod->language = m_langCombo->currentText();

    if (mod->isImportedPackModule()) {
        const QString displayName = m_importDisplayNameEdit->text().trimmed();
        mod->metadata["deltaq.import.display_name"] = displayName.isEmpty() ? mod->name : displayName;
        mod->metadata["deltaq.import.curation_role"] = m_importRoleCombo->currentData().toString();
    }

    const QString whenToUse = m_docWhenToUseEdit->toPlainText().trimmed();
    const QString limitations = m_docLimitationsEdit->toPlainText().trimmed();
    if (!whenToUse.isEmpty())
        mod->metadata["deltaq.doc.when_to_use"] = whenToUse;
    else
        mod->metadata.remove("deltaq.doc.when_to_use");
    if (!limitations.isEmpty())
        mod->metadata["deltaq.doc.limitations"] = limitations;
    else
        mod->metadata.remove("deltaq.doc.limitations");

    // Includes
    mod->includes.clear();
    for (const auto &line : m_includesEdit->toPlainText().split('\n')) {
        QString inc = line.trimmed();
        if (!inc.isEmpty())
            mod->includes.append(inc);
    }

    // Код
    mod->sourceCode = m_codeEdit->toPlainText();

    // Проверяем наличие #include в коде
    if (mod->sourceCode.contains("#include")) {
        m_logView->append(tr("Внимание: обнаружен #include в коде. "
                             "Перенесите зависимости в поле 'Зависимости'."));
    }

    // Автоанализ сигнатуры
    parseSignature(mod->sourceCode);

    // Обновляем превью блока
    updatePreview(*mod);

    // Сохраняем на диск (.dqmod)
    if (saveToDisk(*mod)) {
        mod->storagePath = moduleFilePath(mod->id);
        m_logView->append(tr("Модуль '%1' сохранён на диск").arg(mod->name));
    } else if (!m_projectDir.isEmpty()) {
        m_logView->append(tr("Ошибка записи .dqmod файла"));
    } else {
        m_logView->append(tr("Модуль '%1' сохранён (только в памяти — откройте проект для записи на диск)").arg(mod->name));
    }

    m_modified = false;
    updateModuleStateSummary(*mod);
    updateModuleEcosystemSummary(*mod);
    updateModuleTreeItemState(m_tree->currentItem(), *mod);

    emit moduleChanged(m_currentModuleId);
    {
        const QSignalBlocker blocker(m_tree);
        buildTree();
    }
    openModule(m_currentModuleId);
}

void ModuleManagerWidget::onCompileModule()
{
    if (m_currentModuleId.isEmpty()) return;

    // Сначала сохраняем
    onSaveModule();

    const Module *mod = m_registry->findModule(m_currentModuleId);
    if (!mod) return;

    m_logView->append(tr("Компиляция модуля '%1'...").arg(mod->name));
    TestResult result = m_testRunner->compile(*mod);

    if (result.compiled) {
        m_logView->append(tr("Синтаксис корректен"));
    }
}

void ModuleManagerWidget::onTestModule()
{
    if (m_currentModuleId.isEmpty()) return;

    // Сначала сохраняем
    onSaveModule();

    const Module *mod = m_registry->findModule(m_currentModuleId);
    if (!mod) return;

    // Собираем входные значения из таблицы
    QMap<QString, QString> inputValues;
    for (int i = 0; i < m_testInputs->rowCount(); ++i) {
        auto *nameItem = m_testInputs->item(i, 0);
        auto *valueItem = m_testInputs->item(i, 1);
        if (nameItem && valueItem) {
            inputValues[nameItem->text()] = valueItem->text();
        }
    }

    m_logView->append(tr("Тестирование модуля '%1'...").arg(mod->name));
    m_testRunner->runTest(*mod, inputValues);
}

void ModuleManagerWidget::onCodeChanged()
{
    if (m_currentModuleId.isEmpty() || m_loadingModule) return;

    Module *mod = m_registry->findModule(m_currentModuleId);
    if (!mod)
        return;

    if (!m_modified) {
        m_modified = true;
        invalidateVerificationStatus(*mod);
    }

    updateVerificationPanel(*mod);
    updateModuleStateSummary(*mod);
    updateModuleEcosystemSummary(*mod);
    updateModuleTreeItemState(m_tree->currentItem(), *mod);
}

void ModuleManagerWidget::parseSignature(const QString &code)
{
    // Ищем сигнатуру: тип dq_name(аргументы)
    QRegularExpression re(
        R"((\w+)\s+dq_(\w+)\s*\(([^)]*)\))");
    auto match = re.match(code);
    if (!match.hasMatch()) return;

    QString returnType = match.captured(1);
    QString argsStr = match.captured(3);

    Module *mod = m_registry->findModule(m_currentModuleId);
    if (!mod) return;

    // Очищаем и перестраиваем порты
    mod->inputs.clear();
    mod->outputs.clear();

    // Парсим аргументы
    if (!argsStr.trimmed().isEmpty() && argsStr.trimmed() != "void") {
        QStringList args = argsStr.split(',');
        for (const auto &arg : args) {
            QString trimmed = arg.trimmed();
            // Ищем последний пробел — перед ним тип, после — имя
            int lastSpace = trimmed.lastIndexOf(' ');
            if (lastSpace > 0) {
                QString type = trimmed.left(lastSpace).trimmed();
                QString name = trimmed.mid(lastSpace + 1).trimmed();
                // Убираем указатели/ссылки из имени
                name.remove('*');
                name.remove('&');

                Port p;
                p.name = name;
                // Маппинг C-типов на типы портов
                if (type == "int" || type == "long") p.type = "int";
                else if (type == "float") p.type = "float";
                else if (type == "double") p.type = "double";
                else if (type == "const char*" || type == "char*") p.type = "string";
                else p.type = "int";

                mod->inputs.append(p);
            }
        }
    }

    // Возвращаемый тип → выходной порт
    if (returnType != "void") {
        Port outPort;
        outPort.name = "result";
        if (returnType == "int" || returnType == "long") outPort.type = "int";
        else if (returnType == "float") outPort.type = "float";
        else if (returnType == "double") outPort.type = "double";
        else if (returnType == "const char*" || returnType == "char*") outPort.type = "string";
        else outPort.type = "int";
        mod->outputs.append(outPort);
    }

    updatePortTable(*mod);
}

void ModuleManagerWidget::updateVerificationPanel(const Module &module)
{
    if (!m_testOutputLabel)
        return;

    // Панель тестирования показывает оба независимых этапа проверки атомарного модуля.
    if (module.origin == "core" || module.origin == "ui") {
        m_testOutputLabel->setText(tr("Проверка: встроенный библиотечный модуль"));
        m_testOutputLabel->setStyleSheet("color: #90CAF9;");
        return;
    }

    if (module.isComposite()) {
        m_testOutputLabel->setText(tr("Компиляция: через граф | Тест: через композицию"));
        m_testOutputLabel->setStyleSheet("color: #90CAF9;");
        return;
    }

    const QString text = tr("Компиляция: %1 | Тест: %2")
        .arg(moduleCompileStatusText(module.compileStatus))
        .arg(moduleTestStatusText(module.testStatus));
    m_testOutputLabel->setText(text);

    if (module.compileStatus == "failed" || module.testStatus == "failed")
        m_testOutputLabel->setStyleSheet("color: #F44336;");
    else if (module.compileStatus == "passed" && module.testStatus == "passed")
        m_testOutputLabel->setStyleSheet("color: #4CAF50;");
    else if (module.compileStatus == "modified" || module.testStatus == "modified")
        m_testOutputLabel->setStyleSheet("color: #FF9800;");
    else
        m_testOutputLabel->setStyleSheet("color: #9E9E9E;");
}

void ModuleManagerWidget::invalidateVerificationStatus(Module &module)
{
    // Любая правка контракта/кода/зависимостей делает compile/test результаты устаревшими.
    if (module.isComposite())
        return;

    if (module.compileStatus == "passed" || module.compileStatus == "failed")
        module.compileStatus = "modified";
    if (module.testStatus == "passed" || module.testStatus == "failed")
        module.testStatus = "modified";
}

void ModuleManagerWidget::updateStatusIcon(QTreeWidgetItem *item, const QString &status)
{
    if (!item) return;

    QPixmap px(10, 10);
    if (status == "passed")
        px.fill(QColor(76, 175, 80));       // зелёный
    else if (status == "failed")
        px.fill(QColor(244, 67, 54));       // красный
    else if (status == "modified")
        px.fill(QColor(255, 152, 0));       // оранжевый
    else
        px.fill(QColor(158, 158, 158));     // серый (untested)

    item->setIcon(0, QIcon(px));
}

void ModuleManagerWidget::updateModuleStateSummary(const Module &module)
{
    if (!m_moduleStateBadge || !m_moduleStateDetails)
        return;

    // Синхронизирует явную панель состояния с фактической моделью модуля.
    const ModuleStateSummary summary = buildModuleStateSummary(m_registry, module);
    m_moduleStateBadge->setText(summary.badgeText);
    m_moduleStateBadge->setStyleSheet(summary.badgeStyle);
    m_moduleStateDetails->setText(summary.detailText);
}

void ModuleManagerWidget::updateModuleEcosystemSummary(const Module &module)
{
    if (!m_moduleLayerLabel || !m_moduleRoleLabel || !m_moduleQualityLabel)
        return;

    const ModuleEcosystemSummary summary = buildModuleEcosystemSummary(m_registry, module);
    m_moduleLayerLabel->setText(summary.layerText);
    m_moduleRoleLabel->setText(summary.roleText);
    m_moduleQualityLabel->setText(summary.qualityText);
}

void ModuleManagerWidget::updateModuleTreeItemState(QTreeWidgetItem *item, const Module &module)
{
    if (!item)
        return;

    // Дерево показывает компактную иконку, а tooltip раскрывает причину текущего состояния.
    const ModuleStateSummary summary = buildModuleStateSummary(m_registry, module);
    updateStatusIcon(item, summary.iconStatus);
    item->setText(0, moduleDisplayLabel(module, false));
    QString tooltip = summary.badgeText + "\n" + summary.detailText;

    if (module.origin == "core" || module.id.startsWith("core.")) {
        const StandardLibraryCurationInfo curation = StandardLibrary::curationForModule(module);
        if (curation.isKnown()) {
            tooltip += QObject::tr("\nРоль библиотеки: %1").arg(curation.title);
            if (!curation.guidance.isEmpty())
                tooltip += QObject::tr("\nПодсказка: %1").arg(curation.guidance);
            if (!curation.replacementHint.isEmpty())
                tooltip += QObject::tr("\nАльтернатива: %1").arg(curation.replacementHint);
        }
    }

    if (module.isImportedPackModule()) {
        tooltip += QObject::tr("\nDisplay: %1").arg(module.importedDisplayName());
        tooltip += QObject::tr("\nРоль import/curation: %1").arg(importedCurationRoleTitle(module));
    }

    appendDocumentationTooltip(tooltip, module);

    item->setToolTip(0, tooltip);
}

} // namespace DeltaQ
