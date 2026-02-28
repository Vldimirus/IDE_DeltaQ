// Модуль 2: Визуальный блочный редактор — заглушка
// TODO: реализация в Фазе 2
#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QCoreApplication>

namespace DeltaQ {

class ModuleRegistry;
class CommandBus;

class BlockEditorWidget : public QWidget {
public:
    explicit BlockEditorWidget(ModuleRegistry *registry, CommandBus *bus,
                                QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *layout = new QVBoxLayout(this);
        auto *label = new QLabel(QCoreApplication::translate("BlockEditorWidget",
            "Block Editor\n\n(will be implemented in Phase 2)"), this);
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);
    }
};

} // namespace DeltaQ
