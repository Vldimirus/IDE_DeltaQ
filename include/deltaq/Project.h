#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

namespace DeltaQ {

struct BuildConfig {
    QString system = "cmake";
    QString compiler = "auto";       // project-facing preference: auto | gcc | clang
    QString generator = "auto";      // auto | ninja | unix_makefiles
    QString buildProfile = "Debug";  // Debug | Release
    QString cStandard = "c17";
    QString cxxStandard = "c++20";
    QString outputDir = "build/";    // v1 fixed build root; kept for compatibility
    QStringList extraCFlags;
    QStringList extraCxxFlags;
    QString toolchainMode = "auto";  // auto | manual

    static QJsonArray stringListToJson(const QStringList &values)
    {
        QJsonArray array;
        for (const auto &value : values)
            array.append(value);
        return array;
    }

    static QStringList stringListFromJson(const QJsonValue &value)
    {
        QStringList result;
        for (const auto &entry : value.toArray())
            result.append(entry.toString());
        return result;
    }

    static QString generatorFromLegacyBuildTool(const QString &value)
    {
        const QString normalized = value.trimmed().toLower();
        if (normalized == "make")
            return "unix_makefiles";
        if (normalized == "ninja")
            return "ninja";
        return "auto";
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["system"] = system;
        obj["compiler"] = compiler;
        obj["generator"] = generator;
        obj["profile"] = buildProfile;
        obj["c_standard"] = cStandard;
        obj["cxx_standard"] = cxxStandard;
        obj["output"] = outputDir;
        obj["extra_c_flags"] = stringListToJson(extraCFlags);
        obj["extra_cxx_flags"] = stringListToJson(extraCxxFlags);
        obj["toolchain_mode"] = toolchainMode;
        return obj;
    }

    static BuildConfig fromJson(const QJsonObject &obj) {
        BuildConfig bc;
        bc.system = obj["system"].toString("cmake");
        bc.compiler = obj["compiler"].toString("auto");
        bc.generator = obj["generator"].toString(
            generatorFromLegacyBuildTool(obj["build_tool"].toString("cmake")));
        bc.buildProfile = obj["profile"].toString("Debug");
        bc.cStandard = obj["c_standard"].toString(obj["standard"].toString("c17"));
        bc.cxxStandard = obj["cxx_standard"].toString("c++20");
        bc.outputDir = obj["output"].toString("build/");
        bc.extraCFlags = stringListFromJson(
            obj.contains("extra_c_flags") ? obj["extra_c_flags"] : obj["flags"]);
        bc.extraCxxFlags = stringListFromJson(obj["extra_cxx_flags"]);
        bc.toolchainMode = obj["toolchain_mode"].toString("auto");
        return bc;
    }

    bool operator==(const BuildConfig &other) const {
        return system == other.system
            && compiler == other.compiler
            && generator == other.generator
            && buildProfile == other.buildProfile
            && cStandard == other.cStandard
            && cxxStandard == other.cxxStandard
            && outputDir == other.outputDir
            && extraCFlags == other.extraCFlags
            && extraCxxFlags == other.extraCxxFlags
            && toolchainMode == other.toolchainMode;
    }
};

struct ManualToolchainOverride {
    bool enabled = false;
    QString cmakePath;
    QString cCompilerPath;
    QString cxxCompilerPath;
    QString builderPath;
    QString generator;

    bool isDefault() const {
        return !enabled
            && cmakePath.isEmpty()
            && cCompilerPath.isEmpty()
            && cxxCompilerPath.isEmpty()
            && builderPath.isEmpty()
            && generator.isEmpty();
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["enabled"] = enabled;
        if (!cmakePath.isEmpty())
            obj["cmake_path"] = cmakePath;
        if (!cCompilerPath.isEmpty())
            obj["c_compiler_path"] = cCompilerPath;
        if (!cxxCompilerPath.isEmpty())
            obj["cxx_compiler_path"] = cxxCompilerPath;
        if (!builderPath.isEmpty())
            obj["builder_path"] = builderPath;
        if (!generator.isEmpty())
            obj["generator"] = generator;
        return obj;
    }

    static ManualToolchainOverride fromJson(const QJsonObject &obj)
    {
        ManualToolchainOverride overrideInfo;
        overrideInfo.enabled = obj["enabled"].toBool(false);
        overrideInfo.cmakePath = obj["cmake_path"].toString();
        overrideInfo.cCompilerPath = obj["c_compiler_path"].toString();
        overrideInfo.cxxCompilerPath = obj["cxx_compiler_path"].toString();
        overrideInfo.builderPath = obj["builder_path"].toString();
        overrideInfo.generator = obj["generator"].toString();
        return overrideInfo;
    }

    bool operator==(const ManualToolchainOverride &other) const {
        return enabled == other.enabled
            && cmakePath == other.cmakePath
            && cCompilerPath == other.cCompilerPath
            && cxxCompilerPath == other.cxxCompilerPath
            && builderPath == other.builderPath
            && generator == other.generator;
    }
};

struct ProjectLocalSettings {
    QString selectionMode = "auto";  // auto | kit | manual
    QString selectedKitId;
    ManualToolchainOverride manualOverride;
    QString lastResolvedFingerprint;

    bool isDefault() const {
        return selectionMode == "auto"
            && selectedKitId.isEmpty()
            && manualOverride.isDefault()
            && lastResolvedFingerprint.isEmpty();
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        QJsonObject toolchain;
        toolchain["selection_mode"] = selectionMode;
        if (!selectedKitId.isEmpty())
            toolchain["selected_kit_id"] = selectedKitId;
        toolchain["manual_override"] = manualOverride.toJson();
        if (!lastResolvedFingerprint.isEmpty())
            toolchain["last_resolved_fingerprint"] = lastResolvedFingerprint;
        obj["toolchain"] = toolchain;
        return obj;
    }

    static ProjectLocalSettings fromJson(const QJsonObject &obj)
    {
        ProjectLocalSettings settings;
        const QJsonObject toolchain = obj["toolchain"].toObject();
        settings.selectionMode = toolchain["selection_mode"].toString("auto");
        settings.selectedKitId = toolchain["selected_kit_id"].toString();
        settings.manualOverride = ManualToolchainOverride::fromJson(
            toolchain["manual_override"].toObject());
        settings.lastResolvedFingerprint = toolchain["last_resolved_fingerprint"].toString();
        return settings;
    }

    bool operator==(const ProjectLocalSettings &other) const {
        return selectionMode == other.selectionMode
            && selectedKitId == other.selectedKitId
            && manualOverride == other.manualOverride
            && lastResolvedFingerprint == other.lastResolvedFingerprint;
    }
};

struct Project {
    QString name;
    QString version;
    QString projectFilePath;
    QString projectDir;
    QString projectType;          // "console", "desktop"
    QStringList moduleGlobs;
    QStringList graphGlobs;
    QStringList uiLayoutGlobs;
    BuildConfig build;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["version"] = version;

        auto toArr = [](const QStringList &list) {
            QJsonArray arr;
            for (const auto &s : list)
                arr.append(s);
            return arr;
        };

        obj["modules"] = toArr(moduleGlobs);
        obj["graphs"] = toArr(graphGlobs);
        obj["ui_layouts"] = toArr(uiLayoutGlobs);
        if (!projectType.isEmpty())
            obj["type"] = projectType;
        obj["build"] = build.toJson();
        return obj;
    }

    static Project fromJson(const QJsonObject &obj) {
        Project p;
        p.name = obj["name"].toString();
        p.version = obj["version"].toString("1.0.0");

        auto toList = [](const QJsonArray &arr) {
            QStringList list;
            for (const auto &v : arr)
                list.append(v.toString());
            return list;
        };

        p.moduleGlobs = toList(obj["modules"].toArray());
        p.graphGlobs = toList(obj["graphs"].toArray());
        p.uiLayoutGlobs = toList(obj["ui_layouts"].toArray());
        p.projectType = obj["type"].toString("console");
        p.build = BuildConfig::fromJson(obj["build"].toObject());
        return p;
    }

    bool operator==(const Project &other) const {
        return name == other.name && version == other.version;
    }

    // Валидация: name не пуст
    bool isValid() const {
        return !name.isEmpty();
    }

    static Project createNew(const QString &name) {
        Project p;
        p.name = name;
        p.version = "1.0.0";
        p.moduleGlobs = {"modules/*.dqmod"};
        p.graphGlobs = {"graphs/*.dqgraph"};
        p.uiLayoutGlobs = {"ui/*.dqui"};
        return p;
    }
};

} // namespace DeltaQ
