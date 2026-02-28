#include "GraphStore.h"

#include <QFile>
#include <QJsonDocument>
#include <QDirIterator>

namespace DeltaQ {

GraphStore::GraphStore(QObject *parent)
    : QObject(parent)
{
}

bool GraphStore::registerGraph(const Graph &graph)
{
    if (graph.id.isEmpty())
        return false;

    bool isUpdate = m_graphs.contains(graph.id);
    m_graphs[graph.id] = graph;

    if (isUpdate)
        emit graphUpdated(graph.id);
    else
        emit graphRegistered(graph.id);
    return true;
}

bool GraphStore::unregisterGraph(const QString &id)
{
    if (m_graphs.remove(id) > 0) {
        emit graphUnregistered(id);
        return true;
    }
    return false;
}

Graph *GraphStore::findGraph(const QString &id)
{
    auto it = m_graphs.find(id);
    return it != m_graphs.end() ? &it.value() : nullptr;
}

const Graph *GraphStore::findGraph(const QString &id) const
{
    auto it = m_graphs.find(id);
    return it != m_graphs.end() ? &it.value() : nullptr;
}

Graph *GraphStore::findGraphByName(const QString &name)
{
    for (auto &g : m_graphs) {
        if (g.name == name)
            return &g;
    }
    return nullptr;
}

QVector<const Graph *> GraphStore::allGraphs() const
{
    QVector<const Graph *> result;
    result.reserve(m_graphs.size());
    for (const auto &g : m_graphs)
        result.append(&g);
    return result;
}

bool GraphStore::loadGraphFile(const QString &dqgraphPath)
{
    QFile file(dqgraphPath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError)
        return false;

    Graph graph = Graph::fromJson(doc.object());
    return registerGraph(graph);
}

bool GraphStore::saveGraphFile(const Graph &graph, const QString &dqgraphPath) const
{
    QFile file(dqgraphPath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QJsonDocument doc(graph.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool GraphStore::loadFromDirectory(const QString &projectDir)
{
    QDirIterator it(projectDir, {"*.dqgraph"}, QDir::Files, QDirIterator::Subdirectories);
    bool anyLoaded = false;
    while (it.hasNext()) {
        if (loadGraphFile(it.next()))
            anyLoaded = true;
    }
    return anyLoaded;
}

bool GraphStore::saveAll(const QString &projectDir) const
{
    QDir dir(projectDir + "/graphs");
    if (!dir.exists())
        dir.mkpath(".");

    for (const auto &g : m_graphs) {
        QString path = dir.absoluteFilePath(g.name + ".dqgraph");
        if (!saveGraphFile(g, path))
            return false;
    }
    return true;
}

void GraphStore::clear()
{
    m_graphs.clear();
    emit storeCleared();
}

} // namespace DeltaQ
