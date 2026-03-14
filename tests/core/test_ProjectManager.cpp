// Тесты ProjectManager — создание/открытие/сохранение проекта, интеграция со stores
#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include "ProjectManager.h"
#include "ModuleRegistry.h"
#include "GraphStore.h"
#include "UILayoutStore.h"

using namespace DeltaQ;

class TestProjectManager : public QObject {
    Q_OBJECT

private slots:
    void createProject()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        ModuleRegistry registry;
        GraphStore graphStore;
        UILayoutStore uiLayoutStore;
        ProjectManager pm(&registry, &graphStore, &uiLayoutStore);

        QSignalSpy spy(&pm, &ProjectManager::projectOpened);
        QVERIFY(pm.createProject("TestProject", tmpDir.path() + "/proj"));
        QVERIFY(pm.isProjectOpen());
        QCOMPARE(pm.currentProject().name, "TestProject");
        QCOMPARE(spy.count(), 1);

        // Проверяем создание поддиректорий
        QDir projDir(tmpDir.path() + "/proj");
        QVERIFY(projDir.exists("graphs"));
        QVERIFY(projDir.exists("ui"));
        QVERIFY(projDir.exists("src"));
        QVERIFY(projDir.exists("dqmods"));

        // Проверяем создание .dqproj файла
        QVERIFY(QFile::exists(tmpDir.path() + "/proj/TestProject.dqproj"));
    }

    void openProject()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Создаём проект
        ModuleRegistry reg1;
        GraphStore gs1;
        UILayoutStore us1;
        ProjectManager pm1(&reg1, &gs1, &us1);
        pm1.createProject("OpenTest", tmpDir.path() + "/proj");

        // Добавляем граф и макет для проверки загрузки
        auto g = Graph::create("TestGraph");
        gs1.registerGraph(g);
        gs1.saveAll(tmpDir.path() + "/proj");

        auto l = UILayout::create("TestLayout");
        us1.registerLayout(l);
        us1.saveAll(tmpDir.path() + "/proj");

        pm1.closeProject();

        // Открываем в новом ProjectManager
        ModuleRegistry reg2;
        GraphStore gs2;
        UILayoutStore us2;
        ProjectManager pm2(&reg2, &gs2, &us2);

        QString dqprojPath = tmpDir.path() + "/proj/OpenTest.dqproj";
        QVERIFY(pm2.openProject(dqprojPath));
        QVERIFY(pm2.isProjectOpen());
        QCOMPARE(pm2.currentProject().name, "OpenTest");

        // Графы и макеты загружены
        QCOMPARE(gs2.count(), 1);
        QCOMPARE(us2.count(), 1);
    }

    void saveProject()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        ModuleRegistry registry;
        GraphStore graphStore;
        UILayoutStore uiLayoutStore;
        ProjectManager pm(&registry, &graphStore, &uiLayoutStore);
        pm.createProject("SaveTest", tmpDir.path() + "/proj");

        // Добавляем данные
        auto m = Module::create("Mod1");
        registry.registerModule(m);
        auto g = Graph::create("Graph1");
        graphStore.registerGraph(g);
        auto l = UILayout::create("Layout1");
        uiLayoutStore.registerLayout(l);

        QSignalSpy spy(&pm, &ProjectManager::projectSaved);
        QVERIFY(pm.saveProject());
        QCOMPARE(spy.count(), 1);

        // Проверяем, что файлы графов и макетов сохранены
        QDir graphsDir(tmpDir.path() + "/proj/graphs");
        QVERIFY(graphsDir.entryList({"*.dqgraph"}, QDir::Files).size() > 0);
        QDir uiDir(tmpDir.path() + "/proj/ui");
        QVERIFY(uiDir.entryList({"*.dqui"}, QDir::Files).size() > 0);
    }

    void saveProjectMetadataOnly_doesNotPersistStores()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        ModuleRegistry registry;
        GraphStore graphStore;
        UILayoutStore uiLayoutStore;
        ProjectManager pm(&registry, &graphStore, &uiLayoutStore);
        QVERIFY(pm.createProject("MetaOnly", tmpDir.path() + "/proj"));

        registry.registerModule(Module::create("Mod1"));
        graphStore.registerGraph(Graph::create("Graph1"));
        uiLayoutStore.registerLayout(UILayout::create("Layout1"));

        pm.currentProject().version = "2.1.0";
        pm.currentLocalSettings().selectionMode = "kit";
        pm.currentLocalSettings().selectedKitId = "linux-gcc-ninja";

        QVERIFY(pm.saveProjectMetadataOnly());

        QFile dqproj(tmpDir.path() + "/proj/MetaOnly.dqproj");
        QVERIFY(dqproj.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString projectJson = QString::fromUtf8(dqproj.readAll());
        QVERIFY(projectJson.contains("\"version\": \"2.1.0\""));

        QVERIFY(QFile::exists(tmpDir.path() + "/proj/MetaOnly.dqproj.user"));

        QDir graphsDir(tmpDir.path() + "/proj/graphs");
        QCOMPARE(graphsDir.entryList({"*.dqgraph"}, QDir::Files).size(), 0);
        QDir uiDir(tmpDir.path() + "/proj/ui");
        QCOMPARE(uiDir.entryList({"*.dqui"}, QDir::Files).size(), 0);
        QDir dqmodsDir(tmpDir.path() + "/proj/dqmods");
        QCOMPARE(dqmodsDir.entryList({"*.dqmod"}, QDir::Files).size(), 0);
    }

    void closeProject()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        ModuleRegistry registry;
        GraphStore graphStore;
        UILayoutStore uiLayoutStore;
        ProjectManager pm(&registry, &graphStore, &uiLayoutStore);
        pm.createProject("CloseTest", tmpDir.path() + "/proj");

        graphStore.registerGraph(Graph::create("G"));
        uiLayoutStore.registerLayout(UILayout::create("L"));

        QSignalSpy spy(&pm, &ProjectManager::projectClosed);
        QVERIFY(pm.closeProject());
        QCOMPARE(spy.count(), 1);
        QVERIFY(!pm.isProjectOpen());

        // Stores очищены
        QCOMPARE(registry.count(), 0);
        QCOMPARE(graphStore.count(), 0);
        QCOMPARE(uiLayoutStore.count(), 0);
    }

    void closeWhenNotOpen_fails()
    {
        ModuleRegistry registry;
        ProjectManager pm(&registry);
        QVERIFY(!pm.closeProject());
    }

    void openInvalidPath_fails()
    {
        ModuleRegistry registry;
        ProjectManager pm(&registry);
        QVERIFY(!pm.openProject("/nonexistent/path/file.dqproj"));
        QVERIFY(!pm.isProjectOpen());
    }

    void projectRoundtrip()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Создаём проект с данными
        ModuleRegistry reg1;
        GraphStore gs1;
        UILayoutStore us1;
        ProjectManager pm1(&reg1, &gs1, &us1);
        pm1.createProject("Roundtrip", tmpDir.path() + "/proj");
        pm1.currentProject().version = "2.0.0";
        pm1.saveProject();
        pm1.closeProject();

        // Открываем заново
        ModuleRegistry reg2;
        GraphStore gs2;
        UILayoutStore us2;
        ProjectManager pm2(&reg2, &gs2, &us2);
        pm2.openProject(tmpDir.path() + "/proj/Roundtrip.dqproj");

        QCOMPARE(pm2.currentProject().name, "Roundtrip");
        QCOMPARE(pm2.currentProject().version, "2.0.0");
    }

    void localSettingsRoundtrip()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        ModuleRegistry reg1;
        GraphStore gs1;
        UILayoutStore us1;
        ProjectManager pm1(&reg1, &gs1, &us1);
        QVERIFY(pm1.createProject("LocalRoundtrip", tmpDir.path() + "/proj"));

        pm1.currentLocalSettings().selectionMode = "manual";
        pm1.currentLocalSettings().manualOverride.enabled = true;
        pm1.currentLocalSettings().manualOverride.cmakePath = "/usr/bin/cmake";
        pm1.currentLocalSettings().manualOverride.cCompilerPath = "/usr/bin/gcc";
        pm1.currentLocalSettings().manualOverride.generator = "ninja";
        pm1.currentLocalSettings().lastResolvedFingerprint = "sha256:local";
        QVERIFY(pm1.saveProjectMetadataOnly());
        pm1.closeProject();

        ModuleRegistry reg2;
        GraphStore gs2;
        UILayoutStore us2;
        ProjectManager pm2(&reg2, &gs2, &us2);
        QVERIFY(pm2.openProject(tmpDir.path() + "/proj/LocalRoundtrip.dqproj"));

        QCOMPARE(pm2.currentLocalSettings().selectionMode, QString("manual"));
        QVERIFY(pm2.currentLocalSettings().manualOverride.enabled);
        QCOMPARE(pm2.currentLocalSettings().manualOverride.cmakePath, QString("/usr/bin/cmake"));
        QCOMPARE(pm2.currentLocalSettings().manualOverride.cCompilerPath, QString("/usr/bin/gcc"));
        QCOMPARE(pm2.currentLocalSettings().manualOverride.generator, QString("ninja"));
        QCOMPARE(pm2.currentLocalSettings().lastResolvedFingerprint, QString("sha256:local"));
    }

    void openProjectMigratesLegacyCompilerPathToLocalSettings()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString projectDir = tmpDir.path() + "/proj";
        QVERIFY(QDir().mkpath(projectDir + "/graphs"));
        QVERIFY(QDir().mkpath(projectDir + "/ui"));
        QVERIFY(QDir().mkpath(projectDir + "/src"));
        QVERIFY(QDir().mkpath(projectDir + "/src/ui"));
        QVERIFY(QDir().mkpath(projectDir + "/build"));
        QVERIFY(QDir().mkpath(projectDir + "/dqmods"));

        QFile file(projectDir + "/Legacy.dqproj");
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        const QByteArray legacyProjectJson =
            "{\n"
            "  \"name\": \"Legacy\",\n"
            "  \"version\": \"1.0.0\",\n"
            "  \"type\": \"console\",\n"
            "  \"modules\": [\"dqmods/*.dqmod\"],\n"
            "  \"graphs\": [\"graphs/*.dqgraph\"],\n"
            "  \"ui_layouts\": [\"ui/*.dqui\"],\n"
            "  \"build\": {\n"
            "    \"compiler\": \"auto\",\n"
            "    \"standard\": \"c17\",\n"
            "    \"output\": \"build/\",\n"
            "    \"flags\": [],\n"
            "    \"compiler_path\": \"/usr/bin/clang\",\n"
            "    \"build_tool\": \"make\"\n"
            "  }\n"
            "}\n";
        file.write(legacyProjectJson);
        file.close();

        ModuleRegistry registry;
        GraphStore graphStore;
        UILayoutStore uiLayoutStore;
        ProjectManager pm(&registry, &graphStore, &uiLayoutStore);
        QVERIFY(pm.openProject(projectDir + "/Legacy.dqproj"));

        QCOMPARE(pm.currentLocalSettings().selectionMode, QString("manual"));
        QVERIFY(pm.currentLocalSettings().manualOverride.enabled);
        QCOMPARE(pm.currentLocalSettings().manualOverride.cCompilerPath, QString("/usr/bin/clang"));
        QCOMPARE(pm.currentLocalSettings().manualOverride.generator, QString("unix_makefiles"));
    }
};

QTEST_MAIN(TestProjectManager)
#include "test_ProjectManager.moc"
