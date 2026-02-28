// Мастер импорта библиотеки — реализация
#include "LibraryImportWizard.h"
#include "../core/ModuleRegistry.h"

#include <QLineEdit>
#include <QTreeWidget>
#include <QComboBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QHeaderView>
#include <QWizardPage>
#include <QCheckBox>
#include <QSplitter>

namespace DeltaQ {

LibraryImportWizard::LibraryImportWizard(ModuleRegistry *registry, QWidget *parent)
    : QWizard(parent)
    , m_registry(registry)
{
    setWindowTitle(tr("Import Library"));
    setMinimumSize(700, 500);

    setupPage1_SelectLibrary();
    setupPage2_Parse();
    setupPage3_SelectElements();
    setupPage4_ConfigureModules();
    setupPage5_Generate();
}

// --- Страница 1: Выбор библиотеки ---

void LibraryImportWizard::setupPage1_SelectLibrary()
{
    auto *page = new QWizardPage;
    page->setTitle(tr("Select Library"));
    page->setSubTitle(tr("Choose header files, include paths, and compiler settings."));

    auto *layout = new QVBoxLayout(page);

    // Путь к заголовку
    auto *hdrLayout = new QHBoxLayout;
    auto *hdrLabel = new QLabel(tr("Header file(s):"), page);
    m_headerPathEdit = new QLineEdit(page);
    m_headerPathEdit->setPlaceholderText(tr("/path/to/library.h"));
    auto *browseBtn = new QPushButton(tr("Browse..."), page);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Select Header"),
                                                     QString(), tr("Headers (*.h *.hpp)"));
        if (!path.isEmpty())
            m_headerPathEdit->setText(path);
    });
    hdrLayout->addWidget(hdrLabel);
    hdrLayout->addWidget(m_headerPathEdit, 1);
    hdrLayout->addWidget(browseBtn);
    layout->addLayout(hdrLayout);

    // Include paths
    auto *incLabel = new QLabel(tr("Include paths (semicolon-separated):"), page);
    m_includePathsEdit = new QLineEdit(page);
    m_includePathsEdit->setPlaceholderText("/usr/include;/usr/local/include");
    layout->addWidget(incLabel);
    layout->addWidget(m_includePathsEdit);

    // Defines
    auto *defLabel = new QLabel(tr("Defines (semicolon-separated):"), page);
    m_definesEdit = new QLineEdit(page);
    m_definesEdit->setPlaceholderText("MY_LIB_STATIC;VERSION=2");
    layout->addWidget(defLabel);
    layout->addWidget(m_definesEdit);

    // Стандарт
    auto *stdLayout = new QHBoxLayout;
    auto *stdLabel = new QLabel(tr("Standard:"), page);
    m_standardCombo = new QComboBox(page);
    m_standardCombo->addItems({"c11", "c17", "c++17", "c++20"});
    m_standardCombo->setCurrentText("c17");
    stdLayout->addWidget(stdLabel);
    stdLayout->addWidget(m_standardCombo);
    stdLayout->addStretch();
    layout->addLayout(stdLayout);

    layout->addStretch();
    addPage(page);
}

// --- Страница 2: Парсинг ---

void LibraryImportWizard::setupPage2_Parse()
{
    auto *page = new QWizardPage;
    page->setTitle(tr("Parse Results"));
    page->setSubTitle(tr("Review the parsed declarations from the header files."));

    auto *layout = new QVBoxLayout(page);

    m_parseStatsLabel = new QLabel(page);
    layout->addWidget(m_parseStatsLabel);

    m_parseResultTree = new QTreeWidget(page);
    m_parseResultTree->setHeaderLabels({tr("Name"), tr("Type"), tr("Details")});
    m_parseResultTree->header()->setStretchLastSection(true);
    layout->addWidget(m_parseResultTree);

    // Парсинг при входе на страницу
    connect(this, &QWizard::currentIdChanged, this, [this](int id) {
        if (id != 1) return; // Страница 2

        m_parseResultTree->clear();
        m_parseStatsLabel->clear();

        LibclangParser parser;
        parser.setStandard(m_standardCombo->currentText());

        if (!m_includePathsEdit->text().isEmpty())
            parser.setIncludePaths(m_includePathsEdit->text().split(';', Qt::SkipEmptyParts));
        if (!m_definesEdit->text().isEmpty())
            parser.setDefines(m_definesEdit->text().split(';', Qt::SkipEmptyParts));

        m_parseResult = parser.parseHeader(m_headerPathEdit->text());

        // Заполняем дерево
        auto *funcRoot = new QTreeWidgetItem(m_parseResultTree, {
            tr("Functions"), QString::number(m_parseResult.functions.size()), ""});
        for (const auto &f : m_parseResult.functions) {
            new QTreeWidgetItem(funcRoot, {f.name, f.returnType,
                QString("%1 params").arg(f.parameters.size())});
        }

        auto *structRoot = new QTreeWidgetItem(m_parseResultTree, {
            tr("Structs"), QString::number(m_parseResult.structs.size()), ""});
        for (const auto &s : m_parseResult.structs) {
            new QTreeWidgetItem(structRoot, {s.name, "struct",
                QString("%1 fields").arg(s.fields.size())});
        }

        auto *enumRoot = new QTreeWidgetItem(m_parseResultTree, {
            tr("Enums"), QString::number(m_parseResult.enums.size()), ""});
        for (const auto &e : m_parseResult.enums) {
            new QTreeWidgetItem(enumRoot, {e.name, "enum",
                QString("%1 constants").arg(e.constants.size())});
        }

        auto *classRoot = new QTreeWidgetItem(m_parseResultTree, {
            tr("Classes"), QString::number(m_parseResult.classes.size()), ""});
        for (const auto &c : m_parseResult.classes) {
            new QTreeWidgetItem(classRoot, {c.name, "class",
                QString("%1 methods").arg(c.methods.size())});
        }

        m_parseResultTree->expandAll();

        // Статистика
        int total = m_parseResult.functions.size() + m_parseResult.structs.size() +
                    m_parseResult.enums.size() + m_parseResult.classes.size();
        m_parseStatsLabel->setText(tr("Found %1 declarations, %2 errors, %3 warnings")
            .arg(total)
            .arg(m_parseResult.errors.size())
            .arg(m_parseResult.warnings.size()));

        if (!m_parseResult.errors.isEmpty()) {
            auto *errRoot = new QTreeWidgetItem(m_parseResultTree, {tr("Errors"), "", ""});
            for (const auto &err : m_parseResult.errors)
                new QTreeWidgetItem(errRoot, {err, "", ""});
        }
    });

    addPage(page);
}

// --- Страница 3: Выбор элементов ---

void LibraryImportWizard::setupPage3_SelectElements()
{
    auto *page = new QWizardPage;
    page->setTitle(tr("Select Elements"));
    page->setSubTitle(tr("Choose which declarations to import as modules."));

    auto *layout = new QVBoxLayout(page);

    m_selectTree = new QTreeWidget(page);
    m_selectTree->setHeaderLabels({tr("Name"), tr("Type")});
    layout->addWidget(m_selectTree);

    connect(this, &QWizard::currentIdChanged, this, [this](int id) {
        if (id != 2) return;

        m_selectTree->clear();

        for (const auto &f : m_parseResult.functions) {
            auto *item = new QTreeWidgetItem(m_selectTree, {f.name, "function"});
            item->setCheckState(0, Qt::Checked);
            item->setData(0, Qt::UserRole, "function");
        }
        for (const auto &c : m_parseResult.classes) {
            auto *item = new QTreeWidgetItem(m_selectTree, {c.name, "class"});
            item->setCheckState(0, Qt::Checked);
            item->setData(0, Qt::UserRole, "class");
        }
    });

    addPage(page);
}

// --- Страница 4: Настройка модулей ---

void LibraryImportWizard::setupPage4_ConfigureModules()
{
    auto *page = new QWizardPage;
    page->setTitle(tr("Configure Modules"));
    page->setSubTitle(tr("Review and configure generated modules."));

    auto *layout = new QVBoxLayout(page);
    auto *splitter = new QSplitter(Qt::Horizontal, page);

    m_modulesTree = new QTreeWidget;
    m_modulesTree->setHeaderLabels({tr("Module"), tr("Ports"), tr("Origin")});
    splitter->addWidget(m_modulesTree);

    m_wrapperPreview = new QTextEdit;
    m_wrapperPreview->setReadOnly(true);
    m_wrapperPreview->setFont(QFont("Monospace", 9));
    splitter->addWidget(m_wrapperPreview);

    splitter->setSizes({350, 350});
    layout->addWidget(splitter);

    connect(this, &QWizard::currentIdChanged, this, [this](int id) {
        if (id != 3) return;

        // Фильтрация по выбранным элементам
        QStringList excludePatterns;
        for (int i = 0; i < m_selectTree->topLevelItemCount(); ++i) {
            auto *item = m_selectTree->topLevelItem(i);
            if (item->checkState(0) != Qt::Checked) {
                excludePatterns.append("^" + QRegularExpression::escape(item->text(0)) + "$");
            }
        }

        // Определяем язык
        QString std = m_standardCombo->currentText();
        m_options.language = std.startsWith("c++") ? "cpp" : "c";
        m_options.excludePatterns = excludePatterns;

        DecompositionResult dr = LibraryDecomposer::decompose(m_parseResult, m_options);
        m_modules = dr.modules;

        // Генерация обёрток
        if (m_options.language == "cpp") {
            m_wrappers = WrapperGenerator::generateCWrapper(m_parseResult.classes,
                                                             m_headerPathEdit->text());
        }

        // Заполняем дерево модулей
        m_modulesTree->clear();
        for (const auto &mod : m_modules) {
            int ports = mod.inputs.size() + mod.outputs.size();
            new QTreeWidgetItem(m_modulesTree, {
                mod.name,
                QString::number(ports),
                mod.origin
            });
        }

        // Предпросмотр обёрточного кода
        if (!m_wrappers.isEmpty()) {
            m_wrapperPreview->clear();
            for (const auto &w : m_wrappers) {
                m_wrapperPreview->append("// === " + w.className + " wrapper ===\n");
                m_wrapperPreview->append(w.header);
                m_wrapperPreview->append("\n// --- Source ---\n");
                m_wrapperPreview->append(w.source);
                m_wrapperPreview->append("\n");
            }
        } else {
            m_wrapperPreview->setPlainText(tr("No C++ wrappers needed (C library)."));
        }
    });

    addPage(page);
}

// --- Страница 5: Генерация ---

void LibraryImportWizard::setupPage5_Generate()
{
    auto *page = new QWizardPage;
    page->setTitle(tr("Import Complete"));
    page->setSubTitle(tr("Modules have been generated and registered."));

    auto *layout = new QVBoxLayout(page);

    m_progressBar = new QProgressBar(page);
    layout->addWidget(m_progressBar);

    m_resultLabel = new QLabel(page);
    layout->addWidget(m_resultLabel);

    layout->addStretch();

    connect(this, &QWizard::currentIdChanged, this, [this](int id) {
        if (id != 4) return;

        m_progressBar->setMaximum(m_modules.size());
        int count = 0;

        for (auto &mod : m_modules) {
            if (m_registry)
                m_registry->registerModule(mod);
            ++count;
            m_progressBar->setValue(count);
        }

        m_resultLabel->setText(tr("Successfully imported %1 module(s).").arg(count));
    });

    addPage(page);
}

} // namespace DeltaQ
