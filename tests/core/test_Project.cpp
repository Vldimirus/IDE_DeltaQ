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
        bc.compiler = "clang";
        bc.standard = "c23";
        bc.outputDir = "out/";
        bc.flags = {"-Wall", "-Wextra", "-O2"};

        auto json = bc.toJson();
        auto restored = BuildConfig::fromJson(json);
        QCOMPARE(restored, bc);
    }

    void buildConfigEquality()
    {
        BuildConfig a, b;
        a.compiler = "gcc";
        a.standard = "c17";
        b.compiler = "gcc";
        b.standard = "c17";
        QVERIFY(a == b);

        b.standard = "c23";
        QVERIFY(!(a == b));
    }

    void buildConfigDefaults()
    {
        BuildConfig bc;
        QCOMPARE(bc.compiler, "gcc");
        QCOMPARE(bc.standard, "c17");
        QCOMPARE(bc.outputDir, "build/");
        QVERIFY(bc.flags.isEmpty());

        // fromJson с пустым объектом → значения по умолчанию
        auto fromEmpty = BuildConfig::fromJson(QJsonObject{});
        QCOMPARE(fromEmpty.compiler, "gcc");
        QCOMPARE(fromEmpty.standard, "c17");
        QCOMPARE(fromEmpty.outputDir, "build/");
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
        p.build.flags = {"-O3"};

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
