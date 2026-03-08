// Компиляция и тестирование модулей — реализация
#include "ModuleTestRunner.h"
#include <deltaq/Module.h>

#include <QDir>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>

namespace DeltaQ {

ModuleTestRunner::ModuleTestRunner(QObject *parent)
    : QObject(parent)
{
}

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
    args << "-fsyntax-only" << "-Wall" << "-Wextra" << sourceFile;

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

TestResult ModuleTestRunner::runTest(const Module &module,
                                      const QMap<QString, QString> &inputValues)
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

    emit testFinished(result);
    return result;
}

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
    for (const auto &port : module.inputs) {
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
            out << "    const char *" << varName << " = \"" << value << "\";\n";
        } else if (port.type == "pointer") {
            out << "    void *" << varName << " = NULL;\n";
        } else {
            out << "    int " << varName << " = " << value << ";\n";
        }
        callArgs.append(varName);
    }

    // Вызов функции
    QString funcName = QString("dq_%1").arg(module.name.toLower().replace(' ', '_'));

    if (!module.outputs.isEmpty()) {
        const auto &outPort = module.outputs.first();
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

TestResult ModuleTestRunner::executeTest(const QString &binaryPath)
{
    TestResult result;
    result.compiled = true;

    QProcess proc;
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

} // namespace DeltaQ
