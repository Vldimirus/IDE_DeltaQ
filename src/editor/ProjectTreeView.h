// Дерево проекта — навигация по файлам
#pragma once

#include <QTreeView>
#include <QFileSystemModel>
#include <QMenu>

namespace DeltaQ {

class ProjectTreeView : public QTreeView {
    Q_OBJECT

public:
    explicit ProjectTreeView(QWidget *parent = nullptr);

    void setRootPath(const QString &path);
    QString rootPath() const { return m_rootPath; }

signals:
    void fileSelected(const QString &path);

private slots:
    void onDoubleClicked(const QModelIndex &index);
    void showContextMenu(const QPoint &pos);

private:
    QFileSystemModel *m_model;
    QString m_rootPath;
};

} // namespace DeltaQ
