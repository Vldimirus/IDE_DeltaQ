// Предпросмотр UI — реализация
#include "UIPreview.h"
#include "SDL2CodeGenerator.h"

#include <deltaq/UILayout.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

namespace DeltaQ {

UIPreview::UIPreview(QObject *parent)
    : QObject(parent)
{
}

void UIPreview::startPreview(const UILayout &layout)
{
    if (isRunning()) {
        stopPreview();
    }

    generateFiles(layout);
    compileAndRun();
}

void UIPreview::stopPreview()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(3000);
        if (m_process->state() != QProcess::NotRunning)
            m_process->kill();
    }
}

bool UIPreview::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void UIPreview::generateFiles(const UILayout &layout)
{
    // Создаём временную директорию
    QTemporaryDir tmpDir;
    tmpDir.setAutoRemove(false);
    m_tempDir = tmpDir.path();

    GeneratedCode code = SDL2CodeGenerator::generate(layout);

    auto writeFile = [&](const QString &name, const QString &content) {
        QFile f(m_tempDir + "/" + name);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(content.toUtf8());
            f.close();
        }
    };

    writeFile("main.c", code.mainFile);
    writeFile("ui.h", code.uiHeader);
    writeFile("ui.c", code.uiSource);
    writeFile("events.h", code.eventsHeader);
    writeFile("events.c", code.eventsSource);
}

void UIPreview::compileAndRun()
{
    // Компилируем с gcc
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(m_tempDir);

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        emit buildOutput(m_process->readAllStandardOutput());
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        emit buildOutput(m_process->readAllStandardError());
    });

    // Сначала компилируем
    QString compileCmd = "gcc";
    QStringList compileArgs = {
        "main.c", "ui.c", "events.c",
        "-o", "preview",
        "-lSDL2", "-lSDL2_ttf",
        "-Wall", "-Wextra"
    };

    auto *compileProcess = new QProcess(this);
    compileProcess->setWorkingDirectory(m_tempDir);

    connect(compileProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, compileProcess](int exitCode, QProcess::ExitStatus) {
        if (exitCode != 0) {
            QString err = compileProcess->readAllStandardError();
            emit previewError(tr("Compilation failed:\n") + err);
            compileProcess->deleteLater();
            return;
        }
        compileProcess->deleteLater();

        // Запускаем предпросмотр
        m_process->start(m_tempDir + "/preview", {});
        if (!m_process->waitForStarted(5000)) {
            emit previewError(tr("Failed to start preview"));
            return;
        }
        emit previewStarted();

        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int code, QProcess::ExitStatus) {
            emit previewStopped(code);
        });
    });

    compileProcess->start(compileCmd, compileArgs);
}

} // namespace DeltaQ
