// Компилятор графов — Graph → IR → C-код
#pragma once

#include <QString>
#include <QStringList>
#include <QMap>

namespace DeltaQ {

struct Graph;
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

class GraphCompiler {
public:
    explicit GraphCompiler(ModuleRegistry *registry);

    // Компиляция графа в C-код
    CompilationResult compile(const Graph &graph);

private:
    // Топологическая сортировка (возвращает упорядоченные nodeId)
    QStringList topologicalSort(const Graph &graph, QStringList &errors);

    // Генерация IR из отсортированного графа
    IR generateIR(const Graph &graph, const QStringList &sortedNodes, CompilationResult &result);

    // Проверка совместимости типов
    bool areTypesCompatible(const QString &from, const QString &to) const;
    bool needsTypeConversion(const QString &from, const QString &to) const;

    // C-тип для типа порта
    QString toCType(const QString &portType) const;

    ModuleRegistry *m_registry;
};

} // namespace DeltaQ
