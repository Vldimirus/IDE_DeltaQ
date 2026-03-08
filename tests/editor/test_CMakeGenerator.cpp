// Тесты генератора CMakeLists.txt
#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QDir>

#include "../../src/editor/CMakeGenerator.h"

using namespace DeltaQ;

class TestCMakeGenerator : public QObject {
    Q_OBJECT

private slots:
    void testGenerateBasic()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Создаём исходный файл
        QFile mainFile(tmpDir.path() + "/main.c");
        QVERIFY(mainFile.open(QIODevice::WriteOnly));
        mainFile.write("int main() { return 0; }\n");
        mainFile.close();

        CMakeGenerator gen;
        QString cmakePath = gen.generate(tmpDir.path(), "TestProject", "17", "20");

        QVERIFY(QFile::exists(cmakePath));

        QFile cmake(cmakePath);
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        QString content = cmake.readAll();
        cmake.close();

        QVERIFY(content.contains("cmake_minimum_required"));
        QVERIFY(content.contains("project(TestProject"));
        QVERIFY(content.contains("CMAKE_C_STANDARD 17"));
        QVERIFY(content.contains("CMAKE_CXX_STANDARD 20"));
        QVERIFY(content.contains("main.c"));
        QVERIFY(content.contains("add_executable(TestProject"));
    }

    void testGenerateWithMultipleFiles()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Создаём несколько файлов
        QDir(tmpDir.path()).mkpath("src");
        for (const auto &name : {"main.cpp", "src/utils.cpp", "src/helper.c"}) {
            QFile f(tmpDir.path() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("// source\n");
            f.close();
        }

        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "MyApp");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        QString content = cmake.readAll();

        QVERIFY(content.contains("main.cpp"));
        QVERIFY(content.contains("src/utils.cpp"));
        QVERIFY(content.contains("src/helper.c"));
    }

    void testSkipBuildDir()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Создаём файлы в build/ — они должны быть пропущены
        QDir(tmpDir.path()).mkpath("build");
        QFile buildFile(tmpDir.path() + "/build/generated.cpp");
        buildFile.open(QIODevice::WriteOnly);
        buildFile.write("// generated\n");
        buildFile.close();

        QFile mainFile(tmpDir.path() + "/main.cpp");
        mainFile.open(QIODevice::WriteOnly);
        mainFile.write("int main() {}\n");
        mainFile.close();

        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "TestProj");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        QString content = cmake.readAll();

        QVERIFY(content.contains("main.cpp"));
        QVERIFY(!content.contains("generated.cpp"));
    }

    void testExtraFlags()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QFile mainFile(tmpDir.path() + "/main.c");
        mainFile.open(QIODevice::WriteOnly);
        mainFile.write("int main() { return 0; }\n");
        mainFile.close();

        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "FlagTest", "17", "20", {"-O2", "-DNDEBUG"});

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        QString content = cmake.readAll();

        QVERIFY(content.contains("-O2"));
        QVERIFY(content.contains("-DNDEBUG"));
        QVERIFY(content.contains("-Wall -Wextra"));
    }

    void testDesktopSDL2Dependencies()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QFile mainFile(tmpDir.path() + "/main.c");
        mainFile.open(QIODevice::WriteOnly);
        mainFile.write("int main() { return 0; }\n");
        mainFile.close();

        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "DesktopApp", "17", "20", {}, "desktop");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        QString content = cmake.readAll();

        QVERIFY(content.contains("find_package(SDL2 QUIET)"));
        QVERIFY(content.contains("find_package(SDL2_ttf QUIET)"));
        QVERIFY(content.contains("pkg_check_modules(PKG_SDL2 QUIET sdl2)"));
        QVERIFY(content.contains("pkg_check_modules(PKG_SDL2_TTF QUIET SDL2_ttf)"));
        QVERIFY(content.contains("Desktop template requires SDL2 and SDL2_ttf"));
    }

    void testEmptyProject()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        // Нет исходных файлов
        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "Empty");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmake.open(QIODevice::ReadOnly));
        QString content = cmake.readAll();

        // CMakeLists.txt должен быть создан даже без исходников
        QVERIFY(content.contains("project(Empty"));
        QVERIFY(content.contains("set(SOURCES"));
    }

    void testAutoGeneratedComment()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QFile f(tmpDir.path() + "/main.c");
        f.open(QIODevice::WriteOnly);
        f.write("int main() {}\n");
        f.close();

        CMakeGenerator gen;
        gen.generate(tmpDir.path(), "Test");

        QFile cmake(tmpDir.path() + "/CMakeLists.txt");
        cmake.open(QIODevice::ReadOnly);
        QString content = cmake.readAll();

        QVERIFY(content.startsWith("# Автоматически сгенерировано DeltaQ IDE"));
    }
};

QTEST_MAIN(TestCMakeGenerator)
#include "test_CMakeGenerator.moc"
