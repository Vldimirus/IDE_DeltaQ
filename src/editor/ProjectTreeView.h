// Дерево проекта — навигация по файлам
#pragma once

#include <QTreeView>
#include <QFileSystemModel>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QMap>
#include <QSet>

namespace DeltaQ {

// Делегат для отрисовки бейджей ошибок/предупреждений
class DiagnosticDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    void setDiagnosticCounts(const QString &path, int errors, int warnings);
    void setGeneratedFile(const QString &path, bool generated);
    void clearGeneratedFiles();
    bool isGeneratedFile(const QString &path) const;

private:
    QMap<QString, QPair<int, int>> m_diagnosticCounts; // path → (errors, warnings)
    QSet<QString> m_generatedFiles;
};

class ProjectTreeView : public QTreeView {
    Q_OBJECT

public:
    explicit ProjectTreeView(QWidget *parent = nullptr);

    void setRootPath(const QString &path);
    QString rootPath() const { return m_rootPath; }

    // Обновить счётчики диагностики для файла
    void updateDiagnosticCounts(const QString &path, int errors, int warnings);
    void markGeneratedFile(const QString &path, bool generated = true);
    bool isGeneratedFile(const QString &path) const;
    QString generatedOriginPath(const QString &path) const;

signals:
    void fileSelected(const QString &path);
    void newFileRequested(const QString &directory);

private slots:
    void onDoubleClicked(const QModelIndex &index);
    void showContextMenu(const QPoint &pos);

private:
    void refreshGeneratedFiles();

    QFileSystemModel *m_model;
    DiagnosticDelegate *m_delegate;
    QString m_rootPath;
};

} // namespace DeltaQ
