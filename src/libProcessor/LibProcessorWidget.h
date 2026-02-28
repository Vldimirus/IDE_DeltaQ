// Модуль 4: Обработчик библиотек — заглушка
// TODO: реализация в Фазе 4
#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QCoreApplication>

namespace DeltaQ {

class ModuleRegistry;

class LibProcessorWidget : public QWidget {
public:
    explicit LibProcessorWidget(ModuleRegistry *registry,
                                 QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *layout = new QVBoxLayout(this);
        auto *label = new QLabel(QCoreApplication::translate("LibProcessorWidget",
            "Library Processor\n\n(will be implemented in Phase 4)"), this);
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);
    }
};

} // namespace DeltaQ
