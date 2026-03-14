#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace DeltaQ {

struct DetectedToolchainKit {
    QString kitId;
    QString displayName;
    QString cmakePath;
    QString cCompilerPath;
    QString cxxCompilerPath;
    QString builderPath;
    QString generator;             // auto | ninja | unix_makefiles
    QString generatorDisplayName;  // Ninja | Unix Makefiles
    QString status = "missing";    // ready | partial | missing

    bool isReady() const { return status == "ready"; }
};

struct ResolvedToolchainConfig {
    QString selectionMode = "auto";
    QString resolvedKitId;
    QString cmakePath = "cmake";
    QString cCompilerPath;
    QString cxxCompilerPath;
    QString builderPath;
    QString generator = "auto";
    QString generatorDisplayName;
    QString buildProfile = "Debug";
    QString fingerprint;
    QStringList issues;
    bool valid = false;
};

struct ProjectBuildRequest {
    QString projectDir;
    QString projectName;
    QString projectType = "console";
    QString cStandard = "c17";
    QString cxxStandard = "c++20";
    QStringList extraCFlags;
    QStringList extraCxxFlags;
    ResolvedToolchainConfig toolchain;
};

} // namespace DeltaQ
