// Диалог создания нового файла — реализация
#include "NewFileDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

namespace DeltaQ {

NewFileDialog::NewFileDialog(const QString &projectDir, QWidget *parent)
    : QDialog(parent)
    , m_projectDir(projectDir)
{
    setWindowTitle(tr("New File"));
    setMinimumSize(420, 340);

    auto *mainLayout = new QVBoxLayout(this);

    // Тип файла
    mainLayout->addWidget(new QLabel(tr("File type:"), this));
    m_typeList = new QListWidget(this);
    m_typeList->addItem(tr("UI Window (.dqui)"));
    m_typeList->addItem(tr("Graph (.dqgraph)"));
    m_typeList->addItem(tr("Module (.dqmod)"));
    m_typeList->addItem(tr("C Source (.c)"));
    m_typeList->addItem(tr("C Header (.h)"));
    m_typeList->setCurrentRow(0);
    mainLayout->addWidget(m_typeList);

    // Имя файла
    mainLayout->addWidget(new QLabel(tr("Name:"), this));
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("file_name"));
    auto *validator = new QRegularExpressionValidator(
        QRegularExpression("^[A-Za-z_][A-Za-z0-9_]*$"), m_nameEdit);
    m_nameEdit->setValidator(validator);
    mainLayout->addWidget(m_nameEdit);

    // Превью полного пути
    m_pathLabel = new QLabel(this);
    m_pathLabel->setStyleSheet("color: gray; padding: 4px;");
    m_pathLabel->setWordWrap(true);
    mainLayout->addWidget(m_pathLabel);

    mainLayout->addStretch();

    // Кнопки
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    mainLayout->addWidget(buttons);

    connect(m_typeList, &QListWidget::currentRowChanged, this, [this]() { updatePreview(); });
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this, buttons]() {
        updatePreview();
        buttons->button(QDialogButtonBox::Ok)->setEnabled(!m_nameEdit->text().trimmed().isEmpty());
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updatePreview();
}

QString NewFileDialog::fileType() const
{
    switch (m_typeList->currentRow()) {
    case 0: return "dqui";
    case 1: return "dqgraph";
    case 2: return "dqmod";
    case 3: return "c";
    case 4: return "h";
    default: return "c";
    }
}

QString NewFileDialog::fileName() const
{
    return m_nameEdit->text().trimmed();
}

QString NewFileDialog::fullPath() const
{
    QString name = fileName();
    if (name.isEmpty()) return {};

    QString type = fileType();
    QString subDir;
    if (type == "dqui")         subDir = "ui";
    else if (type == "dqgraph") subDir = "graphs";
    else if (type == "dqmod")   subDir = "modules";
    else                        subDir = "src";

    return m_projectDir + "/" + subDir + "/" + name + "." + type;
}

void NewFileDialog::updatePreview()
{
    QString path = fullPath();
    if (path.isEmpty())
        m_pathLabel->setText(tr("Enter a file name"));
    else
        m_pathLabel->setText(tr("Path: %1").arg(path));
}

} // namespace DeltaQ
