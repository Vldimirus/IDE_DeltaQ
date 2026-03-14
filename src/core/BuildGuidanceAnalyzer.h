#pragma once

#include <deltaq/BuildTypes.h>

namespace DeltaQ {

struct BuildGuidance {
    bool hasGuidance = false;
    QString title;
    QString summary;
    QString details;
    QStringList nextSteps;
};

class BuildGuidanceAnalyzer {
public:
    static BuildGuidance forToolchainIssues(const QStringList &issues,
                                            const QString &projectType,
                                            const QString &distroId = {});
    static BuildGuidance fromBuildOutput(const ProjectBuildRequest &request,
                                         const QString &buildOutput,
                                         const QString &distroId = {});
    static QString detectDistroId(const QString &osReleasePath = "/etc/os-release");
};

} // namespace DeltaQ
