// Движок компоновки — расположение дочерних виджетов по заданному типу layout
#pragma once

#include <QString>
#include <QSizeF>

namespace DeltaQ {

class WidgetItem;
class DesignScene;

// Параметры компоновки
struct LayoutConstraints {
    qreal margin = 10.0;
    qreal spacing = 5.0;
    qreal stretch = 1.0; // коэффициент растяжения
};

class LayoutEngine {
public:
    // Применить layout к контейнеру: перераспределить дочерние виджеты
    static void applyLayout(WidgetItem *container);
    static void applyLayout(WidgetItem *container, const LayoutConstraints &constraints);

    // Anchor-привязки
    static void applyAnchors(WidgetItem *widget, const QSizeF &parentSize);
    static void applyAnchorsToChildren(WidgetItem *container);
    static void applyAnchorsToRootWidgets(DesignScene *scene);

private:
    static void applyHBoxLayout(WidgetItem *container, const LayoutConstraints &c);
    static void applyVBoxLayout(WidgetItem *container, const LayoutConstraints &c);
    static void applyGridLayout(WidgetItem *container, const LayoutConstraints &c);
    static void applyFlowLayout(WidgetItem *container, const LayoutConstraints &c);
};

} // namespace DeltaQ
