// Дерево проекта — навигация по файлам
#pragma once

#include <QTreeView>
#include <QFileSystemModel>

namespace DeltaQ {

class ProjectTreeView : public QTreeView {
    Q_OBJECT

public:
    explicit ProjectTreeView(QWidget *parent = nullptr);

    void setRootPath(const QString &path);

signals:
    void fileSelected(const QString &path);

private slots:
    void onDoubleClicked(const QModelIndex &index);

private:
    QFileSystemModel *m_model;
};

} // namespace DeltaQ
