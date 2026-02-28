// Менеджер сборки — базовая реализация
#include "BuildManager.h"
#include <QDir>

namespace DeltaQ {

BuildManager::BuildManager(QObject *parent)
    : QObject(parent)
{
}

void BuildManager::build(const QString &projectDir)
{
    if (isBuilding())
        return;

    // Создаём директорию сборки
    QString buildDir = projectDir + "/build";
    QDir().mkpath(buildDir);

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(buildDir);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildManager::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildManager::onProcessOutput);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BuildManager::onProcessFinished);

    emit buildStarted();
    emit buildOutput("=== Сборка проекта ===\n");

    // Сначала cmake, потом make
    m_process->start("cmake", {"--build", ".", "--parallel"});
}

void BuildManager::clean(const QString &projectDir)
{
    QString buildDir = projectDir + "/build";
    QDir(buildDir).removeRecursively();
    emit buildOutput("=== Очистка завершена ===\n");
}

void BuildManager::cancel()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        emit buildOutput("\n=== Сборка отменена ===\n");
    }
}

void BuildManager::onProcessOutput()
{
    if (!m_process)
        return;
    QString out = m_process->readAllStandardOutput();
    QString err = m_process->readAllStandardError();
    if (!out.isEmpty())
        emit buildOutput(out);
    if (!err.isEmpty())
        emit buildOutput(err);
}

void BuildManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    bool success = (status == QProcess::NormalExit && exitCode == 0);
    if (success)
        emit buildOutput("\n=== Сборка завершена успешно ===\n");
    else
        emit buildOutput(QString("\n=== Сборка завершена с ошибкой (код: %1) ===\n").arg(exitCode));

    emit buildFinished(success);
    m_process->deleteLater();
    m_process = nullptr;
}

} // namespace DeltaQ
