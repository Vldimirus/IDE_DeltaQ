// Подсветка синтаксиса C/C++ — заглушка
// TODO: заменить на QScintilla лексер
#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

namespace DeltaQ {

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightRule> m_rules;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_multiLineCommentFormat;
    QRegularExpression m_commentStartExpression;
    QRegularExpression m_commentEndExpression;
};

} // namespace DeltaQ
