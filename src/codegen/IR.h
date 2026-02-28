// Промежуточное представление (Intermediate Representation)
// Используется как мост между графовым представлением и генерацией кода
#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>

namespace DeltaQ {

struct IRInstruction {
    enum Type {
        Call,         // Вызов функции: target = function(args)
        Assign,       // Присваивание: target = args[0]
        TypeConvert,  // Приведение типа: target = (type)args[0]
        DeclareVar,   // Объявление переменной: type target;
        Comment,      // Комментарий
        Return,       // return args[0]
        Branch,       // Ветвление (для будущего)
        Label,        // Метка (для будущего)
        Goto          // Переход (для будущего)
    };

    Type type = Comment;
    QString target;        // Переменная-цель
    QString function;      // Имя функции (для Call)
    QStringList args;      // Аргументы
    QString varType;       // Тип (для DeclareVar, TypeConvert)
    QString comment;       // Текст комментария
    QString sourceNodeId;  // ID узла графа (для sourceMap)

    // Фабричные методы
    static IRInstruction makeCall(const QString &target, const QString &func,
                                  const QStringList &args, const QString &nodeId = {});
    static IRInstruction makeAssign(const QString &target, const QString &value,
                                    const QString &nodeId = {});
    static IRInstruction makeTypeConvert(const QString &target, const QString &fromVar,
                                         const QString &toType, const QString &nodeId = {});
    static IRInstruction makeDeclareVar(const QString &name, const QString &type,
                                        const QString &nodeId = {});
    static IRInstruction makeComment(const QString &text);
    static IRInstruction makeReturn(const QString &value = {});
};

struct IR {
    QStringList includes;                    // #include директивы
    QVector<IRInstruction> instructions;     // Список инструкций
    QMap<QString, QString> variables;        // имя → тип

    void addInclude(const QString &header);
    void addInstruction(const IRInstruction &instr);
    void declareVariable(const QString &name, const QString &type);

    // Генерация C-кода
    QString emitCCode() const;
};

} // namespace DeltaQ
