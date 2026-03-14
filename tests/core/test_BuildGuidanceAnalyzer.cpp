#include "../../src/core/BuildGuidanceAnalyzer.h"

#include <QTest>

using namespace DeltaQ;

class TestBuildGuidanceAnalyzer : public QObject {
    Q_OBJECT

private slots:
    void toolchainIssuesProduceDistroHints()
    {
        const BuildGuidance guidance = BuildGuidanceAnalyzer::forToolchainIssues(
            {"cmake", "C compiler", "builder"},
            "console",
            "ubuntu");

        QVERIFY(guidance.hasGuidance);
        QCOMPARE(guidance.title, QString("Toolchain setup required"));
        QVERIFY(guidance.summary.contains("cmake"));
        QVERIFY(guidance.summary.contains("C compiler"));
        QVERIFY(guidance.nextSteps.join('\n').contains("apt install cmake build-essential ninja-build pkg-config"));
        QVERIFY(guidance.nextSteps.join('\n').contains("Rescan Toolchains"));
    }

    void desktopConfigureFailureProducesSdlHints()
    {
        ProjectBuildRequest request;
        request.projectType = "desktop";

        const QString buildOutput =
            "=== Configuring with CMake ===\n"
            "Desktop template requires SDL2 and SDL2_ttf.\n";

        const BuildGuidance guidance = BuildGuidanceAnalyzer::fromBuildOutput(
            request,
            buildOutput,
            "fedora");

        QVERIFY(guidance.hasGuidance);
        QCOMPARE(guidance.title, QString("Desktop dependencies required"));
        QVERIFY(guidance.summary.contains("SDL2"));
        QVERIFY(guidance.nextSteps.join('\n').contains("dnf install SDL2-devel SDL2_ttf-devel pkgconf-pkg-config"));
    }

    void unrelatedBuildOutputDoesNotProduceGuidance()
    {
        ProjectBuildRequest request;
        request.projectType = "console";

        const BuildGuidance guidance = BuildGuidanceAnalyzer::fromBuildOutput(
            request,
            "src/main.c:10:5: error: expected ';' before 'return'",
            "ubuntu");

        QVERIFY(!guidance.hasGuidance);
        QVERIFY(guidance.title.isEmpty());
    }
};

QTEST_APPLESS_MAIN(TestBuildGuidanceAnalyzer)
#include "test_BuildGuidanceAnalyzer.moc"
