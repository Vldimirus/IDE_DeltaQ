// Тесты Project/BuildConfig — сериализация, operator==, валидация
#include <QtTest>
#include <deltaq/Project.h>

using namespace DeltaQ;

class TestProject : public QObject {
    Q_OBJECT

private slots:
    // --- BuildConfig ---

    void buildConfigRoundtrip()
    {
        BuildConfig bc;
        bc.system = "cmake";
        bc.compiler = "clang";
        bc.generator = "ninja";
        bc.buildProfile = "Release";
        bc.cStandard = "c23";
        bc.cxxStandard = "c++23";
        bc.outputDir = "build/";
        bc.extraCFlags = {"-Wall", "-Wextra", "-O2"};
        bc.extraCxxFlags = {"-stdlib=libc++"};
        bc.toolchainMode = "manual";

        auto json = bc.toJson();
        auto restored = BuildConfig::fromJson(json);
        QCOMPARE(restored, bc);
    }

    void buildConfigEquality()
    {
        BuildConfig a, b;
        a.compiler = "gcc";
        a.cStandard = "c17";
        b.compiler = "gcc";
        b.cStandard = "c17";
        QVERIFY(a == b);

        b.cStandard = "c23";
        QVERIFY(!(a == b));
    }

    void buildConfigDefaults()
    {
        BuildConfig bc;
        QCOMPARE(bc.system, "cmake");
        QCOMPARE(bc.compiler, "auto");
        QCOMPARE(bc.generator, "auto");
        QCOMPARE(bc.buildProfile, "Debug");
        QCOMPARE(bc.cStandard, "c17");
        QCOMPARE(bc.cxxStandard, "c++20");
        QCOMPARE(bc.outputDir, "build/");
        QVERIFY(bc.extraCFlags.isEmpty());
        QVERIFY(bc.extraCxxFlags.isEmpty());
        QCOMPARE(bc.toolchainMode, "auto");

        // fromJson с пустым объектом → значения по умолчанию
        auto fromEmpty = BuildConfig::fromJson(QJsonObject{});
        QCOMPARE(fromEmpty.system, "cmake");
        QCOMPARE(fromEmpty.compiler, "auto");
        QCOMPARE(fromEmpty.generator, "auto");
        QCOMPARE(fromEmpty.buildProfile, "Debug");
        QCOMPARE(fromEmpty.cStandard, "c17");
        QCOMPARE(fromEmpty.cxxStandard, "c++20");
        QCOMPARE(fromEmpty.outputDir, "build/");
    }

    void buildConfigFromLegacyJson()
    {
        QJsonObject json;
        json["compiler"] = "clang";
        json["standard"] = "c23";
        json["output"] = "build/";
        json["flags"] = QJsonArray{"-Wall", "-O2"};
        json["build_tool"] = "make";

        const BuildConfig restored = BuildConfig::fromJson(json);
        QCOMPARE(restored.compiler, QString("clang"));
        QCOMPARE(restored.cStandard, QString("c23"));
        QCOMPARE(restored.cxxStandard, QString("c++20"));
        QCOMPARE(restored.generator, QString("unix_makefiles"));
        QCOMPARE(restored.extraCFlags, QStringList({"-Wall", "-O2"}));
    }

    void localSettingsRoundtrip()
    {
        ProjectLocalSettings settings;
        settings.selectionMode = "kit";
        settings.selectedKitId = "linux-gcc-ninja";
        settings.manualOverride.enabled = false;
        settings.lastResolvedFingerprint = "sha256:test";

        const ProjectLocalSettings restored = ProjectLocalSettings::fromJson(settings.toJson());
        QCOMPARE(restored, settings);
    }

    // --- Project ---

    void projectCreateNew()
    {
        auto p = Project::createNew("MyProject");
        QCOMPARE(p.name, "MyProject");
        QCOMPARE(p.version, "1.0.0");
        QVERIFY(!p.moduleGlobs.isEmpty());
        QVERIFY(!p.graphGlobs.isEmpty());
        QVERIFY(!p.uiLayoutGlobs.isEmpty());
    }

    void projectRoundtrip()
    {
        auto p = Project::createNew("TestProject");
        p.build.compiler = "clang";
        p.build.generator = "ninja";
        p.build.extraCFlags = {"-O3"};

        auto json = p.toJson();
        auto restored = Project::fromJson(json);
        QCOMPARE(restored, p);
        QCOMPARE(restored.moduleGlobs, p.moduleGlobs);
        QCOMPARE(restored.graphGlobs, p.graphGlobs);
        QCOMPARE(restored.uiLayoutGlobs, p.uiLayoutGlobs);
        QCOMPARE(restored.build, p.build);
    }

    void projectEquality()
    {
        auto a = Project::createNew("Alpha");
        auto b = Project::createNew("Alpha");
        QVERIFY(a == b); // одинаковые name + version

        b.name = "Beta";
        QVERIFY(!(a == b));
    }

    void projectIsValid()
    {
        Project p;
        QVERIFY(!p.isValid()); // name пуст

        p.name = "OK";
        QVERIFY(p.isValid());
    }
};

QTEST_MAIN(TestProject)
#include "test_Project.moc"
