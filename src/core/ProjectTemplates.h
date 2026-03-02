// Копирование файловых шаблонов при создании проекта
#pragma once

#include <QString>

namespace DeltaQ {

class ProjectTemplates {
public:
    // Копирование шаблонных файлов по типу проекта ("console" / "desktop")
    static bool generate(const QString &type,
                         const QString &projectDir,
                         const QString &name);

private:
    // Путь к каталогу шаблонов рядом с исполняемым файлом
    static QString templatesDir();

    // Рекурсивное копирование каталога src → dst
    static bool copyDirectory(const QString &src, const QString &dst);
};

} // namespace DeltaQ
