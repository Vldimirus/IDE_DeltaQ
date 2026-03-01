// Дерево проекта — навигация по файлам
#pragma once

#include <QTreeView>
#include <QFileSystemModel>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QMap>

namespace DeltaQ {

// Делегат для отрисовки бейджей ошибок/предупреждений
class DiagnosticDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    void setDiagnosticCounts(const QString &path, int errors, int warnings);

private:
    QMap<QString, QPair<int, int>> m_diagnosticCounts; // path → (errors, warnings)
};

class ProjectTreeView : public QTreeView {
    Q_OBJECT

public:
    explicit ProjectTreeView(QWidget *parent = nullptr);

    void setRootPath(const QString &path);
    QString rootPath() const { return m_rootPath; }

    // Обновить счётчики диагностики для файла
    void updateDiagnosticCounts(const QString &path, int errors, int warnings);

signals:
    void fileSelected(const QString &path);
    void newFileRequested(const QString &directory);

private slots:
    void onDoubleClicked(const QModelIndex &index);
    void showContextMenu(const QPoint &pos);

private:
    QFileSystemModel *m_model;
    DiagnosticDelegate *m_delegate;
    QString m_rootPath;
};

} // namespace DeltaQ
