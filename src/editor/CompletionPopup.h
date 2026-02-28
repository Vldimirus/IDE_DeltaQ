// Popup-окно автодополнения
#pragma once

#include <QFrame>
#include <QListWidget>
#include <QVector>
#include "../lsp/LSPTypes.h"

namespace DeltaQ {

class CompletionPopup : public QFrame {
    Q_OBJECT

public:
    explicit CompletionPopup(QWidget *parent = nullptr);

    void setItems(const QVector<CompletionItem> &items);
    void popup(const QPoint &globalPos);
    void updateFilter(const QString &prefix);

signals:
    void itemSelected(const QString &text);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void insertCurrentItem();
    QIcon iconForKind(CompletionItemKind kind) const;

    QListWidget *m_listWidget;
    QVector<CompletionItem> m_allItems;
    QString m_prefix;
};

} // namespace DeltaQ
