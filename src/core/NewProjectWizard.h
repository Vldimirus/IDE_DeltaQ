// Мастер создания нового проекта (QWizard с 3 страницами)
#pragma once

#include <QWizard>
#include <QVector>

#include "ProjectTemplates.h"

class QListWidget;
class QLineEdit;
class QLabel;

namespace DeltaQ {

class NewProjectWizard : public QWizard {
    Q_OBJECT

public:
    explicit NewProjectWizard(QWidget *parent = nullptr);

    QString projectName() const;
    QString projectDir() const;
    QString projectType() const;
    QString selectedTemplateId() const;
    void setDefaultDir(const QString &dir);

private:
    QWizardPage *createTypePage();
    QWizardPage *createNamePage();
    QWizardPage *createSummaryPage();

    void updateSummary();
    const ProjectTemplateInfo *selectedTemplate() const;

    // Страница 1 — тип проекта
    QListWidget *m_typeList = nullptr;
    QVector<ProjectTemplateInfo> m_templates;

    // Страница 2 — имя и расположение
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_dirEdit = nullptr;
    QLabel *m_fullPathLabel = nullptr;

    // Страница 3 — сводка
    QLabel *m_summaryLabel = nullptr;
};

} // namespace DeltaQ
