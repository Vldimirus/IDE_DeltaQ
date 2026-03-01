// Диалог настроек IDE — реализация
#include "SettingsDialog.h"
#include "SessionManager.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QCheckBox>
#include <QMessageBox>

namespace DeltaQ {

SettingsDialog::SettingsDialog(SessionManager *session, QWidget *parent)
    : QDialog(parent)
    , m_session(session)
{
    setWindowTitle(tr("Settings"));
    setMinimumSize(480, 360);

    auto *mainLayout = new QVBoxLayout(this);

    auto *tabs = new QTabWidget(this);

    // === Вкладка «Общие» ===
    auto *generalPage = new QWidget;
    auto *generalLayout = new QFormLayout(generalPage);

    m_defaultDirEdit = new QLineEdit(m_session->defaultProjectDir(), generalPage);
    auto *browseBtn = new QPushButton(tr("Browse..."), generalPage);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(
            this, tr("Default Project Directory"), m_defaultDirEdit->text());
        if (!dir.isEmpty())
            m_defaultDirEdit->setText(dir);
    });

    auto *dirRow = new QHBoxLayout;
    dirRow->addWidget(m_defaultDirEdit);
    dirRow->addWidget(browseBtn);
    generalLayout->addRow(tr("Projects directory:"), dirRow);

    tabs->addTab(generalPage, tr("General"));

    // === Вкладка «Редактор» ===
    auto *editorPage = new QWidget;
    auto *editorLayout = new QFormLayout(editorPage);

    m_fontCombo = new QFontComboBox(editorPage);
    m_fontCombo->setCurrentFont(QFont(m_session->fontFamily()));
    editorLayout->addRow(tr("Font:"), m_fontCombo);

    m_fontSizeSpin = new QSpinBox(editorPage);
    m_fontSizeSpin->setRange(8, 48);
    m_fontSizeSpin->setValue(m_session->fontSize());
    editorLayout->addRow(tr("Font size:"), m_fontSizeSpin);

    m_tabWidthSpin = new QSpinBox(editorPage);
    m_tabWidthSpin->setRange(1, 16);
    m_tabWidthSpin->setValue(m_session->tabWidth());
    editorLayout->addRow(tr("Tab width:"), m_tabWidthSpin);

    tabs->addTab(editorPage, tr("Editor"));

    // === Вкладка «Язык» ===
    auto *langPage = new QWidget;
    auto *langLayout = new QFormLayout(langPage);

    m_langCombo = new QComboBox(langPage);
    m_langCombo->addItem("English", "en");
    m_langCombo->addItem(QString::fromUtf8("Русский"), "ru");
    // Выбираем текущий
    int langIdx = m_langCombo->findData(m_session->language());
    if (langIdx >= 0)
        m_langCombo->setCurrentIndex(langIdx);
    langLayout->addRow(tr("Interface language:"), m_langCombo);

    auto *langNote = new QLabel(tr("Language change requires restart."), langPage);
    langNote->setStyleSheet("color: gray; font-style: italic;");
    langLayout->addRow(langNote);

    tabs->addTab(langPage, tr("Language"));

    mainLayout->addWidget(tabs);

    // Кнопки OK / Cancel
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        apply();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

void SettingsDialog::apply()
{
    // Общие
    m_session->setDefaultProjectDir(m_defaultDirEdit->text());

    // Редактор
    m_session->setFontFamily(m_fontCombo->currentFont().family());
    m_session->setFontSize(m_fontSizeSpin->value());
    m_session->setTabWidth(m_tabWidthSpin->value());

    // Язык
    QString newLang = m_langCombo->currentData().toString();
    if (newLang != m_session->language()) {
        m_session->setLanguage(newLang);
    }
}

} // namespace DeltaQ
