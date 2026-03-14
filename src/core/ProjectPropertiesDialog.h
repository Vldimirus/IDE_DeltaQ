#pragma once

#include "../codegen/ToolchainResolver.h"

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace DeltaQ {

class ProjectPropertiesDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProjectPropertiesDialog(const Project &project,
                                     const ProjectLocalSettings &localSettings,
                                     QWidget *parent = nullptr);

    void setToolchainInventory(const ToolchainInventory &inventory);

    const Project &project() const { return m_project; }
    const ProjectLocalSettings &localSettings() const { return m_localSettings; }

public slots:
    void accept() override;

signals:
    void rescanRequested();

private slots:
    void updateUiState();

private:
    void setupUi();
    void populateFromModels();
    void rebuildKitCombo();
    void updateResolvedSummary();
    QString kitStatusLabel(const QString &status) const;
    QStringList parseFlags(const QString &text) const;

    Project m_project;
    ProjectLocalSettings m_localSettings;
    ToolchainInventory m_inventory;
    QVector<DetectedToolchainKit> m_detectedKits;

    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_versionEdit = nullptr;
    QComboBox *m_typeCombo = nullptr;

    QComboBox *m_profileCombo = nullptr;
    QComboBox *m_cStandardCombo = nullptr;
    QComboBox *m_cxxStandardCombo = nullptr;
    QComboBox *m_compilerPreferenceCombo = nullptr;
    QComboBox *m_generatorPreferenceCombo = nullptr;
    QLineEdit *m_extraCFlagsEdit = nullptr;
    QLineEdit *m_extraCxxFlagsEdit = nullptr;

    QComboBox *m_selectionModeCombo = nullptr;
    QComboBox *m_detectedKitCombo = nullptr;
    QLineEdit *m_manualCMakePathEdit = nullptr;
    QLineEdit *m_manualCCompilerPathEdit = nullptr;
    QLineEdit *m_manualCxxCompilerPathEdit = nullptr;
    QLineEdit *m_manualBuilderPathEdit = nullptr;
    QComboBox *m_manualGeneratorCombo = nullptr;
    QLabel *m_toolchainSummaryLabel = nullptr;
    QPushButton *m_rescanButton = nullptr;
};

} // namespace DeltaQ
