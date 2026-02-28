// Вкладка редактора кода с нумерацией строк
#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QTextBlock>
#include <QString>
#include <QSet>
#include <QTimer>
#include <QPoint>
#include "../lsp/LSPTypes.h"

namespace DeltaQ {

class CommandBus;
class SyntaxHighlighter;
class CompletionPopup;

// Подкласс QPlainTextEdit — открывает protected-методы для нумерации строк
class CodePlainTextEdit : public QPlainTextEdit {
    Q_OBJECT

public:
    using QPlainTextEdit::QPlainTextEdit;

    // Открываем доступ к protected-методам для LineNumberArea
    QTextBlock getFirstVisibleBlock() const { return firstVisibleBlock(); }
    QRectF getBlockBoundingGeometry(const QTextBlock &block) const { return blockBoundingGeometry(block); }
    QPointF getContentOffset() const { return contentOffset(); }
    QRectF getBlockBoundingRect(const QTextBlock &block) const { return blockBoundingRect(block); }
    void setEditorViewportMargins(int left, int top, int right, int bottom) {
        setViewportMargins(left, top, right, bottom);
    }

signals:
    void hoverRequested(int line, int character);
    void completionRequested(int line, int character);
    void breakpointToggled(int line);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QTimer m_hoverTimer;
    QPoint m_lastMousePos;
};

// Область номеров строк
class LineNumberArea : public QWidget {
    Q_OBJECT
public:
    explicit LineNumberArea(CodePlainTextEdit *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    CodePlainTextEdit *m_editor;
};

class CodeEditorTab : public QWidget {
    Q_OBJECT

public:
    explicit CodeEditorTab(const QString &filePath, CommandBus *bus,
                            QWidget *parent = nullptr);

    bool loadFile(const QString &path);
    bool saveFile();
    bool saveFileAs(const QString &path);

    QString filePath() const { return m_filePath; }
    bool isModified() const;
    QPlainTextEdit *editor() const { return m_editor; }

    // Диагностика
    void setDiagnostics(const QVector<LSPDiagnostic> &diagnostics);
    const QVector<LSPDiagnostic> &diagnostics() const { return m_diagnostics; }

    // Hover-подсказки
    void showHoverTooltip(const QString &text, const QPoint &globalPos);

    // Маркеры отладки
    void addBreakpointMarker(int line);
    void removeBreakpointMarker(int line);
    bool hasBreakpoint(int line) const { return m_breakpointLines.contains(line); }
    const QSet<int> &breakpointLines() const { return m_breakpointLines; }
    void setDebugCurrentLine(int line);
    void clearDebugCurrentLine();

    // Автодополнение
    void showCompletion(const QVector<CompletionItem> &items);
    CompletionPopup *completionPopup() const { return m_completionPopup; }

signals:
    void modificationChanged(bool modified);
    void hoverRequested(const QString &filePath, int line, int character);
    void completionRequested(const QString &filePath, int line, int character);
    void breakpointToggleRequested(const QString &filePath, int line);

private slots:
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateExtraSelections();
    void updateLineNumberAreaWidth(int newBlockCount);
    void onHoverRequested(int line, int character);

private:
    void setupEditor();
    int lineNumberAreaWidth() const;
    // Подсветка парных скобок
    void highlightMatchingBrackets(QList<QTextEdit::ExtraSelection> &selections);
    // Подсветка диагностики (WaveUnderline)
    void addDiagnosticSelections(QList<QTextEdit::ExtraSelection> &selections);
    // Подсветка текущей строки отладки
    void addDebugLineSelection(QList<QTextEdit::ExtraSelection> &selections);

    QString m_filePath;
    CodePlainTextEdit *m_editor;
    LineNumberArea *m_lineNumberArea;
    SyntaxHighlighter *m_highlighter;
    CommandBus *m_commandBus;
    CompletionPopup *m_completionPopup = nullptr;

    // Диагностика
    QVector<LSPDiagnostic> m_diagnostics;

    // Маркеры отладки
    QSet<int> m_breakpointLines;
    int m_debugCurrentLine = -1;

    friend class LineNumberArea;
};

} // namespace DeltaQ
