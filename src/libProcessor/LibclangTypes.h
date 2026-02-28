// Типы для парсинга C/C++ заголовков через libclang
#pragma once

#include <QString>
#include <QVector>

namespace DeltaQ {

// Параметр функции
struct ParameterDecl {
    QString name;
    QString type;
    QString defaultValue;
};

// Объявление функции
struct FunctionDecl {
    QString name;
    QString returnType;
    QVector<ParameterDecl> parameters;
    bool isStatic = false;
    bool isVariadic = false;
    QString comment;
};

// Поле структуры/класса
struct FieldDecl {
    QString name;
    QString type;
    QString comment;
};

// Объявление структуры
struct StructDecl {
    QString name;
    QVector<FieldDecl> fields;
    QString comment;
};

// Константа перечисления
struct EnumConstant {
    QString name;
    long long value = 0;
};

// Объявление перечисления
struct EnumDecl {
    QString name;
    QVector<EnumConstant> constants;
    QString comment;
};

// Метод класса
struct MethodDecl {
    QString name;
    QString returnType;
    QVector<ParameterDecl> parameters;
    bool isConst = false;
    bool isStatic = false;
    bool isVirtual = false;
    bool isPureVirtual = false;
    QString accessSpecifier; // "public", "protected", "private"
    QString comment;
};

// Конструктор класса
struct ConstructorDecl {
    QVector<ParameterDecl> parameters;
    bool isExplicit = false;
    QString accessSpecifier;
};

// Объявление класса
struct ClassDecl {
    QString name;
    QVector<ConstructorDecl> constructors;
    QVector<MethodDecl> methods;
    QVector<FieldDecl> fields;
    QStringList baseClasses;
    QString comment;
};

// Typedef
struct TypedefDecl {
    QString name;
    QString underlyingType;
};

} // namespace DeltaQ
