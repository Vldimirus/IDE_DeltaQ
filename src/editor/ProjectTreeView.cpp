// Дерево проекта — навигация по файлам
#include "ProjectTreeView.h"
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDir>

namespace DeltaQ {

ProjectTreeView::ProjectTreeView(QWidget *parent)
    : QTreeView(parent)
    , m_model(new QFileSystemModel(this))
{
    m_model->setNameFilters({"*.c", "*.cpp", "*.h", "*.hpp",
                              "*.dqmod", "*.dqgraph", "*.dqui", "*.dqproj",
                              "CMakeLists.txt"});
    m_model->setNameFilterDisables(false);
    setModel(m_model);

    // Скрываем лишние колонки (размер, тип, дата)
    for (int i = 1; i < m_model->columnCount(); ++i)
        hideColumn(i);

    setHeaderHidden(true);
    setAnimated(true);
    setSortingEnabled(true);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QTreeView::doubleClicked, this, &ProjectTreeView::onDoubleClicked);
    connect(this, &QWidget::customContextMenuRequested, this, &ProjectTreeView::showContextMenu);
}

void ProjectTreeView::setRootPath(const QString &path)
{
    m_rootPath = path;
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

void ProjectTreeView::showContextMenu(const QPoint &pos)
{
    QModelIndex index = indexAt(pos);
    QString path = index.isValid() ? m_model->filePath(index) : m_rootPath;
    QFileInfo fi(path);

    QMenu menu(this);

    if (fi.isFile()) {
        menu.addAction(tr("Open"), this, [this, path]() {
            emit fileSelected(path);
        });
        menu.addSeparator();
    }

    // Определяем директорию для создания файла
    QString dir = fi.isDir() ? path : fi.absolutePath();

    menu.addAction(tr("New File..."), this, [this, dir]() {
        QString name = QInputDialog::getText(this, tr("New File"), tr("File name:"));
        if (name.isEmpty()) return;
        QString filePath = dir + "/" + name;
        QFile file(filePath);
        if (file.exists()) {
            QMessageBox::warning(this, tr("Error"), tr("File already exists"));
            return;
        }
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            emit fileSelected(filePath);
        }
    });

    menu.addAction(tr("New Folder..."), this, [this, dir]() {
        QString name = QInputDialog::getText(this, tr("New Folder"), tr("Folder name:"));
        if (name.isEmpty()) return;
        QDir(dir).mkdir(name);
    });

    if (fi.isFile()) {
        menu.addSeparator();
        menu.addAction(tr("Delete"), this, [this, path, fi]() {
            auto result = QMessageBox::question(this, tr("Delete File"),
                tr("Delete '%1'?").arg(fi.fileName()),
                QMessageBox::Yes | QMessageBox::No);
            if (result == QMessageBox::Yes)
                QFile::remove(path);
        });
    }

    menu.addSeparator();
    menu.addAction(tr("Show in File Manager"), this, [dir]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });

    menu.exec(viewport()->mapToGlobal(pos));
}

} // namespace DeltaQ
