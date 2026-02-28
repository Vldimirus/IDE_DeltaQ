// Предпросмотр UI — генерация, компиляция и запуск SDL2-приложения
#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

namespace DeltaQ {

struct UILayout;

class UIPreview : public QObject {
    Q_OBJECT

public:
    explicit UIPreview(QObject *parent = nullptr);

    // Запуск предпросмотра: генерация → компиляция → запуск
    void startPreview(const UILayout &layout);

    // Остановка
    void stopPreview();

    bool isRunning() const;

signals:
    void previewStarted();
    void previewStopped(int exitCode);
    void previewError(const QString &error);
    void buildOutput(const QString &text);

private:
    void generateFiles(const UILayout &layout);
    void compileAndRun();

    QProcess *m_process = nullptr;
    QString m_tempDir;
};

} // namespace DeltaQ
