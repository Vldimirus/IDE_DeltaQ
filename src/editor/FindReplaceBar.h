// Панель поиска и замены в редакторе кода
#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPlainTextEdit>

namespace DeltaQ {

class FindReplaceBar : public QWidget {
    Q_OBJECT

public:
    explicit FindReplaceBar(QWidget *parent = nullptr);

    // Установить редактор для поиска
    void setEditor(QPlainTextEdit *editor);

    // Показать панель в режиме поиска (Ctrl+F)
    void showFind();
    // Показать панель в режиме поиска и замены (Ctrl+H)
    void showReplace();

    // Получить текст поиска
    QString findText() const;

signals:
    // Количество найденных совпадений изменилось
    void matchCountChanged(int count);

public slots:
    void findNext();
    void findPrevious();
    void replace();
    void replaceAll();
    void close();

private slots:
    void onFindTextChanged(const QString &text);
    void updateMatchCount();

private:
    void setupUI();
    QTextDocument::FindFlags buildFlags(bool backward = false) const;
    void highlightAllMatches();
    void clearHighlights();

    QPlainTextEdit *m_editor = nullptr;

    // Поиск
    QLineEdit *m_findEdit = nullptr;
    QPushButton *m_findNextBtn = nullptr;
    QPushButton *m_findPrevBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QLabel *m_matchLabel = nullptr;

    // Замена
    QWidget *m_replaceRow = nullptr;
    QLineEdit *m_replaceEdit = nullptr;
    QPushButton *m_replaceBtn = nullptr;
    QPushButton *m_replaceAllBtn = nullptr;

    // Опции
    QCheckBox *m_caseSensitive = nullptr;
    QCheckBox *m_wholeWords = nullptr;

    int m_matchCount = 0;
};

} // namespace DeltaQ
