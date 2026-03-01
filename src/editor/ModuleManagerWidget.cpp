// Менеджер модулей — реализация
#include "ModuleManagerWidget.h"
#include "ModuleTestRunner.h"
#include "../core/ModuleRegistry.h"
#include <deltaq/Module.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QDir>
#include <QFile>

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

    // === Левая панель: библиотека ===
    auto *leftPanel = new QWidget(this);
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
    m_newBtn = new QPushButton(tr("Новый модуль"), this);
    m_deleteBtn = new QPushButton(tr("Удалить"), this);
    m_deleteBtn->setEnabled(false);
    btnLayout->addWidget(m_newBtn);
    btnLayout->addWidget(m_deleteBtn);
    leftLayout->addLayout(btnLayout);

    // === Правая панель: редактор модуля ===
    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 4, 4, 4);

    // Шапка: имя, описание, категория, язык
    auto *headerGroup = new QGroupBox(tr("Свойства модуля"), this);
    auto *headerLayout = new QFormLayout(headerGroup);

    m_nameEdit = new QLineEdit(this);
    headerLayout->addRow(tr("Имя:"), m_nameEdit);

    m_descEdit = new QLineEdit(this);
    headerLayout->addRow(tr("Описание:"), m_descEdit);

    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->setEditable(true);
    m_categoryCombo->addItems({"math", "logic", "io", "string", "ui", "custom"});
    headerLayout->addRow(tr("Категория:"), m_categoryCombo);

    m_langCombo = new QComboBox(this);
    m_langCombo->addItems({"c", "cpp"});
    headerLayout->addRow(tr("Язык:"), m_langCombo);

    rightLayout->addWidget(headerGroup);

    // Зависимости (#include)
    auto *includesGroup = new QGroupBox(tr("Зависимости (#include)"), this);
    auto *includesLayout = new QVBoxLayout(includesGroup);
    m_includesEdit = new QTextEdit(this);
    m_includesEdit->setMaximumHeight(60);
    m_includesEdit->setPlaceholderText(tr("По одному на строку: stdlib.h, math.h ..."));
    includesLayout->addWidget(m_includesEdit);
    rightLayout->addWidget(includesGroup);

    // Редактор кода
    auto *codeGroup = new QGroupBox(tr("Исходный код (тело функции)"), this);
    auto *codeLayout = new QVBoxLayout(codeGroup);
    m_codeEdit = new QPlainTextEdit(this);
    m_codeEdit->setFont(QFont("Monospace", 10));
    m_codeEdit->setPlaceholderText(
        tr("int dq_example(int a, int b) {\n    return a + b;\n}"));
    codeLayout->addWidget(m_codeEdit);
    rightLayout->addWidget(codeGroup);

    // Таблица портов (автоопределение)
    auto *portGroup = new QGroupBox(tr("Порты (автоопределение из сигнатуры)"), this);
    auto *portLayout = new QVBoxLayout(portGroup);
    m_portTable = new QTableWidget(0, 3, this);
    m_portTable->setHorizontalHeaderLabels({tr("Имя"), tr("Тип"), tr("Направление")});
    m_portTable->horizontalHeader()->setStretchLastSection(true);
    m_portTable->setMaximumHeight(120);
    m_portTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    portLayout->addWidget(m_portTable);
    rightLayout->addWidget(portGroup);

    // Кнопки: Сохранить, Компилировать, Тестировать
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
    rightLayout->addLayout(actionLayout);

    // Панель тестирования
    auto *testGroup = new QGroupBox(tr("Тестирование"), this);
    auto *testLayout = new QVBoxLayout(testGroup);
    m_testInputs = new QTableWidget(0, 2, this);
    m_testInputs->setHorizontalHeaderLabels({tr("Порт (вход)"), tr("Значение")});
    m_testInputs->horizontalHeader()->setStretchLastSection(true);
    m_testInputs->setMaximumHeight(80);
    testLayout->addWidget(m_testInputs);
    m_testOutputLabel = new QLabel(tr("Результат: —"), this);
    testLayout->addWidget(m_testOutputLabel);
    rightLayout->addWidget(testGroup);

    // Журнал
    auto *logGroup = new QGroupBox(tr("Журнал компиляции/запуска"), this);
    auto *logLayout = new QVBoxLayout(logGroup);
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumHeight(100);
    m_logView->setFont(QFont("Monospace", 9));
    logLayout->addWidget(m_logView);
    rightLayout->addWidget(logGroup);

    // Splitter
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setSizes({250, 600});

    mainLayout->addWidget(splitter);

    // Подключения сигналов
    connect(m_tree, &QTreeWidget::currentItemChanged, this, &ModuleManagerWidget::onModuleSelected);
    connect(m_newBtn, &QPushButton::clicked, this, &ModuleManagerWidget::onNewModule);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ModuleManagerWidget::onDeleteModule);
    connect(m_saveBtn, &QPushButton::clicked, this, &ModuleManagerWidget::onSaveModule);
    connect(m_compileBtn, &QPushButton::clicked, this, &ModuleManagerWidget::onCompileModule);
    connect(m_testBtn, &QPushButton::clicked, this, &ModuleManagerWidget::onTestModule);
    connect(m_codeEdit, &QPlainTextEdit::textChanged, this, &ModuleManagerWidget::onCodeChanged);

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
            auto *catItem = m_tree->topLevelItem(i);
            bool catVisible = false;
            for (int j = 0; j < catItem->childCount(); ++j) {
                auto *modItem = catItem->child(j);
                bool match = text.isEmpty() ||
                             modItem->text(0).contains(text, Qt::CaseInsensitive);
                modItem->setHidden(!match);
                if (match) catVisible = true;
            }
            catItem->setHidden(!catVisible);
        }
    });

    connect(m_langFilter, &QComboBox::currentTextChanged, this, [this](const QString &lang) {
        setLanguageFilter(lang);
    });
}

void ModuleManagerWidget::buildTree()
{
    m_tree->clear();
    if (!m_registry) return;

    QStringList cats = m_registry->categories();
    cats.sort();

    QString langFilter;
    if (m_langFilter && m_langFilter->currentIndex() > 0) {
        langFilter = (m_langFilter->currentIndex() == 1) ? "c" : "cpp";
    }

    for (const auto &cat : cats) {
        auto *catItem = new QTreeWidgetItem(m_tree, {cat});
        catItem->setFlags(catItem->flags() & ~Qt::ItemIsDragEnabled);

        // Цветная иконка по категории
        QPixmap px(12, 12);
        QColor color;
        if (cat == "math")        color = QColor(50, 100, 180);
        else if (cat == "logic")  color = QColor(50, 140, 80);
        else if (cat == "io")     color = QColor(200, 120, 40);
        else if (cat == "string") color = QColor(140, 80, 180);
        else if (cat == "ui")     color = QColor(220, 80, 80);
        else                      color = QColor(100, 100, 100);
        px.fill(color);
        catItem->setIcon(0, QIcon(px));

        auto modules = m_registry->modulesByCategory(cat);
        bool hasVisibleChild = false;

        for (const auto *mod : modules) {
            // Фильтрация по языку
            if (!langFilter.isEmpty()) {
                bool compatible = (mod->language == langFilter) ||
                    (langFilter == "c" && mod->language == "cpp") ||
                    (langFilter == "cpp" && mod->language == "c");
                if (!compatible) continue;
            }

            auto *modItem = new QTreeWidgetItem(catItem, {mod->name});
            modItem->setData(0, Qt::UserRole, mod->id);

            // Иконка статуса тестов
            updateStatusIcon(modItem, mod->testStatus);

            hasVisibleChild = true;
        }

        catItem->setHidden(!hasVisibleChild);
        catItem->setExpanded(true);
    }
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

QString ModuleManagerWidget::moduleFilePath(const QString &moduleId) const
{
    if (m_projectDir.isEmpty()) return {};
    return m_projectDir + "/modules/" + moduleId + ".dqmod";
}

bool ModuleManagerWidget::saveToDisk(const Module &module)
{
    if (m_projectDir.isEmpty()) return false;

    // Создаём директорию modules/ если нет
    QString modulesDir = m_projectDir + "/modules";
    QDir().mkpath(modulesDir);

    QString path = moduleFilePath(module.id);
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
    if (!item || !item->parent()) {
        clearEditor();
        m_deleteBtn->setEnabled(false);
        return;
    }

    QString moduleId = item->data(0, Qt::UserRole).toString();
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

    // UI-модули — только для просмотра
    bool isUI = (module.origin == "ui");

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

    // UI-модули: только просмотр, блокируем редактирование
    m_nameEdit->setReadOnly(isUI);
    m_descEdit->setReadOnly(isUI);
    m_categoryCombo->setEnabled(!isUI);
    m_langCombo->setEnabled(!isUI);
    m_includesEdit->setReadOnly(isUI);
    m_codeEdit->setReadOnly(isUI);
    m_saveBtn->setEnabled(!isUI);
    m_compileBtn->setEnabled(!isUI);
    m_testBtn->setEnabled(!isUI);
    m_deleteBtn->setEnabled(!isUI);

    if (isUI) {
        m_logView->append(tr("UI-модуль '%1' — только для просмотра").arg(module.name));
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
    m_saveBtn->setEnabled(false);
    m_compileBtn->setEnabled(false);
    m_testBtn->setEnabled(false);
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
    mod.testStatus = "untested";
    mod.sourceCode = QString("int dq_new_module(int a) {\n    return a;\n}");

    m_registry->registerModule(mod);
    saveToDisk(mod);
    buildTree();

    // Выбираем созданный модуль
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto *catItem = m_tree->topLevelItem(i);
        for (int j = 0; j < catItem->childCount(); ++j) {
            auto *modItem = catItem->child(j);
            if (modItem->data(0, Qt::UserRole).toString() == mod.id) {
                m_tree->setCurrentItem(modItem);
                return;
            }
        }
    }
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

    // UI-модули нельзя редактировать
    if (mod->origin == "ui") {
        m_logView->append(tr("UI-модули фиксированы и не могут быть изменены."));
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
