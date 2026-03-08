// Тесты StandardLibrary — обязательные категории и аудит checked-in core библиотеки.
#include <QtTest>

#include "../../src/core/ModuleRegistry.h"
#include "../../src/core/StandardLibrary.h"

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>

using namespace DeltaQ;

class TestStandardLibrary : public QObject {
    Q_OBJECT

private:
    static Module makeCoreModule(const QString &id, const QString &name, const QString &category)
    {
        Module mod = Module::create(name, "c");
        mod.id = id;
        mod.origin = "core";
        mod.category = category;
        mod.sourceCode = "int dq_stub(void) {\n    return 0;\n}\n";
        return mod;
    }

    static QString modulesRootDir()
    {
        const QString packPath = QFINDTESTDATA("../../modules/core/pack.json");
        if (packPath.isEmpty())
            return {};

        QDir coreDir = QFileInfo(packPath).dir();
        coreDir.cdUp();
        return coreDir.absolutePath();
    }

private slots:
    void requiredV1CategoriesAreFixed()
    {
        QCOMPARE(StandardLibrary::requiredV1Categories(), QStringList({
            "control",
            "io",
            "string",
            "conversion",
            "logic",
            "math",
            "desktop"
        }));
    }

    void orderingHelpersFollowV1Baseline()
    {
        QCOMPARE(StandardLibrary::orderCategoriesForDisplay({"math", "custom", "io", "control"}),
                 QStringList({"control", "io", "math", "custom"}));

        Module sequence = makeCoreModule("core.control.sequence", "sequence", "control");
        Module ifBranch = makeCoreModule("core.control.if_branch", "if_branch", "control");
        Module println = makeCoreModule("core.io.println", "println", "io");
        Module print = makeCoreModule("core.io.print", "print", "io");

        QVector<const Module *> modules = {&sequence, &println, &print, &ifBranch};
        const QVector<const Module *> ordered = StandardLibrary::orderModulesForDisplay(modules);

        QCOMPARE(ordered.size(), 4);
        QCOMPARE(ordered.at(0)->id, QString("core.control.if_branch"));
        QCOMPARE(ordered.at(1)->id, QString("core.control.sequence"));
        QCOMPARE(ordered.at(2)->id, QString("core.io.print"));
        QCOMPARE(ordered.at(3)->id, QString("core.io.println"));
    }

    void curationMarksCoreRoles()
    {
        Module essential = makeCoreModule("core.io.print", "print", "io");
        Module convenience = makeCoreModule("core.io.quick_text", "quick_text", "io");
        Module legacy = makeCoreModule("core.io.print_int", "print_int", "io");
        Module specialized = makeCoreModule("core.string.str_compare", "str_compare", "string");
        Module foreign = Module::create("local_mod", "c");

        const StandardLibraryCurationInfo essentialInfo = StandardLibrary::curationForModule(essential);
        const StandardLibraryCurationInfo convenienceInfo = StandardLibrary::curationForModule(convenience);
        const StandardLibraryCurationInfo legacyInfo = StandardLibrary::curationForModule(legacy);
        const StandardLibraryCurationInfo specializedInfo = StandardLibrary::curationForModule(specialized);
        const StandardLibraryCurationInfo foreignInfo = StandardLibrary::curationForModule(foreign);

        QCOMPARE(essentialInfo.tier, QString("essential"));
        QVERIFY(essentialInfo.title.contains(QString::fromUtf8("Опорный")));

        QCOMPARE(convenienceInfo.tier, QString("convenience"));
        QVERIFY(convenienceInfo.guidance.contains(QString::fromUtf8("ускорять")));

        QCOMPARE(legacyInfo.tier, QString("legacy"));
        QVERIFY(legacyInfo.title.contains(QString("Legacy")));
        QVERIFY(legacyInfo.replacementHint.contains(QString("int_to_string")));

        QCOMPARE(specializedInfo.tier, QString("specialized"));
        QVERIFY(specializedInfo.guidance.contains(QString::fromUtf8("базового набора")));

        QVERIFY(!foreignInfo.isKnown());
    }

    void installedCoreLibraryPassesAudit()
    {
        const QString modulesDir = modulesRootDir();
        QVERIFY2(!modulesDir.isEmpty(), "modules/core not found via QFINDTESTDATA");

        ModuleRegistry registry;
        registry.loadGlobalModules(modulesDir);

        QVector<Module> modules;
        for (const auto *module : registry.allModules())
            modules.append(*module);

        const StandardLibraryAuditReport report = StandardLibrary::audit(modules);
        QVERIFY2(report.missingCategories.isEmpty(),
                 qPrintable(QString("Missing categories: %1").arg(report.missingCategories.join(", "))));
        QVERIFY2(report.missingModuleIds.isEmpty(),
                 qPrintable(QString("Missing module ids: %1").arg(report.missingModuleIds.join(", "))));
        QVERIFY2(report.issues.isEmpty(),
                 qPrintable(QString("Audit issues: %1").arg(report.issues.join(" | "))));
        QVERIFY(report.isHealthy());
    }

    void createAllReadsSourceOfTruthCorePack()
    {
        const QString modulesDir = modulesRootDir();
        QVERIFY2(!modulesDir.isEmpty(), "modules/core not found via QFINDTESTDATA");

        ModuleRegistry registry;
        registry.loadGlobalModules(modulesDir);

        QStringList expectedIds;
        for (const auto *module : registry.allModules()) {
            if (!module || module->origin != "core")
                continue;
            expectedIds.append(module->id);
        }
        expectedIds.sort();

        const QVector<Module> modules = StandardLibrary::createAll();
        QVERIFY2(!modules.isEmpty(), "StandardLibrary::createAll() returned no core modules");

        QStringList actualIds;
        for (const auto &module : modules)
            actualIds.append(module.id);
        actualIds.sort();

        QCOMPARE(actualIds, expectedIds);

        bool sawDocumentedModule = false;
        for (const auto &module : modules) {
            if (module.id != "core.io.println")
                continue;
            QVERIFY(!module.documentationWhenToUse().isEmpty());
            QVERIFY(!module.documentationLimitations().isEmpty());
            sawDocumentedModule = true;
            break;
        }
        QVERIFY(sawDocumentedModule);
    }

    void installCopiesSourceOfTruthCorePack()
    {
        const QString modulesDir = modulesRootDir();
        QVERIFY2(!modulesDir.isEmpty(), "modules/core not found via QFINDTESTDATA");

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString installedModulesRoot = tempDir.path() + "/modules";
        const QString installedCoreDir = installedModulesRoot + "/core";
        StandardLibrary::install(installedCoreDir);

        QVERIFY(StandardLibrary::isUpToDate(installedCoreDir));
        QVERIFY(QFileInfo::exists(installedCoreDir + "/pack.json"));

        ModuleRegistry installedRegistry;
        installedRegistry.loadGlobalModules(installedModulesRoot);

        ModuleRegistry sourceRegistry;
        sourceRegistry.loadGlobalModules(modulesDir);

        QStringList installedIds;
        for (const auto *module : installedRegistry.allModules()) {
            if (!module || module->origin != "core")
                continue;
            installedIds.append(module->id);
        }
        installedIds.sort();

        QStringList sourceIds;
        for (const auto *module : sourceRegistry.allModules()) {
            if (!module || module->origin != "core")
                continue;
            sourceIds.append(module->id);
        }
        sourceIds.sort();

        QCOMPARE(installedIds, sourceIds);
    }

    void installedCoreModulesExposeDocumentation()
    {
        const QString modulesDir = modulesRootDir();
        QVERIFY2(!modulesDir.isEmpty(), "modules/core not found via QFINDTESTDATA");

        ModuleRegistry registry;
        registry.loadGlobalModules(modulesDir);

        QStringList missingSummary;
        QStringList missingWhenToUse;
        QStringList missingLimitations;

        // Core pack должен давать не только контракт, но и user-visible объяснение,
        // иначе палитра и менеджер модулей снова превращаются в список имён без смысла.
        for (const auto *module : registry.allModules()) {
            if (!module || module->origin != "core")
                continue;

            if (module->documentationSummary().isEmpty())
                missingSummary.append(module->id);
            if (module->documentationWhenToUse().isEmpty())
                missingWhenToUse.append(module->id);
            if (module->documentationLimitations().isEmpty())
                missingLimitations.append(module->id);
        }

        QVERIFY2(missingSummary.isEmpty(),
                 qPrintable(QString("Missing core summaries: %1").arg(missingSummary.join(", "))));
        QVERIFY2(missingWhenToUse.isEmpty(),
                 qPrintable(QString("Missing core when_to_use docs: %1").arg(missingWhenToUse.join(", "))));
        QVERIFY2(missingLimitations.isEmpty(),
                 qPrintable(QString("Missing core limitation docs: %1").arg(missingLimitations.join(", "))));
    }

    void installedCoreLibraryUsesReviewedCurationMatrix()
    {
        const QString modulesDir = modulesRootDir();
        QVERIFY2(!modulesDir.isEmpty(), "modules/core not found via QFINDTESTDATA");

        ModuleRegistry registry;
        registry.loadGlobalModules(modulesDir);

        QHash<QString, int> counts;
        QStringList unknown;

        // Checked-in core pack теперь должен проходить через explicit curation review,
        // поэтому каждая роль обязана быть известна и стабильна по текущей матрице.
        for (const auto *module : registry.allModules()) {
            if (!module || module->origin != "core")
                continue;

            const StandardLibraryCurationInfo info = StandardLibrary::curationForModule(*module);
            if (!info.isKnown()) {
                unknown.append(module->id);
                continue;
            }
            counts[info.tier] += 1;
        }

        QVERIFY2(unknown.isEmpty(),
                 qPrintable(QString("Unknown curation for core modules: %1").arg(unknown.join(", "))));
        QCOMPARE(counts.value("essential"), 28);
        QCOMPARE(counts.value("specialized"), 3);
        QCOMPARE(counts.value("legacy"), 12);
        QCOMPARE(counts.value("convenience"), 0);
    }
};

QTEST_MAIN(TestStandardLibrary)
#include "test_StandardLibrary.moc"
