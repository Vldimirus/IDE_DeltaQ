// Генератор SDL2 C-кода из UILayout
#pragma once

#include <QString>
#include <QMap>

namespace DeltaQ {

struct UILayout;
struct UIWidget;

// Результат генерации: 5 файлов
struct GeneratedCode {
    QString baseName;      // базовое имя (например "window_01")
    QString mainFile;      // main.c
    QString uiHeader;      // {baseName}.h
    QString uiSource;      // {baseName}.c
    QString eventsHeader;  // {baseName}_events.h
    QString eventsSource;  // {baseName}_events.c
};

class SDL2CodeGenerator {
public:
    // Генерация кода из UILayout с именованием по baseName
    static GeneratedCode generate(const UILayout &layout, const QString &baseName = "ui");

private:
    static QString generateMainFile(const UILayout &layout, const QString &baseName);
    static QString generateUIHeader(const UILayout &layout, const QString &baseName);
    static QString generateUISource(const UILayout &layout, const QString &baseName);
    static QString generateEventsHeader(const UILayout &layout, const QString &baseName);
    static QString generateEventsSource(const UILayout &layout, const QString &baseName);

    // Вспомогательные
    static QString widgetStructName(const QString &type);
    static QString widgetRenderFunc(const QString &type);
    static void collectWidgets(const UIWidget &widget, QVector<const UIWidget *> &out);
    static void collectEvents(const UIWidget &widget, QMap<QString, QString> &out);
    static QString sanitizeName(const QString &name);
};

} // namespace DeltaQ
