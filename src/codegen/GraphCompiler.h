// Компилятор графов — Graph → IR → C-код
#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QSet>

namespace DeltaQ {

struct Graph;
struct GraphNode;
struct Module;
struct IR;
class ModuleRegistry;

struct CompilationResult {
    bool success = false;
    QString generatedCode;
    QStringList errors;
    QStringList warnings;
    QMap<int, QString> sourceMap;  // строка кода → nodeId (для отладки)
};

class GraphStore;

class GraphCompiler {
public:
    explicit GraphCompiler(ModuleRegistry *registry);

    // Установить GraphStore для рекурсивной компиляции подмодулей
    void setGraphStore(GraphStore *store) { m_graphStore = store; }

    // Компиляция графа в C-код
    CompilationResult compile(const Graph &graph);

private:
    // Топологическая сортировка (возвращает упорядоченные nodeId)
    QStringList topologicalSort(const Graph &graph, QStringList &errors);

    // Топологическая сортировка только по data-связям (алгоритм Кана)
    QStringList topologicalSortByData(const Graph &graph, QStringList &errors);

    // Сортировка по execution flow с учётом data-зависимостей
    QStringList topologicalSortByExecution(const Graph &graph, QStringList &errors);

    // Рекурсивная вставка data-зависимостей перед exec-узлом
    void insertWithDataDeps(const QString &nodeId, const Graph &graph,
                            QSet<QString> &visited, QStringList &result,
                            const QMap<QString, QSet<QString>> &dataDeps);

    // Генерация IR из отсортированного графа
    IR generateIR(const Graph &graph, const QStringList &sortedNodes, CompilationResult &result);

    // Специальные inline-модули desktop-runtime
    bool isInlineDesktopModule(const QString &moduleId) const;
    bool emitInlineDesktopModuleIR(const GraphNode &node, const Module &mod,
                                   const QStringList &callArgs,
                                   const QMap<QString, QString> &portVarMap,
                                   IR &ir, CompilationResult &result);

    // Рекурсивная компиляция подмодуля
    QString compileSubModule(const Module &mod, CompilationResult &result);

    // Проверка совместимости типов
    bool areTypesCompatible(const QString &from, const QString &to) const;
    bool needsTypeConversion(const QString &from, const QString &to) const;

    // C-тип для типа порта
    QString toCType(const QString &portType) const;

    ModuleRegistry *m_registry;
    GraphStore *m_graphStore = nullptr;
    QSet<QString> m_compiledSubModules; // Отслеживание уже скомпилированных подмодулей
};

} // namespace DeltaQ
