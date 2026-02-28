// Генератор CMakeLists.txt для пользовательских проектов DeltaQ
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace DeltaQ {

struct Project;
struct BuildConfig;

class CMakeGenerator : public QObject {
    Q_OBJECT

public:
    explicit CMakeGenerator(QObject *parent = nullptr);

    // Генерация CMakeLists.txt на основе проекта
    // Возвращает путь к сгенерированному файлу
    QString generate(const QString &projectDir, const QString &projectName,
                     const QString &cStandard = "17",
                     const QString &cxxStandard = "20",
                     const QStringList &extraFlags = {},
                     const QString &projectType = "console");

    // Запуск cmake configure
    bool configure(const QString &projectDir);

private:
    // Сбор исходных файлов из директории проекта
    QStringList collectSources(const QString &projectDir) const;

    // Генерация текста CMakeLists.txt
    QString generateContent(const QString &projectName,
                            const QStringList &sources,
                            const QString &cStandard,
                            const QString &cxxStandard,
                            const QStringList &extraFlags,
                            const QString &projectType) const;
};

} // namespace DeltaQ
