// Парсер заголовочных файлов через libclang — реализация
#include "LibclangParser.h"

#ifdef DQ_HAS_LIBCLANG

#include <clang-c/Index.h>
#include <QFileInfo>

namespace DeltaQ {

LibclangParser::LibclangParser() = default;
LibclangParser::~LibclangParser() = default;

void LibclangParser::setIncludePaths(const QStringList &paths)
{
    m_includePaths = paths;
}

void LibclangParser::setDefines(const QStringList &defines)
{
    m_defines = defines;
}

void LibclangParser::setStandard(const QString &standard)
{
    m_standard = standard;
}

QString LibclangParser::clangTypeToDqType(const QString &clangType)
{
    QString t = clangType.trimmed();

    // Удаляем const/volatile
    t.remove("const ");
    t.remove("volatile ");
    t = t.trimmed();

    if (t == "void")                return "void";
    if (t == "_Bool" || t == "bool") return "bool";
    if (t == "char" || t == "signed char" || t == "unsigned char")
                                    return "int";
    if (t == "short" || t == "unsigned short")
                                    return "int";
    if (t == "int" || t == "unsigned int" || t == "long" || t == "unsigned long")
                                    return "int";
    if (t == "long long" || t == "unsigned long long")
                                    return "int";
    if (t == "float")               return "float";
    if (t == "double" || t == "long double")
                                    return "double";
    if (t == "const char *" || t == "char *")
                                    return "string";

    // Указатели
    if (t.endsWith('*'))            return "pointer";

    // Структуры
    if (t.startsWith("struct "))    return "struct:" + t.mid(7).trimmed();

    // Enum → int
    if (t.startsWith("enum "))      return "int";

    return t;
}

// Вспомогательные: извлечение строки из CXString
static QString cxStr(CXString s)
{
    const char *cstr = clang_getCString(s);
    QString result = cstr ? QString::fromUtf8(cstr) : QString();
    clang_disposeString(s);
    return result;
}

// Извлечение типа курсора как строки
static QString cursorTypeStr(CXCursor cursor)
{
    CXType type = clang_getCursorType(cursor);
    return cxStr(clang_getTypeSpelling(type));
}

// Извлечение типа результата функции
static QString resultTypeStr(CXCursor cursor)
{
    CXType type = clang_getCursorResultType(cursor);
    return cxStr(clang_getTypeSpelling(type));
}

// Парсинг параметра
static ParameterDecl parseParameter(CXCursor cursor)
{
    ParameterDecl p;
    p.name = cxStr(clang_getCursorSpelling(cursor));
    p.type = cursorTypeStr(cursor);
    return p;
}

// Парсинг функции
static FunctionDecl parseFunctionDecl(CXCursor cursor)
{
    FunctionDecl f;
    f.name = cxStr(clang_getCursorSpelling(cursor));
    f.returnType = resultTypeStr(cursor);
    f.isVariadic = clang_Cursor_isVariadic(cursor);
    f.comment = cxStr(clang_Cursor_getRawCommentText(cursor));

    int numArgs = clang_Cursor_getNumArguments(cursor);
    for (int i = 0; i < numArgs; ++i) {
        CXCursor arg = clang_Cursor_getArgument(cursor, i);
        f.parameters.append(parseParameter(arg));
    }

    return f;
}

// Парсинг структуры
static StructDecl parseStructDecl(CXCursor cursor)
{
    StructDecl s;
    s.name = cxStr(clang_getCursorSpelling(cursor));
    s.comment = cxStr(clang_Cursor_getRawCommentText(cursor));

    clang_visitChildren(cursor, [](CXCursor child, CXCursor, CXClientData data) -> CXChildVisitResult {
        auto *sd = static_cast<StructDecl *>(data);
        if (clang_getCursorKind(child) == CXCursor_FieldDecl) {
            FieldDecl field;
            field.name = cxStr(clang_getCursorSpelling(child));
            field.type = cursorTypeStr(child);
            field.comment = cxStr(clang_Cursor_getRawCommentText(child));
            sd->fields.append(field);
        }
        return CXChildVisit_Continue;
    }, &s);

    return s;
}

// Парсинг enum
static EnumDecl parseEnumDecl(CXCursor cursor)
{
    EnumDecl e;
    e.name = cxStr(clang_getCursorSpelling(cursor));
    e.comment = cxStr(clang_Cursor_getRawCommentText(cursor));

    clang_visitChildren(cursor, [](CXCursor child, CXCursor, CXClientData data) -> CXChildVisitResult {
        auto *ed = static_cast<EnumDecl *>(data);
        if (clang_getCursorKind(child) == CXCursor_EnumConstantDecl) {
            EnumConstant ec;
            ec.name = cxStr(clang_getCursorSpelling(child));
            ec.value = clang_getEnumConstantDeclValue(child);
            ed->constants.append(ec);
        }
        return CXChildVisit_Continue;
    }, &e);

    return e;
}

// Парсинг класса
static ClassDecl parseClassDecl(CXCursor cursor)
{
    ClassDecl c;
    c.name = cxStr(clang_getCursorSpelling(cursor));
    c.comment = cxStr(clang_Cursor_getRawCommentText(cursor));

    clang_visitChildren(cursor, [](CXCursor child, CXCursor, CXClientData data) -> CXChildVisitResult {
        auto *cd = static_cast<ClassDecl *>(data);
        CXCursorKind kind = clang_getCursorKind(child);

        QString access;
        CX_CXXAccessSpecifier spec = clang_getCXXAccessSpecifier(child);
        if (spec == CX_CXXPublic) access = "public";
        else if (spec == CX_CXXProtected) access = "protected";
        else if (spec == CX_CXXPrivate) access = "private";

        if (kind == CXCursor_Constructor) {
            ConstructorDecl ctor;
            ctor.accessSpecifier = access;
            int numArgs = clang_Cursor_getNumArguments(child);
            for (int i = 0; i < numArgs; ++i)
                ctor.parameters.append(parseParameter(clang_Cursor_getArgument(child, i)));
            cd->constructors.append(ctor);
        }
        else if (kind == CXCursor_CXXMethod) {
            MethodDecl m;
            m.name = cxStr(clang_getCursorSpelling(child));
            m.returnType = resultTypeStr(child);
            m.isConst = clang_CXXMethod_isConst(child);
            m.isStatic = clang_CXXMethod_isStatic(child);
            m.isVirtual = clang_CXXMethod_isVirtual(child);
            m.isPureVirtual = clang_CXXMethod_isPureVirtual(child);
            m.accessSpecifier = access;
            m.comment = cxStr(clang_Cursor_getRawCommentText(child));
            int numArgs = clang_Cursor_getNumArguments(child);
            for (int i = 0; i < numArgs; ++i)
                m.parameters.append(parseParameter(clang_Cursor_getArgument(child, i)));
            cd->methods.append(m);
        }
        else if (kind == CXCursor_FieldDecl) {
            FieldDecl field;
            field.name = cxStr(clang_getCursorSpelling(child));
            field.type = cursorTypeStr(child);
            field.comment = cxStr(clang_Cursor_getRawCommentText(child));
            cd->fields.append(field);
        }
        else if (kind == CXCursor_CXXBaseSpecifier) {
            cd->baseClasses.append(cxStr(clang_getCursorSpelling(child)));
        }

        return CXChildVisit_Continue;
    }, &c);

    return c;
}

// --- Основной метод парсинга ---

ParseResult LibclangParser::parseHeader(const QString &headerPath)
{
    ParseResult result;

    QFileInfo fi(headerPath);
    if (!fi.exists()) {
        result.errors.append(QString("File not found: %1").arg(headerPath));
        return result;
    }

    // Формируем аргументы для clang
    QVector<QByteArray> argStorage;
    QVector<const char *> args;

    // Стандарт
    QString stdArg = "-std=" + m_standard;
    argStorage.append(stdArg.toUtf8());
    args.append(argStorage.last().constData());

    // Include paths
    for (const auto &path : m_includePaths) {
        QByteArray inc = ("-I" + path).toUtf8();
        argStorage.append(inc);
        args.append(argStorage.last().constData());
    }

    // Defines
    for (const auto &def : m_defines) {
        QByteArray d = ("-D" + def).toUtf8();
        argStorage.append(d);
        args.append(argStorage.last().constData());
    }

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit tu = nullptr;

    QByteArray filePath = headerPath.toUtf8();
    CXErrorCode err = clang_parseTranslationUnit2(
        index, filePath.constData(),
        args.data(), args.size(),
        nullptr, 0,
        CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_DetailedPreprocessingRecord,
        &tu);

    if (err != CXError_Success || !tu) {
        result.errors.append(QString("Failed to parse: %1 (error code: %2)")
                             .arg(headerPath).arg(static_cast<int>(err)));
        if (tu) clang_disposeTranslationUnit(tu);
        clang_disposeIndex(index);
        return result;
    }

    // Собираем диагностику
    unsigned numDiags = clang_getNumDiagnostics(tu);
    for (unsigned i = 0; i < numDiags; ++i) {
        CXDiagnostic diag = clang_getDiagnostic(tu, i);
        CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diag);
        QString msg = cxStr(clang_formatDiagnostic(diag, CXDiagnostic_DisplaySourceLocation));

        if (severity >= CXDiagnostic_Error)
            result.errors.append(msg);
        else if (severity == CXDiagnostic_Warning)
            result.warnings.append(msg);

        clang_disposeDiagnostic(diag);
    }

    // Обход AST верхнего уровня
    CXCursor rootCursor = clang_getTranslationUnitCursor(tu);
    QString headerPathCopy = headerPath;

    struct VisitorData {
        ParseResult *result;
        QString headerPath;
    } visitorData = {&result, headerPath};

    clang_visitChildren(rootCursor, [](CXCursor cursor, CXCursor, CXClientData data) -> CXChildVisitResult {
        auto *vd = static_cast<VisitorData *>(data);

        // Обрабатываем только сущности из нашего файла
        CXSourceLocation loc = clang_getCursorLocation(cursor);
        if (clang_Location_isInSystemHeader(loc))
            return CXChildVisit_Continue;

        CXFile file;
        clang_getFileLocation(loc, &file, nullptr, nullptr, nullptr);
        QString filePath = cxStr(clang_getFileName(file));

        // Пропускаем если не наш файл
        if (!filePath.isEmpty() && filePath != vd->headerPath) {
            // Но можем включать из поддиректорий
            // Для простоты: пропускаем системные хедеры
        }

        CXCursorKind kind = clang_getCursorKind(cursor);

        switch (kind) {
        case CXCursor_FunctionDecl:
            vd->result->functions.append(parseFunctionDecl(cursor));
            break;
        case CXCursor_StructDecl:
            if (!clang_Cursor_isAnonymous(cursor))
                vd->result->structs.append(parseStructDecl(cursor));
            break;
        case CXCursor_EnumDecl:
            if (!clang_Cursor_isAnonymous(cursor))
                vd->result->enums.append(parseEnumDecl(cursor));
            break;
        case CXCursor_ClassDecl:
            if (!clang_Cursor_isAnonymous(cursor))
                vd->result->classes.append(parseClassDecl(cursor));
            break;
        case CXCursor_TypedefDecl: {
            TypedefDecl td;
            td.name = cxStr(clang_getCursorSpelling(cursor));
            CXType underlying = clang_getTypedefDeclUnderlyingType(cursor);
            td.underlyingType = cxStr(clang_getTypeSpelling(underlying));
            vd->result->typedefs.append(td);
            break;
        }
        default:
            break;
        }

        return CXChildVisit_Continue;
    }, &visitorData);

    result.success = result.errors.isEmpty();

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return result;
}

ParseResult LibclangParser::parseLibrary(const QStringList &headerPaths)
{
    ParseResult combined;
    combined.success = true;

    for (const auto &path : headerPaths) {
        ParseResult r = parseHeader(path);
        combined.functions.append(r.functions);
        combined.structs.append(r.structs);
        combined.enums.append(r.enums);
        combined.classes.append(r.classes);
        combined.typedefs.append(r.typedefs);
        combined.errors.append(r.errors);
        combined.warnings.append(r.warnings);
        if (!r.success)
            combined.success = false;
    }

    return combined;
}

} // namespace DeltaQ

#endif // DQ_HAS_LIBCLANG
