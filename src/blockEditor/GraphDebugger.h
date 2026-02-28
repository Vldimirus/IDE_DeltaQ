// Визуальная отладка графов — маппинг строк кода на узлы, подсветка
#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QVector>
#include "../debug/DebugTypes.h"

namespace DeltaQ {

class BlockScene;
class DebugManager;

class GraphDebugger : public QObject {
    Q_OBJECT

public:
    GraphDebugger(BlockScene *scene, DebugManager *debugManager,
                  QObject *parent = nullptr);

    // Установка маппинга строк → узлов (из CompilationResult::sourceMap)
    void setSourceMap(const QMap<int, QString> &sourceMap);

    // Очистка визуального состояния
    void clearDebugState();

public slots:
    void onBreakpointHit(const QString &file, int line);
    void onStepped(const QString &file, int line);
    void onDebugStopped();
    void onVariablesUpdated(const QVector<Variable> &vars);

private:
    void highlightNode(const QString &nodeId);
    void updatePortValues(const QVector<Variable> &vars);

    BlockScene *m_scene;
    DebugManager *m_debugManager;
    QMap<int, QString> m_lineToNode;     // строка → nodeId
    QString m_currentNodeId;             // текущий подсвеченный узел
    QStringList m_completedNodes;        // выполненные узлы
    QString m_generatedFile;             // путь к сгенерированному файлу
};

} // namespace DeltaQ
