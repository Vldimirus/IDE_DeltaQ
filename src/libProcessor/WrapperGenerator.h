// Генератор C-обёрток для C++ классов
#pragma once

#include "LibclangTypes.h"
#include <QString>

namespace DeltaQ {

// Результат генерации обёртки
struct WrapperCode {
    QString header;     // .h файл
    QString source;     // .cpp файл
    QString className;
};

class WrapperGenerator {
public:
    // Генерация C-обёртки для одного класса
    static WrapperCode generateCppClassWrapper(const ClassDecl &cls,
                                                 const QString &originalHeader);

    // Генерация C-обёртки для набора классов
    static QVector<WrapperCode> generateCWrapper(const QVector<ClassDecl> &classes,
                                                   const QString &originalHeader);

private:
    static QString sanitizeName(const QString &name);
};

} // namespace DeltaQ
