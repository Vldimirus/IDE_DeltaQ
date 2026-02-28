// Парсер заголовочных файлов через libclang
#pragma once

#include "LibclangTypes.h"
#include <QStringList>

namespace DeltaQ {

// Результат парсинга
struct ParseResult {
    QVector<FunctionDecl> functions;
    QVector<StructDecl> structs;
    QVector<EnumDecl> enums;
    QVector<ClassDecl> classes;
    QVector<TypedefDecl> typedefs;
    QStringList errors;
    QStringList warnings;
    bool success = false;
};

#ifdef DQ_HAS_LIBCLANG

class LibclangParser {
public:
    LibclangParser();
    ~LibclangParser();

    // Парсинг одного заголовочного файла
    ParseResult parseHeader(const QString &headerPath);

    // Парсинг нескольких заголовков (библиотека)
    ParseResult parseLibrary(const QStringList &headerPaths);

    // Настройки
    void setIncludePaths(const QStringList &paths);
    void setDefines(const QStringList &defines);
    void setStandard(const QString &standard); // "c11", "c17", "c++17", "c++20"

private:
    // Маппинг типов libclang → DeltaQ
    static QString clangTypeToDqType(const QString &clangType);

    QStringList m_includePaths;
    QStringList m_defines;
    QString m_standard = "c17";
};

#else // !DQ_HAS_LIBCLANG

// Заглушка без libclang
class LibclangParser {
public:
    LibclangParser() = default;
    ~LibclangParser() = default;

    ParseResult parseHeader(const QString &) {
        ParseResult r;
        r.errors.append("libclang not available");
        return r;
    }

    ParseResult parseLibrary(const QStringList &) {
        ParseResult r;
        r.errors.append("libclang not available");
        return r;
    }

    void setIncludePaths(const QStringList &) {}
    void setDefines(const QStringList &) {}
    void setStandard(const QString &) {}
};

#endif // DQ_HAS_LIBCLANG

} // namespace DeltaQ
