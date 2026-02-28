// Парсер вывода компилятора (GCC/Clang)
#include "CompilerOutputParser.h"

namespace DeltaQ {

CompilerOutputParser::CompilerOutputParser(QObject *parent)
    : QObject(parent)
    // GCC/Clang: /path/file.cpp:10:5: error: message
    , m_gccPattern(R"(^(.+?):(\d+):(\d+):\s*(error|warning|note):\s*(.+)$)")
    // CMake: CMake Error at file:line
    , m_cmakeErrorPattern(R"(^CMake\s+(Error|Warning)(?:\s+at\s+(.+?):(\d+))?.*?:\s*(.+)$)")
{
}

bool CompilerOutputParser::parseLine(const QString &line)
{
    // Пробуем GCC/Clang формат
    auto match = m_gccPattern.match(line);
    if (match.hasMatch()) {
        CompilerError err;
        err.file = match.captured(1);
        err.line = match.captured(2).toInt();
        err.column = match.captured(3).toInt();

        QString sev = match.captured(4);
        if (sev == "error")
            err.severity = CompilerSeverity::Error;
        else if (sev == "warning")
            err.severity = CompilerSeverity::Warning;
        else
            err.severity = CompilerSeverity::Note;

        err.message = match.captured(5);
        m_errors.append(err);
        emit errorFound(err);
        return true;
    }

    // Пробуем CMake формат
    auto cmakeMatch = m_cmakeErrorPattern.match(line);
    if (cmakeMatch.hasMatch()) {
        CompilerError err;
        QString sev = cmakeMatch.captured(1);
        err.severity = (sev == "Error") ? CompilerSeverity::Error : CompilerSeverity::Warning;
        err.file = cmakeMatch.captured(2);
        err.line = cmakeMatch.captured(3).toInt();
        err.message = cmakeMatch.captured(4);
        m_errors.append(err);
        emit errorFound(err);
        return true;
    }

    return false;
}

QVector<CompilerError> CompilerOutputParser::parseAll(const QString &output)
{
    clear();
    const auto lines = output.split('\n');
    for (const auto &line : lines)
        parseLine(line);
    return m_errors;
}

int CompilerOutputParser::errorCount() const
{
    int count = 0;
    for (const auto &e : m_errors)
        if (e.isError()) ++count;
    return count;
}

int CompilerOutputParser::warningCount() const
{
    int count = 0;
    for (const auto &e : m_errors)
        if (e.isWarning()) ++count;
    return count;
}

void CompilerOutputParser::clear()
{
    m_errors.clear();
}

} // namespace DeltaQ
