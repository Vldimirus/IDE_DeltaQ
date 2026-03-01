// Хлебные крошки навигации по подмодулям
#pragma once

#include <QWidget>
#include <QStringList>

namespace DeltaQ {

class BreadcrumbBar : public QWidget {
    Q_OBJECT

public:
    explicit BreadcrumbBar(QWidget *parent = nullptr);

    // Установить путь навигации
    void setPath(const QStringList &labels);

    // Очистить путь
    void clearPath();

signals:
    // Клик по кнопке «←» — назад на один уровень
    void backClicked();

    // Клик по конкретному уровню в пути
    void levelClicked(int level);

private:
    void rebuild();

    QStringList m_labels;
};

} // namespace DeltaQ
