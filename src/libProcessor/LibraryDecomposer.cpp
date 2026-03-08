// Декомпозиция ParseResult в модули DeltaQ — реализация
#include "LibraryDecomposer.h"

#include <QRegularExpression>
#include <QUuid>

namespace DeltaQ {

namespace {

// Приводит сырые C/C++ типы из парсера к тем типам, которые понимает DeltaQ graph/codegen.
QString importedTypeToDeltaQ(const QString &rawType)
{
    QString type = rawType.trimmed();
    type.remove("const ");
    type.remove("volatile ");
    type = type.trimmed();

    if (type == "void")
        return "void";
    if (type == "_Bool" || type == "bool")
        return "bool";
    if (type == "char" || type == "signed char" || type == "unsigned char")
        return "int";
    if (type == "short" || type == "unsigned short")
        return "int";
    if (type == "int" || type == "unsigned int" || type == "long" || type == "unsigned long")
        return "int";
    if (type == "long long" || type == "unsigned long long")
        return "int";
    if (type == "float")
        return "float";
    if (type == "double" || type == "long double")
        return "double";
    if (type == "char *" || type == "const char *")
        return "string";
    if (type.endsWith('*'))
        return "pointer";
    if (type.startsWith("struct "))
        return "pointer";
    if (type.startsWith("enum "))
        return "int";
    return "pointer";
}

} // namespace

DecompositionResult LibraryDecomposer::decompose(const ParseResult &parseResult,
                                                   const DecompositionOptions &options)
{
    DecompositionResult result;

    // C-функции → модули
    for (const auto &func : parseResult.functions) {
        if (isExcluded(func.name, options.excludePatterns))
            continue;
        result.modules.append(functionToModule(func, options));
    }

    // C++ классы → модули (create + destroy + методы)
    for (const auto &cls : parseResult.classes) {
        if (isExcluded(cls.name, options.excludePatterns))
            continue;
        result.modules.append(classToModules(cls, options));
    }

    return result;
}

Module LibraryDecomposer::functionToModule(const FunctionDecl &func,
                                             const DecompositionOptions &options)
{
    Module m;
    m.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m.name = func.name;
    m.version = "1.0.0";
    m.language = options.language;
    m.description = func.comment.isEmpty() ?
        QString("Imported function: %1").arg(func.name) : func.comment;
    m.category = options.category;
    m.origin = "library";

    // Параметры → входные порты
    for (const auto &param : func.parameters) {
        Port p;
        p.name = param.name.isEmpty() ? QString("arg%1").arg(m.inputs.size()) : param.name;
        p.type = importedTypeToDeltaQ(param.type);
        if (!param.defaultValue.isEmpty())
            p.defaultValue = param.defaultValue;
        m.inputs.append(p);
    }

    // Возвращаемый тип → выходной порт
    const QString returnType = importedTypeToDeltaQ(func.returnType);
    if (returnType != "void") {
        Port p;
        p.name = "result";
        p.type = returnType;
        m.outputs.append(p);
    }

    return m;
}

QVector<Module> LibraryDecomposer::classToModules(const ClassDecl &cls,
                                                    const DecompositionOptions &options)
{
    QVector<Module> modules;
    QString clsLower = cls.name.toLower();

    // 1. Create — конструктор
    {
        Module m;
        m.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m.name = cls.name + "_create";
        m.version = "1.0.0";
        m.language = options.language;
        m.description = QString("Create %1 instance").arg(cls.name);
        m.category = options.category;
        m.origin = "library";

        // Параметры первого публичного конструктора
        for (const auto &ctor : cls.constructors) {
            if (ctor.accessSpecifier != "public") continue;
            for (const auto &param : ctor.parameters) {
                Port p;
                p.name = param.name.isEmpty() ?
                    QString("arg%1").arg(m.inputs.size()) : param.name;
                p.type = importedTypeToDeltaQ(param.type);
                m.inputs.append(p);
            }
            break; // Берём первый публичный конструктор
        }

        // Выход: instance (pointer)
        Port outPort;
        outPort.name = "instance";
        outPort.type = "pointer";
        m.outputs.append(outPort);

        modules.append(m);
    }

    // 2. Destroy — деструктор
    {
        Module m;
        m.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m.name = cls.name + "_destroy";
        m.version = "1.0.0";
        m.language = options.language;
        m.description = QString("Destroy %1 instance").arg(cls.name);
        m.category = options.category;
        m.origin = "library";

        Port inPort;
        inPort.name = "instance";
        inPort.type = "pointer";
        m.inputs.append(inPort);

        modules.append(m);
    }

    // 3. Методы
    for (const auto &method : cls.methods) {
        if (method.accessSpecifier != "public" && !options.includePrivateMethods)
            continue;
        if (isExcluded(method.name, options.excludePatterns))
            continue;

        Module m;
        m.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m.name = cls.name + "_" + method.name;
        m.version = "1.0.0";
        m.language = options.language;
        m.description = method.comment.isEmpty() ?
            QString("Method %1::%2").arg(cls.name, method.name) : method.comment;
        m.category = options.category;
        m.origin = "library";

        // Первый вход: instance (pointer) — кроме статических методов
        if (!method.isStatic) {
            Port instanceIn;
            instanceIn.name = "instance";
            instanceIn.type = "pointer";
            m.inputs.append(instanceIn);
        }

        // Параметры метода → входные порты
        for (const auto &param : method.parameters) {
            Port p;
            p.name = param.name.isEmpty() ?
                QString("arg%1").arg(m.inputs.size()) : param.name;
            p.type = importedTypeToDeltaQ(param.type);
            m.inputs.append(p);
        }

        // Возвращаемый тип → выход
        const QString methodReturnType = importedTypeToDeltaQ(method.returnType);
        if (methodReturnType != "void") {
            Port resultPort;
            resultPort.name = "result";
            resultPort.type = methodReturnType;
            m.outputs.append(resultPort);
        }

        // Для нестатических методов: instance_out для chaining
        if (!method.isStatic) {
            Port instanceOut;
            instanceOut.name = "instance_out";
            instanceOut.type = "pointer";
            m.outputs.append(instanceOut);
        }

        modules.append(m);
    }

    return modules;
}

bool LibraryDecomposer::isExcluded(const QString &name, const QStringList &patterns)
{
    for (const auto &pattern : patterns) {
        QRegularExpression re(pattern);
        if (re.match(name).hasMatch())
            return true;
    }
    return false;
}

} // namespace DeltaQ
