// Диалог создания нового файла внутри проекта
#pragma once

#include <QDialog>

class QListWidget;
class QLineEdit;
class QLabel;

namespace DeltaQ {

class NewFileDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewFileDialog(const QString &projectDir, QWidget *parent = nullptr);

    // Тип файла: "dqui", "dqgraph", "scratchpad", "dqmod", "c", "h"
    QString fileType() const;
    // Имя файла (без расширения)
    QString fileName() const;
    // Полный путь к создаваемому файлу
    QString fullPath() const;

private:
    void updatePreview();

    QString m_projectDir;
    QListWidget *m_typeList = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QLabel *m_pathLabel = nullptr;
};

} // namespace DeltaQ
