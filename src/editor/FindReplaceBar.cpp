// Панель поиска и замены в редакторе кода
#include "FindReplaceBar.h"
#include <QKeyEvent>
#include <QTextBlock>
#include <QApplication>

namespace DeltaQ {

FindReplaceBar::FindReplaceBar(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    hide(); // по умолчанию скрыта
}

void FindReplaceBar::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 2, 4, 2);
    mainLayout->setSpacing(2);

    // --- Строка поиска ---
    auto *findRow = new QHBoxLayout();
    findRow->setSpacing(4);

    m_findEdit = new QLineEdit(this);
    m_findEdit->setPlaceholderText(tr("Find..."));
    m_findEdit->setClearButtonEnabled(true);
    m_findEdit->setMinimumWidth(200);
    findRow->addWidget(m_findEdit);

    m_findPrevBtn = new QPushButton(tr("Previous"), this);
    m_findPrevBtn->setFixedWidth(80);
    findRow->addWidget(m_findPrevBtn);

    m_findNextBtn = new QPushButton(tr("Next"), this);
    m_findNextBtn->setFixedWidth(80);
    findRow->addWidget(m_findNextBtn);

    m_caseSensitive = new QCheckBox(tr("Case"), this);
    m_caseSensitive->setToolTip(tr("Case sensitive"));
    findRow->addWidget(m_caseSensitive);

    m_wholeWords = new QCheckBox(tr("Words"), this);
    m_wholeWords->setToolTip(tr("Whole words only"));
    findRow->addWidget(m_wholeWords);

    m_matchLabel = new QLabel(this);
    m_matchLabel->setMinimumWidth(70);
    findRow->addWidget(m_matchLabel);

    findRow->addStretch();

    m_closeBtn = new QPushButton("×", this);
    m_closeBtn->setFixedSize(24, 24);
    m_closeBtn->setFlat(true);
    m_closeBtn->setToolTip(tr("Close (Esc)"));
    findRow->addWidget(m_closeBtn);

    mainLayout->addLayout(findRow);

    // --- Строка замены ---
    m_replaceRow = new QWidget(this);
    auto *replaceLayout = new QHBoxLayout(m_replaceRow);
    replaceLayout->setContentsMargins(0, 0, 0, 0);
    replaceLayout->setSpacing(4);

    m_replaceEdit = new QLineEdit(m_replaceRow);
    m_replaceEdit->setPlaceholderText(tr("Replace..."));
    m_replaceEdit->setClearButtonEnabled(true);
    m_replaceEdit->setMinimumWidth(200);
    replaceLayout->addWidget(m_replaceEdit);

    m_replaceBtn = new QPushButton(tr("Replace"), m_replaceRow);
    m_replaceBtn->setFixedWidth(80);
    replaceLayout->addWidget(m_replaceBtn);

    m_replaceAllBtn = new QPushButton(tr("Replace All"), m_replaceRow);
    m_replaceAllBtn->setFixedWidth(80);
    replaceLayout->addWidget(m_replaceAllBtn);

    replaceLayout->addStretch();

    mainLayout->addWidget(m_replaceRow);

    // Подключения
    connect(m_findEdit, &QLineEdit::textChanged, this, &FindReplaceBar::onFindTextChanged);
    connect(m_findEdit, &QLineEdit::returnPressed, this, &FindReplaceBar::findNext);
    connect(m_findNextBtn, &QPushButton::clicked, this, &FindReplaceBar::findNext);
    connect(m_findPrevBtn, &QPushButton::clicked, this, &FindReplaceBar::findPrevious);
    connect(m_replaceBtn, &QPushButton::clicked, this, &FindReplaceBar::replace);
    connect(m_replaceAllBtn, &QPushButton::clicked, this, &FindReplaceBar::replaceAll);
    connect(m_closeBtn, &QPushButton::clicked, this, &FindReplaceBar::close);

    // При изменении опций — обновить подсветку
    connect(m_caseSensitive, &QCheckBox::toggled, this, [this]() { onFindTextChanged(m_findEdit->text()); });
    connect(m_wholeWords, &QCheckBox::toggled, this, [this]() { onFindTextChanged(m_findEdit->text()); });
}

void FindReplaceBar::setEditor(QPlainTextEdit *editor)
{
    if (m_editor)
        clearHighlights();
    m_editor = editor;
    if (isVisible())
        onFindTextChanged(m_findEdit->text());
}

void FindReplaceBar::showFind()
{
    m_replaceRow->hide();
    show();
    m_findEdit->setFocus();
    m_findEdit->selectAll();

    // Если есть выделение в редакторе — подставить в поле поиска
    if (m_editor) {
        QString selected = m_editor->textCursor().selectedText();
        if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator))
            m_findEdit->setText(selected);
    }
}

void FindReplaceBar::showReplace()
{
    m_replaceRow->show();
    show();
    m_findEdit->setFocus();
    m_findEdit->selectAll();

    if (m_editor) {
        QString selected = m_editor->textCursor().selectedText();
        if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator))
            m_findEdit->setText(selected);
    }
}

QString FindReplaceBar::findText() const
{
    return m_findEdit->text();
}

void FindReplaceBar::findNext()
{
    if (!m_editor || m_findEdit->text().isEmpty())
        return;

    auto flags = buildFlags(false);
    bool found = m_editor->find(m_findEdit->text(), flags);

    // Если не найдено — попробовать с начала документа
    if (!found) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::Start);
        m_editor->setTextCursor(cursor);
        m_editor->find(m_findEdit->text(), flags);
    }
}

void FindReplaceBar::findPrevious()
{
    if (!m_editor || m_findEdit->text().isEmpty())
        return;

    auto flags = buildFlags(true);
    bool found = m_editor->find(m_findEdit->text(), flags);

    // Если не найдено — попробовать с конца документа
    if (!found) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_editor->setTextCursor(cursor);
        m_editor->find(m_findEdit->text(), flags);
    }
}

void FindReplaceBar::replace()
{
    if (!m_editor || m_findEdit->text().isEmpty())
        return;

    QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == m_findEdit->text()) {
        cursor.insertText(m_replaceEdit->text());
        updateMatchCount();
    }
    findNext();
}

void FindReplaceBar::replaceAll()
{
    if (!m_editor || m_findEdit->text().isEmpty())
        return;

    QTextCursor cursor = m_editor->textCursor();
    cursor.beginEditBlock();

    // Перемещаемся в начало документа
    cursor.movePosition(QTextCursor::Start);
    m_editor->setTextCursor(cursor);

    auto flags = buildFlags(false);
    int count = 0;

    while (m_editor->find(m_findEdit->text(), flags)) {
        QTextCursor found = m_editor->textCursor();
        found.insertText(m_replaceEdit->text());
        ++count;
    }

    cursor.endEditBlock();
    updateMatchCount();
}

void FindReplaceBar::close()
{
    clearHighlights();
    hide();
    if (m_editor)
        m_editor->setFocus();
}

void FindReplaceBar::onFindTextChanged(const QString &text)
{
    Q_UNUSED(text)
    highlightAllMatches();
    updateMatchCount();
}

void FindReplaceBar::updateMatchCount()
{
    if (!m_editor || m_findEdit->text().isEmpty()) {
        m_matchCount = 0;
        m_matchLabel->clear();
        emit matchCountChanged(0);
        return;
    }

    // Подсчёт совпадений
    QTextDocument *doc = m_editor->document();
    QTextCursor cursor(doc);
    auto flags = buildFlags(false);
    int count = 0;

    while (true) {
        cursor = doc->find(m_findEdit->text(), cursor, flags);
        if (cursor.isNull())
            break;
        ++count;
    }

    m_matchCount = count;
    if (count == 0)
        m_matchLabel->setText(tr("No matches"));
    else
        m_matchLabel->setText(tr("%1 found").arg(count));

    emit matchCountChanged(count);
}

QTextDocument::FindFlags FindReplaceBar::buildFlags(bool backward) const
{
    QTextDocument::FindFlags flags;
    if (m_caseSensitive->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (m_wholeWords->isChecked())
        flags |= QTextDocument::FindWholeWords;
    if (backward)
        flags |= QTextDocument::FindBackward;
    return flags;
}

void FindReplaceBar::highlightAllMatches()
{
    clearHighlights();
    if (!m_editor || m_findEdit->text().isEmpty())
        return;

    QTextDocument *doc = m_editor->document();
    QTextCursor cursor(doc);
    auto flags = buildFlags(false);

    QList<QTextEdit::ExtraSelection> selections = m_editor->extraSelections();

    QTextCharFormat fmt;
    fmt.setBackground(QColor(255, 255, 0, 80)); // Полупрозрачный жёлтый

    while (true) {
        cursor = doc->find(m_findEdit->text(), cursor, flags);
        if (cursor.isNull())
            break;

        QTextEdit::ExtraSelection sel;
        sel.cursor = cursor;
        sel.format = fmt;
        selections.append(sel);
    }

    m_editor->setExtraSelections(selections);
}

void FindReplaceBar::clearHighlights()
{
    if (!m_editor)
        return;

    // Оставляем только подсветку текущей строки (первый элемент)
    auto selections = m_editor->extraSelections();
    if (selections.size() > 1)
        selections = selections.mid(0, 1);
    m_editor->setExtraSelections(selections);
}

} // namespace DeltaQ
