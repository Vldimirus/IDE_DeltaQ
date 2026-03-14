#include "BuildGuidanceAnalyzer.h"

#include <QFile>
#include <QSet>
#include <QTextStream>

namespace DeltaQ {

namespace {

QString normalizedDistroId(const QString &distroId)
{
    return distroId.trimmed().remove('"').toLower();
}

QStringList distroToolchainSteps(const QString &distroId)
{
    const QString normalized = normalizedDistroId(distroId);
    if (normalized == "ubuntu" || normalized == "debian" || normalized == "linuxmint") {
        return {
            "sudo apt install cmake build-essential ninja-build pkg-config",
            QObject::tr("Then run Project -> Rescan Toolchains.")
        };
    }
    if (normalized == "fedora") {
        return {
            "sudo dnf install cmake gcc-c++ make ninja-build pkgconf-pkg-config",
            QObject::tr("Then run Project -> Rescan Toolchains.")
        };
    }
    if (normalized == "arch" || normalized == "manjaro") {
        return {
            "sudo pacman -S cmake base-devel ninja pkgconf",
            QObject::tr("Then run Project -> Rescan Toolchains.")
        };
    }
    return {
        QObject::tr("Install CMake, a C/C++ compiler and a builder (Ninja or Make)."),
        QObject::tr("Then run Project -> Rescan Toolchains.")
    };
}

QStringList distroDesktopSteps(const QString &distroId)
{
    const QString normalized = normalizedDistroId(distroId);
    if (normalized == "ubuntu" || normalized == "debian" || normalized == "linuxmint") {
        return {
            "sudo apt install libsdl2-dev libsdl2-ttf-dev pkg-config",
            QObject::tr("Then rebuild the desktop project.")
        };
    }
    if (normalized == "fedora") {
        return {
            "sudo dnf install SDL2-devel SDL2_ttf-devel pkgconf-pkg-config",
            QObject::tr("Then rebuild the desktop project.")
        };
    }
    if (normalized == "arch" || normalized == "manjaro") {
        return {
            "sudo pacman -S sdl2 sdl2_ttf pkgconf",
            QObject::tr("Then rebuild the desktop project.")
        };
    }
    return {
        QObject::tr("Install SDL2 development files, SDL2_ttf development files and pkg-config."),
        QObject::tr("Then rebuild the desktop project.")
    };
}

QString issueListLabel(const QStringList &issues)
{
    QStringList normalizedIssues = issues;
    normalizedIssues.removeAll(QString());
    normalizedIssues.removeDuplicates();
    return normalizedIssues.join(", ");
}

} // namespace

BuildGuidance BuildGuidanceAnalyzer::forToolchainIssues(const QStringList &issues,
                                                        const QString &projectType,
                                                        const QString &distroId)
{
    Q_UNUSED(projectType);

    const QStringList normalizedIssues = issues;
    if (normalizedIssues.isEmpty())
        return {};

    BuildGuidance guidance;
    guidance.hasGuidance = true;
    guidance.title = QObject::tr("Toolchain setup required");
    guidance.summary = QObject::tr("DeltaQ cannot start the build because required tools are missing: %1")
        .arg(issueListLabel(normalizedIssues));
    guidance.details = QObject::tr("Open Project Properties to choose another kit, or install the missing tools and rescan.");
    guidance.nextSteps = distroToolchainSteps(distroId.isEmpty() ? detectDistroId() : distroId);
    return guidance;
}

BuildGuidance BuildGuidanceAnalyzer::fromBuildOutput(const ProjectBuildRequest &request,
                                                     const QString &buildOutput,
                                                     const QString &distroId)
{
    if (request.projectType != "desktop")
        return {};

    if (!buildOutput.contains("Desktop template requires SDL2 and SDL2_ttf"))
        return {};

    BuildGuidance guidance;
    guidance.hasGuidance = true;
    guidance.title = QObject::tr("Desktop dependencies required");
    guidance.summary = QObject::tr("The desktop project cannot be configured because SDL2 and SDL2_ttf development packages are missing.");
    guidance.details = QObject::tr("This is a system dependency issue, not a graph or code generation error.");
    guidance.nextSteps = distroDesktopSteps(distroId.isEmpty() ? detectDistroId() : distroId);
    return guidance;
}

QString BuildGuidanceAnalyzer::detectDistroId(const QString &osReleasePath)
{
    QFile file(osReleasePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (!line.startsWith("ID="))
            continue;
        return normalizedDistroId(line.mid(3));
    }

    return {};
}

} // namespace DeltaQ
