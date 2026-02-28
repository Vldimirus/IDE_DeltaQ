// Дерево проекта — навигация по файлам
#include "ProjectTreeView.h"
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QPainter>

namespace DeltaQ {

// --- DiagnosticDelegate ---

void DiagnosticDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    // Получаем путь файла из модели
    auto *model = qobject_cast<const QFileSystemModel *>(index.model());
    if (!model) return;

    QString path = model->filePath(index);
    auto it = m_diagnosticCounts.find(path);
    if (it == m_diagnosticCounts.end()) return;

    int errors = it.value().first;
    int warnings = it.value().second;
    if (errors == 0 && warnings == 0) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    int badgeHeight = 14;
    int x = option.rect.right() - 4;

    // Бейдж предупреждений (жёлтый) — рисуем первым (левее)
    if (warnings > 0) {
        QString text = QString::number(warnings);
        int textWidth = painter->fontMetrics().horizontalAdvance(text) + 8;
        QRect badgeRect(x - textWidth, option.rect.top() + (option.rect.height() - badgeHeight) / 2,
                        textWidth, badgeHeight);
        painter->setBrush(QColor(255, 180, 0));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(badgeRect, 3, 3);
        painter->setPen(Qt::white);
        QFont font = painter->font();
        font.setPixelSize(10);
        font.setBold(true);
        painter->setFont(font);
        painter->drawText(badgeRect, Qt::AlignCenter, text);
        x = badgeRect.left() - 4;
    }

    // Бейдж ошибок (красный)
    if (errors > 0) {
        QString text = QString::number(errors);
        int textWidth = painter->fontMetrics().horizontalAdvance(text) + 8;
        QRect badgeRect(x - textWidth, option.rect.top() + (option.rect.height() - badgeHeight) / 2,
                        textWidth, badgeHeight);
        painter->setBrush(QColor(220, 50, 50));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(badgeRect, 3, 3);
        painter->setPen(Qt::white);
        QFont font = painter->font();
        font.setPixelSize(10);
        font.setBold(true);
        painter->setFont(font);
        painter->drawText(badgeRect, Qt::AlignCenter, text);
    }

    painter->restore();
}

void DiagnosticDelegate::setDiagnosticCounts(const QString &path, int errors, int warnings)
{
    if (errors == 0 && warnings == 0)
        m_diagnosticCounts.remove(path);
    else
        m_diagnosticCounts[path] = {errors, warnings};
}

// --- ProjectTreeView ---

ProjectTreeView::ProjectTreeView(QWidget *parent)
    : QTreeView(parent)
    , m_model(new QFileSystemModel(this))
    , m_delegate(new DiagnosticDelegate(this))
{
    m_model->setNameFilters({"*.c", "*.cpp", "*.h", "*.hpp",
                              "*.dqmod", "*.dqgraph", "*.dqui", "*.dqproj",
                              "CMakeLists.txt"});
    m_model->setNameFilterDisables(false);
    setModel(m_model);
    setItemDelegate(m_delegate);

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

void ProjectTreeView::updateDiagnosticCounts(const QString &path, int errors, int warnings)
{
    m_delegate->setDiagnosticCounts(path, errors, warnings);
    viewport()->update();
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
