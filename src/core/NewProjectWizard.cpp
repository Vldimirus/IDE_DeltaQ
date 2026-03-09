// Мастер создания нового проекта — реализация
#include "NewProjectWizard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

namespace DeltaQ {

namespace {

constexpr int TemplateIdRole = Qt::UserRole + 1;
constexpr int SummaryPageId = 2;

QString renderTemplatePath(QString path, const QString &projectName)
{
    path.replace("__PROJECT_NAME__", projectName);
    return path;
}

} // namespace

// Страница с валидацией: имя и директория не пусты
class NameLocationPage : public QWizardPage {
public:
    explicit NameLocationPage(QWidget *parent = nullptr)
        : QWizardPage(parent) {}

    void setEdits(QLineEdit *nameEdit, QLineEdit *dirEdit) {
        m_nameEdit = nameEdit;
        m_dirEdit = dirEdit;
        // Обновляем состояние кнопки Next при изменении полей
        connect(m_nameEdit, &QLineEdit::textChanged,
                this, &QWizardPage::completeChanged);
        connect(m_dirEdit, &QLineEdit::textChanged,
                this, &QWizardPage::completeChanged);
    }

    bool isComplete() const override {
        return m_nameEdit && m_dirEdit
            && !m_nameEdit->text().trimmed().isEmpty()
            && !m_dirEdit->text().isEmpty();
    }

private:
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_dirEdit = nullptr;
};

NewProjectWizard::NewProjectWizard(QWidget *parent)
    : QWizard(parent)
    , m_templates(ProjectTemplates::availableTemplates())
{
    setWindowTitle(tr("New Project"));
    setMinimumSize(550, 400);

    addPage(createTypePage());
    addPage(createNamePage());
    addPage(createSummaryPage());
}

// --- Публичный API ---

QString NewProjectWizard::projectName() const
{
    return m_nameEdit->text().trimmed();
}

QString NewProjectWizard::projectDir() const
{
    return m_dirEdit->text();
}

QString NewProjectWizard::projectType() const
{
    const ProjectTemplateInfo *info = selectedTemplate();
    return info ? info->projectType : QString("console");
}

QString NewProjectWizard::selectedTemplateId() const
{
    if (!m_typeList || !m_typeList->currentItem())
        return {};
    return m_typeList->currentItem()->data(TemplateIdRole).toString();
}

// --- Страница 1: Тип проекта ---

QWizardPage *NewProjectWizard::createTypePage()
{
    auto *page = new QWizardPage;
    page->setTitle(tr("Project Type"));
    page->setSubTitle(tr("Select the type of project to create."));

    m_typeList = new QListWidget;
    auto *descLabel = new QLabel;
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: gray; padding: 8px;");

    for (const auto &tmpl : m_templates) {
        auto *item = new QListWidgetItem(tmpl.name);
        item->setData(TemplateIdRole, tmpl.id);
        item->setToolTip(tmpl.description);
        m_typeList->addItem(item);
    }

    if (m_typeList->count() > 0)
        m_typeList->setCurrentRow(0);

    auto updateDescription = [descLabel, this]() {
        const ProjectTemplateInfo *tmpl = selectedTemplate();
        descLabel->setText(tmpl ? tmpl->description : tr("No project templates were found."));
    };
    connect(m_typeList, &QListWidget::currentRowChanged, this, updateDescription);
    updateDescription();

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_typeList);
    layout->addWidget(descLabel);
    page->setLayout(layout);

    return page;
}

// --- Страница 2: Имя и расположение ---

QWizardPage *NewProjectWizard::createNamePage()
{
    auto *page = new NameLocationPage;
    page->setTitle(tr("Name and Location"));
    page->setSubTitle(tr("Enter the project name and select a directory."));

    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText(tr("ProjectName"));
    // Валидация: буквы, цифры, подчёркивание, без пробелов
    auto *validator = new QRegularExpressionValidator(
        QRegularExpression("^[A-Za-z_][A-Za-z0-9_]*$"), m_nameEdit);
    m_nameEdit->setValidator(validator);

    m_dirEdit = new QLineEdit;
    m_dirEdit->setPlaceholderText(tr("/path/to/projects"));
    m_dirEdit->setText(QDir::homePath());

    page->setEdits(m_nameEdit, m_dirEdit);

    auto *browseBtn = new QPushButton(tr("Browse..."));
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(
            this, tr("Select Project Directory"), m_dirEdit->text());
        if (!dir.isEmpty())
            m_dirEdit->setText(dir);
    });

    m_fullPathLabel = new QLabel;
    m_fullPathLabel->setStyleSheet("color: gray; padding: 4px;");

    auto updatePath = [this]() {
        QString name = m_nameEdit->text().trimmed();
        QString dir = m_dirEdit->text();
        if (!name.isEmpty() && !dir.isEmpty())
            m_fullPathLabel->setText(tr("Project path: %1/%2").arg(dir, name));
        else
            m_fullPathLabel->setText(QString());
    };
    connect(m_nameEdit, &QLineEdit::textChanged, this, updatePath);
    connect(m_dirEdit, &QLineEdit::textChanged, this, updatePath);

    auto *dirLayout = new QHBoxLayout;
    dirLayout->addWidget(m_dirEdit);
    dirLayout->addWidget(browseBtn);

    auto *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(tr("Project name:")));
    layout->addWidget(m_nameEdit);
    layout->addSpacing(10);
    layout->addWidget(new QLabel(tr("Directory:")));
    layout->addLayout(dirLayout);
    layout->addSpacing(10);
    layout->addWidget(m_fullPathLabel);
    layout->addStretch();
    page->setLayout(layout);

    return page;
}

// --- Страница 3: Сводка ---

QWizardPage *NewProjectWizard::createSummaryPage()
{
    auto *page = new QWizardPage;
    page->setTitle(tr("Summary"));
    page->setSubTitle(tr("Review the project settings before creation."));

    m_summaryLabel = new QLabel;
    m_summaryLabel->setWordWrap(true);
    m_summaryLabel->setTextFormat(Qt::RichText);

    // Обновляем сводку при показе страницы
    connect(this, &QWizard::currentIdChanged, this, [this](int id) {
        if (id == SummaryPageId)
            updateSummary();
    });

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_summaryLabel);
    layout->addStretch();
    page->setLayout(layout);

    return page;
}

void NewProjectWizard::updateSummary()
{
    const QString name = projectName();
    const QString dir = projectDir() + "/" + name;
    const ProjectTemplateInfo *tmpl = selectedTemplate();

    QString files = "<ul>";
    for (const QString &path : ProjectTemplates::summaryFiles(selectedTemplateId(), name))
        files += "<li>" + renderTemplatePath(path, name).toHtmlEscaped() + "</li>";
    files += "</ul>";

    m_summaryLabel->setText(
        "<b>" + tr("Template:") + "</b> "
            + (tmpl ? tmpl->name.toHtmlEscaped() : tr("Unknown")) + "<br><br>"
        "<b>" + tr("Type:") + "</b> " + projectType().toHtmlEscaped() + "<br><br>"
        "<b>" + tr("Name:") + "</b> " + name.toHtmlEscaped() + "<br><br>"
        "<b>" + tr("Path:") + "</b> " + dir.toHtmlEscaped() + "<br><br>"
        "<b>" + tr("Files to be created:") + "</b>" + files
    );
}

void NewProjectWizard::setDefaultDir(const QString &dir)
{
    if (m_dirEdit && !dir.isEmpty())
        m_dirEdit->setText(dir);
}

const ProjectTemplateInfo *NewProjectWizard::selectedTemplate() const
{
    const QString id = selectedTemplateId();
    for (const auto &tmpl : m_templates) {
        if (tmpl.id == id)
            return &tmpl;
    }
    return nullptr;
}

} // namespace DeltaQ
