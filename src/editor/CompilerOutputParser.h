// Парсер вывода компилятора (GCC/Clang) — извлечение ошибок и предупреждений
#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QRegularExpression>

namespace DeltaQ {

// Тяжесть ошибки компилятора
enum class CompilerSeverity {
    Error,
    Warning,
    Note
};

// Ошибка/предупреждение компилятора
struct CompilerError {
    QString file;
    int line = 0;
    int column = 0;
    CompilerSeverity severity = CompilerSeverity::Error;
    QString message;

    bool isError() const { return severity == CompilerSeverity::Error; }
    bool isWarning() const { return severity == CompilerSeverity::Warning; }
};

class CompilerOutputParser : public QObject {
    Q_OBJECT

public:
    explicit CompilerOutputParser(QObject *parent = nullptr);

    // Парсинг одной строки вывода — возвращает true если найдена ошибка
    bool parseLine(const QString &line);

    // Парсинг всего вывода сразу
    QVector<CompilerError> parseAll(const QString &output);

    // Результаты
    const QVector<CompilerError> &errors() const { return m_errors; }
    int errorCount() const;
    int warningCount() const;

    // Очистка
    void clear();

signals:
    void errorFound(const CompilerError &error);

private:
    QVector<CompilerError> m_errors;

    // GCC/Clang формат: file:line:col: severity: message
    QRegularExpression m_gccPattern;
    // CMake ошибки
    QRegularExpression m_cmakeErrorPattern;
};

} // namespace DeltaQ
