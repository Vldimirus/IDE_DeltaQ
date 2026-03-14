#include "ToolchainResolver.h"
#include "CompilerDetector.h"

#include <algorithm>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace DeltaQ {

namespace {

struct CompilerFamily {
    QString id;
    QString displayName;
    QString cPath;
    QString cxxPath;
};

bool isUsableExecutable(const QString &path)
{
    if (path.trimmed().isEmpty())
        return false;
    if (QDir::isAbsolutePath(path)) {
        const QFileInfo info(path);
        return info.exists() && info.isFile() && info.isExecutable();
    }
    return !QStandardPaths::findExecutable(path).isEmpty();
}

QString resolveExecutablePath(const QString &path)
{
    if (!isUsableExecutable(path))
        return {};
    if (QDir::isAbsolutePath(path))
        return QFileInfo(path).canonicalFilePath();
    const QString resolved = QStandardPaths::findExecutable(path);
    return resolved.isEmpty() ? path : resolved;
}

template <typename T>
const T *findAvailableByName(const QVector<T> &items, const QString &name)
{
    for (const auto &item : items) {
        if (item.name == name && item.available)
            return &item;
    }
    return nullptr;
}

QString generatorForBuildTool(const QString &buildToolName)
{
    if (buildToolName == "ninja")
        return "ninja";
    if (buildToolName == "make")
        return "unix_makefiles";
    return {};
}

CompilerFamily bestSystemFamily(const ToolchainInventory &inventory)
{
    const CompilerInfo *bestC = nullptr;
    for (const auto &candidate : inventory.compilers) {
        if (!candidate.available)
            continue;
        if (candidate.name == "gcc" || candidate.name == "clang" || candidate.name == "cc") {
            bestC = &candidate;
            break;
        }
    }

    const CompilerInfo *bestCpp = nullptr;
    for (const auto &candidate : inventory.compilers) {
        if (!candidate.available)
            continue;
        if (candidate.name == "g++" || candidate.name == "clang++" || candidate.name == "c++") {
            bestCpp = &candidate;
            break;
        }
    }

    CompilerFamily family;
    family.id = "system";
    family.displayName = "System";
    family.cPath = bestC ? bestC->path : QString();
    family.cxxPath = bestCpp ? bestCpp->path : QString();
    return family;
}

QVector<CompilerFamily> detectCompilerFamilies(const ToolchainInventory &inventory)
{
    QVector<CompilerFamily> families;

    if (const CompilerInfo *gcc = findAvailableByName(inventory.compilers, "gcc")) {
        if (const CompilerInfo *gxx = findAvailableByName(inventory.compilers, "g++")) {
            CompilerFamily family;
            family.id = "gcc";
            family.displayName = "GCC";
            family.cPath = gcc->path;
            family.cxxPath = gxx->path;
            families.append(family);
        }
    }

    if (const CompilerInfo *clang = findAvailableByName(inventory.compilers, "clang")) {
        if (const CompilerInfo *clangxx = findAvailableByName(inventory.compilers, "clang++")) {
            CompilerFamily family;
            family.id = "clang";
            family.displayName = "Clang";
            family.cPath = clang->path;
            family.cxxPath = clangxx->path;
            families.append(family);
        }
    }

    const CompilerFamily systemFamily = bestSystemFamily(inventory);
    if (!systemFamily.cPath.isEmpty() && !systemFamily.cxxPath.isEmpty()) {
        const bool duplicate = std::any_of(families.begin(), families.end(),
                                           [&systemFamily](const CompilerFamily &family) {
            return family.cPath == systemFamily.cPath
                && family.cxxPath == systemFamily.cxxPath;
        });
        if (!duplicate)
            families.append(systemFamily);
    }

    return families;
}

QString preferredFamilyId(const BuildConfig &buildConfig)
{
    const QString compiler = buildConfig.compiler.trimmed().toLower();
    if (compiler == "gcc" || compiler == "clang")
        return compiler;
    return "gcc";
}

QString fingerprintFor(const ResolvedToolchainConfig &config)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    const QStringList parts = {
        config.selectionMode,
        config.resolvedKitId,
        config.cmakePath,
        config.cCompilerPath,
        config.cxxCompilerPath,
        config.builderPath,
        config.generator,
        config.generatorDisplayName,
        config.buildProfile
    };
    for (const QString &part : parts) {
        hash.addData(part.toUtf8());
        hash.addData(QByteArrayView("\n", 1));
    }
    return "sha256:" + hash.result().toHex();
}

const BuildToolInfo *findAvailableBuildTool(const ToolchainInventory &inventory, const QString &name)
{
    return findAvailableByName(inventory.buildTools, name);
}

QString builderPathForGenerator(const ToolchainInventory &inventory, const QString &generatorId)
{
    if (generatorId == "ninja") {
        if (const BuildToolInfo *tool = findAvailableBuildTool(inventory, "ninja"))
            return tool->path;
        return {};
    }
    if (generatorId == "unix_makefiles") {
        if (const BuildToolInfo *tool = findAvailableBuildTool(inventory, "make"))
            return tool->path;
        return {};
    }
    return {};
}

const DetectedToolchainKit *findReadyKitById(const QVector<DetectedToolchainKit> &kits, const QString &kitId)
{
    for (const auto &kit : kits) {
        if (kit.kitId == kitId && kit.isReady())
            return &kit;
    }
    return nullptr;
}

const DetectedToolchainKit *findBestAutoKit(const QVector<DetectedToolchainKit> &kits,
                                            const BuildConfig &buildConfig)
{
    QVector<const DetectedToolchainKit *> readyKits;
    for (const auto &kit : kits) {
        if (kit.isReady())
            readyKits.append(&kit);
    }
    if (readyKits.isEmpty())
        return nullptr;

    const QString preferredGenerator = ToolchainResolver::normalizedGeneratorId(buildConfig.generator);
    const QString preferredFamily = preferredFamilyId(buildConfig);

    auto matchKit = [&](const QString &familyId, const QString &generatorId) -> const DetectedToolchainKit * {
        for (const auto *kit : readyKits) {
            if (kit->kitId == QString("linux-%1-%2").arg(familyId, generatorId))
                return kit;
        }
        return nullptr;
    };

    if (preferredGenerator != "auto") {
        if (const auto *kit = matchKit(preferredFamily, preferredGenerator))
            return kit;
    }

    const QStringList familyPriority = preferredFamily == "clang"
        ? QStringList{"clang", "gcc", "system"}
        : QStringList{"gcc", "clang", "system"};
    const QStringList generatorPriority = preferredGenerator == "unix_makefiles"
        ? QStringList{"unix_makefiles", "ninja"}
        : QStringList{"ninja", "unix_makefiles"};

    for (const QString &familyId : familyPriority) {
        for (const QString &generatorId : generatorPriority) {
            if (const auto *kit = matchKit(familyId, generatorId))
                return kit;
        }
    }

    return readyKits.first();
}

void applyManualOverrides(ResolvedToolchainConfig &resolved,
                          const ProjectLocalSettings &localSettings)
{
    if (localSettings.selectionMode != "manual" && !localSettings.manualOverride.enabled)
        return;

    resolved.selectionMode = "manual";
    const auto &manual = localSettings.manualOverride;

    if (!manual.cmakePath.trimmed().isEmpty())
        resolved.cmakePath = manual.cmakePath.trimmed();
    if (!manual.cCompilerPath.trimmed().isEmpty())
        resolved.cCompilerPath = manual.cCompilerPath.trimmed();
    if (!manual.cxxCompilerPath.trimmed().isEmpty())
        resolved.cxxCompilerPath = manual.cxxCompilerPath.trimmed();
    if (!manual.builderPath.trimmed().isEmpty())
        resolved.builderPath = manual.builderPath.trimmed();
    if (!manual.generator.trimmed().isEmpty())
        resolved.generator = ToolchainResolver::normalizedGeneratorId(manual.generator);
}

} // namespace

ToolchainInventory ToolchainResolver::inventoryFromDetector(const CompilerDetector &detector)
{
    ToolchainInventory inventory;
    inventory.compilers = detector.compilers();
    inventory.buildTools = detector.buildTools();
    return inventory;
}

QVector<DetectedToolchainKit> ToolchainResolver::detectKits(const ToolchainInventory &inventory)
{
    QVector<DetectedToolchainKit> kits;

    const QString cmakePath = findAvailableBuildTool(inventory, "cmake")
        ? findAvailableBuildTool(inventory, "cmake")->path
        : QString();
    const QVector<CompilerFamily> families = detectCompilerFamilies(inventory);
    const QStringList buildTools = {"ninja", "make"};

    for (const auto &family : families) {
        for (const auto &buildToolName : buildTools) {
            DetectedToolchainKit kit;
            kit.generator = generatorForBuildTool(buildToolName);
            kit.generatorDisplayName = cmakeGeneratorName(kit.generator);
            kit.kitId = QString("linux-%1-%2").arg(family.id, kit.generator);
            kit.displayName = QString("%1 + %2").arg(family.displayName,
                                                     kit.generator == "ninja" ? "Ninja" : "Make");
            kit.cmakePath = cmakePath;
            kit.cCompilerPath = family.cPath;
            kit.cxxCompilerPath = family.cxxPath;
            kit.builderPath = builderPathForGenerator(inventory, kit.generator);

            const bool complete = !kit.cmakePath.isEmpty()
                && !kit.cCompilerPath.isEmpty()
                && !kit.cxxCompilerPath.isEmpty()
                && !kit.builderPath.isEmpty();
            const bool partial = !kit.cCompilerPath.isEmpty() && !kit.cxxCompilerPath.isEmpty();

            if (complete)
                kit.status = "ready";
            else if (partial)
                kit.status = "partial";
            else
                kit.status = "missing";

            kits.append(kit);
        }
    }

    return kits;
}

ResolvedToolchainConfig ToolchainResolver::resolve(const BuildConfig &buildConfig,
                                                   const ProjectLocalSettings &localSettings,
                                                   const ToolchainInventory &inventory)
{
    ResolvedToolchainConfig resolved;
    resolved.selectionMode = localSettings.selectionMode;
    resolved.buildProfile = normalizedBuildProfile(buildConfig.buildProfile);

    const QVector<DetectedToolchainKit> kits = detectKits(inventory);
    const DetectedToolchainKit *baseKit = nullptr;

    if (localSettings.selectionMode == "kit")
        baseKit = findReadyKitById(kits, localSettings.selectedKitId);
    if (!baseKit)
        baseKit = findBestAutoKit(kits, buildConfig);

    if (baseKit) {
        resolved.resolvedKitId = baseKit->kitId;
        resolved.cmakePath = baseKit->cmakePath;
        resolved.cCompilerPath = baseKit->cCompilerPath;
        resolved.cxxCompilerPath = baseKit->cxxCompilerPath;
        resolved.builderPath = baseKit->builderPath;
        resolved.generator = baseKit->generator;
        resolved.generatorDisplayName = baseKit->generatorDisplayName;
    }

    applyManualOverrides(resolved, localSettings);

    if (resolved.selectionMode.isEmpty())
        resolved.selectionMode = "auto";

    if (resolved.cmakePath.isEmpty()) {
        if (const BuildToolInfo *cmake = findAvailableBuildTool(inventory, "cmake"))
            resolved.cmakePath = cmake->path;
    }

    if (resolved.generator == "auto" || resolved.generator.isEmpty()) {
        const QString preferredGenerator = normalizedGeneratorId(buildConfig.generator);
        resolved.generator = preferredGenerator == "auto"
            ? (builderPathForGenerator(inventory, "ninja").isEmpty() ? "unix_makefiles" : "ninja")
            : preferredGenerator;
    }
    resolved.generatorDisplayName = cmakeGeneratorName(resolved.generator);

    if (resolved.builderPath.isEmpty())
        resolved.builderPath = builderPathForGenerator(inventory, resolved.generator);

    if (resolved.cCompilerPath.isEmpty() || resolved.cxxCompilerPath.isEmpty()) {
        const QVector<CompilerFamily> families = detectCompilerFamilies(inventory);
        const QString preferredFamily = preferredFamilyId(buildConfig);
        for (const auto &family : families) {
            if (family.id == preferredFamily || preferredFamily == "auto" || preferredFamily == "gcc") {
                if (resolved.cCompilerPath.isEmpty())
                    resolved.cCompilerPath = family.cPath;
                if (resolved.cxxCompilerPath.isEmpty())
                    resolved.cxxCompilerPath = family.cxxPath;
                break;
            }
        }
    }

    resolved.cmakePath = resolveExecutablePath(resolved.cmakePath);
    resolved.cCompilerPath = resolveExecutablePath(resolved.cCompilerPath);
    resolved.cxxCompilerPath = resolveExecutablePath(resolved.cxxCompilerPath);
    resolved.builderPath = resolveExecutablePath(resolved.builderPath);

    if (resolved.cmakePath.isEmpty())
        resolved.issues.append("cmake");
    if (resolved.cCompilerPath.isEmpty())
        resolved.issues.append("C compiler");
    if (resolved.cxxCompilerPath.isEmpty())
        resolved.issues.append("C++ compiler");
    if (resolved.generatorDisplayName.isEmpty())
        resolved.issues.append("generator");
    if (resolved.builderPath.isEmpty())
        resolved.issues.append("builder");

    resolved.valid = resolved.issues.isEmpty();
    resolved.fingerprint = fingerprintFor(resolved);
    return resolved;
}

QString ToolchainResolver::cmakeGeneratorName(const QString &generatorId)
{
    const QString normalized = normalizedGeneratorId(generatorId);
    if (normalized == "ninja")
        return "Ninja";
    if (normalized == "unix_makefiles")
        return "Unix Makefiles";
    return {};
}

QString ToolchainResolver::normalizedGeneratorId(const QString &generatorId)
{
    const QString normalized = generatorId.trimmed().toLower();
    if (normalized == "ninja")
        return "ninja";
    if (normalized == "unix_makefiles" || normalized == "make" || normalized == "unix makefiles")
        return "unix_makefiles";
    return "auto";
}

QString ToolchainResolver::normalizedBuildProfile(const QString &profile)
{
    const QString normalized = profile.trimmed().toLower();
    if (normalized == "release")
        return "Release";
    if (normalized == "relwithdebinfo")
        return "RelWithDebInfo";
    if (normalized == "minsizerel")
        return "MinSizeRel";
    return "Debug";
}

} // namespace DeltaQ
