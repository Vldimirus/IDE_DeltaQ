// Тесты LibraryPackager — сохранение imported C library как extension pack
#include <QTest>

#include "../../src/libProcessor/LibraryPackager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>

using namespace DeltaQ;

class TestLibraryPackager : public QObject {
    Q_OBJECT

private slots:
    void writesImportedPackWithStableIds()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        Module module = Module::create("sensor_read", "c");
        module.origin = "library";
        module.category = "sensors";
        module.inputs = {{"address", "int", "0"}};
        module.outputs = {{"result", "int", ""}};

        ImportedLibraryPackSpec spec;
        spec.packName = "Sensor SDK";
        spec.displayName = "Sensor SDK";
        spec.author = "Tester";
        spec.description = "Imported sensor functions";
        spec.category = "sensors";
        spec.language = "c";
        spec.standard = "c17";
        spec.headerPaths = {"/opt/vendor/sensor.h"};
        spec.linkLibraries = {"sensor_sdk"};

        const ImportedLibraryPackResult result =
            LibraryPackager::writeImportedPack(tempDir.path(), spec, {module});

        QVERIFY2(result.success(), qPrintable(result.errors.join(" | ")));
        QCOMPARE(result.writtenModuleFiles.size(), 1);
        QVERIFY(QFile::exists(tempDir.path() + "/sensor_sdk/pack.json"));
        QVERIFY(QFile::exists(result.writtenModuleFiles.first()));

        QFile moduleFile(result.writtenModuleFiles.first());
        QVERIFY(moduleFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QJsonObject obj = QJsonDocument::fromJson(moduleFile.readAll()).object();

        QCOMPARE(obj["id"].toString(), QString("ext.sensor_sdk.sensor_read"));
        QCOMPARE(obj["origin"].toString(), QString("extension"));
        QCOMPARE(obj["category"].toString(), QString("sensors"));
        QVERIFY(obj["source_code"].toString().contains(QString("return sensor_read(address);")));

        const QJsonArray includes = obj["includes"].toArray();
        QCOMPARE(includes.size(), 1);
        QCOMPARE(includes.first().toString(), QString("\"/opt/vendor/sensor.h\""));

        const QJsonObject metadata = obj["metadata"].toObject();
        QCOMPARE(metadata["deltaq.import.kind"].toString(), QString("library_pack_module"));
        QCOMPARE(metadata["deltaq.import.pack_name"].toString(), QString("sensor_sdk"));
        QCOMPARE(metadata["deltaq.import.original_symbol"].toString(), QString("sensor_read"));
        QCOMPARE(metadata["deltaq.import.display_name"].toString(), QString("sensor_read"));
        QCOMPARE(metadata["deltaq.import.curation_role"].toString(), QString("raw_wrapper"));
        QCOMPARE(metadata["deltaq.doc.when_to_use"].toString(),
                 QString::fromUtf8("Использовать как raw wrapper при curation imported pack-а "
                                   "или для точечного low-level доступа к символу библиотеки."));
        QCOMPARE(metadata["deltaq.doc.limitations"].toString(),
                 QString::fromUtf8("Низкоуровневый imported wrapper: может напрямую отражать "
                                   "внешний API без дополнительной адаптации под граф."));
        QCOMPARE(obj["description"].toString(), QString("Imported wrapper for 'sensor_read'"));
    }

    void writesWrapperFilesWhenProvided()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        ImportedLibraryPackSpec spec;
        spec.packName = "Cpp SDK";
        spec.displayName = "Cpp SDK";

        WrapperCode wrapper;
        wrapper.className = "Camera";
        wrapper.header = "// header";
        wrapper.source = "// source";

        const ImportedLibraryPackResult result =
            LibraryPackager::writeImportedPack(tempDir.path(), spec, {}, {wrapper});

        QVERIFY2(result.success(), qPrintable(result.errors.join(" | ")));
        QCOMPARE(result.writtenWrapperFiles.size(), 2);
        QVERIFY(QFile::exists(tempDir.path() + "/cpp_sdk/wrappers/camera.h"));
        QVERIFY(QFile::exists(tempDir.path() + "/cpp_sdk/wrappers/camera.cpp"));
    }
};

QTEST_APPLESS_MAIN(TestLibraryPackager)
#include "test_LibraryPackager.moc"
