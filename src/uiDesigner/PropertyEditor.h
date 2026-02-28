// Редактор свойств виджета — динамические свойства выбранного WidgetItem
#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QFormLayout>
#include <QMap>
#include <QString>
#include <QVariant>

namespace DeltaQ {

class WidgetItem;

class PropertyEditor : public QWidget {
    Q_OBJECT

public:
    explicit PropertyEditor(QWidget *parent = nullptr);

    // Установить текущий виджет
    void setWidget(WidgetItem *widget);
    void clearWidget();

    WidgetItem *currentWidget() const { return m_currentWidget; }

signals:
    void propertyChanged(const QString &widgetId, const QString &key,
                         const QVariant &oldValue, const QVariant &newValue);
    void eventBindRequested(const QString &widgetId);

private:
    void buildPropertyList();
    void addProperty(const QString &label, const QString &key, const QVariant &value,
                     const QString &type = "string");
    void addSectionHeader(const QString &title);
    QWidget *createEditor(const QString &key, const QVariant &value, const QString &type);
    void clearLayout();

    WidgetItem *m_currentWidget = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_contentWidget = nullptr;
    QFormLayout *m_formLayout = nullptr;
    bool m_updating = false; // флаг для предотвращения рекурсии
};

} // namespace DeltaQ
