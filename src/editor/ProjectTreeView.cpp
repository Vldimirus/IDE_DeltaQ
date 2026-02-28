// Дерево проекта — навигация по файлам
#include "ProjectTreeView.h"
#include <QFileInfo>

namespace DeltaQ {

ProjectTreeView::ProjectTreeView(QWidget *parent)
    : QTreeView(parent)
    , m_model(new QFileSystemModel(this))
{
    m_model->setNameFilters({"*.c", "*.cpp", "*.h", "*.hpp",
                              "*.dqmod", "*.dqgraph", "*.dqui", "*.dqproj"});
    m_model->setNameFilterDisables(false);
    setModel(m_model);

    // Скрываем лишние колонки (размер, тип, дата)
    for (int i = 1; i < m_model->columnCount(); ++i)
        hideColumn(i);

    setHeaderHidden(true);
    setAnimated(true);
    setSortingEnabled(true);

    connect(this, &QTreeView::doubleClicked, this, &ProjectTreeView::onDoubleClicked);
}

void ProjectTreeView::setRootPath(const QString &path)
{
    auto idx = m_model->setRootPath(path);
    setRootIndex(idx);
}

void ProjectTreeView::onDoubleClicked(const QModelIndex &index)
{
    QString path = m_model->filePath(index);
    QFileInfo fi(path);
    if (fi.isFile())
        emit fileSelected(path);
}

} // namespace DeltaQ
