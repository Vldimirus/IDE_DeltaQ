// Тесты reproducible raw import baseline для mini_sensor_sdk
#include <QTest>

#include "../../src/libProcessor/LibraryDecomposer.h"
#include "../../src/libProcessor/LibraryPackager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QTemporaryDir>

using namespace DeltaQ;

class TestImportedPackRawBaseline : public QObject {
    Q_OBJECT

private:
    // Собирает фиксированный ParseResult для fixture SDK, чтобы raw import baseline
    // не зависел от наличия libclang в текущем окружении.
    static ParseResult makeMiniSensorParseResult()
    {
        ParseResult result;
        result.success = true;

        FunctionDecl init;
        init.name = "mini_sensor_init";
        init.returnType = "mini_sensor_context *";
        init.parameters.append({"device_name", "const char *", ""});
        result.functions.append(init);

        FunctionDecl read;
        read.name = "mini_sensor_read";
        read.returnType = "int";
        read.parameters.append({"ctx", "mini_sensor_context *", ""});
        result.functions.append(read);

        FunctionDecl scale;
        scale.name = "mini_sensor_scale";
        scale.returnType = "int";
        scale.parameters.append({"ctx", "mini_sensor_context *", ""});
        scale.parameters.append({"factor", "int", ""});
        result.functions.append(scale);

        FunctionDecl lastError;
        lastError.name = "mini_sensor_last_error";
        lastError.returnType = "const char *";
        lastError.parameters.append({"ctx", "mini_sensor_context *", ""});
        result.functions.append(lastError);

        FunctionDecl shutdown;
        shutdown.name = "mini_sensor_shutdown";
        shutdown.returnType = "void";
        shutdown.parameters.append({"ctx", "mini_sensor_context *", ""});
        result.functions.append(shutdown);

        return result;
    }

    // Находит модуль по имени, чтобы проверки оставались читаемыми.
    static const Module *findModuleByName(const QVector<Module> &modules, const QString &name)
    {
        for (const auto &module : modules) {
            if (module.name == name)
                return &module;
        }
        return nullptr;
    }

    // Читает все файлы пакета относительно его корня и возвращает детерминированный снимок.
    static QMap<QString, QByteArray> readPackSnapshot(const QString &packDir)
    {
        QMap<QString, QByteArray> snapshot;
        QDir root(packDir);
        const QStringList files = root.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto &fileName : files) {
            QFile file(root.filePath(fileName));
            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
                snapshot[fileName] = file.readAll();
        }

        const QDir categoryRoot(packDir + "/sensor_raw");
        const QStringList moduleFiles =
            categoryRoot.entryList({"*.dqmod"}, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto &fileName : moduleFiles) {
            QFile file(categoryRoot.filePath(fileName));
            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
                snapshot["sensor_raw/" + fileName] = file.readAll();
        }

        return snapshot;
    }

private slots:
    void miniSensorSdkRawImportBaselineIsStable()
    {
        const QString fixtureHeader =
            QFINDTESTDATA("../../resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk/include/mini_sensor_sdk.h");
        QVERIFY2(!fixtureHeader.isEmpty(), "Fixture header for mini_sensor_sdk was not found");

        const QString fixtureReadme =
            QFINDTESTDATA("../../resources/examples/imported_pack_sensor_sdk/README.md");
        QVERIFY2(!fixtureReadme.isEmpty(), "Imported pack fixture README was not found");

        const QString includeDir = QFileInfo(fixtureHeader).absolutePath();

        DecompositionOptions options;
        options.category = "sensor_raw";
        options.language = "c";

        const DecompositionResult decomposition =
            LibraryDecomposer::decompose(makeMiniSensorParseResult(), options);
        QCOMPARE(decomposition.modules.size(), 5);

        const Module *init = findModuleByName(decomposition.modules, "mini_sensor_init");
        const Module *read = findModuleByName(decomposition.modules, "mini_sensor_read");
        const Module *scale = findModuleByName(decomposition.modules, "mini_sensor_scale");
        const Module *lastError = findModuleByName(decomposition.modules, "mini_sensor_last_error");
        const Module *shutdown = findModuleByName(decomposition.modules, "mini_sensor_shutdown");

        QVERIFY(init != nullptr);
        QVERIFY(read != nullptr);
        QVERIFY(scale != nullptr);
        QVERIFY(lastError != nullptr);
        QVERIFY(shutdown != nullptr);

        QCOMPARE(init->inputs.size(), 1);
        QCOMPARE(init->inputs[0].name, QString("device_name"));
        QCOMPARE(init->inputs[0].type, QString("string"));
        QCOMPARE(init->outputs.size(), 1);
        QCOMPARE(init->outputs[0].type, QString("pointer"));

        QCOMPARE(read->inputs.size(), 1);
        QCOMPARE(read->inputs[0].type, QString("pointer"));
        QCOMPARE(read->outputs.size(), 1);
        QCOMPARE(read->outputs[0].type, QString("int"));

        QCOMPARE(scale->inputs.size(), 2);
        QCOMPARE(scale->inputs[0].type, QString("pointer"));
        QCOMPARE(scale->inputs[1].name, QString("factor"));
        QCOMPARE(scale->inputs[1].type, QString("int"));
        QCOMPARE(scale->outputs.size(), 1);
        QCOMPARE(scale->outputs[0].type, QString("int"));

        QCOMPARE(lastError->inputs.size(), 1);
        QCOMPARE(lastError->inputs[0].type, QString("pointer"));
        QCOMPARE(lastError->outputs.size(), 1);
        QCOMPARE(lastError->outputs[0].type, QString("string"));

        QCOMPARE(shutdown->inputs.size(), 1);
        QCOMPARE(shutdown->inputs[0].type, QString("pointer"));
        QCOMPARE(shutdown->outputs.size(), 0);

        ImportedLibraryPackSpec spec;
        spec.packName = "mini_sensor_sdk_raw";
        spec.displayName = "Mini Sensor SDK Raw";
        spec.author = "DeltaQ";
        spec.description = "Raw wrapper baseline for mini_sensor_sdk";
        spec.category = "sensor_raw";
        spec.language = "c";
        spec.standard = "c17";
        spec.headerPaths = {fixtureHeader};
        spec.includePaths = {includeDir};
        spec.linkLibraries = {"mini_sensor_sdk"};

        QTemporaryDir firstDir;
        QTemporaryDir secondDir;
        QVERIFY(firstDir.isValid());
        QVERIFY(secondDir.isValid());

        const ImportedLibraryPackResult first =
            LibraryPackager::writeImportedPack(firstDir.path(), spec, decomposition.modules);
        const ImportedLibraryPackResult second =
            LibraryPackager::writeImportedPack(secondDir.path(), spec, decomposition.modules);

        QVERIFY2(first.success(), qPrintable(first.errors.join(" | ")));
        QVERIFY2(second.success(), qPrintable(second.errors.join(" | ")));
        QCOMPARE(first.writtenModuleFiles.size(), 5);
        QCOMPARE(second.writtenModuleFiles.size(), 5);

        const QString firstPackDir = firstDir.path() + "/mini_sensor_sdk_raw";
        const QString secondPackDir = secondDir.path() + "/mini_sensor_sdk_raw";
        QVERIFY(QFile::exists(firstPackDir + "/pack.json"));
        QVERIFY(QFile::exists(secondPackDir + "/pack.json"));

        const QMap<QString, QByteArray> firstSnapshot = readPackSnapshot(firstPackDir);
        const QMap<QString, QByteArray> secondSnapshot = readPackSnapshot(secondPackDir);
        QCOMPARE(firstSnapshot, secondSnapshot);

        QFile packFile(firstPackDir + "/pack.json");
        QVERIFY(packFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QJsonObject packJson = QJsonDocument::fromJson(packFile.readAll()).object();
        QCOMPARE(packJson["kind"].toString(), QString("imported_library_pack"));
        QCOMPARE(packJson["language"].toString(), QString("c"));
        QCOMPARE(packJson["category"].toString(), QString("sensor_raw"));
        QCOMPARE(packJson["standard"].toString(), QString("c17"));
        QCOMPARE(packJson["link_libraries"].toArray().first().toString(), QString("mini_sensor_sdk"));
        QCOMPARE(packJson["include_paths"].toArray().first().toString(), includeDir);

        QFile initFile(firstPackDir + "/sensor_raw/mini_sensor_init.dqmod");
        QVERIFY(initFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QJsonObject initJson = QJsonDocument::fromJson(initFile.readAll()).object();
        QCOMPARE(initJson["id"].toString(), QString("ext.mini_sensor_sdk_raw.mini_sensor_init"));
        QCOMPARE(initJson["origin"].toString(), QString("extension"));
        QCOMPARE(initJson["category"].toString(), QString("sensor_raw"));
        QVERIFY(initJson["source_code"].toString().contains("return mini_sensor_init(device_name);"));
        QCOMPARE(initJson["includes"].toArray().first().toString(),
                 QString("\"%1\"").arg(QDir::fromNativeSeparators(fixtureHeader)));

        const QJsonObject initMetadata = initJson["metadata"].toObject();
        QCOMPARE(initMetadata["deltaq.import.pack_name"].toString(), QString("mini_sensor_sdk_raw"));
        QCOMPARE(initMetadata["deltaq.import.original_symbol"].toString(), QString("mini_sensor_init"));
        QCOMPARE(initMetadata["deltaq.import.link_libraries"].toArray().first().toString(),
                 QString("mini_sensor_sdk"));
    }
};

QTEST_APPLESS_MAIN(TestImportedPackRawBaseline)
#include "test_ImportedPackRawBaseline.moc"
