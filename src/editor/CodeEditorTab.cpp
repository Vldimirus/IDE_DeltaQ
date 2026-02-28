// Вкладка редактора кода с нумерацией строк
#include "CodeEditorTab.h"
#include "SyntaxHighlighter.h"
#include "../core/CommandBus.h"

#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QPainter>
#include <QTextBlock>
#include <QFont>
#include <QScrollBar>

namespace DeltaQ {

// --- LineNumberArea ---

LineNumberArea::LineNumberArea(CodePlainTextEdit *editor)
    : QWidget(editor)
    , m_editor(editor)
{
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(static_cast<CodeEditorTab *>(parent()->parent())->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    auto *tab = static_cast<CodeEditorTab *>(parent()->parent());
    QPainter painter(this);
    painter.fillRect(event->rect(), QColor(245, 245, 245));

    QTextBlock block = m_editor->getFirstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(m_editor->getBlockBoundingGeometry(block)
                      .translated(m_editor->getContentOffset()).top());
    int bottom = top + qRound(m_editor->getBlockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(130, 130, 130));
            painter.drawText(0, top, width() - 4, m_editor->fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(m_editor->getBlockBoundingRect(block).height());
        ++blockNumber;
    }
}

// --- CodeEditorTab ---

CodeEditorTab::CodeEditorTab(const QString &filePath, CommandBus *bus, QWidget *parent)
    : QWidget(parent)
    , m_filePath(filePath)
    , m_commandBus(bus)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_editor = new CodePlainTextEdit(this);
    m_lineNumberArea = new LineNumberArea(m_editor);
    m_highlighter = new SyntaxHighlighter(m_editor->document());

    layout->addWidget(m_editor);
    setupEditor();
}

void CodeEditorTab::setupEditor()
{
    QFont font("Monospace", 11);
    font.setFixedPitch(true);
    m_editor->setFont(font);
    m_editor->setTabStopDistance(QFontMetricsF(font).horizontalAdvance(' ') * 4);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);

    // Подключаем обновление области нумерации строк
    connect(m_editor, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditorTab::updateLineNumberAreaWidth);
    connect(m_editor, &QPlainTextEdit::updateRequest,
            this, &CodeEditorTab::updateLineNumberArea);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditorTab::highlightCurrentLine);

    connect(m_editor->document(), &QTextDocument::modificationChanged,
            this, &CodeEditorTab::modificationChanged);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

bool CodeEditorTab::loadFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    m_editor->setPlainText(in.readAll());
    m_editor->document()->setModified(false);
    m_filePath = path;
    return true;
}

bool CodeEditorTab::saveFile()
{
    return saveFileAs(m_filePath);
}

bool CodeEditorTab::saveFileAs(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << m_editor->toPlainText();
    m_editor->document()->setModified(false);
    m_filePath = path;
    return true;
}

bool CodeEditorTab::isModified() const
{
    return m_editor->document()->isModified();
}

int CodeEditorTab::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, m_editor->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 8 + m_editor->fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditorTab::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    m_editor->setEditorViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditorTab::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(m_editor->viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditorTab::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!m_editor->isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(255, 255, 220);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = m_editor->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    m_editor->setExtraSelections(extraSelections);
}

} // namespace DeltaQ
