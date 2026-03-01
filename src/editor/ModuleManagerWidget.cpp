// Менеджер модулей — реализация
#include "ModuleManagerWidget.h"
#include "ModuleTestRunner.h"
#include "../core/ModuleRegistry.h"
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
#include <QTabWidget>
#include <functional>

namespace DeltaQ {

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
    });

    connect(m_testRunner, &ModuleTestRunner::testFinished, this,
            [this](const TestResult &result) {
        if (result.passed) {
            m_logView->append(tr("Тест пройден"));
            m_testOutputLabel->setText(tr("Результат: PASSED"));
            m_testOutputLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");

            // Обновляем статус модуля
            if (!m_currentModuleId.isEmpty()) {
                Module *mod = m_registry->findModule(m_currentModuleId);
                if (mod) {
                    mod->testStatus = "passed";
                    updateStatusIcon(m_tree->currentItem(), "passed");
                }
            }
        } else {
            m_logView->append(tr("Тест не пройден"));
            m_testOutputLabel->setText(tr("Результат: FAILED"));
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

    // 2. Зависимости (#include)
    auto *includesGroup = new QGroupBox(tr("Зависимости"), this);
    auto *includesLayout = new QVBoxLayout(includesGroup);
    includesLayout->setContentsMargins(4, 4, 4, 4);
    m_includesEdit = new QTextEdit(this);
    m_includesEdit->setMaximumHeight(80);
    m_includesEdit->setPlaceholderText(tr("stdlib.h\nmath.h"));
    m_includesEdit->setFont(QFont("Monospace", 9));
    includesLayout->addWidget(m_includesEdit);
    metaBar->addWidget(includesGroup, 1);

    // 3. Порты (компактная таблица)
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

    // 4. Превью блока на графе
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

    // --- Центральный вертикальный сплиттер: код + табы внизу ---
    auto *centerSplitter = new QSplitter(Qt::Vertical, this);

    // Верх: редактор кода с подсветкой синтаксиса
    auto *codeWidget = new QWidget(this);
    auto *codeLayout = new QVBoxLayout(codeWidget);
    codeLayout->setContentsMargins(0, 0, 0, 0);
    codeLayout->setSpacing(4);

    m_codeEdit = new QPlainTextEdit(this);
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

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        // Рекурсивная фильтрация 3-уровневого дерева
        for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
            auto *sectionItem = m_tree->topLevelItem(i);
            bool sectionVisible = false;

            for (int j = 0; j < sectionItem->childCount(); ++j) {
                auto *child = sectionItem->child(j);
                if (child->childCount() > 0) {
                    // Подкатегория — фильтруем детей
                    bool catVisible = false;
                    for (int k = 0; k < child->childCount(); ++k) {
                        auto *modItem = child->child(k);
                        bool match = text.isEmpty() ||
                                     modItem->text(0).contains(text, Qt::CaseInsensitive);
                        modItem->setHidden(!match);
                        if (match) catVisible = true;
                    }
                    child->setHidden(!catVisible);
                    if (catVisible) sectionVisible = true;
                } else {
                    // Модуль напрямую в секции
                    bool match = text.isEmpty() ||
                                 child->text(0).contains(text, Qt::CaseInsensitive);
                    child->setHidden(!match);
                    if (match) sectionVisible = true;
                }
            }
            sectionItem->setHidden(!sectionVisible);
        }
    });

    connect(m_langFilter, &QComboBox::currentTextChanged, this, [this](const QString &lang) {
        setLanguageFilter(lang);
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

    QStringList coreCats = coreByCategory.keys();
    coreCats.sort();
    bool coreHasChildren = false;

    for (const auto &cat : coreCats) {
        auto *catItem = new QTreeWidgetItem(coreRoot, {cat});
        QPixmap px(12, 12); px.fill(managerCategoryColor(cat));
        catItem->setIcon(0, QIcon(px));

        bool catHasChildren = false;
        for (const auto *mod : coreByCategory[cat]) {
            if (!langFilter.isEmpty()) {
                bool compat = (mod->language == langFilter) ||
                    (langFilter == "c" && mod->language == "cpp") ||
                    (langFilter == "cpp" && mod->language == "c");
                if (!compat) continue;
            }
            auto *item = new QTreeWidgetItem(catItem, {mod->name});
            item->setData(0, Qt::UserRole, mod->id);
            updateStatusIcon(item, mod->testStatus);
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
            auto *item = new QTreeWidgetItem(uiRoot, {mod->name});
            item->setData(0, Qt::UserRole, mod->id);
            updateStatusIcon(item, mod->testStatus);
            uiHasChildren = true;
        }
    }
    uiRoot->setHidden(!uiHasChildren);
    uiRoot->setExpanded(true);

    // === Секция 3: Расширения ===
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
            && mod->origin != "local" && mod->origin != "graph") {
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
            if (!langFilter.isEmpty()) {
                bool compat = (mod->language == langFilter) ||
                    (langFilter == "c" && mod->language == "cpp") ||
                    (langFilter == "cpp" && mod->language == "c");
                if (!compat) continue;
            }
            auto *item = new QTreeWidgetItem(catItem, {mod->name});
            item->setData(0, Qt::UserRole, mod->id);
            updateStatusIcon(item, mod->testStatus);
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
            if (!langFilter.isEmpty()) {
                bool compat = (mod->language == langFilter) ||
                    (langFilter == "c" && mod->language == "cpp") ||
                    (langFilter == "cpp" && mod->language == "c");
                if (!compat && !mod->language.isEmpty()) continue;
            }
            auto *item = new QTreeWidgetItem(localRoot, {mod->name});
            item->setData(0, Qt::UserRole, mod->id);
            updateStatusIcon(item, mod->testStatus);
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

QString ModuleManagerWidget::moduleFilePath(const QString &moduleId) const
{
    // Проверяем, является ли модуль локальным (проектным)
    const Module *mod = m_registry->findModule(moduleId);
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

    m_testOutputLabel->setText(tr("Статус: %1").arg(module.testStatus));
    if (module.testStatus == "passed")
        m_testOutputLabel->setStyleSheet("color: #4CAF50;");
    else if (module.testStatus == "failed")
        m_testOutputLabel->setStyleSheet("color: #F44336;");
    else if (module.testStatus == "modified")
        m_testOutputLabel->setStyleSheet("color: #FF9800;");
    else
        m_testOutputLabel->setStyleSheet("color: #9E9E9E;");

    // Core и UI модули: только просмотр, блокируем редактирование
    m_nameEdit->setReadOnly(isReadOnly);
    m_descEdit->setReadOnly(isReadOnly);
    m_categoryCombo->setEnabled(!isReadOnly);
    m_langCombo->setEnabled(!isReadOnly);
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
}

void ModuleManagerWidget::clearEditor()
{
    m_currentModuleId.clear();
    m_modified = false;
    m_nameEdit->clear();
    m_descEdit->clear();
    m_categoryCombo->setCurrentIndex(0);
    m_langCombo->setCurrentIndex(0);
    m_includesEdit->clear();
    m_codeEdit->clear();
    m_portTable->setRowCount(0);
    m_testInputs->setRowCount(0);
    m_testOutputLabel->setText(tr("Результат: —"));
    m_testOutputLabel->setStyleSheet("");
    m_logView->clear();
    m_previewScene->clear();
    m_saveBtn->setEnabled(false);
    m_compileBtn->setEnabled(false);
    m_testBtn->setEnabled(false);
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
        m_logView->append(tr("Модуль '%1' сохранён на диск").arg(mod->name));
    } else if (!m_projectDir.isEmpty()) {
        m_logView->append(tr("Ошибка записи .dqmod файла"));
    } else {
        m_logView->append(tr("Модуль '%1' сохранён (только в памяти — откройте проект для записи на диск)").arg(mod->name));
    }

    m_modified = false;

    emit moduleChanged(m_currentModuleId);
    buildTree();
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
    if (m_currentModuleId.isEmpty()) return;

    if (!m_modified) {
        m_modified = true;
        // Предупреждение: статус сбросится
        Module *mod = m_registry->findModule(m_currentModuleId);
        if (mod && mod->testStatus == "passed") {
            mod->testStatus = "modified";
            m_testOutputLabel->setText(tr("Статус: modified"));
            m_testOutputLabel->setStyleSheet("color: #FF9800;");

            auto *item = m_tree->currentItem();
            if (item && item->parent())
                updateStatusIcon(item, "modified");
        }
    }
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

} // namespace DeltaQ
