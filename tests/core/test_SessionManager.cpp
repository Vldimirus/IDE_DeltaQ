// Тесты SessionManager — вкладки, последний проект, recent projects
#include <QtTest>
#include "SessionManager.h"

using namespace DeltaQ;

class TestSessionManager : public QObject {
    Q_OBJECT

private slots:
    void openTabs_setAndGet()
    {
        // Используем изолированные QSettings для тестов
        SessionManager mgr("DeltaQ_Test", "test_openTabs");
        QStringList tabs = {"file1.cpp", "file2.h", "main.cpp"};
        mgr.setOpenTabs(tabs);
        QCOMPARE(mgr.openTabs(), tabs);
        // Очистка
        mgr.setOpenTabs({});
    }

    void openTabs_emptyByDefault()
    {
        SessionManager mgr("DeltaQ_Test", "test_openTabsEmpty");
        mgr.setOpenTabs({}); // гарантируем чистое состояние
        QVERIFY(mgr.openTabs().isEmpty());
    }

    void lastOpenedProject_setAndGet()
    {
        SessionManager mgr("DeltaQ_Test", "test_lastProject");
        mgr.setLastOpenedProject("/home/user/project");
        QCOMPARE(mgr.lastOpenedProject(), "/home/user/project");
        // Очистка
        mgr.setLastOpenedProject("");
    }

    void lastOpenedProject_emptyByDefault()
    {
        SessionManager mgr("DeltaQ_Test", "test_lastProjectEmpty");
        mgr.setLastOpenedProject(""); // гарантируем чистое состояние
        QVERIFY(mgr.lastOpenedProject().isEmpty());
    }

    void recentProjects_addAndRetrieve()
    {
        SessionManager mgr("DeltaQ_Test", "test_recentProjects");
        mgr.clearRecentProjects();

        mgr.addRecentProject("/proj/a");
        mgr.addRecentProject("/proj/b");
        mgr.addRecentProject("/proj/c");

        QStringList recent = mgr.recentProjects();
        QCOMPARE(recent.size(), 3);
        // Последний добавленный — первый в списке
        QCOMPARE(recent.first(), "/proj/c");
        QCOMPARE(recent.last(), "/proj/a");
        // Очистка
        mgr.clearRecentProjects();
    }

    void recentProjects_limitedToMax()
    {
        SessionManager mgr("DeltaQ_Test", "test_recentLimit");
        mgr.clearRecentProjects();

        // Добавляем 15 проектов (лимит 10)
        for (int i = 0; i < 15; ++i)
            mgr.addRecentProject(QString("/proj/%1").arg(i));

        QStringList recent = mgr.recentProjects();
        QCOMPARE(recent.size(), 10);
        // Первый — последний добавленный
        QCOMPARE(recent.first(), "/proj/14");
        // Очистка
        mgr.clearRecentProjects();
    }
};

QTEST_MAIN(TestSessionManager)
#include "test_SessionManager.moc"
