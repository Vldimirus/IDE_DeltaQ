#include "../../src/codegen/ToolchainResolver.h"
#include "../../src/codegen/CompilerDetector.h"

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

using namespace DeltaQ;

class TestToolchainResolver : public QObject {
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

    static ToolchainInventory fullInventory(QTemporaryDir &dir)
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
    void detectKitsCreatesStableReadyIds()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const auto kits = ToolchainResolver::detectKits(fullInventory(dir));
        QStringList ids;
        for (const auto &kit : kits) {
            ids.append(kit.kitId);
            QVERIFY(kit.isReady());
        }

        QVERIFY(ids.contains("linux-gcc-ninja"));
        QVERIFY(ids.contains("linux-gcc-unix_makefiles"));
        QVERIFY(ids.contains("linux-clang-ninja"));
        QVERIFY(ids.contains("linux-clang-unix_makefiles"));
    }

    void resolveAutoPrefersReadyNinjaKit()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        BuildConfig buildConfig;
        ProjectLocalSettings localSettings;
        const auto resolved = ToolchainResolver::resolve(buildConfig, localSettings, fullInventory(dir));

        QVERIFY(resolved.valid);
        QCOMPARE(resolved.resolvedKitId, QString("linux-gcc-ninja"));
        QCOMPARE(resolved.generator, QString("ninja"));
        QCOMPARE(resolved.generatorDisplayName, QString("Ninja"));
        QVERIFY(resolved.fingerprint.startsWith("sha256:"));
    }

    void resolveSelectedKitWinsOverProjectGeneratorPreference()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        BuildConfig buildConfig;
        buildConfig.generator = "unix_makefiles";
        ProjectLocalSettings localSettings;
        localSettings.selectionMode = "kit";
        localSettings.selectedKitId = "linux-clang-ninja";

        const auto resolved = ToolchainResolver::resolve(buildConfig, localSettings, fullInventory(dir));
        QVERIFY(resolved.valid);
        QCOMPARE(resolved.resolvedKitId, QString("linux-clang-ninja"));
        QCOMPARE(resolved.generator, QString("ninja"));
        QVERIFY(resolved.cCompilerPath.endsWith("/clang"));
    }

    void resolveManualOverrideWinsOverSelectedKit()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString manualC = makeExecutable(dir, "manual-clang");
        const QString manualCxx = makeExecutable(dir, "manual-clangxx");

        BuildConfig buildConfig;
        ProjectLocalSettings localSettings;
        localSettings.selectionMode = "manual";
        localSettings.selectedKitId = "linux-gcc-ninja";
        localSettings.manualOverride.enabled = true;
        localSettings.manualOverride.cCompilerPath = manualC;
        localSettings.manualOverride.cxxCompilerPath = manualCxx;
        localSettings.manualOverride.generator = "unix_makefiles";

        const auto resolved = ToolchainResolver::resolve(buildConfig, localSettings, fullInventory(dir));
        QVERIFY(resolved.valid);
        QCOMPARE(resolved.selectionMode, QString("manual"));
        QCOMPARE(resolved.generator, QString("unix_makefiles"));
        QCOMPARE(resolved.cCompilerPath, QFileInfo(manualC).canonicalFilePath());
        QCOMPARE(resolved.cxxCompilerPath, QFileInfo(manualCxx).canonicalFilePath());
        QVERIFY(!resolved.builderPath.isEmpty());
    }
};

QTEST_APPLESS_MAIN(TestToolchainResolver)
#include "test_ToolchainResolver.moc"
