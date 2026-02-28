// Генерация шаблонных файлов — реализация
#include "ProjectTemplates.h"

#include <deltaq/UILayout.h>
#include <deltaq/Module.h>
#include <deltaq/Graph.h>
#include "../uiDesigner/SDL2CodeGenerator.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

namespace DeltaQ {

// --- Вспомогательная функция записи файла ---

static bool writeJsonFile(const QString &path, const QJsonObject &obj)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    QJsonDocument doc(obj);
    f.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

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

    if (!writeTextFile(projectDir + "/src/main.c", mainC))
        return false;

    return generateConsoleModulesAndGraphs(projectDir);
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

    return generateDesktopModulesAndGraphs(projectDir);
}

// --- Console: модуль hello + граф main ---

bool ProjectTemplates::generateConsoleModulesAndGraphs(const QString &projectDir)
{
    QDir().mkpath(projectDir + "/modules");
    QDir().mkpath(projectDir + "/graphs");

    // Модуль hello (category=io, вход text:string)
    Module hello = Module::create("hello");
    hello.category = "io";
    hello.description = "Выводит текст в консоль";
    hello.inputs.append(Port{"text", "string", "Hello from DeltaQ!"});
    if (!writeJsonFile(projectDir + "/modules/hello.dqmod", hello.toJson()))
        return false;

    // Граф main (1 узел — hello)
    Graph mainGraph = Graph::create("main");
    GraphNode helloNode = GraphNode::create(hello.id, QPointF(100, 100));
    mainGraph.addNode(helloNode);
    if (!writeJsonFile(projectDir + "/graphs/main.dqgraph", mainGraph.toJson()))
        return false;

    return true;
}

// --- Desktop: модули event_handler + update_label + граф main ---

bool ProjectTemplates::generateDesktopModulesAndGraphs(const QString &projectDir)
{
    QDir().mkpath(projectDir + "/modules");
    QDir().mkpath(projectDir + "/graphs");

    // Модуль event_handler (category=events, вход event → выход action)
    Module eventHandler = Module::create("event_handler");
    eventHandler.category = "events";
    eventHandler.description = "Обрабатывает SDL-события";
    eventHandler.inputs.append(Port{"event", "SDL_Event", ""});
    eventHandler.outputs.append(Port{"action", "string", ""});
    if (!writeJsonFile(projectDir + "/modules/event_handler.dqmod", eventHandler.toJson()))
        return false;

    // Модуль update_label (category=ui, входы action + text)
    Module updateLabel = Module::create("update_label");
    updateLabel.category = "ui";
    updateLabel.description = "Обновляет текст метки";
    updateLabel.inputs.append(Port{"action", "string", ""});
    updateLabel.inputs.append(Port{"text", "string", "Hello DeltaQ"});
    if (!writeJsonFile(projectDir + "/modules/update_label.dqmod", updateLabel.toJson()))
        return false;

    // Граф main (2 узла, 1 соединение action→action)
    Graph mainGraph = Graph::create("main");
    GraphNode ehNode = GraphNode::create(eventHandler.id, QPointF(100, 100));
    GraphNode ulNode = GraphNode::create(updateLabel.id, QPointF(350, 100));
    mainGraph.addNode(ehNode);
    mainGraph.addNode(ulNode);

    GraphConnection conn;
    conn.from = {ehNode.id, "action"};
    conn.to = {ulNode.id, "action"};
    mainGraph.addConnection(conn);

    if (!writeJsonFile(projectDir + "/graphs/main.dqgraph", mainGraph.toJson()))
        return false;

    return true;
}

} // namespace DeltaQ
