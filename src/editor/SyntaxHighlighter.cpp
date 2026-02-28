// Подсветка синтаксиса C/C++ — базовая реализация
// TODO: заменить на QScintilla лексер
#include "SyntaxHighlighter.h"

namespace DeltaQ {

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Ключевые слова C/C++
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor("#569CD6"));
    keywordFormat.setFontWeight(QFont::Bold);

    const QStringList keywords = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "int", "long", "register", "return", "short", "signed", "sizeof", "static",
        "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while",
        // C++ дополнения
        "class", "namespace", "template", "typename", "public", "private", "protected",
        "virtual", "override", "final", "new", "delete", "this", "nullptr",
        "true", "false", "bool", "using", "throw", "try", "catch",
        "const_cast", "static_cast", "dynamic_cast", "reinterpret_cast",
        "explicit", "inline", "noexcept", "constexpr"
    };

    for (const QString &kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + kw + "\\b");
        rule.format = keywordFormat;
        m_rules.append(rule);
    }

    // Строки
    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor("#CE9178"));
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\"[^\"]*\"");
        rule.format = stringFormat;
        m_rules.append(rule);
    }
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("'[^']*'");
        rule.format = stringFormat;
        m_rules.append(rule);
    }

    // Числа
    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor("#B5CEA8"));
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?[fFlLuU]*\\b");
        rule.format = numberFormat;
        m_rules.append(rule);
    }
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b0[xX][0-9a-fA-F]+[uUlL]*\\b");
        rule.format = numberFormat;
        m_rules.append(rule);
    }

    // Препроцессор
    QTextCharFormat preprocessorFormat;
    preprocessorFormat.setForeground(QColor("#C586C0"));
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("^\\s*#\\s*\\w+");
        rule.format = preprocessorFormat;
        m_rules.append(rule);
    }

    // Однострочные комментарии
    m_commentFormat.setForeground(QColor("#6A9955"));
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = m_commentFormat;
        m_rules.append(rule);
    }

    // Многострочные комментарии
    m_multiLineCommentFormat.setForeground(QColor("#6A9955"));
    m_commentStartExpression = QRegularExpression("/\\*");
    m_commentEndExpression = QRegularExpression("\\*/");
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    // Применяем правила
    for (const HighlightRule &rule : m_rules) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Многострочные комментарии
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(m_commentStartExpression);

    while (startIndex >= 0) {
        auto endMatch = m_commentEndExpression.match(text, startIndex);
        int endIndex = endMatch.capturedStart();
        int commentLength;

        if (endIndex == -1 || !endMatch.hasMatch()) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }

        setFormat(startIndex, commentLength, m_multiLineCommentFormat);
        startIndex = text.indexOf(m_commentStartExpression, startIndex + commentLength);
    }
}

} // namespace DeltaQ
