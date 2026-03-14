#include "ProjectPropertiesDialog.h"

#include <deltaq/Project.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

namespace DeltaQ {

ProjectPropertiesDialog::ProjectPropertiesDialog(const Project &project,
                                                 const ProjectLocalSettings &localSettings,
                                                 QWidget *parent)
    : QDialog(parent)
    , m_project(project)
    , m_localSettings(localSettings)
{
    setWindowTitle(tr("Project Properties"));
    setMinimumSize(680, 460);

    setupUi();
    populateFromModels();
    updateUiState();
}

void ProjectPropertiesDialog::setToolchainInventory(const ToolchainInventory &inventory)
{
    m_inventory = inventory;
    m_detectedKits = ToolchainResolver::detectKits(m_inventory);
    rebuildKitCombo();
    updateResolvedSummary();
}

void ProjectPropertiesDialog::accept()
{
    m_project.version = m_versionEdit->text().trimmed();
    if (m_project.version.isEmpty())
        m_project.version = "1.0.0";

    m_project.build.compiler = m_compilerPreferenceCombo->currentData().toString();
    m_project.build.generator = m_generatorPreferenceCombo->currentData().toString();
    m_project.build.buildProfile = m_profileCombo->currentData().toString();
    m_project.build.cStandard = m_cStandardCombo->currentData().toString();
    m_project.build.cxxStandard = m_cxxStandardCombo->currentData().toString();
    m_project.build.extraCFlags = parseFlags(m_extraCFlagsEdit->text());
    m_project.build.extraCxxFlags = parseFlags(m_extraCxxFlagsEdit->text());

    m_localSettings.selectionMode = m_selectionModeCombo->currentData().toString();
    m_localSettings.selectedKitId = (m_localSettings.selectionMode == "kit")
        ? m_detectedKitCombo->currentData().toString()
        : QString();
    m_localSettings.manualOverride.enabled = (m_localSettings.selectionMode == "manual");
    m_localSettings.manualOverride.cmakePath = m_manualCMakePathEdit->text().trimmed();
    m_localSettings.manualOverride.cCompilerPath = m_manualCCompilerPathEdit->text().trimmed();
    m_localSettings.manualOverride.cxxCompilerPath = m_manualCxxCompilerPathEdit->text().trimmed();
    m_localSettings.manualOverride.builderPath = m_manualBuilderPathEdit->text().trimmed();
    m_localSettings.manualOverride.generator = m_manualGeneratorCombo->currentData().toString();

    QDialog::accept();
}

void ProjectPropertiesDialog::updateUiState()
{
    const QString selectionMode = m_selectionModeCombo->currentData().toString();
    const bool kitMode = selectionMode == "kit";
    const bool manualMode = selectionMode == "manual";

    m_detectedKitCombo->setEnabled(kitMode);
    m_manualCMakePathEdit->setEnabled(manualMode);
    m_manualCCompilerPathEdit->setEnabled(manualMode);
    m_manualCxxCompilerPathEdit->setEnabled(manualMode);
    m_manualBuilderPathEdit->setEnabled(manualMode);
    m_manualGeneratorCombo->setEnabled(manualMode);

    updateResolvedSummary();
}

void ProjectPropertiesDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    auto *tabs = new QTabWidget(this);
    tabs->setObjectName("projectPropertiesTabs");

    auto *generalPage = new QWidget(this);
    auto *generalLayout = new QFormLayout(generalPage);
    m_nameEdit = new QLineEdit(generalPage);
    m_nameEdit->setObjectName("projectNameEdit");
    m_nameEdit->setReadOnly(true);
    generalLayout->addRow(tr("Name:"), m_nameEdit);

    m_versionEdit = new QLineEdit(generalPage);
    m_versionEdit->setObjectName("projectVersionEdit");
    generalLayout->addRow(tr("Version:"), m_versionEdit);

    m_typeCombo = new QComboBox(generalPage);
    m_typeCombo->setObjectName("projectTypeCombo");
    m_typeCombo->addItem(tr("Console"), "console");
    m_typeCombo->addItem(tr("Desktop"), "desktop");
    m_typeCombo->setEnabled(false);
    m_typeCombo->setToolTip(tr("Project type migration is not supported in v1."));
    generalLayout->addRow(tr("Type:"), m_typeCombo);
    tabs->addTab(generalPage, tr("General"));

    auto *buildPage = new QWidget(this);
    auto *buildLayout = new QFormLayout(buildPage);
    m_profileCombo = new QComboBox(buildPage);
    m_profileCombo->setObjectName("buildProfileCombo");
    m_profileCombo->addItem("Debug", "Debug");
    m_profileCombo->addItem("Release", "Release");
    m_profileCombo->addItem("RelWithDebInfo", "RelWithDebInfo");
    m_profileCombo->addItem("MinSizeRel", "MinSizeRel");
    buildLayout->addRow(tr("Build profile:"), m_profileCombo);

    m_cStandardCombo = new QComboBox(buildPage);
    m_cStandardCombo->setObjectName("cStandardCombo");
    m_cStandardCombo->addItem("c11", "c11");
    m_cStandardCombo->addItem("c17", "c17");
    m_cStandardCombo->addItem("c23", "c23");
    buildLayout->addRow(tr("C standard:"), m_cStandardCombo);

    m_cxxStandardCombo = new QComboBox(buildPage);
    m_cxxStandardCombo->setObjectName("cxxStandardCombo");
    m_cxxStandardCombo->addItem("c++17", "c++17");
    m_cxxStandardCombo->addItem("c++20", "c++20");
    m_cxxStandardCombo->addItem("c++23", "c++23");
    buildLayout->addRow(tr("C++ standard:"), m_cxxStandardCombo);

    m_compilerPreferenceCombo = new QComboBox(buildPage);
    m_compilerPreferenceCombo->setObjectName("compilerPreferenceCombo");
    m_compilerPreferenceCombo->addItem(tr("Auto"), "auto");
    m_compilerPreferenceCombo->addItem("GCC", "gcc");
    m_compilerPreferenceCombo->addItem("Clang", "clang");
    buildLayout->addRow(tr("Compiler preference:"), m_compilerPreferenceCombo);

    m_generatorPreferenceCombo = new QComboBox(buildPage);
    m_generatorPreferenceCombo->setObjectName("generatorPreferenceCombo");
    m_generatorPreferenceCombo->addItem(tr("Auto"), "auto");
    m_generatorPreferenceCombo->addItem("Ninja", "ninja");
    m_generatorPreferenceCombo->addItem("Unix Makefiles", "unix_makefiles");
    buildLayout->addRow(tr("Generator preference:"), m_generatorPreferenceCombo);

    m_extraCFlagsEdit = new QLineEdit(buildPage);
    m_extraCFlagsEdit->setObjectName("extraCFlagsEdit");
    m_extraCFlagsEdit->setPlaceholderText("-O2 -Wall");
    buildLayout->addRow(tr("Extra C flags:"), m_extraCFlagsEdit);

    m_extraCxxFlagsEdit = new QLineEdit(buildPage);
    m_extraCxxFlagsEdit->setObjectName("extraCxxFlagsEdit");
    m_extraCxxFlagsEdit->setPlaceholderText("-O2 -stdlib=libc++");
    buildLayout->addRow(tr("Extra C++ flags:"), m_extraCxxFlagsEdit);
    tabs->addTab(buildPage, tr("Build"));

    auto *toolchainPage = new QWidget(this);
    auto *toolchainLayout = new QFormLayout(toolchainPage);
    m_selectionModeCombo = new QComboBox(toolchainPage);
    m_selectionModeCombo->setObjectName("toolchainSelectionModeCombo");
    m_selectionModeCombo->addItem(tr("Auto"), "auto");
    m_selectionModeCombo->addItem(tr("Selected Kit"), "kit");
    m_selectionModeCombo->addItem(tr("Manual Override"), "manual");
    toolchainLayout->addRow(tr("Selection mode:"), m_selectionModeCombo);

    m_detectedKitCombo = new QComboBox(toolchainPage);
    m_detectedKitCombo->setObjectName("detectedKitCombo");
    toolchainLayout->addRow(tr("Detected kit:"), m_detectedKitCombo);

    auto *rescanRow = new QHBoxLayout;
    m_rescanButton = new QPushButton(tr("Rescan Toolchains"), toolchainPage);
    m_rescanButton->setObjectName("rescanToolchainsButton");
    rescanRow->addWidget(m_rescanButton);
    rescanRow->addStretch();
    toolchainLayout->addRow(QString(), rescanRow);

    m_manualCMakePathEdit = new QLineEdit(toolchainPage);
    m_manualCMakePathEdit->setObjectName("manualCMakePathEdit");
    toolchainLayout->addRow(tr("Manual CMake path:"), m_manualCMakePathEdit);

    m_manualCCompilerPathEdit = new QLineEdit(toolchainPage);
    m_manualCCompilerPathEdit->setObjectName("manualCCompilerPathEdit");
    toolchainLayout->addRow(tr("Manual C compiler path:"), m_manualCCompilerPathEdit);

    m_manualCxxCompilerPathEdit = new QLineEdit(toolchainPage);
    m_manualCxxCompilerPathEdit->setObjectName("manualCxxCompilerPathEdit");
    toolchainLayout->addRow(tr("Manual C++ compiler path:"), m_manualCxxCompilerPathEdit);

    m_manualBuilderPathEdit = new QLineEdit(toolchainPage);
    m_manualBuilderPathEdit->setObjectName("manualBuilderPathEdit");
    toolchainLayout->addRow(tr("Manual builder path:"), m_manualBuilderPathEdit);

    m_manualGeneratorCombo = new QComboBox(toolchainPage);
    m_manualGeneratorCombo->setObjectName("manualGeneratorCombo");
    m_manualGeneratorCombo->addItem(tr("Auto"), "");
    m_manualGeneratorCombo->addItem("Ninja", "ninja");
    m_manualGeneratorCombo->addItem("Unix Makefiles", "unix_makefiles");
    toolchainLayout->addRow(tr("Manual generator:"), m_manualGeneratorCombo);

    m_toolchainSummaryLabel = new QLabel(toolchainPage);
    m_toolchainSummaryLabel->setObjectName("toolchainSummaryLabel");
    m_toolchainSummaryLabel->setWordWrap(true);
    m_toolchainSummaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    toolchainLayout->addRow(tr("Resolved toolchain:"), m_toolchainSummaryLabel);
    tabs->addTab(toolchainPage, tr("Toolchain"));

    mainLayout->addWidget(tabs);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("projectPropertiesButtons");
    connect(buttons, &QDialogButtonBox::accepted, this, &ProjectPropertiesDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);

    connect(m_selectionModeCombo, &QComboBox::currentIndexChanged,
            this, &ProjectPropertiesDialog::updateUiState);
    connect(m_detectedKitCombo, &QComboBox::currentIndexChanged,
            this, &ProjectPropertiesDialog::updateResolvedSummary);
    connect(m_profileCombo, &QComboBox::currentIndexChanged,
            this, &ProjectPropertiesDialog::updateResolvedSummary);
    connect(m_cStandardCombo, &QComboBox::currentIndexChanged,
            this, &ProjectPropertiesDialog::updateResolvedSummary);
    connect(m_cxxStandardCombo, &QComboBox::currentIndexChanged,
            this, &ProjectPropertiesDialog::updateResolvedSummary);
    connect(m_compilerPreferenceCombo, &QComboBox::currentIndexChanged,
            this, &ProjectPropertiesDialog::updateResolvedSummary);
    connect(m_generatorPreferenceCombo, &QComboBox::currentIndexChanged,
            this, &ProjectPropertiesDialog::updateResolvedSummary);
    connect(m_manualGeneratorCombo, &QComboBox::currentIndexChanged,
            this, &ProjectPropertiesDialog::updateResolvedSummary);
    connect(m_manualCMakePathEdit, &QLineEdit::textChanged,
            this, &ProjectPropertiesDialog::updateResolvedSummary);
    connect(m_manualCCompilerPathEdit, &QLineEdit::textChanged,
            this, &ProjectPropertiesDialog::updateResolvedSummary);
    connect(m_manualCxxCompilerPathEdit, &QLineEdit::textChanged,
            this, &ProjectPropertiesDialog::updateResolvedSummary);
    connect(m_manualBuilderPathEdit, &QLineEdit::textChanged,
            this, &ProjectPropertiesDialog::updateResolvedSummary);
    connect(m_rescanButton, &QPushButton::clicked, this, &ProjectPropertiesDialog::rescanRequested);
}

void ProjectPropertiesDialog::populateFromModels()
{
    m_nameEdit->setText(m_project.name);
    m_versionEdit->setText(m_project.version);

    int typeIndex = m_typeCombo->findData(m_project.projectType);
    if (typeIndex >= 0)
        m_typeCombo->setCurrentIndex(typeIndex);

    int profileIndex = m_profileCombo->findData(m_project.build.buildProfile);
    if (profileIndex >= 0)
        m_profileCombo->setCurrentIndex(profileIndex);

    int cStandardIndex = m_cStandardCombo->findData(m_project.build.cStandard);
    if (cStandardIndex >= 0)
        m_cStandardCombo->setCurrentIndex(cStandardIndex);

    int cxxStandardIndex = m_cxxStandardCombo->findData(m_project.build.cxxStandard);
    if (cxxStandardIndex >= 0)
        m_cxxStandardCombo->setCurrentIndex(cxxStandardIndex);

    int compilerIndex = m_compilerPreferenceCombo->findData(m_project.build.compiler);
    if (compilerIndex >= 0)
        m_compilerPreferenceCombo->setCurrentIndex(compilerIndex);

    int generatorIndex = m_generatorPreferenceCombo->findData(m_project.build.generator);
    if (generatorIndex >= 0)
        m_generatorPreferenceCombo->setCurrentIndex(generatorIndex);

    m_extraCFlagsEdit->setText(m_project.build.extraCFlags.join(' '));
    m_extraCxxFlagsEdit->setText(m_project.build.extraCxxFlags.join(' '));

    int selectionModeIndex = m_selectionModeCombo->findData(m_localSettings.selectionMode);
    if (selectionModeIndex >= 0)
        m_selectionModeCombo->setCurrentIndex(selectionModeIndex);

    m_manualCMakePathEdit->setText(m_localSettings.manualOverride.cmakePath);
    m_manualCCompilerPathEdit->setText(m_localSettings.manualOverride.cCompilerPath);
    m_manualCxxCompilerPathEdit->setText(m_localSettings.manualOverride.cxxCompilerPath);
    m_manualBuilderPathEdit->setText(m_localSettings.manualOverride.builderPath);
    const int manualGeneratorIndex =
        m_manualGeneratorCombo->findData(m_localSettings.manualOverride.generator);
    if (manualGeneratorIndex >= 0)
        m_manualGeneratorCombo->setCurrentIndex(manualGeneratorIndex);
}

void ProjectPropertiesDialog::rebuildKitCombo()
{
    const QString selectedKitId = m_localSettings.selectedKitId;

    m_detectedKitCombo->blockSignals(true);
    m_detectedKitCombo->clear();
    if (m_detectedKits.isEmpty()) {
        m_detectedKitCombo->addItem(tr("No toolchain kits detected"), "");
    } else {
        for (const auto &kit : m_detectedKits) {
            const QString label = tr("%1 [%2]")
                .arg(kit.displayName, kitStatusLabel(kit.status));
            m_detectedKitCombo->addItem(label, kit.kitId);
        }
    }

    int selectedIndex = m_detectedKitCombo->findData(selectedKitId);
    if (selectedIndex < 0 && !selectedKitId.isEmpty()) {
        m_detectedKitCombo->addItem(tr("%1 [missing]").arg(selectedKitId), selectedKitId);
        selectedIndex = m_detectedKitCombo->count() - 1;
    }
    if (selectedIndex >= 0)
        m_detectedKitCombo->setCurrentIndex(selectedIndex);
    m_detectedKitCombo->blockSignals(false);
}

void ProjectPropertiesDialog::updateResolvedSummary()
{
    BuildConfig buildConfig = m_project.build;
    buildConfig.compiler = m_compilerPreferenceCombo->currentData().toString();
    buildConfig.generator = m_generatorPreferenceCombo->currentData().toString();
    buildConfig.buildProfile = m_profileCombo->currentData().toString();
    buildConfig.cStandard = m_cStandardCombo->currentData().toString();
    buildConfig.cxxStandard = m_cxxStandardCombo->currentData().toString();
    buildConfig.extraCFlags = parseFlags(m_extraCFlagsEdit->text());
    buildConfig.extraCxxFlags = parseFlags(m_extraCxxFlagsEdit->text());

    ProjectLocalSettings localSettings = m_localSettings;
    localSettings.selectionMode = m_selectionModeCombo->currentData().toString();
    localSettings.selectedKitId = (localSettings.selectionMode == "kit")
        ? m_detectedKitCombo->currentData().toString()
        : QString();
    localSettings.manualOverride.enabled = (localSettings.selectionMode == "manual");
    localSettings.manualOverride.cmakePath = m_manualCMakePathEdit->text().trimmed();
    localSettings.manualOverride.cCompilerPath = m_manualCCompilerPathEdit->text().trimmed();
    localSettings.manualOverride.cxxCompilerPath = m_manualCxxCompilerPathEdit->text().trimmed();
    localSettings.manualOverride.builderPath = m_manualBuilderPathEdit->text().trimmed();
    localSettings.manualOverride.generator = m_manualGeneratorCombo->currentData().toString();

    const ResolvedToolchainConfig resolved =
        ToolchainResolver::resolve(buildConfig, localSettings, m_inventory);

    QString text = tr("Mode: %1").arg(localSettings.selectionMode.isEmpty()
                                          ? tr("auto")
                                          : localSettings.selectionMode);
    if (!resolved.resolvedKitId.isEmpty())
        text += tr("\nKit: %1").arg(resolved.resolvedKitId);
    if (!resolved.generatorDisplayName.isEmpty())
        text += tr("\nGenerator: %1").arg(resolved.generatorDisplayName);
    if (!resolved.buildProfile.isEmpty())
        text += tr("\nProfile: %1").arg(resolved.buildProfile);
    if (!resolved.cmakePath.isEmpty())
        text += tr("\nCMake: %1").arg(resolved.cmakePath);
    if (!resolved.cCompilerPath.isEmpty())
        text += tr("\nC compiler: %1").arg(resolved.cCompilerPath);
    if (!resolved.cxxCompilerPath.isEmpty())
        text += tr("\nC++ compiler: %1").arg(resolved.cxxCompilerPath);

    if (resolved.valid) {
        text += tr("\nStatus: ready");
    } else if (!resolved.issues.isEmpty()) {
        text += tr("\nStatus: missing %1").arg(resolved.issues.join(", "));
    }

    m_toolchainSummaryLabel->setText(text);
}

QString ProjectPropertiesDialog::kitStatusLabel(const QString &status) const
{
    if (status == "ready")
        return tr("ready");
    if (status == "partial")
        return tr("partial");
    return tr("missing");
}

QStringList ProjectPropertiesDialog::parseFlags(const QString &text) const
{
    return QProcess::splitCommand(text.trimmed());
}

} // namespace DeltaQ
