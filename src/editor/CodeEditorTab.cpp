// Вкладка редактора кода с нумерацией строк
#include "CodeEditorTab.h"
#include "SyntaxHighlighter.h"
#include "CompletionPopup.h"
#include "../core/CommandBus.h"

#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QPainter>
#include <QTextBlock>
#include <QFont>
#include <QScrollBar>
#include <QMouseEvent>
#include <QToolTip>
#include <QTextCharFormat>

namespace DeltaQ {

// --- CodePlainTextEdit ---

void CodePlainTextEdit::mouseMoveEvent(QMouseEvent *event)
{
    QPlainTextEdit::mouseMoveEvent(event);
    m_lastMousePos = event->globalPosition().toPoint();

    // Перезапускаем таймер hover — 500ms задержка
    m_hoverTimer.stop();
    if (!m_hoverTimer.isActive()) {
        m_hoverTimer.setSingleShot(true);
        m_hoverTimer.setInterval(500);
        // Подключаем только если ещё не подключено
        disconnect(&m_hoverTimer, &QTimer::timeout, nullptr, nullptr);
        connect(&m_hoverTimer, &QTimer::timeout, this, [this]() {
            // Определяем позицию курсора в тексте
            QTextCursor cursor = cursorForPosition(mapFromGlobal(m_lastMousePos));
            if (!cursor.isNull()) {
                emit hoverRequested(cursor.blockNumber(), cursor.columnNumber());
            }
        });
    }
    m_hoverTimer.start();
}

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

    int markerAreaWidth = 16;
    int numberAreaWidth = width() - markerAreaWidth;

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int lineNum = blockNumber + 1;
            int lineHeight = m_editor->fontMetrics().height();

            // Рисуем маркер breakpoint — красный кружок
            if (tab->hasBreakpoint(lineNum)) {
                painter.save();
                painter.setRenderHint(QPainter::Antialiasing);
                int radius = qMin(lineHeight, markerAreaWidth) / 2 - 2;
                int cx = markerAreaWidth / 2;
                int cy = top + lineHeight / 2;
                painter.setBrush(QColor(220, 50, 50));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(QPoint(cx, cy), radius, radius);
                painter.restore();
            }

            // Рисуем маркер текущей строки отладки — жёлтая стрелка
            if (tab->m_debugCurrentLine == lineNum) {
                painter.save();
                painter.setRenderHint(QPainter::Antialiasing);
                int arrowSize = qMin(lineHeight, markerAreaWidth) / 2 - 1;
                int cx = markerAreaWidth / 2;
                int cy = top + lineHeight / 2;
                QPolygon arrow;
                arrow << QPoint(cx - arrowSize, cy - arrowSize)
                      << QPoint(cx + arrowSize, cy)
                      << QPoint(cx - arrowSize, cy + arrowSize);
                painter.setBrush(QColor(255, 200, 0));
                painter.setPen(QPen(QColor(180, 140, 0), 1));
                painter.drawPolygon(arrow);
                painter.restore();
            }

            // Номер строки
            QString number = QString::number(lineNum);
            painter.setPen(QColor(130, 130, 130));
            painter.drawText(markerAreaWidth, top, numberAreaWidth - 4,
                             lineHeight, Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(m_editor->getBlockBoundingRect(block).height());
        ++blockNumber;
    }
}

void LineNumberArea::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->position().x() < 16) {
        // Клик в области маркеров — toggle breakpoint
        QTextBlock block = m_editor->getFirstVisibleBlock();
        int top = qRound(m_editor->getBlockBoundingGeometry(block)
                          .translated(m_editor->getContentOffset()).top());
        int bottom = top + qRound(m_editor->getBlockBoundingRect(block).height());

        while (block.isValid()) {
            if (event->position().y() >= top && event->position().y() < bottom) {
                emit m_editor->breakpointToggled(block.blockNumber() + 1);
                return;
            }
            block = block.next();
            top = bottom;
            bottom = top + qRound(m_editor->getBlockBoundingRect(block).height());
        }
    }
    QWidget::mousePressEvent(event);
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
    m_completionPopup = new CompletionPopup(m_editor);

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
    m_editor->setMouseTracking(true);

    // Подключаем обновление области нумерации строк
    connect(m_editor, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditorTab::updateLineNumberAreaWidth);
    connect(m_editor, &QPlainTextEdit::updateRequest,
            this, &CodeEditorTab::updateLineNumberArea);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditorTab::updateExtraSelections);

    connect(m_editor->document(), &QTextDocument::modificationChanged,
            this, &CodeEditorTab::modificationChanged);

    // Hover-запрос
    connect(m_editor, &CodePlainTextEdit::hoverRequested,
            this, &CodeEditorTab::onHoverRequested);

    // Breakpoint toggle
    connect(m_editor, &CodePlainTextEdit::breakpointToggled,
            this, [this](int line) {
        // Toggle маркер локально
        if (m_breakpointLines.contains(line))
            m_breakpointLines.remove(line);
        else
            m_breakpointLines.insert(line);
        m_lineNumberArea->update();
        emit breakpointToggleRequested(m_filePath, line);
    });

    // Автодополнение по триггерным символам
    connect(m_editor, &QPlainTextEdit::textChanged, this, [this]() {
        QTextCursor cursor = m_editor->textCursor();
        if (cursor.atBlockStart()) return;

        // Получаем последний введённый символ
        QString blockText = cursor.block().text();
        int col = cursor.columnNumber();
        if (col == 0) return;

        QChar lastChar = blockText.at(col - 1);

        if (lastChar == '.' || lastChar == '>') {
            // Проверяем -> для >
            if (lastChar == '>' && col >= 2 && blockText.at(col - 2) == '-') {
                emit completionRequested(m_filePath, cursor.blockNumber(), col);
            } else if (lastChar == '.') {
                emit completionRequested(m_filePath, cursor.blockNumber(), col);
            }
        } else if (lastChar == ':' && col >= 2 && blockText.at(col - 2) == ':') {
            // ::
            emit completionRequested(m_filePath, cursor.blockNumber(), col);
        }
    });

    updateLineNumberAreaWidth(0);
    updateExtraSelections();
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
    // +16 для области маркеров (breakpoints, debug arrow)
    return space + 16;
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

void CodeEditorTab::updateExtraSelections()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    // 1. Подсветка текущей строки
    if (!m_editor->isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(255, 255, 220);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = m_editor->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    // 2. Подсветка парных скобок
    highlightMatchingBrackets(extraSelections);

    // 3. Подсветка диагностики (волнистое подчёркивание)
    addDiagnosticSelections(extraSelections);

    // 4. Подсветка текущей строки отладки
    addDebugLineSelection(extraSelections);

    m_editor->setExtraSelections(extraSelections);
}

void CodeEditorTab::setDiagnostics(const QVector<LSPDiagnostic> &diagnostics)
{
    m_diagnostics = diagnostics;
    updateExtraSelections();
}

void CodeEditorTab::addDiagnosticSelections(QList<QTextEdit::ExtraSelection> &selections)
{
    QTextDocument *doc = m_editor->document();

    for (const auto &diag : m_diagnostics) {
        QTextEdit::ExtraSelection sel;

        // Выбираем цвет в зависимости от severity
        QColor underlineColor;
        switch (diag.severity) {
        case DiagnosticSeverity::Error:
            underlineColor = QColor(255, 0, 0);     // красный
            break;
        case DiagnosticSeverity::Warning:
            underlineColor = QColor(255, 165, 0);   // оранжевый
            break;
        case DiagnosticSeverity::Information:
            underlineColor = QColor(0, 120, 255);    // синий
            break;
        case DiagnosticSeverity::Hint:
            underlineColor = QColor(150, 150, 150);  // серый
            break;
        }

        sel.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        sel.format.setUnderlineColor(underlineColor);
        sel.format.setToolTip(diag.message);

        // Устанавливаем курсор на диапазон диагностики
        QTextBlock startBlock = doc->findBlockByNumber(diag.range.start.line);
        QTextBlock endBlock = doc->findBlockByNumber(diag.range.end.line);

        if (!startBlock.isValid()) continue;
        if (!endBlock.isValid()) endBlock = startBlock;

        int startPos = startBlock.position() + qMin(diag.range.start.character, startBlock.length() - 1);
        int endPos = endBlock.position() + qMin(diag.range.end.character, endBlock.length() - 1);

        // Если диапазон нулевой — подчёркиваем слово
        if (startPos == endPos) {
            sel.cursor = QTextCursor(doc);
            sel.cursor.setPosition(startPos);
            sel.cursor.select(QTextCursor::WordUnderCursor);
        } else {
            sel.cursor = QTextCursor(doc);
            sel.cursor.setPosition(startPos);
            sel.cursor.setPosition(endPos, QTextCursor::KeepAnchor);
        }

        selections.append(sel);
    }
}

void CodeEditorTab::addDebugLineSelection(QList<QTextEdit::ExtraSelection> &selections)
{
    if (m_debugCurrentLine < 1) return;

    QTextBlock block = m_editor->document()->findBlockByNumber(m_debugCurrentLine - 1);
    if (!block.isValid()) return;

    QTextEdit::ExtraSelection sel;
    sel.format.setBackground(QColor(200, 255, 200)); // зелёный фон
    sel.format.setProperty(QTextFormat::FullWidthSelection, true);
    sel.cursor = QTextCursor(block);
    sel.cursor.clearSelection();
    selections.append(sel);
}

void CodeEditorTab::showHoverTooltip(const QString &text, const QPoint &globalPos)
{
    if (text.isEmpty()) {
        QToolTip::hideText();
        return;
    }
    QToolTip::showText(globalPos, text, m_editor);
}

void CodeEditorTab::onHoverRequested(int line, int character)
{
    emit hoverRequested(m_filePath, line, character);
}

// --- Маркеры отладки ---

void CodeEditorTab::addBreakpointMarker(int line)
{
    m_breakpointLines.insert(line);
    m_lineNumberArea->update();
}

void CodeEditorTab::removeBreakpointMarker(int line)
{
    m_breakpointLines.remove(line);
    m_lineNumberArea->update();
}

void CodeEditorTab::setDebugCurrentLine(int line)
{
    m_debugCurrentLine = line;
    updateExtraSelections();
    m_lineNumberArea->update();
}

void CodeEditorTab::clearDebugCurrentLine()
{
    m_debugCurrentLine = -1;
    updateExtraSelections();
    m_lineNumberArea->update();
}

// --- Автодополнение ---

void CodeEditorTab::showCompletion(const QVector<CompletionItem> &items)
{
    if (!m_completionPopup) return;

    if (items.isEmpty()) {
        m_completionPopup->hide();
        return;
    }

    m_completionPopup->setItems(items);

    // Позиционируем popup рядом с курсором
    QTextCursor cursor = m_editor->textCursor();
    QRect cursorRect = m_editor->cursorRect(cursor);
    QPoint pos = m_editor->mapToGlobal(cursorRect.bottomLeft());
    m_completionPopup->popup(pos);
}

// --- Подсветка парных скобок ---

void CodeEditorTab::highlightMatchingBrackets(QList<QTextEdit::ExtraSelection> &selections)
{
    QTextCursor cursor = m_editor->textCursor();
    QTextDocument *doc = m_editor->document();

    // Проверяем символ под курсором и перед курсором
    int pos = cursor.position();
    QChar charAtCursor, charBeforeCursor;
    if (pos < doc->characterCount())
        charAtCursor = doc->characterAt(pos);
    if (pos > 0)
        charBeforeCursor = doc->characterAt(pos - 1);

    // Определяем тип скобки и направление поиска
    static const QString openBrackets = "({[";
    static const QString closeBrackets = ")}]";

    QChar bracket;
    QChar matchBracket;
    int searchPos = -1;
    int direction = 0; // 1 = вперёд, -1 = назад

    if (openBrackets.contains(charAtCursor)) {
        bracket = charAtCursor;
        matchBracket = closeBrackets[openBrackets.indexOf(bracket)];
        searchPos = pos;
        direction = 1;
    } else if (closeBrackets.contains(charAtCursor)) {
        bracket = charAtCursor;
        matchBracket = openBrackets[closeBrackets.indexOf(bracket)];
        searchPos = pos;
        direction = -1;
    } else if (openBrackets.contains(charBeforeCursor)) {
        bracket = charBeforeCursor;
        matchBracket = closeBrackets[openBrackets.indexOf(bracket)];
        searchPos = pos - 1;
        direction = 1;
    } else if (closeBrackets.contains(charBeforeCursor)) {
        bracket = charBeforeCursor;
        matchBracket = openBrackets[closeBrackets.indexOf(bracket)];
        searchPos = pos - 1;
        direction = -1;
    }

    if (direction == 0)
        return;

    // Ищем парную скобку
    int depth = 0;
    int matchPos = -1;
    int i = searchPos + direction;
    int limit = (direction > 0) ? doc->characterCount() : -1;

    while (i != limit) {
        QChar ch = doc->characterAt(i);
        if (ch == bracket)
            ++depth;
        else if (ch == matchBracket) {
            if (depth == 0) {
                matchPos = i;
                break;
            }
            --depth;
        }
        i += direction;
    }

    // Подсвечиваем обе скобки
    QTextCharFormat bracketFormat;
    bracketFormat.setBackground(QColor(180, 220, 255));
    bracketFormat.setFontWeight(QFont::Bold);

    if (matchPos < 0) {
        // Парная скобка не найдена — красная подсветка
        bracketFormat.setBackground(QColor(255, 180, 180));
    }

    QTextEdit::ExtraSelection sel1;
    sel1.format = bracketFormat;
    sel1.cursor = QTextCursor(doc);
    sel1.cursor.setPosition(searchPos);
    sel1.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    selections.append(sel1);

    if (matchPos >= 0) {
        QTextEdit::ExtraSelection sel2;
        sel2.format = bracketFormat;
        sel2.cursor = QTextCursor(doc);
        sel2.cursor.setPosition(matchPos);
        sel2.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
        selections.append(sel2);
    }
}

} // namespace DeltaQ
