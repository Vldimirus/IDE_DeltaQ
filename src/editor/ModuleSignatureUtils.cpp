#include "ModuleSignatureUtils.h"

#include <deltaq/Module.h>

#include <QRegularExpression>

namespace DeltaQ {

namespace {

// Нормализует распространённые C-типы к словарю типов DeltaQ-портов.
QString mapCTypeToPortType(const QString &type)
{
    if (type == "int" || type == "long")
        return "int";
    if (type == "float")
        return "float";
    if (type == "double")
        return "double";
    if (type == "const char*" || type == "char*")
        return "string";
    return "int";
}

// Обратный маппинг: превращает тип порта обратно в C-сигнатуру функции.
QString mapPortTypeToCType(const QString &type)
{
    if (type == "int")
        return "int";
    if (type == "float")
        return "float";
    if (type == "double")
        return "double";
    if (type == "string")
        return "const char*";
    if (type == "bool")
        return "int";
    return "int";
}

// Функция в исходнике должна оставаться валидным C-идентификатором даже после UI-правок.
QString sanitizeIdentifier(QString value, const QString &fallback)
{
    value = value.trimmed();
    if (value.isEmpty())
        value = fallback;

    for (QChar &ch : value) {
        if (!(ch.isLetterOrNumber() || ch == QChar('_')))
            ch = QChar('_');
    }

    if (value.isEmpty())
        value = fallback;
    if (!value.isEmpty() && value.front().isDigit())
        value.prepend('_');
    return value;
}

} // namespace

// Извлекает data-входы и data-выходы из первой найденной `dq_*` C-сигнатуры,
// сохраняя уже существующие execution-порты графового контракта.
bool parseModuleSignatureIntoContract(Module *module, const QString &code)
{
    if (!module)
        return false;

    // Для v1 authoring path поддерживаем только прямую atomic `dq_*` сигнатуру.
    const QRegularExpression signatureRegex(R"((\w+)\s+dq_(\w+)\s*\(([^)]*)\))");
    const auto match = signatureRegex.match(code);
    if (!match.hasMatch())
        return false;

    const QString returnType = match.captured(1);
    const QString argsString = match.captured(3);

    QVector<Port> execInputs;
    for (const auto &port : module->inputs) {
        if (port.kind == PortKind::Execution)
            execInputs.append(port);
    }

    QVector<Port> execOutputs;
    for (const auto &port : module->outputs) {
        if (port.kind == PortKind::Execution)
            execOutputs.append(port);
    }

    module->inputs = execInputs;
    module->outputs = execOutputs;

    if (!argsString.trimmed().isEmpty() && argsString.trimmed() != "void") {
        const QStringList args = argsString.split(',');
        for (const auto &arg : args) {
            const QString trimmedArg = arg.trimmed();
            const int lastSpace = trimmedArg.lastIndexOf(' ');
            if (lastSpace <= 0)
                continue;

            const QString type = trimmedArg.left(lastSpace).trimmed();
            QString name = trimmedArg.mid(lastSpace + 1).trimmed();
            name.remove('*');
            name.remove('&');

            Port inputPort;
            inputPort.name = name;
            inputPort.type = mapCTypeToPortType(type);
            inputPort.kind = PortKind::Data;
            module->inputs.append(inputPort);
        }
    }

    if (returnType != "void") {
        Port outputPort;
        outputPort.name = "result";
        outputPort.type = mapCTypeToPortType(returnType);
        outputPort.kind = PortKind::Data;
        module->outputs.append(outputPort);
    }

    return true;
}

// Собирает каноническую `dq_*`-сигнатуру только по data-портам:
// execution-flow остаётся частью graph-контракта и не выражается в C-аргументах.
QString buildModuleSignatureFromContract(const Module &module, QString *errorMessage)
{
    const QVector<Port> dataInputs = module.dataInputs();
    const QVector<Port> dataOutputs = module.dataOutputs();

    // Atomic source-backed module пока выражает только один data-output через return value.
    if (dataOutputs.size() > 1) {
        if (errorMessage) {
            *errorMessage = QObject::tr(
                "Atomic source signature can expose at most one data output via return value.");
        }
        return {};
    }

    const QString functionName = QStringLiteral("dq_%1")
        .arg(sanitizeIdentifier(module.name, QStringLiteral("module")));

    QString returnType = "void";
    if (!dataOutputs.isEmpty())
        returnType = mapPortTypeToCType(dataOutputs.first().type.trimmed());

    QStringList arguments;
    for (int i = 0; i < dataInputs.size(); ++i) {
        const Port &port = dataInputs.at(i);
        const QString argumentName = sanitizeIdentifier(port.name, QStringLiteral("arg_%1").arg(i + 1));
        arguments.append(QStringLiteral("%1 %2")
                             .arg(mapPortTypeToCType(port.type.trimmed()), argumentName));
    }

    if (arguments.isEmpty())
        arguments.append("void");

    if (errorMessage)
        errorMessage->clear();
    return QStringLiteral("%1 %2(%3)")
        .arg(returnType, functionName, arguments.join(", "));
}

// Заменяет первую `dq_*` сигнатуру в тексте исходника, не трогая тело функции.
bool rewriteModuleSignatureFromContract(QString *code,
                                        const Module &module,
                                        QString *errorMessage)
{
    if (!code) {
        if (errorMessage)
            *errorMessage = QObject::tr("Source code buffer is null.");
        return false;
    }

    const QString signature = buildModuleSignatureFromContract(module, errorMessage);
    if (signature.isEmpty())
        return false;

    // Переписываем только первую `dq_*` сигнатуру и оставляем тело функции нетронутым.
    static const QRegularExpression signatureRegex(R"((\w+)\s+dq_(\w+)\s*\(([^)]*)\))");
    const QRegularExpressionMatch match = signatureRegex.match(*code);
    if (!match.hasMatch()) {
        if (errorMessage) {
            *errorMessage = QObject::tr(
                "Current source does not expose a parseable `dq_*` signature for rewrite.");
        }
        return false;
    }

    code->replace(match.capturedStart(), match.capturedLength(), signature);
    if (errorMessage)
        errorMessage->clear();
    return true;
}

} // namespace DeltaQ
