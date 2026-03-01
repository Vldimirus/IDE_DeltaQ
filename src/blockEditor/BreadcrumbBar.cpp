// Хлебные крошки навигации по подмодулям — реализация
#include "BreadcrumbBar.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

namespace DeltaQ {

BreadcrumbBar::BreadcrumbBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(28);
    setStyleSheet(
        "BreadcrumbBar { background: #2a2a2a; border-bottom: 1px solid #444; }"
    );
}

void BreadcrumbBar::setPath(const QStringList &labels)
{
    m_labels = labels;
    rebuild();
}

void BreadcrumbBar::clearPath()
{
    m_labels.clear();
    rebuild();
}

void BreadcrumbBar::rebuild()
{
    // Удаляем старый layout и все дочерние виджеты
    if (layout()) {
        QLayoutItem *item;
        while ((item = layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete layout();
    }

    if (m_labels.isEmpty()) return;

    auto *hbox = new QHBoxLayout(this);
    hbox->setContentsMargins(4, 2, 4, 2);
    hbox->setSpacing(2);

    // Кнопка «←» — назад
    auto *backBtn = new QPushButton(QString::fromUtf8("\u2190"), this);
    backBtn->setFixedSize(24, 22);
    backBtn->setToolTip(tr("Назад"));
    backBtn->setStyleSheet(
        "QPushButton { background: #3a3a3a; color: white; border: 1px solid #555; "
        "border-radius: 3px; font-size: 14px; }"
        "QPushButton:hover { background: #4a4a4a; }"
    );
    connect(backBtn, &QPushButton::clicked, this, &BreadcrumbBar::backClicked);
    hbox->addWidget(backBtn);

    // Элементы пути
    for (int i = 0; i < m_labels.size(); ++i) {
        if (i > 0) {
            auto *sep = new QLabel(QString::fromUtf8(" \u203A "), this);
            sep->setStyleSheet("color: #888; font-size: 12px;");
            hbox->addWidget(sep);
        }

        auto *btn = new QPushButton(m_labels[i], this);
        btn->setFlat(true);
        bool isLast = (i == m_labels.size() - 1);
        btn->setStyleSheet(
            isLast
            ? "QPushButton { color: white; font-weight: bold; font-size: 11px; "
              "padding: 2px 6px; border: none; }"
            : "QPushButton { color: #aaa; font-size: 11px; padding: 2px 6px; border: none; }"
              "QPushButton:hover { color: #ddd; text-decoration: underline; }"
        );

        if (!isLast) {
            int level = i;
            connect(btn, &QPushButton::clicked, this, [this, level]() {
                emit levelClicked(level);
            });
        }

        hbox->addWidget(btn);
    }

    hbox->addStretch();
}

} // namespace DeltaQ
