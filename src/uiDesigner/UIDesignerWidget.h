// Модуль 3: Дизайнер UI — заглушка
// TODO: реализация в Фазе 3
#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

namespace DeltaQ {

class ModuleRegistry;
class CommandBus;

class UIDesignerWidget : public QWidget {
public:
    explicit UIDesignerWidget(ModuleRegistry *registry, CommandBus *bus,
                               QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *layout = new QVBoxLayout(this);
        auto *label = new QLabel("Дизайнер UI (SDL2)\n\n(будет реализован в Фазе 3)", this);
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);
    }
};

} // namespace DeltaQ
