#include "../../src/core/ProjectPropertiesDialog.h"

#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace DeltaQ;

class TestProjectPropertiesDialog : public QObject {
    Q_OBJECT

private:
    static QString makeExecutable(QTemporaryDir &dir, const QString &name)
    {
        const QString path = dir.path() + "/" + name;
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return {};
        file.write("#!/bin/sh\nexit 0\n");
        file.close();
        QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                        | QFileDevice::ExeOwner | QFileDevice::ReadGroup
                                        | QFileDevice::ExeGroup | QFileDevice::ReadOther
                                        | QFileDevice::ExeOther);
        return path;
    }

    static CompilerInfo compiler(const QString &name, const QString &path)
    {
        CompilerInfo info;
        info.name = name;
        info.path = path;
        info.version = "test";
        info.available = true;
        return info;
    }

    static BuildToolInfo buildTool(const QString &name, const QString &path)
    {
        BuildToolInfo info;
        info.name = name;
        info.path = path;
        info.version = "test";
        info.available = true;
        return info;
    }

    static ToolchainInventory sampleInventory(QTemporaryDir &dir)
    {
        ToolchainInventory inventory;
        inventory.compilers = {
            compiler("gcc", makeExecutable(dir, "gcc")),
            compiler("g++", makeExecutable(dir, "g++")),
            compiler("clang", makeExecutable(dir, "clang")),
            compiler("clang++", makeExecutable(dir, "clang++"))
        };
        inventory.buildTools = {
            buildTool("cmake", makeExecutable(dir, "cmake")),
            buildTool("ninja", makeExecutable(dir, "ninja")),
            buildTool("make", makeExecutable(dir, "make"))
        };
        return inventory;
    }

private slots:
    void roundtripAppliesProjectAndLocalSettings()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Project project = Project::createNew("DialogDemo");
        project.version = "1.2.3";
        project.projectType = "desktop";
        project.build.compiler = "gcc";
        project.build.generator = "ninja";
        project.build.buildProfile = "Debug";
        project.build.cStandard = "c17";
        project.build.cxxStandard = "c++20";

        ProjectLocalSettings localSettings;
        localSettings.selectionMode = "kit";
        localSettings.selectedKitId = "linux-gcc-ninja";

        ProjectPropertiesDialog dialog(project, localSettings);
        dialog.setToolchainInventory(sampleInventory(dir));

        auto *versionEdit = dialog.findChild<QLineEdit *>("projectVersionEdit");
        auto *profileCombo = dialog.findChild<QComboBox *>("buildProfileCombo");
        auto *compilerCombo = dialog.findChild<QComboBox *>("compilerPreferenceCombo");
        auto *generatorCombo = dialog.findChild<QComboBox *>("generatorPreferenceCombo");
        auto *selectionModeCombo = dialog.findChild<QComboBox *>("toolchainSelectionModeCombo");
        auto *detectedKitCombo = dialog.findChild<QComboBox *>("detectedKitCombo");
        auto *manualCCompilerEdit = dialog.findChild<QLineEdit *>("manualCCompilerPathEdit");
        auto *manualGeneratorCombo = dialog.findChild<QComboBox *>("manualGeneratorCombo");
        auto *extraCFlagsEdit = dialog.findChild<QLineEdit *>("extraCFlagsEdit");
        auto *extraCxxFlagsEdit = dialog.findChild<QLineEdit *>("extraCxxFlagsEdit");
        auto *summaryLabel = dialog.findChild<QLabel *>("toolchainSummaryLabel");

        QVERIFY(versionEdit != nullptr);
        QVERIFY(profileCombo != nullptr);
        QVERIFY(compilerCombo != nullptr);
        QVERIFY(generatorCombo != nullptr);
        QVERIFY(selectionModeCombo != nullptr);
        QVERIFY(detectedKitCombo != nullptr);
        QVERIFY(manualCCompilerEdit != nullptr);
        QVERIFY(manualGeneratorCombo != nullptr);
        QVERIFY(extraCFlagsEdit != nullptr);
        QVERIFY(extraCxxFlagsEdit != nullptr);
        QVERIFY(summaryLabel != nullptr);

        versionEdit->setText("2.0.0");
        profileCombo->setCurrentIndex(profileCombo->findData("Release"));
        compilerCombo->setCurrentIndex(compilerCombo->findData("clang"));
        generatorCombo->setCurrentIndex(generatorCombo->findData("unix_makefiles"));
        extraCFlagsEdit->setText("-O3 -Wall");
        extraCxxFlagsEdit->setText("-O2 -stdlib=libc++");
        selectionModeCombo->setCurrentIndex(selectionModeCombo->findData("manual"));
        manualCCompilerEdit->setText(makeExecutable(dir, "manual-clang"));
        manualGeneratorCombo->setCurrentIndex(manualGeneratorCombo->findData("ninja"));

        QVERIFY(!summaryLabel->text().isEmpty());
        QCOMPARE(detectedKitCombo->currentData().toString(), QString("linux-gcc-ninja"));

        dialog.accept();

        QCOMPARE(dialog.project().version, QString("2.0.0"));
        QCOMPARE(dialog.project().build.buildProfile, QString("Release"));
        QCOMPARE(dialog.project().build.compiler, QString("clang"));
        QCOMPARE(dialog.project().build.generator, QString("unix_makefiles"));
        QCOMPARE(dialog.project().build.extraCFlags, QStringList({"-O3", "-Wall"}));
        QCOMPARE(dialog.project().build.extraCxxFlags, QStringList({"-O2", "-stdlib=libc++"}));

        QCOMPARE(dialog.localSettings().selectionMode, QString("manual"));
        QVERIFY(dialog.localSettings().manualOverride.enabled);
        QCOMPARE(dialog.localSettings().manualOverride.generator, QString("ninja"));
        QVERIFY(!dialog.localSettings().manualOverride.cCompilerPath.isEmpty());
    }

    void rescanButtonEmitsSignal()
    {
        Project project = Project::createNew("DialogDemo");
        ProjectLocalSettings localSettings;
        ProjectPropertiesDialog dialog(project, localSettings);

        auto *rescanButton = dialog.findChild<QPushButton *>("rescanToolchainsButton");
        QVERIFY(rescanButton != nullptr);

        QSignalSpy spy(&dialog, &ProjectPropertiesDialog::rescanRequested);
        rescanButton->click();
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(TestProjectPropertiesDialog)
#include "test_ProjectPropertiesDialog.moc"
