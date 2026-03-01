// Компиляция и тестирование модулей — gcc синтаксис, запуск тестов
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QProcess>

namespace DeltaQ {

struct Module;

struct TestResult {
    bool compiled = false;
    bool ran = false;
    bool passed = false;
    QString compilerOutput;
    QString runOutput;
    QMap<QString, QString> outputValues; // порт → значение
    QStringList errors;
};

class ModuleTestRunner : public QObject {
    Q_OBJECT

public:
    explicit ModuleTestRunner(QObject *parent = nullptr);

    // Проверка синтаксиса через gcc -fsyntax-only
    TestResult compile(const Module &module);

    // Генерация тестовой обвязки + компиляция + запуск
    TestResult runTest(const Module &module, const QMap<QString, QString> &inputValues);

signals:
    void compilationFinished(bool success, const QString &output);
    void testFinished(const TestResult &result);

private:
    // Генерация тестового main.c
    QString generateTestHarness(const Module &module,
                                 const QMap<QString, QString> &inputValues);

    // Запуск бинарника с таймаутом
    TestResult executeTest(const QString &binaryPath);

    // Парсинг вывода: OUTPUT:port=value
    QMap<QString, QString> parseOutput(const QString &output);

    QString m_tempDir;
};

} // namespace DeltaQ
