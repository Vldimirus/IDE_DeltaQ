// Промежуточное представление — реализация
#include "IR.h"

namespace DeltaQ {

// --- IRInstruction фабричные методы ---

IRInstruction IRInstruction::makeCall(const QString &target, const QString &func,
                                      const QStringList &args, const QString &nodeId)
{
    IRInstruction i;
    i.type = Call;
    i.target = target;
    i.function = func;
    i.args = args;
    i.sourceNodeId = nodeId;
    return i;
}

IRInstruction IRInstruction::makeAssign(const QString &target, const QString &value,
                                        const QString &nodeId)
{
    IRInstruction i;
    i.type = Assign;
    i.target = target;
    i.args = {value};
    i.sourceNodeId = nodeId;
    return i;
}

IRInstruction IRInstruction::makeTypeConvert(const QString &target, const QString &fromVar,
                                              const QString &toType, const QString &nodeId)
{
    IRInstruction i;
    i.type = TypeConvert;
    i.target = target;
    i.args = {fromVar};
    i.varType = toType;
    i.sourceNodeId = nodeId;
    return i;
}

IRInstruction IRInstruction::makeDeclareVar(const QString &name, const QString &type,
                                             const QString &nodeId)
{
    IRInstruction i;
    i.type = DeclareVar;
    i.target = name;
    i.varType = type;
    i.sourceNodeId = nodeId;
    return i;
}

IRInstruction IRInstruction::makeComment(const QString &text)
{
    IRInstruction i;
    i.type = Comment;
    i.comment = text;
    return i;
}

IRInstruction IRInstruction::makeReturn(const QString &value)
{
    IRInstruction i;
    i.type = Return;
    if (!value.isEmpty())
        i.args = {value};
    return i;
}

// --- IR ---

void IR::addInclude(const QString &header)
{
    if (!includes.contains(header))
        includes.append(header);
}

void IR::addInstruction(const IRInstruction &instr)
{
    instructions.append(instr);
}

void IR::declareVariable(const QString &name, const QString &type)
{
    variables[name] = type;
}

QString IR::emitCCode() const
{
    QString code;

    // Секция 1: Includes (дедуплицированные)
    for (const auto &inc : includes)
        code += QString("#include %1\n").arg(inc);
    if (!includes.isEmpty())
        code += "\n";

    // Секция 2: Определения модулей (каждый ровно один раз)
    if (!moduleSources.isEmpty()) {
        for (const auto &src : moduleSources) {
            code += src + "\n\n";
        }
    }

    // Секция 3: main()
    code += "int main(void) {\n";

    // Инструкции
    int lineNum = code.count('\n') + 1;
    Q_UNUSED(lineNum)

    for (const auto &instr : instructions) {
        switch (instr.type) {
        case IRInstruction::DeclareVar:
            code += QString("    %1 %2;\n").arg(instr.varType, instr.target);
            break;

        case IRInstruction::Call:
            if (instr.target.isEmpty())
                code += QString("    %1(%2);\n").arg(instr.function, instr.args.join(", "));
            else
                code += QString("    %1 = %2(%3);\n").arg(instr.target, instr.function, instr.args.join(", "));
            break;

        case IRInstruction::Assign:
            code += QString("    %1 = %2;\n").arg(instr.target, instr.args.value(0));
            break;

        case IRInstruction::TypeConvert:
            code += QString("    %1 = (%2)%3;\n").arg(instr.target, instr.varType, instr.args.value(0));
            break;

        case IRInstruction::Comment:
            code += QString("    // %1\n").arg(instr.comment);
            break;

        case IRInstruction::Return:
            if (instr.args.isEmpty())
                code += "    return 0;\n";
            else
                code += QString("    return %1;\n").arg(instr.args.value(0));
            break;

        default:
            break;
        }
    }

    code += "    return 0;\n";
    code += "}\n";
    return code;
}

} // namespace DeltaQ
