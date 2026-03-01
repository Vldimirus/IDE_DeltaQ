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
    UILayout layout = UILayout::create("window1");

    // Кнопка "Click Me" (y=40 — ниже title bar высотой 30px)
    UIWidget button = UIWidget::create("Button", "btnClickMe");
    button.geometry = QRectF(20, 50, 120, 40);
    button.properties["text"] = "Click Me";
    button.events["onClick"] = "on_btnClickMe_click";
    layout.window.children.append(button);

    // Метка "Hello DeltaQ"
    UIWidget label = UIWidget::create("Label", "lblHello");
    label.geometry = QRectF(20, 100, 200, 30);
    label.properties["text"] = "Hello DeltaQ";
    layout.window.children.append(label);

    // 2. Сохраняем layout в ui/window1.dqui
    QDir().mkpath(projectDir + "/ui");
    {
        QFile f(projectDir + "/ui/window1.dqui");
        if (!f.open(QIODevice::WriteOnly))
            return false;
        QJsonDocument doc(layout.toJson());
        f.write(doc.toJson(QJsonDocument::Indented));
    }

    // 3. Генерируем SDL2 C-код
    GeneratedCode code = SDL2CodeGenerator::generate(layout, "window1");

    // 4. Записываем UI-файлы в ui/, main.c в src/
    QDir().mkpath(projectDir + "/src");

    if (!writeTextFile(projectDir + "/src/main.c", code.mainFile))
        return false;
    if (!writeTextFile(projectDir + "/ui/window1.h", code.uiHeader))
        return false;
    if (!writeTextFile(projectDir + "/ui/window1.c", code.uiSource))
        return false;
    if (!writeTextFile(projectDir + "/ui/window1_events.h", code.eventsHeader))
        return false;
    if (!writeTextFile(projectDir + "/ui/window1_events.c", code.eventsSource))
        return false;

    return generateDesktopModulesAndGraphs(projectDir);
}

// --- Console: граф main с узлом println (из стандартной библиотеки) ---

bool ProjectTemplates::generateConsoleModulesAndGraphs(const QString &projectDir)
{
    QDir().mkpath(projectDir + "/graphs");

    // Граф main — узел println из стандартной библиотеки
    Graph mainGraph = Graph::create("main");
    GraphNode printNode = GraphNode::create("core.io.println", QPointF(100, 100));
    mainGraph.addNode(printNode);
    if (!writeJsonFile(projectDir + "/graphs/main.dqgraph", mainGraph.toJson()))
        return false;

    return true;
}

// --- Desktop: граф main + onClick-обработчик для кнопки ---

bool ProjectTemplates::generateDesktopModulesAndGraphs(const QString &projectDir)
{
    QDir().mkpath(projectDir + "/graphs");

    // Граф main — узел println с приветствием
    Graph mainGraph = Graph::create("main");
    GraphNode printNode = GraphNode::create("core.io.println", QPointF(100, 100));
    printNode.properties["text"] = "\"Application started\"";
    mainGraph.addNode(printNode);
    if (!writeJsonFile(projectDir + "/graphs/main.dqgraph", mainGraph.toJson()))
        return false;

    // Граф onClick_btnClickMe — обработчик нажатия кнопки
    // string_constant("Hello World!!!") → println
    Graph onClickGraph = Graph::create("onClick_btnClickMe");

    GraphNode constNode = GraphNode::create("core.io.string_constant", QPointF(80, 100));
    constNode.properties["value"] = "\"Hello World!!!\"";
    onClickGraph.addNode(constNode);

    GraphNode printNode2 = GraphNode::create("core.io.println", QPointF(350, 100));
    onClickGraph.addNode(printNode2);

    // Соединение: string_constant.out → println.text
    GraphConnection conn;
    conn.from = {constNode.id, "out"};
    conn.to = {printNode2.id, "text"};
    onClickGraph.addConnection(conn);

    if (!writeJsonFile(projectDir + "/graphs/onClick_btnClickMe.dqgraph", onClickGraph.toJson()))
        return false;

    return true;
}

} // namespace DeltaQ
