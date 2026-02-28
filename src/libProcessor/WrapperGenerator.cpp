// Генератор C-обёрток для C++ классов — реализация
#include "WrapperGenerator.h"

#include <QTextStream>

namespace DeltaQ {

QString WrapperGenerator::sanitizeName(const QString &name)
{
    QString result;
    for (QChar c : name) {
        if (c.isLetterOrNumber() || c == '_')
            result += c;
        else
            result += '_';
    }
    return result;
}

WrapperCode WrapperGenerator::generateCppClassWrapper(const ClassDecl &cls,
                                                         const QString &originalHeader)
{
    WrapperCode code;
    code.className = cls.name;
    QString prefix = "dq_" + sanitizeName(cls.name).toLower();
    QString handleType = "DQ_" + cls.name;

    // --- Header ---
    {
        QString s;
        QTextStream out(&s);

        out << "// Автогенерация DeltaQ IDE — C-обёртка для " << cls.name << "\n";
        out << "#pragma once\n\n";
        out << "#ifdef __cplusplus\n";
        out << "extern \"C\" {\n";
        out << "#endif\n\n";

        out << "typedef void* " << handleType << ";\n\n";

        // create
        out << handleType << " " << prefix << "_create(";
        bool first = true;
        for (const auto &ctor : cls.constructors) {
            if (ctor.accessSpecifier != "public") continue;
            for (const auto &param : ctor.parameters) {
                if (!first) out << ", ";
                out << param.type << " " << param.name;
                first = false;
            }
            break;
        }
        if (first) out << "void";
        out << ");\n";

        // destroy
        out << "void " << prefix << "_destroy(" << handleType << " handle);\n\n";

        // Методы
        for (const auto &method : cls.methods) {
            if (method.accessSpecifier != "public") continue;

            out << method.returnType << " " << prefix << "_" << sanitizeName(method.name) << "(";

            if (!method.isStatic) {
                out << handleType << " handle";
                first = false;
            } else {
                first = true;
            }

            for (const auto &param : method.parameters) {
                if (!first) out << ", ";
                out << param.type << " " << param.name;
                first = false;
            }
            if (first) out << "void";
            out << ");\n";
        }

        out << "\n#ifdef __cplusplus\n";
        out << "}\n";
        out << "#endif\n";

        code.header = s;
    }

    // --- Source ---
    {
        QString s;
        QTextStream out(&s);

        out << "// Автогенерация DeltaQ IDE — C-обёртка для " << cls.name << "\n";
        out << "#include \"" << originalHeader << "\"\n";
        out << "#include \"" << prefix << ".h\"\n\n";

        out << "extern \"C\" {\n\n";

        // create
        out << handleType << " " << prefix << "_create(";
        bool first = true;
        QStringList ctorArgs;
        for (const auto &ctor : cls.constructors) {
            if (ctor.accessSpecifier != "public") continue;
            for (const auto &param : ctor.parameters) {
                if (!first) out << ", ";
                out << param.type << " " << param.name;
                ctorArgs.append(param.name);
                first = false;
            }
            break;
        }
        if (first) out << "void";
        out << ") {\n";
        out << "    return static_cast<" << handleType << ">(new " << cls.name << "(";
        out << ctorArgs.join(", ");
        out << "));\n";
        out << "}\n\n";

        // destroy
        out << "void " << prefix << "_destroy(" << handleType << " handle) {\n";
        out << "    delete static_cast<" << cls.name << " *>(handle);\n";
        out << "}\n\n";

        // Методы
        for (const auto &method : cls.methods) {
            if (method.accessSpecifier != "public") continue;

            out << method.returnType << " " << prefix << "_" << sanitizeName(method.name) << "(";

            if (!method.isStatic) {
                out << handleType << " handle";
                first = false;
            } else {
                first = true;
            }

            QStringList methodArgs;
            for (const auto &param : method.parameters) {
                if (!first) out << ", ";
                out << param.type << " " << param.name;
                methodArgs.append(param.name);
                first = false;
            }
            if (first) out << "void";
            out << ") {\n";

            if (!method.isStatic) {
                out << "    auto *obj = static_cast<" << cls.name << " *>(handle);\n";
                if (method.returnType != "void")
                    out << "    return obj->" << method.name << "(" << methodArgs.join(", ") << ");\n";
                else
                    out << "    obj->" << method.name << "(" << methodArgs.join(", ") << ");\n";
            } else {
                if (method.returnType != "void")
                    out << "    return " << cls.name << "::" << method.name << "(" << methodArgs.join(", ") << ");\n";
                else
                    out << "    " << cls.name << "::" << method.name << "(" << methodArgs.join(", ") << ");\n";
            }

            out << "}\n\n";
        }

        out << "} // extern \"C\"\n";

        code.source = s;
    }

    return code;
}

QVector<WrapperCode> WrapperGenerator::generateCWrapper(const QVector<ClassDecl> &classes,
                                                           const QString &originalHeader)
{
    QVector<WrapperCode> result;
    for (const auto &cls : classes)
        result.append(generateCppClassWrapper(cls, originalHeader));
    return result;
}

} // namespace DeltaQ
