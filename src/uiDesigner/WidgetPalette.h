// Палитра виджетов — боковая панель с деревом категорий и поиском
#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QMimeData>

namespace DeltaQ {

// Подкласс QTreeWidget с корректным MIME для drag&drop
class PaletteTreeWidget : public QTreeWidget {
    Q_OBJECT
public:
    using QTreeWidget::QTreeWidget;

protected:
    // Qt вызывает этот метод при начале drag — возвращаем наш MIME
    QMimeData *mimeData(const QList<QTreeWidgetItem *> &items) const override
    {
        if (items.isEmpty()) return nullptr;
        auto *item = items.first();
        if (!item->parent()) return nullptr; // категория — не перетаскивать
        QString widgetType = item->data(0, Qt::UserRole).toString();
        if (widgetType.isEmpty()) return nullptr;

        auto *data = new QMimeData;
        data->setData("application/x-dqwidget", widgetType.toUtf8());
        return data;
    }

    // Разрешаем перетаскивание только для листовых элементов
    QStringList mimeTypes() const override
    {
        return {"application/x-dqwidget"};
    }
};

class WidgetPalette : public QWidget {
    Q_OBJECT

public:
    explicit WidgetPalette(QWidget *parent = nullptr);

    void setFilter(const QString &text);

private:
    void buildTree();
    void filterTree(const QString &text);

    QLineEdit *m_searchEdit = nullptr;
    PaletteTreeWidget *m_tree = nullptr;
};

} // namespace DeltaQ
