// Менеджер сборки — запуск компиляции через CMake
// TODO: полная реализация с парсингом ошибок
#pragma once

#include <QObject>
#include <QProcess>

namespace DeltaQ {

class BuildManager : public QObject {
    Q_OBJECT

public:
    explicit BuildManager(QObject *parent = nullptr);

    void build(const QString &projectDir);
    void clean(const QString &projectDir);
    void cancel();

    bool isBuilding() const { return m_process && m_process->state() != QProcess::NotRunning; }

signals:
    void buildStarted();
    void buildOutput(const QString &text);
    void buildFinished(bool success);

private slots:
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    QProcess *m_process = nullptr;
};

} // namespace DeltaQ
