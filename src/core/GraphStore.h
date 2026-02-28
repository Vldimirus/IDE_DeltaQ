#pragma once

#include <QObject>
#include <QMap>
#include <QVector>
#include <deltaq/Graph.h>

namespace DeltaQ {

// Хранилище графов — управление коллекцией Graph с файловой персистентностью (.dqgraph)
class GraphStore : public QObject {
    Q_OBJECT

public:
    explicit GraphStore(QObject *parent = nullptr);

    // Управление графами
    bool registerGraph(const Graph &graph);
    bool unregisterGraph(const QString &id);
    Graph *findGraph(const QString &id);
    const Graph *findGraph(const QString &id) const;
    Graph *findGraphByName(const QString &name);
    QVector<const Graph *> allGraphs() const;

    // Файловый I/O
    bool loadGraphFile(const QString &dqgraphPath);
    bool saveGraphFile(const Graph &graph, const QString &dqgraphPath) const;

    // Загрузка всех .dqgraph из директории проекта
    bool loadFromDirectory(const QString &projectDir);
    bool saveAll(const QString &projectDir) const;

    int count() const { return m_graphs.size(); }
    void clear();

signals:
    void graphRegistered(const QString &id);
    void graphUnregistered(const QString &id);
    void graphUpdated(const QString &id);
    void storeCleared();

private:
    QMap<QString, Graph> m_graphs; // id -> Graph
};

} // namespace DeltaQ
