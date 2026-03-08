// Стандартная библиотека модулей DeltaQ — реализация
#include "StandardLibrary.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>
#include <QHash>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <optional>

namespace DeltaQ {

struct ExpectedPortSpec {
    QString name;
    QString type;
    PortKind kind = PortKind::Data;
};

struct ExpectedModuleSignature {
    QString moduleId;
    QVector<ExpectedPortSpec> inputs;
    QVector<ExpectedPortSpec> outputs;
};

struct ReviewedCurationSpec {
    QString moduleId;
    QString tier;
    QString title;
    QString guidance;
    QString replacementHint;
};

// Сравнивает фактические порты с ожидаемым контрактом ключевого библиотечного модуля.
static bool portsMatchExpected(const QVector<Port> &actual, const QVector<ExpectedPortSpec> &expected)
{
    if (actual.size() != expected.size())
        return false;

    for (int i = 0; i < actual.size(); ++i) {
        if (actual[i].name != expected[i].name
            || actual[i].type != expected[i].type
            || actual[i].kind != expected[i].kind) {
            return false;
        }
    }

    return true;
}

// Возвращает эталонные сигнатуры модулей, которые образуют ядро v1 standard library.
static QVector<ExpectedModuleSignature> requiredModuleSignatures()
{
    return {
        {
            "core.io.print",
            {{"flow_in", "exec", PortKind::Execution}, {"text", "string", PortKind::Data}},
            {{"flow_out", "exec", PortKind::Execution}}
        },
        {
            "core.io.println",
            {{"flow_in", "exec", PortKind::Execution}, {"text", "string", PortKind::Data}},
            {{"flow_out", "exec", PortKind::Execution}}
        },
        {
            "core.io.read_line",
            {{"flow_in", "exec", PortKind::Execution}},
            {{"flow_out", "exec", PortKind::Execution}, {"text", "string", PortKind::Data}}
        },
        {
            "core.io.int_constant",
            {{"value", "int", PortKind::Data}},
            {{"out", "int", PortKind::Data}}
        },
        {
            "core.io.string_constant",
            {{"value", "string", PortKind::Data}},
            {{"out", "string", PortKind::Data}}
        },
        {
            "core.math.add",
            {{"a", "int", PortKind::Data}, {"b", "int", PortKind::Data}},
            {{"result", "int", PortKind::Data}}
        },
        {
            "core.math.subtract",
            {{"a", "int", PortKind::Data}, {"b", "int", PortKind::Data}},
            {{"result", "int", PortKind::Data}}
        },
        {
            "core.math.multiply",
            {{"a", "int", PortKind::Data}, {"b", "int", PortKind::Data}},
            {{"result", "int", PortKind::Data}}
        },
        {
            "core.math.divide",
            {{"a", "int", PortKind::Data}, {"b", "int", PortKind::Data}},
            {{"result", "int", PortKind::Data}}
        },
        {
            "core.logic.and",
            {{"a", "bool", PortKind::Data}, {"b", "bool", PortKind::Data}},
            {{"result", "bool", PortKind::Data}}
        },
        {
            "core.logic.or",
            {{"a", "bool", PortKind::Data}, {"b", "bool", PortKind::Data}},
            {{"result", "bool", PortKind::Data}}
        },
        {
            "core.logic.not",
            {{"value", "bool", PortKind::Data}},
            {{"result", "bool", PortKind::Data}}
        },
        {
            "core.conversion.int_to_string",
            {{"value", "int", PortKind::Data}},
            {{"result", "string", PortKind::Data}}
        },
        {
            "core.conversion.string_to_int",
            {{"text", "string", PortKind::Data}},
            {{"result", "int", PortKind::Data}}
        },
        {
            "core.control.if_branch",
            {{"flow_in", "exec", PortKind::Execution}, {"condition", "bool", PortKind::Data}},
            {{"flow_true", "exec", PortKind::Execution}, {"flow_false", "exec", PortKind::Execution}}
        },
        {
            "core.control.for_loop",
            {{"flow_in", "exec", PortKind::Execution}, {"count", "int", PortKind::Data}},
            {{"flow_body", "exec", PortKind::Execution}, {"flow_done", "exec", PortKind::Execution}, {"index", "int", PortKind::Data}}
        },
        {
            "core.control.sequence",
            {{"flow_in", "exec", PortKind::Execution}},
            {{"then_0", "exec", PortKind::Execution}, {"then_1", "exec", PortKind::Execution}, {"then_2", "exec", PortKind::Execution}}
        }
    };
}

// Базовая проверка snake_case для единообразия имён в standard library.
static bool isSnakeCaseName(const QString &name)
{
    static const QRegularExpression re("^[a-z][a-z0-9_]*$");
    return re.match(name).hasMatch();
}

// Приоритет ролей standard library для curated display-порядка.
static int curationTierRank(const QString &tier)
{
    if (tier == "essential")
        return 0;
    if (tier == "convenience")
        return 1;
    if (tier == "specialized")
        return 2;
    if (tier == "legacy")
        return 3;
    return 4;
}

// Явная reviewed-таблица для всех checked-in core-модулей вне essential baseline.
// Если модуль не попал ни в requiredV1, ни сюда, audit должен считать его нерассмотренным.
static QVector<ReviewedCurationSpec> reviewedNonEssentialCurations()
{
    return {
        {
            "core.io.print_int",
            "legacy",
            QObject::tr("Legacy shortcut"),
            QObject::tr("Сохранён для совместимости со старыми графами, но в новых схемах лучше "
                        "не дублировать typed-output shortcut отдельным модулем."),
            QObject::tr("Для более явного графа можно использовать int_to_string + print/println.")
        },
        {
            "core.io.print_float",
            "legacy",
            QObject::tr("Legacy float shortcut"),
            QObject::tr("Сохранён для совместимости, но как отдельный typed-output shortcut "
                        "раздувает core без формирования целостного float baseline."),
            QObject::tr("Для новых сценариев лучше выносить float-output во внешний numeric pack или собирать его явной цепочкой.")
        },
        {
            "core.control.if_then",
            "legacy",
            QObject::tr("Legacy typed ternary helper"),
            QObject::tr("Сохранён для совместимости, но узкий int-only выбор значения "
                        "не усиливает базовую exec/data-flow модель core и лучше "
                        "оформляется как локальный helper или внешний pack."),
            QObject::tr("Для новых сценариев лучше использовать if_branch для exec-ветвления "
                        "или вынести типизированный выбор значения в project/submodule helper.")
        },
        {
            "core.control.delay_ms",
            "legacy",
            QObject::tr("Legacy runtime helper"),
            QObject::tr("Сохранён для совместимости, но блокирующая пауза внутри core-model "
                        "хуже подходит для современного workflow DeltaQ и должна постепенно "
                        "уходить во внешние runtime packs."),
            QObject::tr("Для новых сценариев лучше выносить задержки и таймеры в специализированный extension pack.")
        },
        {
            "core.conversion.float_to_int",
            "legacy",
            QObject::tr("Legacy float conversion helper"),
            QObject::tr("Сохранён для совместимости, но partial float conversion внутри core "
                        "не образует достаточно сильный baseline и лучше живёт во внешнем numeric pack-е."),
            QObject::tr("Для новых сценариев лучше собирать float conversion из внешнего numeric pack-а.")
        },
        {
            "core.conversion.int_to_float",
            "legacy",
            QObject::tr("Legacy float conversion helper"),
            QObject::tr("Сохранён для совместимости, но partial float conversion внутри core "
                        "не образует достаточно сильный baseline и лучше живёт во внешнем numeric pack-е."),
            QObject::tr("Для новых сценариев лучше собирать float conversion из внешнего numeric pack-а.")
        },
        {
            "core.math.abs",
            "legacy",
            QObject::tr("Legacy int helper"),
            QObject::tr("Сохранён для совместимости, но узкий прикладной int-helper "
                        "не усиливает checked-in baseline core и лучше живёт во внешнем numeric pack-е "
                        "или как локальный project helper."),
            QObject::tr("Для новых сценариев лучше выносить прикладные integer helper-ы во внешний numeric pack.")
        },
        {
            "core.math.add_float",
            "legacy",
            QObject::tr("Legacy float helper"),
            QObject::tr("Сохранён для совместимости, но partial float arithmetic внутри core "
                        "получилась неполной и не выглядит сильной частью checked-in baseline."),
            QObject::tr("Для новых сценариев лучше выносить float arithmetic в отдельный numeric pack.")
        },
        {
            "core.math.mod",
            "legacy",
            QObject::tr("Legacy int helper"),
            QObject::tr("Сохранён для совместимости, но узкий прикладной int-helper "
                        "не усиливает checked-in baseline core и лучше живёт во внешнем numeric pack-е "
                        "или как локальный project helper."),
            QObject::tr("Для новых сценариев лучше выносить прикладные integer helper-ы во внешний numeric pack.")
        },
        {
            "core.math.multiply_float",
            "legacy",
            QObject::tr("Legacy float helper"),
            QObject::tr("Сохранён для совместимости, но partial float arithmetic внутри core "
                        "получилась неполной и не выглядит сильной частью checked-in baseline."),
            QObject::tr("Для новых сценариев лучше выносить float arithmetic в отдельный numeric pack.")
        },
        {
            "core.math.pow",
            "legacy",
            QObject::tr("Legacy libm wrapper"),
            QObject::tr("Сохранён для совместимости, но как thin wrapper над libm хуже подходит "
                        "для checked-in core-библиотеки, чем для внешнего math pack-а."),
            QObject::tr("Для новых сценариев лучше выносить advanced math во внешний pack с явной линковкой.")
        },
        {
            "core.math.sqrt",
            "legacy",
            QObject::tr("Legacy libm wrapper"),
            QObject::tr("Сохранён для совместимости, но как thin wrapper над libm хуже подходит "
                        "для checked-in core-библиотеки, чем для внешнего math pack-а."),
            QObject::tr("Для новых сценариев лучше выносить advanced math во внешний pack с явной линковкой.")
        },
        {
            "core.string.str_compare",
            "specialized",
            QObject::tr("Дополнительный прикладной модуль"),
            QObject::tr("Полезен для прикладных графов, но не обязателен для базового набора v1."),
            {}
        },
        {
            "core.string.str_concat",
            "specialized",
            QObject::tr("Дополнительный прикладной модуль"),
            QObject::tr("Полезен для прикладных графов, но не обязателен для базового набора v1."),
            {}
        },
        {
            "core.string.str_length",
            "specialized",
            QObject::tr("Дополнительный прикладной модуль"),
            QObject::tr("Полезен для прикладных графов, но не обязателен для базового набора v1."),
            {}
        }
    };
}

// Быстрый lookup reviewed-решений для не-essential части core pack.
static std::optional<ReviewedCurationSpec> reviewedNonEssentialCuration(const QString &moduleId)
{
    static const QVector<ReviewedCurationSpec> reviewed = reviewedNonEssentialCurations();
    for (const auto &entry : reviewed) {
        if (entry.moduleId == moduleId)
            return entry;
    }
    return std::nullopt;
}

// Полный набор checked-in core-модулей, для которых есть осознанное curation-решение.
static QSet<QString> reviewedCoreModuleIds()
{
    const QStringList essentialIds = StandardLibrary::requiredV1ModuleIds();
    QSet<QString> reviewed(essentialIds.begin(), essentialIds.end());
    for (const auto &entry : reviewedNonEssentialCurations())
        reviewed.insert(entry.moduleId);
    return reviewed;
}

// Читает версию pack.json для конкретной директории core pack-а.
static QString packVersionForCoreDir(const QString &coreDir)
{
    QFile packFile(QDir(coreDir).filePath("pack.json"));
    if (!packFile.open(QIODevice::ReadOnly))
        return {};

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(packFile.readAll(), &err);
    if (err.error != QJsonParseError::NoError)
        return {};

    return doc.object()["version"].toString();
}

// Ищет source-of-truth core pack на диске: сначала рядом с приложением, затем от cwd вверх по дереву.
static QString discoverCoreSourceOfTruthDir()
{
    QStringList startPoints;
    if (!QCoreApplication::applicationDirPath().isEmpty())
        startPoints.append(QCoreApplication::applicationDirPath());
    if (!QDir::currentPath().isEmpty())
        startPoints.append(QDir::currentPath());

    QSet<QString> visitedStarts;
    for (const QString &startPoint : startPoints) {
        const QString canonicalStart = QFileInfo(startPoint).canonicalFilePath();
        if (!canonicalStart.isEmpty() && visitedStarts.contains(canonicalStart))
            continue;
        if (!canonicalStart.isEmpty())
            visitedStarts.insert(canonicalStart);

        QDir dir(startPoint);
        for (int depth = 0; depth < 8; ++depth) {
            const QString modulesCandidate = dir.absoluteFilePath("modules/core/pack.json");
            if (QFileInfo::exists(modulesCandidate))
                return QFileInfo(modulesCandidate).absolutePath();

            const QString directCoreCandidate = dir.absoluteFilePath("core/pack.json");
            if (QFileInfo::exists(directCoreCandidate))
                return QFileInfo(directCoreCandidate).absolutePath();

            if (!dir.cdUp())
                break;
        }
    }

    return {};
}

// Загружает checked-in core pack как набор Module, сохраняя файловую source-of-truth семантику.
static QVector<Module> loadCoreModulesFromDir(const QString &coreDir, QStringList *issues = nullptr)
{
    QVector<Module> modules;
    if (coreDir.isEmpty()) {
        if (issues)
            issues->append(QObject::tr("core source-of-truth directory was not found"));
        return modules;
    }

    QDirIterator it(coreDir, {"*.dqmod"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString dqmodPath = it.next();
        QFile file(dqmodPath);
        if (!file.open(QIODevice::ReadOnly)) {
            if (issues)
                issues->append(QObject::tr("failed to open core module file: %1").arg(dqmodPath));
            continue;
        }

        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            if (issues) {
                issues->append(QObject::tr("failed to parse core module file '%1': %2")
                                   .arg(dqmodPath, error.errorString()));
            }
            continue;
        }

        Module module = Module::fromJson(doc.object());
        if (module.id.isEmpty()) {
            if (issues)
                issues->append(QObject::tr("core module file has empty id: %1").arg(dqmodPath));
            continue;
        }

        module.storagePath = dqmodPath;
        modules.append(module);
    }

    std::sort(modules.begin(), modules.end(), [](const Module &lhs, const Module &rhs) {
        return lhs.id < rhs.id;
    });
    return modules;
}

QVector<Module> StandardLibrary::createAll()
{
    return loadCoreModulesFromDir(discoverCoreSourceOfTruthDir());
}

QStringList StandardLibrary::requiredV1Categories()
{
    return {
        "control",
        "io",
        "string",
        "conversion",
        "logic",
        "math",
        "desktop"
    };
}

QStringList StandardLibrary::requiredV1ModuleIds()
{
    QStringList ids;
    for (const auto &signature : requiredModuleSignatures())
        ids.append(signature.moduleId);

    ids.append({
        "core.desktop.sdl_init",
        "core.desktop.ttf_init",
        "core.desktop.create_window",
        "core.desktop.create_renderer",
        "core.desktop.ui_init",
        "core.desktop.event_loop",
        "core.desktop.ui_cleanup_font",
        "core.desktop.destroy_renderer",
        "core.desktop.destroy_window",
        "core.desktop.ttf_quit",
        "core.desktop.sdl_quit"
    });
    return ids;
}

StandardLibraryCurationInfo StandardLibrary::curationForModule(const Module &module)
{
    StandardLibraryCurationInfo info;
    if (module.origin != "core" && !module.id.startsWith("core."))
        return info;

    if (requiredV1ModuleIds().contains(module.id)) {
        info.tier = "essential";
        info.title = QObject::tr("Опорный модуль v1");
        info.guidance = QObject::tr(
            "Это часть минимального набора, на котором строится основной workflow DeltaQ.");
        return info;
    }

    if (const auto reviewed = reviewedNonEssentialCuration(module.id); reviewed.has_value()) {
        // Не-essential часть библиотеки должна идти через explicit review-решение,
        // а не через случайные category-based эвристики.
        info.tier = reviewed->tier;
        info.title = reviewed->title;
        info.guidance = reviewed->guidance;
        info.replacementHint = reviewed->replacementHint;
        return info;
    }

    if (module.category == "math" || module.category == "string" || module.category == "conversion") {
        info.tier = "specialized";
        info.title = QObject::tr("Дополнительный прикладной модуль");
        info.guidance = QObject::tr(
            "Полезен для прикладных графов, но не обязателен для базового набора v1.");
        return info;
    }

    info.tier = "convenience";
    info.title = QObject::tr("Дополнительный convenience-модуль");
    info.guidance = QObject::tr(
        "Может ускорять сборку графа, но не считается обязательным элементом v1 baseline.");
    return info;
}

QStringList StandardLibrary::orderCategoriesForDisplay(const QStringList &categories)
{
    QStringList ordered;
    const QStringList preferred = requiredV1Categories();
    for (const auto &category : preferred) {
        if (categories.contains(category))
            ordered.append(category);
    }

    QStringList extra = categories;
    extra.removeIf([&](const QString &category) {
        return ordered.contains(category);
    });
    extra.sort();
    ordered.append(extra);
    return ordered;
}

QVector<const Module *> StandardLibrary::orderModulesForDisplay(const QVector<const Module *> &modules)
{
    QHash<QString, int> categoryOrder;
    const QStringList orderedCategories = requiredV1Categories();
    for (int index = 0; index < orderedCategories.size(); ++index)
        categoryOrder.insert(orderedCategories.at(index), index);

    QHash<QString, int> moduleOrder;
    const QStringList orderedModuleIds = requiredV1ModuleIds();
    for (int index = 0; index < orderedModuleIds.size(); ++index)
        moduleOrder.insert(orderedModuleIds.at(index), index);

    QVector<const Module *> ordered = modules;

    // Сначала показываем системно важные v1 модули, а затем добиваем категорию алфавитным хвостом.
    std::sort(ordered.begin(), ordered.end(),
              [&](const Module *lhs, const Module *rhs) {
        const int lhsCategoryRank = categoryOrder.value(lhs->category, orderedCategories.size());
        const int rhsCategoryRank = categoryOrder.value(rhs->category, orderedCategories.size());
        if (lhsCategoryRank != rhsCategoryRank)
            return lhsCategoryRank < rhsCategoryRank;

        const int lhsTierRank = curationTierRank(curationForModule(*lhs).tier);
        const int rhsTierRank = curationTierRank(curationForModule(*rhs).tier);
        if (lhsTierRank != rhsTierRank)
            return lhsTierRank < rhsTierRank;

        const int lhsModuleRank = moduleOrder.value(lhs->id, orderedModuleIds.size());
        const int rhsModuleRank = moduleOrder.value(rhs->id, orderedModuleIds.size());
        if (lhsModuleRank != rhsModuleRank)
            return lhsModuleRank < rhsModuleRank;

        const int nameCompare = QString::localeAwareCompare(lhs->name, rhs->name);
        if (nameCompare != 0)
            return nameCompare < 0;

        return lhs->id < rhs->id;
    });

    return ordered;
}

StandardLibraryAuditReport StandardLibrary::audit(const QVector<Module> &modules)
{
    StandardLibraryAuditReport report;
    report.requiredCategories = requiredV1Categories();
    report.requiredModuleIds = requiredV1ModuleIds();

    QMap<QString, const Module *> byId;
    QSet<QString> presentCategorySet;
    QSet<QString> seenIds;
    QMap<QString, QSet<QString>> namesByCategory;

    for (const auto &module : modules) {
        if (module.origin != "core" && !module.id.startsWith("core."))
            continue;

        if (seenIds.contains(module.id))
            report.issues.append(QObject::tr("duplicate core module id: %1").arg(module.id));
        seenIds.insert(module.id);
        byId.insert(module.id, &module);

        presentCategorySet.insert(module.category);

        if (!isSnakeCaseName(module.name)) {
            report.issues.append(
                QObject::tr("core module name must be snake_case: %1").arg(module.name));
        }

        const QString expectedId = QString("core.%1.%2").arg(module.category, module.name);
        if (module.id != expectedId) {
            report.issues.append(
                QObject::tr("core module id/category/name mismatch: expected %1, got %2")
                    .arg(expectedId, module.id));
        }

        if (namesByCategory[module.category].contains(module.name)) {
            report.issues.append(
                QObject::tr("duplicate core module name in category '%1': %2")
                    .arg(module.category, module.name));
        }
        namesByCategory[module.category].insert(module.name);

        if (!reviewedCoreModuleIds().contains(module.id)) {
            report.issues.append(
                QObject::tr("core module lacks explicit curation review: %1").arg(module.id));
        }
    }

    for (const auto &category : presentCategorySet)
        report.presentCategories.append(category);
    report.presentCategories.sort();

    for (const auto &category : report.requiredCategories) {
        if (!presentCategorySet.contains(category))
            report.missingCategories.append(category);
    }

    for (const auto &moduleId : report.requiredModuleIds) {
        if (!byId.contains(moduleId))
            report.missingModuleIds.append(moduleId);
    }

    for (const auto &signature : requiredModuleSignatures()) {
        const Module *module = byId.value(signature.moduleId, nullptr);
        if (!module)
            continue;

        if (!portsMatchExpected(module->inputs, signature.inputs)) {
            report.issues.append(
                QObject::tr("core module input contract mismatch: %1").arg(signature.moduleId));
        }
        if (!portsMatchExpected(module->outputs, signature.outputs)) {
            report.issues.append(
                QObject::tr("core module output contract mismatch: %1").arg(signature.moduleId));
        }
    }

    return report;
}

void StandardLibrary::install(const QString &coreDir)
{
    if (isUpToDate(coreDir))
        return;

    const QString sourceCoreDir = discoverCoreSourceOfTruthDir();
    if (sourceCoreDir.isEmpty())
        return;

    const QString sourceCanonical = QFileInfo(sourceCoreDir).canonicalFilePath();
    const QString targetCanonical = QFileInfo(coreDir).canonicalFilePath();
    if (!sourceCanonical.isEmpty() && !targetCanonical.isEmpty() && sourceCanonical == targetCanonical)
        return;

    QDir().mkpath(coreDir);

    const QDir sourceDir(sourceCoreDir);
    QDirIterator it(sourceCoreDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString sourcePath = it.next();
        const QString relativePath = sourceDir.relativeFilePath(sourcePath);
        const QString targetPath = QDir(coreDir).filePath(relativePath);

        QDir().mkpath(QFileInfo(targetPath).absolutePath());
        QFile::remove(targetPath);
        QFile::copy(sourcePath, targetPath);
    }
}

bool StandardLibrary::isUpToDate(const QString &coreDir)
{
    const QString sourceVersion = packVersionForCoreDir(discoverCoreSourceOfTruthDir());
    if (sourceVersion.isEmpty())
        return false;

    const QString installedVersion = packVersionForCoreDir(coreDir);
    return !installedVersion.isEmpty() && installedVersion == sourceVersion;
}

} // namespace DeltaQ
