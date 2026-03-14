#pragma once

#include "CompilerDetector.h"

#include <deltaq/BuildTypes.h>
#include <deltaq/Project.h>

#include <QVector>

namespace DeltaQ {

struct ToolchainInventory {
    QVector<CompilerInfo> compilers;
    QVector<BuildToolInfo> buildTools;
};

class ToolchainResolver {
public:
    static ToolchainInventory inventoryFromDetector(const CompilerDetector &detector);
    static QVector<DetectedToolchainKit> detectKits(const ToolchainInventory &inventory);
    static ResolvedToolchainConfig resolve(const BuildConfig &buildConfig,
                                           const ProjectLocalSettings &localSettings,
                                           const ToolchainInventory &inventory);

    static QString cmakeGeneratorName(const QString &generatorId);
    static QString normalizedGeneratorId(const QString &generatorId);
    static QString normalizedBuildProfile(const QString &profile);
};

} // namespace DeltaQ
