// Генерация шаблонных файлов — реализация
#include "ProjectTemplates.h"

#include <deltaq/UILayout.h>
#include "../uiDesigner/SDL2CodeGenerator.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>

namespace DeltaQ {

// --- Вспомогательная функция записи файла ---

static bool writeTextFile(const QString &path, const QString &content)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    f.write(content.toUtf8());
    return true;
}

// --- Публичный API ---

bool ProjectTemplates::generate(const QString &type,
                                const QString &projectDir,
                                const QString &name)
{
    if (type == "desktop")
        return generateDesktopTemplate(projectDir, name);
    return generateConsoleTemplate(projectDir, name);
}

// --- Console: src/main.c с hello world ---

bool ProjectTemplates::generateConsoleTemplate(const QString &projectDir,
                                               const QString &name)
{
    Q_UNUSED(name)
    QDir().mkpath(projectDir + "/src");

    QString mainC =
        "#include <stdio.h>\n"
        "\n"
        "int main(void)\n"
        "{\n"
        "    printf(\"Hello from DeltaQ!\\n\");\n"
        "    return 0;\n"
        "}\n";

    return writeTextFile(projectDir + "/src/main.c", mainC);
}

// --- Desktop: UILayout + SDL2CodeGenerator ---

bool ProjectTemplates::generateDesktopTemplate(const QString &projectDir,
                                               const QString &name)
{
    Q_UNUSED(name)

    // 1. Создаём UILayout программно
    UILayout layout = UILayout::create("main");

    // Кнопка "Click Me"
    UIWidget button = UIWidget::create("Button", "btnClickMe");
    button.geometry = QRectF(10, 10, 120, 40);
    button.properties["text"] = "Click Me";
    button.events["onClick"] = "on_btnClickMe_click";
    layout.window.children.append(button);

    // Метка "Hello DeltaQ"
    UIWidget label = UIWidget::create("Label", "lblHello");
    label.geometry = QRectF(10, 60, 200, 30);
    label.properties["text"] = "Hello DeltaQ";
    layout.window.children.append(label);

    // 2. Сохраняем layout в ui/main.dqui
    QDir().mkpath(projectDir + "/ui");
    {
        QFile f(projectDir + "/ui/main.dqui");
        if (!f.open(QIODevice::WriteOnly))
            return false;
        QJsonDocument doc(layout.toJson());
        f.write(doc.toJson(QJsonDocument::Indented));
    }

    // 3. Генерируем SDL2 C-код
    GeneratedCode code = SDL2CodeGenerator::generate(layout);

    // 4. Записываем 5 файлов в src/
    QDir().mkpath(projectDir + "/src");

    if (!writeTextFile(projectDir + "/src/main.c", code.mainFile))
        return false;
    if (!writeTextFile(projectDir + "/src/ui.h", code.uiHeader))
        return false;
    if (!writeTextFile(projectDir + "/src/ui.c", code.uiSource))
        return false;
    if (!writeTextFile(projectDir + "/src/events.h", code.eventsHeader))
        return false;
    if (!writeTextFile(projectDir + "/src/events.c", code.eventsSource))
        return false;

    return true;
}

} // namespace DeltaQ
