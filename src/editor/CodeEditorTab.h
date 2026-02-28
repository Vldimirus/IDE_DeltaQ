// Вкладка редактора кода с нумерацией строк
#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QString>

namespace DeltaQ {

class CommandBus;
class SyntaxHighlighter;

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
};

// Область номеров строк
class LineNumberArea : public QWidget {
    Q_OBJECT
public:
    explicit LineNumberArea(CodePlainTextEdit *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

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

signals:
    void modificationChanged(bool modified);

private slots:
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();
    void updateLineNumberAreaWidth(int newBlockCount);

private:
    void setupEditor();
    int lineNumberAreaWidth() const;

    QString m_filePath;
    CodePlainTextEdit *m_editor;
    LineNumberArea *m_lineNumberArea;
    SyntaxHighlighter *m_highlighter;
    CommandBus *m_commandBus;

    friend class LineNumberArea;
};

} // namespace DeltaQ
