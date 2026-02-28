// Генерация шаблонных файлов при создании проекта
#pragma once

#include <QString>

namespace DeltaQ {

class ProjectTemplates {
public:
    // Генерация шаблонных файлов по типу проекта ("console" / "desktop")
    static bool generate(const QString &type,
                         const QString &projectDir,
                         const QString &name);

private:
    static bool generateConsoleTemplate(const QString &projectDir,
                                        const QString &name);
    static bool generateDesktopTemplate(const QString &projectDir,
                                        const QString &name);

    // Генерация .dqmod и .dqgraph файлов
    static bool generateConsoleModulesAndGraphs(const QString &projectDir);
    static bool generateDesktopModulesAndGraphs(const QString &projectDir);
};

} // namespace DeltaQ
