// Генератор SDL2 C-кода из UILayout
#pragma once

#include <QString>
#include <QMap>

namespace DeltaQ {

struct UILayout;
struct UIWidget;

// Результат генерации: 5 файлов
struct GeneratedCode {
    QString mainFile;      // main.c
    QString uiHeader;      // ui.h
    QString uiSource;      // ui.c
    QString eventsHeader;  // events.h
    QString eventsSource;  // events.c
};

class SDL2CodeGenerator {
public:
    // Генерация кода из UILayout
    static GeneratedCode generate(const UILayout &layout);

private:
    static QString generateMainFile(const UILayout &layout);
    static QString generateUIHeader(const UILayout &layout);
    static QString generateUISource(const UILayout &layout);
    static QString generateEventsHeader(const UILayout &layout);
    static QString generateEventsSource(const UILayout &layout);

    // Вспомогательные
    static QString widgetStructName(const QString &type);
    static QString widgetRenderFunc(const QString &type);
    static void collectWidgets(const UIWidget &widget, QVector<const UIWidget *> &out);
    static void collectEvents(const UIWidget &widget, QMap<QString, QString> &out);
    static QString sanitizeName(const QString &name);
};

} // namespace DeltaQ
