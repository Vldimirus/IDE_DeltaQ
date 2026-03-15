// Компиляция и тестирование модулей — реализация
#include "ModuleTestRunner.h"
#include <deltaq/Module.h>

#include <QDir>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QTextStream>
#include <QProcess>

namespace DeltaQ {

namespace {

// Возвращает metadata-массив строк без дублирования однотипного JSON-кода.
QStringList metadataStringList(const Module &module, const QString &key)
{
    QStringList result;
    const QJsonArray values = module.metadata.value(key).toArray();
    for (const auto &value : values) {
        const QString text = value.toString().trimmed();
        if (!text.isEmpty())
            result.append(text);
    }
    return result;
}

// Ищет корень imported pack-а по storagePath модуля, чтобы verification path видел local headers.
QString modulePackRootDir(const Module &module)
{
    if (module.storagePath.trimmed().isEmpty())
        return {};

    QDir dir(QFileInfo(module.storagePath).absolutePath());
    for (int depth = 0; depth < 6; ++depth) {
        if (QFileInfo::exists(dir.filePath("pack.json")))
            return dir.absolutePath();
        if (!dir.cdUp())
            break;
    }
    return {};
}

// Возвращает local include-dir imported pack-а, если он лежит рядом с `.dqmod`.
QString modulePackLocalIncludeDir(const Module &module)
{
    const QString packRoot = modulePackRootDir(module);
    if (packRoot.isEmpty())
        return {};

    const QString includeDir = QDir(packRoot).filePath("include");
    return QFileInfo::exists(includeDir) ? includeDir : QString();
}

// Собирает include-path аргументы для compile/verify path imported pack-а.
QStringList moduleIncludeArgs(const Module &module)
{
    QStringList args;
    const QString localIncludeDir = modulePackLocalIncludeDir(module);
    if (!localIncludeDir.isEmpty())
        args << QString("-I%1").arg(localIncludeDir);

    for (const QString &includePath : metadataStringList(module, "deltaq.import.include_paths"))
        args << QString("-I%1").arg(includePath);
    return args;
}

// Преобразует metadata link requirements в аргументы linker-а для verification harness.
QStringList moduleLinkArgs(const Module &module)
{
    QStringList args;
    for (const QString &linkValue : metadataStringList(module, "deltaq.import.link_libraries")) {
        if (linkValue.startsWith('-')) {
            args << linkValue;
        } else if (QFileInfo::exists(linkValue) || linkValue.contains('/')) {
            args << linkValue;
        } else {
            args << QString("-l%1").arg(linkValue);
        }
    }
    return args;
}

// Экранирует строку для безопасной подстановки в generated C string literal.
QString escapeCStringLiteral(const QString &value)
{
    QString escaped = value;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    escaped.replace("\n", "\\n");
    escaped.replace("\r", "\\r");
    escaped.replace("\t", "\\t");
    return escaped;
}

}

ModuleTestRunner::ModuleTestRunner(QObject *parent)
    : QObject(parent)
{
}

// Проверяет, что модуль компилируется сам по себе вместе с imported-pack include path.
TestResult ModuleTestRunner::compile(const Module &module)
{
    TestResult result;

    QTemporaryDir tmpDir;
    if (!tmpDir.isValid()) {
        result.errors.append(tr("Не удалось создать временную директорию"));
        return result;
    }
    tmpDir.setAutoRemove(true);

    // Формируем исходный файл: includes + тело функции
    QString sourceFile = tmpDir.path() + "/module_check.c";
    QFile f(sourceFile);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.errors.append(tr("Не удалось создать файл для компиляции"));
        return result;
    }

    QTextStream out(&f);

    // Стандартные включения
    out << "#include <stdio.h>\n";
    out << "#include <stdlib.h>\n";

    // Включения модуля
    for (const auto &inc : module.includes) {
        if (inc.startsWith('<') || inc.startsWith('"'))
            out << "#include " << inc << "\n";
        else
            out << "#include <" << inc << ">\n";
    }
    out << "\n";

    // Тело функции
    out << module.sourceCode << "\n";
    f.close();

    // Запускаем gcc -fsyntax-only
    QProcess gcc;
    QString compiler = (module.language == "cpp") ? "g++" : "gcc";
    QStringList args;
    args << "-fsyntax-only" << "-Wall" << "-Wextra";
    args << moduleIncludeArgs(module);
    args << sourceFile;

    gcc.start(compiler, args);
    gcc.waitForFinished(10000); // 10 секунд таймаут

    result.compilerOutput = gcc.readAllStandardError();
    result.compiled = (gcc.exitCode() == 0);

    if (!result.compiled) {
        result.errors.append(tr("Ошибка компиляции"));
    }

    emit compilationFinished(result.compiled, result.compilerOutput);
    return result;
}

// Генерирует harness, собирает его с imported-pack requirements и запускает verification scenario.
TestResult ModuleTestRunner::runTest(const Module &module,
                                     const QMap<QString, QString> &inputValues,
                                     const QMap<QString, QString> &expectedOutputValues)
{
    TestResult result;

    QTemporaryDir tmpDir;
    if (!tmpDir.isValid()) {
        result.errors.append(tr("Не удалось создать временную директорию"));
        return result;
    }
    tmpDir.setAutoRemove(true);
    m_tempDir = tmpDir.path();

    // Генерируем тестовый файл
    QString testSource = generateTestHarness(module, inputValues);
    QString sourceFile = tmpDir.path() + "/test_main.c";
    QFile f(sourceFile);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.errors.append(tr("Не удалось создать тестовый файл"));
        return result;
    }
    f.write(testSource.toUtf8());
    f.close();

    // Компиляция
    QString binaryFile = tmpDir.path() + "/test_binary";
    QProcess gcc;
    QString compiler = (module.language == "cpp") ? "g++" : "gcc";
    QStringList args;
    args << "-o" << binaryFile << sourceFile << "-lm";
    args << moduleIncludeArgs(module);
    args << moduleLinkArgs(module);

    gcc.start(compiler, args);
    gcc.waitForFinished(10000);

    result.compilerOutput = gcc.readAllStandardError();
    result.compiled = (gcc.exitCode() == 0);
    emit compilationFinished(result.compiled, result.compilerOutput);

    if (!result.compiled) {
        result.errors.append(tr("Ошибка компиляции тестовой обвязки"));
        emit testFinished(result);
        return result;
    }

    // Запуск
    result = executeTest(binaryFile);
    result.compiled = true;
    result.expectedOutputValues = expectedOutputValues;
    applyExpectedOutputs(&result, expectedOutputValues);

    emit testFinished(result);
    return result;
}

// Строит минимальный `main()` для одного атомарного модуля с учётом exec/data contract.
QString ModuleTestRunner::generateTestHarness(const Module &module,
                                                const QMap<QString, QString> &inputValues)
{
    QString code;
    QTextStream out(&code);

    // Includes
    out << "#include <stdio.h>\n";
    out << "#include <stdlib.h>\n";

    for (const auto &inc : module.includes) {
        if (inc.startsWith('<') || inc.startsWith('"'))
            out << "#include " << inc << "\n";
        else
            out << "#include <" << inc << ">\n";
    }
    out << "\n";

    // Тело функции модуля
    out << module.sourceCode << "\n\n";

    // Тестовый main()
    out << "int main(void) {\n";

    // Подготовка аргументов
    QStringList callArgs;
    for (const auto &port : module.dataInputs()) {
        QString varName = "input_" + port.name;
        QString value = inputValues.value(port.name, port.defaultValue);
        if (value.isEmpty()) value = "0";

        if (port.type == "int" || port.type == "bool") {
            out << "    int " << varName << " = " << value << ";\n";
        } else if (port.type == "float") {
            out << "    float " << varName << " = " << value << "f;\n";
        } else if (port.type == "double") {
            out << "    double " << varName << " = " << value << ";\n";
        } else if (port.type == "string") {
            out << "    const char *" << varName << " = \"" << escapeCStringLiteral(value) << "\";\n";
        } else if (port.type == "pointer") {
            out << "    void *" << varName << " = NULL;\n";
        } else {
            out << "    int " << varName << " = " << value << ";\n";
        }
        callArgs.append(varName);
    }

    // Вызов функции
    QString funcName = QString("dq_%1").arg(module.name.toLower().replace(' ', '_'));
    const QVector<Port> dataOutputs = module.dataOutputs();

    if (!dataOutputs.isEmpty()) {
        const auto &outPort = dataOutputs.first();
        QString cType = "int";
        if (outPort.type == "float") cType = "float";
        else if (outPort.type == "double") cType = "double";
        else if (outPort.type == "string") cType = "const char*";
        else if (outPort.type == "pointer") cType = "void *";

        out << "    " << cType << " result = " << funcName
            << "(" << callArgs.join(", ") << ");\n";

        // Вывод результата в формате OUTPUT:port=value
        if (outPort.type == "int" || outPort.type == "bool")
            out << "    printf(\"OUTPUT:" << outPort.name << "=%d\\n\", result);\n";
        else if (outPort.type == "float")
            out << "    printf(\"OUTPUT:" << outPort.name << "=%f\\n\", result);\n";
        else if (outPort.type == "double")
            out << "    printf(\"OUTPUT:" << outPort.name << "=%lf\\n\", result);\n";
        else if (outPort.type == "string")
            out << "    printf(\"OUTPUT:" << outPort.name << "=%s\\n\", result);\n";
        else if (outPort.type == "pointer")
            out << "    printf(\"OUTPUT:" << outPort.name << "=%p\\n\", result);\n";
    } else {
        out << "    " << funcName << "(" << callArgs.join(", ") << ");\n";
    }

    out << "    return 0;\n";
    out << "}\n";

    return code;
}

// Запускает собранный verification binary внутри temp workspace сценария.
TestResult ModuleTestRunner::executeTest(const QString &binaryPath)
{
    TestResult result;
    result.compiled = true;

    QProcess proc;
    if (!m_tempDir.trimmed().isEmpty())
        proc.setWorkingDirectory(m_tempDir);
    proc.start(binaryPath, {});
    bool finished = proc.waitForFinished(5000); // 5 секунд таймаут

    if (!finished) {
        proc.kill();
        proc.waitForFinished(1000);
        result.errors.append(tr("Таймаут выполнения (5 сек)"));
        result.ran = false;
        return result;
    }

    result.ran = true;
    result.runOutput = proc.readAllStandardOutput();
    QString errOutput = proc.readAllStandardError();

    if (proc.exitCode() != 0) {
        result.errors.append(tr("Процесс завершился с кодом %1").arg(proc.exitCode()));
        if (!errOutput.isEmpty())
            result.errors.append(errOutput);
        result.passed = false;
        return result;
    }

    // Парсим вывод
    result.outputValues = parseOutput(result.runOutput);
    result.passed = true;
    return result;
}

// Разбирает формат `OUTPUT:port=value`, который генерирует verification harness.
QMap<QString, QString> ModuleTestRunner::parseOutput(const QString &output)
{
    QMap<QString, QString> values;
    for (const auto &line : output.split('\n')) {
        if (line.startsWith("OUTPUT:")) {
            QString data = line.mid(7).trimmed();
            int eq = data.indexOf('=');
            if (eq > 0) {
                values[data.left(eq)] = data.mid(eq + 1);
            }
        }
    }
    return values;
}

// Сравнивает фактические outputs со сценарием и переводит run-result в verify-result.
void ModuleTestRunner::applyExpectedOutputs(TestResult *result,
                                            const QMap<QString, QString> &expectedOutputValues)
{
    if (!result)
        return;
    if (!result->compiled || !result->ran)
        return;

    for (auto it = expectedOutputValues.begin(); it != expectedOutputValues.end(); ++it) {
        const QString expectedValue = it.value().trimmed();
        if (expectedValue.isEmpty())
            continue;

        if (!result->outputValues.contains(it.key())) {
            result->errors.append(tr("Не найден output '%1' для сравнения со сценарием").arg(it.key()));
            result->passed = false;
            continue;
        }

        const QString actualValue = result->outputValues.value(it.key()).trimmed();
        if (actualValue != expectedValue) {
            result->errors.append(
                tr("Output '%1': expected '%2', actual '%3'")
                    .arg(it.key(), expectedValue, actualValue));
            result->passed = false;
        }
    }
}

} // namespace DeltaQ
