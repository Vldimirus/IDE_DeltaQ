# DeltaQ IDE - Модуль 2: Блочный редактор (Block Editor)

## Обзор

Блочный редактор -- **визуальное отображение архитектуры программы**. Каждый блок на холсте — это модуль (= .cpp файл), а соединения между блоками — это вызовы функций и передача данных. Блочный редактор не является отдельным инструментом программирования — он показывает реальную структуру ПО и позволяет собирать программы из готовых модулей.

### Ключевые принципы

| Принцип | Описание |
|---------|----------|
| **Блок = Модуль = .cpp файл** | Написал `add.cpp` с `@dqmodule` → он появляется как блок в палитре |
| **Граф = Составной модуль** | Соединил блоки → граф сам становится новым модулем → появляется в палитре |
| **Матрёшка** | Составной модуль используется как блок в другом графе, без ограничения глубины |
| **Двойной клик** | Атомарный блок → открывает .cpp в Code Editor. Составной блок → раскрывает вложенный граф |
| **Визуал = структура** | Граф — это не абстрактная диаграмма, а отображение реальной структуры программы |

---

## Технологическая основа: Qt Graphics View

### Архитектура компонента

```
┌────────────────────────────────────────────────────────┐
│                  Block Editor Module                    │
│                                                         │
│  ┌──────────────┐  ┌───────────────┐  ┌─────────────┐ │
│  │  Module       │  │ Block Editor  │  │  Minimap    │ │
│  │  Palette      │  │ Canvas        │  │             │ │
│  └──────┬───────┘  └───────┬───────┘  └──────┬──────┘ │
│         │                  │                  │         │
│  ┌──────┴──────────────────┴──────────────────┴──────┐ │
│  │              BlockEditorWidget                     │ │
│  └────────────────────────┬──────────────────────────┘ │
│                           │                             │
│  ┌────────────────────────┴──────────────────────────┐ │
│  │              Graph Processing                      │ │
│  │  ┌──────────┐  ┌──────────────┐  ┌─────────────┐ │ │
│  │  │  Graph   │  │    Graph     │  │   Visual    │ │ │
│  │  │  Compiler│  │   Validator  │  │   Debugger  │ │ │
│  │  └──────────┘  └──────────────┘  └─────────────┘ │ │
│  └───────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────┘
```

### Ключевые классы Qt Graphics View

```cpp
// Сцена -- содержит все узлы и соединения
class BlockScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit BlockScene(CommandBus* commandBus, QObject* parent = nullptr);

    // Управление узлами
    NodeItem* addNode(const Module* module, const QPointF& position);
    void removeNode(const QString& nodeId);
    NodeItem* nodeById(const QString& id) const;
    QList<NodeItem*> selectedNodes() const;

    // Управление соединениями
    ConnectionItem* addConnection(const QString& fromNode, const QString& fromPort,
                                   const QString& toNode, const QString& toPort);
    void removeConnection(const QString& connectionId);
    QList<ConnectionItem*> connections() const;

    // Валидация
    bool canConnect(const PortItem* from, const PortItem* to) const;

    // Сериализация
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);

signals:
    void nodeAdded(NodeItem* node);
    void nodeRemoved(const QString& id);
    void connectionAdded(ConnectionItem* conn);
    void connectionRemoved(const QString& id);
    void selectionChanged();

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    CommandBus* m_commandBus;
    QMap<QString, NodeItem*> m_nodes;
    QList<ConnectionItem*> m_connections;
    ConnectionItem* m_draggingConnection;  // Временное соединение при перетаскивании
};

// Виджет-обёртка с зумом, прокруткой, миникартой
class BlockEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit BlockEditorWidget(CommandBus* commandBus, QWidget* parent = nullptr);

    void loadGraph(const QString& graphPath);
    void saveGraph();

    BlockScene* scene() const;

private:
    void setupView();
    void setupToolbar();
    void setupPalette();
    void setupMinimap();

    QGraphicsView* m_view;
    BlockScene* m_scene;
    ModulePalette* m_palette;
    MinimapWidget* m_minimap;
    CommandBus* m_commandBus;
};
```

---

## Узлы (Nodes)

### Класс NodeItem

```cpp
class NodeItem : public QGraphicsObject {
    Q_OBJECT
public:
    NodeItem(const Module* module, const QString& nodeId, QGraphicsItem* parent = nullptr);

    // Идентификация
    QString nodeId() const;
    const Module* module() const;

    // Порты
    QList<PortItem*> inputPorts() const;
    QList<PortItem*> outputPorts() const;
    QList<PortItem*> execInputPorts() const;
    QList<PortItem*> execOutputPorts() const;
    PortItem* portByName(const QString& name) const;

    // Свойства
    void setProperty(const QString& portName, const QVariant& value);
    QVariant property(const QString& portName) const;

    // Состояние
    bool isSelected() const;
    void setHighlighted(bool highlighted);  // Для отладки
    void setError(bool error, const QString& message = "");

    // QGraphicsItem
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    void layoutPorts();
    void drawHeader(QPainter* painter);
    void drawBody(QPainter* painter);
    void drawPorts(QPainter* painter);

    QString m_nodeId;
    const Module* m_module;
    QList<PortItem*> m_inputPorts;
    QList<PortItem*> m_outputPorts;
    QList<PortItem*> m_execInputPorts;
    QList<PortItem*> m_execOutputPorts;

    bool m_highlighted;
    bool m_hasError;
    QString m_errorMessage;
    QColor m_color;
};
```

### Визуальное представление узла

Блоки отрисовываются **просто и функционально** — без лишней графики. Прямоугольник с чётким разделением:

```
┌──────────────────────────────┐
│  ▶ exec_in       exec_out ▶  │  ← Порты выполнения (вверху)
├──────────────────────────────┤
│          Сложение             │  ← Имя модуля (заголовок)
│           (add)               │
├──────────────────────────────┤
│  Складывает два числа         │  ← Описание (под линией)
├──────────────────────────────┤
│  ● a [int]        result ● │  ← Входы слева, выходы справа
│  ● b [int]                   │     с указанием типа
│    [10]                      │  ← Значение по умолчанию
└──────────────────────────────┘
```

**Принципы визуала:**
- Имя модуля сверху, отделено линией
- Описание под заголовком
- Входные порты с типом — слева
- Выходные порты с типом — справа
- Красивая графика не нужна — важна читаемость и функциональность

### Отрисовка узла

```cpp
void NodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                      QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    const qreal width = 180;
    const qreal headerHeight = 30;
    const qreal portHeight = 25;
    const qreal totalHeight = headerHeight +
        qMax(m_inputPorts.size(), m_outputPorts.size()) * portHeight + 10;

    // Тень
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 60));
    painter->drawRoundedRect(QRectF(3, 3, width, totalHeight), 8, 8);

    // Фон
    painter->setBrush(QColor("#2d2d2d"));
    if (m_hasError) {
        painter->setPen(QPen(QColor("#e51400"), 2));
    } else if (isSelected()) {
        painter->setPen(QPen(QColor("#007acc"), 2));
    } else if (m_highlighted) {
        painter->setPen(QPen(QColor("#e8e800"), 2));
    } else {
        painter->setPen(QPen(QColor("#3e3e3e"), 1));
    }
    painter->drawRoundedRect(QRectF(0, 0, width, totalHeight), 8, 8);

    // Заголовок
    painter->setBrush(m_color);
    painter->setPen(Qt::NoPen);
    QPainterPath headerPath;
    headerPath.addRoundedRect(QRectF(0, 0, width, headerHeight), 8, 8);
    headerPath.addRect(QRectF(0, headerHeight - 8, width, 8));
    painter->drawPath(headerPath);

    // Текст заголовка
    painter->setPen(Qt::white);
    painter->setFont(QFont("Segoe UI", 10, QFont::Bold));
    painter->drawText(QRectF(10, 0, width - 20, headerHeight),
                      Qt::AlignVCenter, m_module->displayName());

    // Порты рисуются отдельно через PortItem
}
```

---

## Соединения (Connections)

### Класс ConnectionItem

```cpp
class ConnectionItem : public QGraphicsPathItem {
public:
    enum ConnectionType {
        DataConnection,      // Передача данных (цветная линия)
        ExecutionConnection  // Поток выполнения (белая линия)
    };

    ConnectionItem(PortItem* fromPort, PortItem* toPort,
                   ConnectionType type, QGraphicsItem* parent = nullptr);

    QString connectionId() const;
    PortItem* fromPort() const;
    PortItem* toPort() const;
    ConnectionType connectionType() const;

    void updatePath();  // Пересчёт пути при перемещении узлов
    void setHighlighted(bool highlighted);

    // QGraphicsItem
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;
    QPainterPath shape() const override;

private:
    QPainterPath computeBezierPath() const;
    QColor colorForType(const QString& dataType) const;

    QString m_connectionId;
    PortItem* m_fromPort;
    PortItem* m_toPort;
    ConnectionType m_type;
    bool m_highlighted;
};
```

### Отрисовка соединений (кривые Безье)

```cpp
QPainterPath ConnectionItem::computeBezierPath() const {
    QPointF start = m_fromPort->scenePos();
    QPointF end = m_toPort->scenePos();

    qreal dx = qAbs(end.x() - start.x());
    qreal offset = qMax(dx * 0.5, 50.0);

    QPointF ctrl1(start.x() + offset, start.y());
    QPointF ctrl2(end.x() - offset, end.y());

    QPainterPath path;
    path.moveTo(start);
    path.cubicTo(ctrl1, ctrl2, end);

    return path;
}

void ConnectionItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                            QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPen pen;
    if (m_type == ExecutionConnection) {
        pen.setColor(m_highlighted ? QColor("#ffffff") : QColor("#cccccc"));
        pen.setWidth(3);
        pen.setStyle(Qt::SolidLine);
    } else {
        QColor color = colorForType(m_fromPort->dataType());
        if (m_highlighted) {
            color = color.lighter(150);
        }
        pen.setColor(color);
        pen.setWidth(2);
    }

    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path());

    // Стрелка на конце
    if (m_type == ExecutionConnection) {
        drawArrowHead(painter, end, angle);
    }
}

QColor ConnectionItem::colorForType(const QString& dataType) const {
    static const QMap<QString, QColor> typeColors = {
        {"int",     QColor("#4FC3F7")},  // Голубой
        {"float",   QColor("#81C784")},  // Зелёный
        {"double",  QColor("#AED581")},  // Светло-зелёный
        {"bool",    QColor("#FF8A65")},  // Оранжевый
        {"string",  QColor("#CE93D8")},  // Фиолетовый
        {"pointer", QColor("#FFD54F")},  // Жёлтый
    };
    return typeColors.value(dataType, QColor("#90A4AE"));  // Серый по умолчанию
}
```

---

## Порты выполнения и данных

### Типы портов

| Тип | Визуал | Назначение |
|-----|--------|-----------|
| **Execution Input** | Белый треугольник (▶) слева | Входная точка потока выполнения |
| **Execution Output** | Белый треугольник (▶) справа | Выходная точка потока выполнения |
| **Data Input** | Цветной кружок (●) слева | Приём данных определённого типа |
| **Data Output** | Цветной кружок (●) справа | Отправка данных определённого типа |

### Правила соединения

```cpp
bool BlockScene::canConnect(const PortItem* from, const PortItem* to) const {
    // Нельзя соединить порт сам с собой
    if (from == to) return false;

    // Нельзя соединить порты одного узла
    if (from->parentNode() == to->parentNode()) return false;

    // Направление: output -> input
    if (from->direction() != PortItem::Output) return false;
    if (to->direction() != PortItem::Input) return false;

    // Тип соединения должен совпадать
    if (from->isExecutionPort() != to->isExecutionPort()) return false;

    // Для портов данных -- проверка совместимости типов
    if (!from->isExecutionPort()) {
        if (!isTypeCompatible(from->dataType(), to->dataType())) {
            return false;
        }
    }

    // Входной порт данных может иметь только одно соединение
    if (!to->isExecutionPort() && to->hasConnection()) {
        return false;
    }

    // Проверка на циклы (для execution-портов)
    if (from->isExecutionPort()) {
        if (wouldCreateCycle(from->parentNode(), to->parentNode())) {
            return false;
        }
    }

    return true;
}

bool BlockScene::isTypeCompatible(const QString& fromType, const QString& toType) const {
    if (fromType == toType) return true;

    // Неявные преобразования
    static const QMap<QString, QStringList> implicitConversions = {
        {"int",    {"float", "double", "int64", "string"}},
        {"float",  {"double", "string"}},
        {"int8",   {"int", "int16", "int32", "int64", "float", "double"}},
        {"int16",  {"int", "int32", "int64", "float", "double"}},
        {"int32",  {"int64", "float", "double"}},
        {"bool",   {"int", "string"}},
    };

    return implicitConversions.value(fromType).contains(toType);
}
```

---

## Обработка ошибок компиляции графа

Когда пользователь компилирует граф, ошибки отображаются непосредственно на холсте:

| Тип ошибки | Визуальное отображение |
|------------|----------------------|
| Цикл в графе | Узлы цикла обведены красным, стрелки цикла подсвечены |
| Несовместимые типы портов | Соединение окрашено красным, тултип с описанием |
| Непротестированный модуль | Узел с жёлтой рамкой и значком ⚠ |
| Ошибка компиляции C-кода | Узел с красной рамкой, двойной клик → переход к ошибке в Code Editor |
| Неподключённый обязательный порт | Порт мигает красным |

Ошибки от компилятора (GCC/Clang/MSVC) парсятся и маппятся обратно на узлы графа через файл `.dqmap` (см. `07_codegen.md`). Пользователь видит ошибку прямо на блоке, а не в тексте вывода компилятора.

---

## Компилятор графов (Graph Compiler)

Компилятор графов преобразует визуальный граф в исполняемый код через промежуточное представление (IR). IR передаётся в активный LanguageBackend для генерации кода на целевом языке.

### Конвейер компиляции

```
┌─────────┐     ┌──────────────┐     ┌─────────┐     ┌──────────┐
│  Graph  │────▶│  Topological │────▶│   IR    │────▶│  C/C++   │
│  (.dqgraph)   │  Sort        │     │  Generate│     │  Emit    │
└─────────┘     └──────────────┘     └─────────┘     └──────────┘
```

### Этап 1: Топологическая сортировка

```cpp
class GraphCompiler {
public:
    struct CompilationResult {
        bool success;
        QString generatedCode;
        QStringList errors;
        QStringList warnings;
    };

    CompilationResult compile(const Graph& graph, const ModuleRegistry& registry);

private:
    // Этап 1: Топологическая сортировка
    QList<Node*> topologicalSort(const Graph& graph);

    // Этап 2: Генерация IR
    IR generateIR(const QList<Node*>& sortedNodes, const ModuleRegistry& registry);

    // Этап 3: Генерация C/C++ кода
    QString emitCode(const IR& ir);
};

QList<Node*> GraphCompiler::topologicalSort(const Graph& graph) {
    QList<Node*> result;
    QSet<QString> visited;
    QSet<QString> inStack;  // Для обнаружения циклов

    std::function<bool(Node*)> visit = [&](Node* node) -> bool {
        if (inStack.contains(node->id())) {
            // Обнаружен цикл
            m_errors << QString("Обнаружен цикл в графе, включающий узел '%1'")
                        .arg(node->displayName());
            return false;
        }
        if (visited.contains(node->id())) return true;

        inStack.insert(node->id());

        // Рекурсивно посещаем все зависимости (узлы, от которых получаем данные)
        for (const auto& conn : graph.incomingDataConnections(node->id())) {
            Node* dependency = graph.nodeById(conn.fromNodeId);
            if (!visit(dependency)) return false;
        }

        inStack.remove(node->id());
        visited.insert(node->id());
        result.append(node);
        return true;
    };

    // Начинаем с entry point или со всех узлов без входящих execution-соединений
    for (auto* node : graph.nodes()) {
        if (!visited.contains(node->id())) {
            if (!visit(node)) return {};
        }
    }

    return result;
}
```

### Этап 2: Промежуточное представление (IR)

```cpp
struct IRInstruction {
    enum Type {
        Call,           // Вызов функции модуля
        Assign,         // Присвоение переменной
        TypeConvert,    // Приведение типа
        Branch,         // Условное ветвление
        Label,          // Метка
        Goto,           // Безусловный переход
        Return,         // Возврат
        DeclareVar,     // Объявление переменной
        Comment         // Комментарий (для читаемости сгенерированного кода)
    };

    Type type;
    QString target;      // Имя переменной-результата
    QString function;    // Имя вызываемой функции
    QStringList args;    // Аргументы
    QString label;       // Метка для Branch/Goto
    QString comment;     // Комментарий
};

struct IR {
    QList<QString> includes;
    QList<IRInstruction> instructions;
    QMap<QString, QString> variables;  // имя -> тип
};
```

### Этап 3: Генерация C/C++ кода

```cpp
QString GraphCompiler::emitCode(const IR& ir) {
    QString code;

    // Заголовочные файлы
    for (const auto& inc : ir.includes) {
        code += QString("#include \"%1\"\n").arg(inc);
    }
    code += "#include <stdio.h>\n";
    code += "#include <stdlib.h>\n\n";

    // Главная функция
    code += "int main(int argc, char* argv[]) {\n";

    // Объявление переменных
    for (auto it = ir.variables.begin(); it != ir.variables.end(); ++it) {
        code += QString("    %1 %2;\n").arg(it.value(), it.key());
    }
    code += "\n";

    // Инструкции
    for (const auto& instr : ir.instructions) {
        switch (instr.type) {
        case IRInstruction::Comment:
            code += QString("    // %1\n").arg(instr.comment);
            break;

        case IRInstruction::Call:
            if (instr.target.isEmpty()) {
                code += QString("    %1(%2);\n")
                    .arg(instr.function, instr.args.join(", "));
            } else {
                code += QString("    %1 = %2(%3);\n")
                    .arg(instr.target, instr.function, instr.args.join(", "));
            }
            break;

        case IRInstruction::Assign:
            code += QString("    %1 = %2;\n")
                .arg(instr.target, instr.args.first());
            break;

        case IRInstruction::TypeConvert:
            code += QString("    %1 = (%2)%3;\n")
                .arg(instr.target, instr.function, instr.args.first());
            break;

        case IRInstruction::Branch:
            code += QString("    if (%1) goto %2; else goto %3;\n")
                .arg(instr.args[0], instr.args[1], instr.args[2]);
            break;

        case IRInstruction::Label:
            code += QString("%1:\n").arg(instr.label);
            break;

        case IRInstruction::Goto:
            code += QString("    goto %1;\n").arg(instr.label);
            break;

        case IRInstruction::Return:
            code += QString("    return %1;\n")
                .arg(instr.args.isEmpty() ? "0" : instr.args.first());
            break;
        }
    }

    code += "\n    return 0;\n";
    code += "}\n";

    return code;
}
```

### Пример: граф и сгенерированный код

Граф:

```
[ReadInt "a"] ──result──▶ a ──[Add]──result──▶ value ──[Print]
[ReadInt "b"] ──result──▶ b ──┘
```

Сгенерированный код:

```c
#include "read_int.h"
#include "add.h"
#include "print.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    int var_n1_result;
    int var_n2_result;
    int var_n3_result;

    // Узел n1: read_int (Чтение "a")
    var_n1_result = dq_read_int();

    // Узел n2: read_int (Чтение "b")
    var_n2_result = dq_read_int();

    // Узел n3: add (Сложение)
    var_n3_result = dq_add(var_n1_result, var_n2_result);

    // Узел n4: print (Вывод)
    dq_print_int(var_n3_result);

    return 0;
}
```

---

## Визуальная отладка

### Режим отладки графа

При запуске отладки блочный редактор переходит в режим визуальной отладки:

```cpp
class GraphDebugger : public QObject {
    Q_OBJECT
public:
    explicit GraphDebugger(BlockScene* scene, DebugManager* debugMgr,
                           QObject* parent = nullptr);

    void startDebugging();
    void stopDebugging();

    // Точки останова на узлах
    void addNodeBreakpoint(const QString& nodeId);
    void removeNodeBreakpoint(const QString& nodeId);

    // Пошаговое выполнение по узлам
    void stepToNextNode();
    void continueToBreakpoint();

signals:
    void nodeActivated(const QString& nodeId);          // Узел начал выполнение
    void nodeCompleted(const QString& nodeId);           // Узел завершил выполнение
    void connectionActivated(const QString& connId);     // Данные передаются по соединению
    void variableValueChanged(const QString& nodeId,
                               const QString& portName,
                               const QString& value);    // Значение на порту изменилось

private:
    void mapSourceLineToNode(int line, const QString& file);

    BlockScene* m_scene;
    DebugManager* m_debugMgr;
    QMap<int, QString> m_lineToNode;  // строка сгенерированного кода -> nodeId
};
```

### Визуализация во время отладки

| Элемент | Обычный режим | Режим отладки |
|---------|--------------|---------------|
| **Текущий узел** | Обычная рамка | Жёлтая рамка, пульсация |
| **Выполненные узлы** | Обычная рамка | Зелёная рамка |
| **Активное соединение** | Обычная линия | Анимированная линия (бегущие точки) |
| **Значения на портах** | Не отображаются | Всплывающие подписи со значениями |
| **Ошибка** | Не отображается | Красная рамка, сообщение об ошибке |

```cpp
void GraphDebugger::onNodeActivated(const QString& nodeId) {
    // Подсветить текущий узел
    NodeItem* node = m_scene->nodeById(nodeId);
    node->setHighlighted(true);

    // Прокрутить вид к узлу
    m_scene->views().first()->centerOn(node);

    // Анимация пульсации
    QPropertyAnimation* anim = new QPropertyAnimation(node, "opacity");
    anim->setDuration(500);
    anim->setKeyValueAt(0, 1.0);
    anim->setKeyValueAt(0.5, 0.7);
    anim->setKeyValueAt(1, 1.0);
    anim->setLoopCount(-1);
    anim->start();
}

void GraphDebugger::onVariableValueChanged(const QString& nodeId,
                                            const QString& portName,
                                            const QString& value) {
    NodeItem* node = m_scene->nodeById(nodeId);
    PortItem* port = node->portByName(portName);

    // Показать значение рядом с портом
    port->showValueTooltip(value);
}
```

---

## Палитра модулей

### Структура палитры

```cpp
class ModulePalette : public QWidget {
    Q_OBJECT
public:
    explicit ModulePalette(ModuleRegistry* registry, QWidget* parent = nullptr);

    void setFilter(const QString& text);
    void setCategory(const QString& category);

private:
    void buildTree();
    void onModuleLoaded(const Module* module);
    void onModuleUnloaded(const QString& id);

    QTreeWidget* m_tree;
    QLineEdit* m_searchField;
    ModuleRegistry* m_registry;
};
```

Палитра отображает доступные модули, сгруппированные по категориям:

```
📁 math
  ├── add (Сложение)
  ├── subtract (Вычитание)
  ├── multiply (Умножение)
  └── divide (Деление)
📁 logic
  ├── and (И)
  ├── or (ИЛИ)
  ├── not (НЕ)
  └── branch (Ветвление)
📁 io
  ├── print (Вывод)
  ├── read_int (Чтение числа)
  └── read_string (Чтение строки)
📁 custom
  └── (пользовательские модули)
```

---

## Составные модули и навигация (матрёшка)

### Граф как модуль

Любой граф в блочном редакторе автоматически становится составным модулем:

1. Пользователь размещает специальные узлы `GraphInput` и `GraphOutput` для определения внешних портов
2. При сохранении графа IDE автоматически:
   - Компилирует граф в C-функцию
   - Генерирует `.dqmod` с `origin: "graph"`
   - Регистрирует составной модуль в палитре
3. Теперь этот граф можно перетащить как блок в другой граф

### Визуальное отличие блоков

```
┌─────────────────────────┐     ┌═════════════════════════╗
│  ▶ exec_in    exec_out ▶│     ║  ▶ exec_in    exec_out ▶║
├─────────────────────────┤     ╠═════════════════════════╣
│        Сложение          │     ║      Вычисление    [⊞]  ║  ← иконка [⊞] = составной
│         (add)            │     ║       (calc)             ║
├─────────────────────────┤     ╠═════════════════════════╣
│  ● a [int]     result ● │     ║  ● a [int]     result ● ║
│  ● b [int]              │     ║  ● b [int]              ║
└─────────────────────────┘     ╚═════════════════════════╝
     Атомарный модуль                Составной модуль
     (одинарная рамка)               (двойная рамка)
```

Составные модули отличаются визуально:
- **Двойная рамка** — сигнал что блок можно раскрыть
- **Иконка [⊞]** в заголовке — индикатор вложенности
- **Другой цвет заголовка** (например, тёмно-синий vs зелёный для атомарных)

### Навигация по вложенности

```cpp
class BlockEditorWidget : public QWidget {
    // ...

    // Навигация по иерархии модулей
    void drillInto(NodeItem* compositeNode);  // Двойной клик → раскрыть
    void drillUp();                            // Кнопка "Назад" → к родителю
    void drillToRoot();                        // К корневому графу

    // Хлебные крошки (breadcrumbs): root > calc > inner_logic
    QStringList navigationPath() const;

private:
    QStack<QString> m_navigationStack;  // Стек открытых графов
    QString m_currentGraphPath;
};
```

При двойном клике на узел:

| Тип модуля | Действие |
|-----------|----------|
| **Атомарный** (`origin: "user"` / `"library"`) | Открыть исходный .cpp файл в Code Editor (Module 1) |
| **Составной** (`origin: "graph"`) | Загрузить вложенный .dqgraph на холст, показать его содержимое |

### Хлебные крошки (Breadcrumbs)

В верхней части Block Editor отображается путь навигации:

```
📁 main_app  ▶  📦 calc  ▶  📦 inner_logic
```

Клик на любой элемент — переход к этому уровню. Это аналог навигации по файловой системе, но по структуре программы.

### Пример рекурсивной вложенности

```
main_app.dqgraph
├── [read_input]     → атомарный (read_input.cpp)
├── [calc]           → составной (calc.dqgraph)
│   ├── [add]        → атомарный (add.cpp)
│   └── [multiply]   → атомарный (multiply.cpp)
├── [format_output]  → составной (format_output.dqgraph)
│   ├── [to_string]  → атомарный (to_string.cpp)
│   └── [concat]     → атомарный (concat.cpp)
└── [print]          → атомарный (print.cpp)
```

При компиляции это дерево разворачивается в линейную последовательность вызовов C-функций.

---

## Операции с графом

### Поддерживаемые действия (все через CommandBus)

| Действие | Команда | Undo |
|----------|---------|------|
| Добавить узел | `AddNodeCommand` | Удаляет узел |
| Удалить узел | `RemoveNodeCommand` | Восстанавливает узел со всеми соединениями |
| Переместить узел | `MoveNodeCommand` | Возвращает на старую позицию |
| Соединить порты | `ConnectPortsCommand` | Удаляет соединение |
| Удалить соединение | `DisconnectPortsCommand` | Восстанавливает соединение |
| Изменить значение | `ChangeNodePropertyCommand` | Возвращает старое значение |
| Копировать | `CopyNodesCommand` | -- (в буфер обмена, без undo). Копируются **только узлы без связей** — пользователь соединяет сам |
| Вставить | `PasteNodesCommand` | Удаляет вставленные узлы. Узлы вставляются без соединений |
| Группировать | `GroupNodesCommand` | Разгруппировывает |
| Выравнивание | `AlignNodesCommand` | Возвращает на старые позиции |

### Горячие клавиши Block Editor

| Действие | Комбинация |
|----------|-----------|
| Удалить выделенное | `Delete` |
| Копировать | `Ctrl+C` |
| Вставить | `Ctrl+V` |
| Выделить все | `Ctrl+A` |
| Увеличить масштаб | `Ctrl++` / колесо мыши |
| Уменьшить масштаб | `Ctrl+-` / колесо мыши |
| Вписать в окно | `Ctrl+0` |
| Выровнять по сетке | `Ctrl+Shift+G` |
| Автоматическая компоновка | `Ctrl+L` |

---

## Многопоточные блоки (ThreadModule)

Графическая система не ограничивается однопоточным выполнением. Специальный блок **«Независимый поток»** позволяет вынести часть логики в отдельный поток выполнения.

### Визуальное представление на графе

```
  Основной поток                    Отдельный поток
  ─────────────                     ─────────────────
  [read_data] ──▶ [process] ──▶ [display]
                      │
                 sync_out ●━━━━━━━━━● sync_in
                                        │
                              [heavy_compute] ──▶ [result]
                                                      │
                                              sync_out ●━━━━━━━● sync_in ──▶ [merge]
```

### Порты синхронизации

| Тип порта | Описание |
|-----------|----------|
| `sync_out` | Отправка данных в другой поток (неблокирующая очередь) |
| `sync_in` | Получение данных из другого потока (блокирующее ожидание / poll) |

Это визуальный аналог `std::thread` + `std::mutex` / очередей сообщений. Пользователь видит на графе потоки выполнения и точки синхронизации, не работая напрямую с мьютексами.

### Генерация кода

ThreadModule компилируется в:
- **C:** `pthread_create()` + `pthread_mutex_t` + очередь сообщений
- **Python:** `threading.Thread` + `queue.Queue`
- **Rust:** `std::thread::spawn` + `mpsc::channel`
