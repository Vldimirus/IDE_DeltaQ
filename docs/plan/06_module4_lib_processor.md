# DeltaQ IDE - Модуль 4: Обработчик библиотек (Library Processor)

## Обзор

Library Processor -- инструмент для автоматического импорта существующих C/C++ библиотек в экосистему DeltaQ IDE. Модуль использует libclang для парсинга заголовочных файлов, анализа AST (Abstract Syntax Tree) и автоматической генерации .dqmod модулей из найденных функций, структур и классов.

---

## Архитектура модуля

```
┌────────────────────────────────────────────────────────┐
│                 Library Processor Module                 │
│                                                         │
│  ┌──────────────┐  ┌───────────────┐  ┌─────────────┐ │
│  │   Library    │  │  AST Viewer   │  │  Module     │ │
│  │   Browser    │  │  (отладка)    │  │  Preview    │ │
│  └──────┬───────┘  └───────┬───────┘  └──────┬──────┘ │
│         │                  │                  │         │
│  ┌──────┴──────────────────┴──────────────────┴──────┐ │
│  │              LibraryProcessorWidget                │ │
│  └────────────────────────┬──────────────────────────┘ │
│                           │                             │
│  ┌────────────────────────┴──────────────────────────┐ │
│  │              Processing Pipeline                   │ │
│  │  ┌──────────┐  ┌──────────────┐  ┌─────────────┐ │ │
│  │  │ libclang │  │  Decomposer  │  │  Wrapper    │ │ │
│  │  │ Parser   │  │              │  │  Generator  │ │ │
│  │  └──────────┘  └──────────────┘  └─────────────┘ │ │
│  └───────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────┘
```

---

## Парсинг AST через libclang

### LibclangParser

```cpp
class LibclangParser {
public:
    struct ParseResult {
        bool success;
        QList<FunctionDecl> functions;
        QList<StructDecl> structs;
        QList<EnumDecl> enums;
        QList<TypedefDecl> typedefs;
        QList<ClassDecl> classes;        // C++
        QList<NamespaceDecl> namespaces;  // C++
        QList<TemplateDecl> templates;    // C++
        QStringList errors;
        QStringList warnings;
    };

    LibclangParser();
    ~LibclangParser();

    // Парсинг заголовочного файла
    ParseResult parseHeader(const QString& headerPath,
                            const QStringList& includePaths = {},
                            const QStringList& defines = {},
                            const QString& standard = "c17");

    // Парсинг всех заголовков библиотеки
    ParseResult parseLibrary(const QString& libraryPath,
                             const QStringList& headerPatterns = {"*.h", "*.hpp"});

    // Настройки
    void setIncludePaths(const QStringList& paths);
    void addIncludePath(const QString& path);
    void setDefines(const QStringList& defines);
    void setLanguageStandard(const QString& standard);

private:
    void visitNode(CXCursor cursor, ParseResult& result);
    FunctionDecl parseFunctionDecl(CXCursor cursor);
    StructDecl parseStructDecl(CXCursor cursor);
    EnumDecl parseEnumDecl(CXCursor cursor);
    ClassDecl parseClassDecl(CXCursor cursor);
    TemplateDecl parseTemplateDecl(CXCursor cursor);

    QString clangTypeToString(CXType type) const;
    QString clangTypeToDqType(CXType type) const;

    CXIndex m_index;
    QStringList m_includePaths;
    QStringList m_defines;
    QString m_standard;
};
```

### Использование libclang API

```cpp
ParseResult LibclangParser::parseHeader(const QString& headerPath,
                                         const QStringList& includePaths,
                                         const QStringList& defines,
                                         const QString& standard) {
    ParseResult result;
    result.success = true;

    // Формирование аргументов компилятора
    QList<QByteArray> argStorage;
    QVector<const char*> args;

    // Стандарт
    QString stdArg = QString("-std=%1").arg(standard);
    argStorage << stdArg.toUtf8();
    args << argStorage.last().constData();

    // Include-пути
    for (const auto& path : includePaths) {
        QString incArg = QString("-I%1").arg(path);
        argStorage << incArg.toUtf8();
        args << argStorage.last().constData();
    }

    // Определения
    for (const auto& def : defines) {
        QString defArg = QString("-D%1").arg(def);
        argStorage << defArg.toUtf8();
        args << argStorage.last().constData();
    }

    // Создание единицы трансляции
    CXTranslationUnit tu;
    CXErrorCode err = clang_parseTranslationUnit2(
        m_index,
        headerPath.toUtf8().constData(),
        args.constData(), args.size(),
        nullptr, 0,
        CXTranslationUnit_DetailedPreprocessingRecord |
        CXTranslationUnit_SkipFunctionBodies,
        &tu
    );

    if (err != CXError_Success) {
        result.success = false;
        result.errors << QString("Ошибка парсинга: код %1").arg((int)err);
        return result;
    }

    // Проверка диагностик
    unsigned numDiags = clang_getNumDiagnostics(tu);
    for (unsigned i = 0; i < numDiags; i++) {
        CXDiagnostic diag = clang_getDiagnostic(tu, i);
        CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diag);
        CXString message = clang_getDiagnosticSpelling(diag);

        QString msg = QString::fromUtf8(clang_getCString(message));
        if (severity >= CXDiagnostic_Error) {
            result.errors << msg;
        } else if (severity == CXDiagnostic_Warning) {
            result.warnings << msg;
        }

        clang_disposeString(message);
        clang_disposeDiagnostic(diag);
    }

    // Обход AST
    CXCursor rootCursor = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(rootCursor,
        [](CXCursor cursor, CXCursor parent, CXClientData clientData) -> CXChildVisitResult {
            auto* parser = static_cast<LibclangParser*>(clientData);
            auto* result = parser->m_currentResult;

            // Пропускаем элементы из системных заголовков
            CXSourceLocation loc = clang_getCursorLocation(cursor);
            if (clang_Location_isInSystemHeader(loc)) {
                return CXChildVisit_Continue;
            }

            CXCursorKind kind = clang_getCursorKind(cursor);

            switch (kind) {
            case CXCursor_FunctionDecl:
                result->functions << parser->parseFunctionDecl(cursor);
                break;
            case CXCursor_StructDecl:
                result->structs << parser->parseStructDecl(cursor);
                break;
            case CXCursor_EnumDecl:
                result->enums << parser->parseEnumDecl(cursor);
                break;
            case CXCursor_ClassDecl:
                result->classes << parser->parseClassDecl(cursor);
                break;
            case CXCursor_ClassTemplate:
                result->templates << parser->parseTemplateDecl(cursor);
                break;
            case CXCursor_Namespace:
                return CXChildVisit_Recurse;  // Заходим внутрь namespace
            case CXCursor_TypedefDecl:
                result->typedefs << parser->parseTypedefDecl(cursor);
                break;
            default:
                break;
            }

            return CXChildVisit_Continue;
        },
        this
    );

    clang_disposeTranslationUnit(tu);
    return result;
}
```

### Парсинг объявлений функций

```cpp
FunctionDecl LibclangParser::parseFunctionDecl(CXCursor cursor) {
    FunctionDecl func;

    // Имя функции
    CXString name = clang_getCursorSpelling(cursor);
    func.name = QString::fromUtf8(clang_getCString(name));
    clang_disposeString(name);

    // Тип возвращаемого значения
    CXType returnType = clang_getCursorResultType(cursor);
    func.returnType = clangTypeToString(returnType);
    func.dqReturnType = clangTypeToDqType(returnType);

    // Параметры
    int numArgs = clang_Cursor_getNumArguments(cursor);
    for (int i = 0; i < numArgs; i++) {
        CXCursor argCursor = clang_Cursor_getArgument(cursor, i);

        ParameterDecl param;
        CXString argName = clang_getCursorSpelling(argCursor);
        param.name = QString::fromUtf8(clang_getCString(argName));
        clang_disposeString(argName);

        if (param.name.isEmpty()) {
            param.name = QString("arg%1").arg(i);
        }

        CXType argType = clang_getCursorType(argCursor);
        param.cType = clangTypeToString(argType);
        param.dqType = clangTypeToDqType(argType);

        func.parameters << param;
    }

    // Документация (Doxygen-комментарий)
    CXString comment = clang_Cursor_getBriefCommentText(cursor);
    func.description = QString::fromUtf8(clang_getCString(comment));
    clang_disposeString(comment);

    // Linkage (static, extern, etc.)
    func.linkage = clang_getCursorLinkage(cursor);

    // Расположение в файле
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    CXFile file;
    unsigned line, column;
    clang_getFileLocation(loc, &file, &line, &column, nullptr);
    CXString fileName = clang_getFileName(file);
    func.file = QString::fromUtf8(clang_getCString(fileName));
    func.line = line;
    clang_disposeString(fileName);

    return func;
}
```

### Маппинг типов C -> DeltaQ

```cpp
QString LibclangParser::clangTypeToDqType(CXType type) const {
    CXTypeKind kind = type.kind;

    switch (kind) {
    case CXType_Void:     return "void";
    case CXType_Bool:     return "bool";
    case CXType_Char_S:
    case CXType_Char_U:
    case CXType_SChar:    return "int8";
    case CXType_UChar:    return "uint8";
    case CXType_Short:    return "int16";
    case CXType_UShort:   return "uint16";
    case CXType_Int:      return "int";
    case CXType_UInt:     return "uint";
    case CXType_Long:     return "int32";
    case CXType_ULong:    return "uint32";
    case CXType_LongLong: return "int64";
    case CXType_ULongLong:return "uint64";
    case CXType_Float:    return "float";
    case CXType_Double:   return "double";

    case CXType_Pointer: {
        CXType pointee = clang_getPointeeType(type);
        // const char* -> string
        if (pointee.kind == CXType_Char_S || pointee.kind == CXType_Char_U) {
            if (clang_isConstQualifiedType(pointee)) {
                return "string";
            }
        }
        // void* -> pointer
        if (pointee.kind == CXType_Void) {
            return "pointer";
        }
        // T* -> pointer (с метаданными о типе)
        return "pointer";
    }

    case CXType_Record: {
        CXString name = clang_getTypeSpelling(type);
        QString typeName = QString::fromUtf8(clang_getCString(name));
        clang_disposeString(name);
        return QString("struct:%1").arg(typeName);
    }

    case CXType_Enum: {
        return "int";  // Enum -> int
    }

    default:
        return "pointer";  // Неизвестные типы -- как pointer
    }
}
```

---

## Декомпозиция в .dqmod

### Decomposer

```cpp
class LibraryDecomposer {
public:
    struct DecompositionOptions {
        bool includeStaticFunctions = false;
        bool includeInlineFunctions = true;
        bool generateWrappers = true;
        bool flattenClasses = true;         // Методы классов -> отдельные модули
        bool includeConstructors = true;
        bool includeDestructors = true;
        QString modulePrefix = "";          // Префикс для имён модулей
        QString category = "imported";      // Категория модулей
        QStringList excludePatterns = {};   // Паттерны имён для исключения
    };

    struct DecompositionResult {
        QList<ModuleDefinition> modules;
        QList<WrapperCode> wrappers;
        QStringList skipped;   // Пропущенные функции (с причиной)
    };

    DecompositionResult decompose(const LibclangParser::ParseResult& parseResult,
                                   const DecompositionOptions& options = {});

private:
    ModuleDefinition functionToModule(const FunctionDecl& func,
                                       const DecompositionOptions& options);
    QList<ModuleDefinition> classToModules(const ClassDecl& cls,
                                            const DecompositionOptions& options);
    ModuleDefinition methodToModule(const ClassDecl& cls,
                                      const MethodDecl& method,
                                      const DecompositionOptions& options);
};
```

### Преобразование функции в модуль

```cpp
ModuleDefinition LibraryDecomposer::functionToModule(
        const FunctionDecl& func, const DecompositionOptions& options) {
    ModuleDefinition mod;

    mod.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    mod.name = options.modulePrefix.isEmpty()
               ? func.name
               : options.modulePrefix + "_" + func.name;
    mod.version = "1.0.0";
    mod.language = "c";
    mod.category = options.category;
    mod.description = func.description;

    // Входные порты из параметров
    for (const auto& param : func.parameters) {
        PortDefinition port;
        port.name = param.name;
        port.type = param.dqType;
        port.direction = PortDefinition::Input;
        mod.ports << port;
    }

    // Выходной порт из возвращаемого значения
    if (func.dqReturnType != "void") {
        PortDefinition port;
        port.name = "result";
        port.type = func.dqReturnType;
        port.direction = PortDefinition::Output;
        mod.ports << port;
    }

    // Порты выполнения
    PortDefinition execIn;
    execIn.name = "exec_in";
    execIn.direction = PortDefinition::ExecInput;
    mod.ports << execIn;

    PortDefinition execOut;
    execOut.name = "exec_out";
    execOut.direction = PortDefinition::ExecOutput;
    mod.ports << execOut;

    return mod;
}
```

### Декомпозиция C++ класса

```cpp
QList<ModuleDefinition> LibraryDecomposer::classToModules(
        const ClassDecl& cls, const DecompositionOptions& options) {
    QList<ModuleDefinition> modules;

    // Модуль "конструктор" -- создаёт экземпляр
    if (options.includeConstructors) {
        for (const auto& ctor : cls.constructors) {
            ModuleDefinition mod;
            mod.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            mod.name = QString("%1_create").arg(cls.name.toLower());
            mod.description = QString("Создать экземпляр %1").arg(cls.name);
            mod.category = options.category;
            mod.language = "cpp";

            // Параметры конструктора -> входные порты
            for (const auto& param : ctor.parameters) {
                PortDefinition port;
                port.name = param.name;
                port.type = param.dqType;
                port.direction = PortDefinition::Input;
                mod.ports << port;
            }

            // Выход -- указатель на созданный объект
            PortDefinition outPort;
            outPort.name = "instance";
            outPort.type = "pointer";
            outPort.direction = PortDefinition::Output;
            mod.ports << outPort;

            mod.needsWrapper = true;
            modules << mod;
        }
    }

    // Модуль "деструктор" -- уничтожает экземпляр
    if (options.includeDestructors && cls.hasDestructor) {
        ModuleDefinition mod;
        mod.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        mod.name = QString("%1_destroy").arg(cls.name.toLower());
        mod.description = QString("Уничтожить экземпляр %1").arg(cls.name);
        mod.category = options.category;
        mod.language = "cpp";

        PortDefinition inPort;
        inPort.name = "instance";
        inPort.type = "pointer";
        inPort.direction = PortDefinition::Input;
        mod.ports << inPort;

        mod.needsWrapper = true;
        modules << mod;
    }

    // Каждый публичный метод -> отдельный модуль
    for (const auto& method : cls.publicMethods) {
        modules << methodToModule(cls, method, options);
    }

    return modules;
}

ModuleDefinition LibraryDecomposer::methodToModule(
        const ClassDecl& cls, const MethodDecl& method,
        const DecompositionOptions& options) {
    ModuleDefinition mod;

    mod.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    mod.name = QString("%1_%2")
                   .arg(cls.name.toLower(), method.name);
    mod.description = method.description.isEmpty()
                      ? QString("Вызвать %1::%2").arg(cls.name, method.name)
                      : method.description;
    mod.category = options.category;
    mod.language = "cpp";

    // Первый входной порт -- указатель на экземпляр (this)
    if (!method.isStatic) {
        PortDefinition selfPort;
        selfPort.name = "instance";
        selfPort.type = "pointer";
        selfPort.direction = PortDefinition::Input;
        selfPort.description = QString("Экземпляр %1").arg(cls.name);
        mod.ports << selfPort;
    }

    // Параметры метода -> входные порты
    for (const auto& param : method.parameters) {
        PortDefinition port;
        port.name = param.name;
        port.type = param.dqType;
        port.direction = PortDefinition::Input;
        mod.ports << port;
    }

    // Возвращаемое значение -> выходной порт
    if (method.dqReturnType != "void") {
        PortDefinition outPort;
        outPort.name = "result";
        outPort.type = method.dqReturnType;
        outPort.direction = PortDefinition::Output;
        mod.ports << outPort;
    }

    // Передаём instance на выход для цепочки вызовов
    if (!method.isStatic) {
        PortDefinition outSelf;
        outSelf.name = "instance_out";
        outSelf.type = "pointer";
        outSelf.direction = PortDefinition::Output;
        outSelf.description = "Экземпляр после вызова метода";
        mod.ports << outSelf;
    }

    mod.needsWrapper = true;
    return mod;
}
```

---

## Генерация обёрточного (wrapper) кода

### WrapperGenerator

```cpp
class WrapperGenerator {
public:
    struct WrapperCode {
        QString headerContent;   // .h файл
        QString sourceContent;   // .c/.cpp файл
        QString headerPath;
        QString sourcePath;
    };

    // Генерация обёртки для C-функции (обычно не нужна, но может потребоваться
    // для нормализации интерфейса -- например, превращения указателя-аргумента
    // в выходной порт)
    WrapperCode generateCWrapper(const FunctionDecl& func,
                                  const ModuleDefinition& mod);

    // Генерация C-обёртки для C++ класса
    WrapperCode generateCppClassWrapper(const ClassDecl& cls,
                                         const QList<ModuleDefinition>& modules);

    // Генерация обёртки для шаблонного класса (конкретная инстанция)
    WrapperCode generateTemplateWrapper(const TemplateDecl& tmpl,
                                         const QStringList& templateArgs);

private:
    QString generateConstructorWrapper(const ClassDecl& cls,
                                        const ConstructorDecl& ctor);
    QString generateDestructorWrapper(const ClassDecl& cls);
    QString generateMethodWrapper(const ClassDecl& cls,
                                    const MethodDecl& method);
};
```

### Пример: обёртка для C++ класса

Исходный C++ класс:

```cpp
// vector2d.h
class Vector2D {
public:
    Vector2D(float x, float y);
    ~Vector2D();

    float length() const;
    Vector2D normalize() const;
    float dot(const Vector2D& other) const;

    float x, y;
};
```

Сгенерированная C-обёртка:

```c
// vector2d_wrapper.h
#ifndef DQ_VECTOR2D_WRAPPER_H
#define DQ_VECTOR2D_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* DQ_Vector2D;

// Конструктор
DQ_Vector2D dq_vector2d_create(float x, float y);

// Деструктор
void dq_vector2d_destroy(DQ_Vector2D instance);

// Методы
float dq_vector2d_length(DQ_Vector2D instance);
DQ_Vector2D dq_vector2d_normalize(DQ_Vector2D instance);
float dq_vector2d_dot(DQ_Vector2D instance, DQ_Vector2D other);

// Доступ к полям
float dq_vector2d_get_x(DQ_Vector2D instance);
float dq_vector2d_get_y(DQ_Vector2D instance);
void dq_vector2d_set_x(DQ_Vector2D instance, float value);
void dq_vector2d_set_y(DQ_Vector2D instance, float value);

#ifdef __cplusplus
}
#endif

#endif
```

```cpp
// vector2d_wrapper.cpp
#include "vector2d_wrapper.h"
#include "vector2d.h"

extern "C" {

DQ_Vector2D dq_vector2d_create(float x, float y) {
    return static_cast<DQ_Vector2D>(new Vector2D(x, y));
}

void dq_vector2d_destroy(DQ_Vector2D instance) {
    delete static_cast<Vector2D*>(instance);
}

float dq_vector2d_length(DQ_Vector2D instance) {
    return static_cast<Vector2D*>(instance)->length();
}

DQ_Vector2D dq_vector2d_normalize(DQ_Vector2D instance) {
    Vector2D result = static_cast<Vector2D*>(instance)->normalize();
    return static_cast<DQ_Vector2D>(new Vector2D(result));
}

float dq_vector2d_dot(DQ_Vector2D instance, DQ_Vector2D other) {
    return static_cast<Vector2D*>(instance)->dot(
        *static_cast<Vector2D*>(other)
    );
}

float dq_vector2d_get_x(DQ_Vector2D instance) {
    return static_cast<Vector2D*>(instance)->x;
}

float dq_vector2d_get_y(DQ_Vector2D instance) {
    return static_cast<Vector2D*>(instance)->y;
}

void dq_vector2d_set_x(DQ_Vector2D instance, float value) {
    static_cast<Vector2D*>(instance)->x = value;
}

void dq_vector2d_set_y(DQ_Vector2D instance, float value) {
    static_cast<Vector2D*>(instance)->y = value;
}

}
```

### Сгенерированные .dqmod модули

```json
[
  {
    "id": "uuid-1",
    "name": "vector2d_create",
    "version": "1.0.0",
    "language": "cpp",
    "category": "imported",
    "description": "Создать экземпляр Vector2D",
    "ports": {
      "input": [
        {"name": "x", "type": "float"},
        {"name": "y", "type": "float"}
      ],
      "output": [
        {"name": "instance", "type": "pointer"}
      ]
    },
    "source": "wrappers/vector2d_wrapper.cpp",
    "header": "wrappers/vector2d_wrapper.h"
  },
  {
    "id": "uuid-2",
    "name": "vector2d_length",
    "version": "1.0.0",
    "language": "cpp",
    "category": "imported",
    "description": "Вызвать Vector2D::length",
    "ports": {
      "input": [
        {"name": "instance", "type": "pointer"}
      ],
      "output": [
        {"name": "result", "type": "float"},
        {"name": "instance_out", "type": "pointer"}
      ]
    },
    "source": "wrappers/vector2d_wrapper.cpp",
    "header": "wrappers/vector2d_wrapper.h"
  },
  {
    "id": "uuid-3",
    "name": "vector2d_dot",
    "version": "1.0.0",
    "language": "cpp",
    "category": "imported",
    "description": "Вызвать Vector2D::dot",
    "ports": {
      "input": [
        {"name": "instance", "type": "pointer"},
        {"name": "other", "type": "pointer"}
      ],
      "output": [
        {"name": "result", "type": "float"},
        {"name": "instance_out", "type": "pointer"}
      ]
    },
    "source": "wrappers/vector2d_wrapper.cpp",
    "header": "wrappers/vector2d_wrapper.h"
  }
]
```

---

## Обработка C++ шаблонов

### Стратегия работы с шаблонами

Шаблоны не могут быть напрямую преобразованы в модули, так как требуют конкретных параметров типов. Library Processor использует следующий подход:

1. **Обнаружение шаблона** -- libclang находит `ClassTemplate` или `FunctionTemplate`
2. **Запрос параметров** -- пользователю предлагается указать конкретные типы для инстанцирования
3. **Генерация инстанций** -- для каждого набора типов генерируется конкретный модуль

```cpp
class TemplateInstantiator {
public:
    struct TemplateInstance {
        QString templateName;
        QStringList typeArgs;       // Конкретные типы
        ModuleDefinition module;    // Сгенерированный модуль
        WrapperCode wrapper;        // Обёрточный код
    };

    // Получить параметры шаблона
    QList<TemplateParam> getTemplateParams(const TemplateDecl& tmpl) const;

    // Создать инстанцию с конкретными типами
    TemplateInstance instantiate(const TemplateDecl& tmpl,
                                 const QStringList& typeArgs);

    // Предложить типичные инстанции (например, vector<int>, vector<float>)
    QList<QStringList> suggestInstantiations(const TemplateDecl& tmpl) const;
};
```

### Пример: std::vector

```
Шаблон:  std::vector<T>
Пользователь выбирает: T = int

Результат:
  - vector_int_create      (-> vector<int>*)
  - vector_int_destroy     (vector<int>* ->)
  - vector_int_push_back   (vector<int>*, int ->)
  - vector_int_pop_back    (vector<int>* ->)
  - vector_int_at          (vector<int>*, int -> int)
  - vector_int_size        (vector<int>* -> int)
  - vector_int_clear       (vector<int>* ->)
```

---

## Обработка пространств имён (Namespaces)

```cpp
// При парсинге namespace добавляется как префикс к имени модуля

// Исходный код:
// namespace math {
//     namespace linear {
//         float dot(float* a, float* b, int size);
//     }
// }

// Результат:
// Имя модуля: math_linear_dot
// Имя функции обёртки: dq_math_linear_dot
```

---

## Пользовательский интерфейс Library Processor

### Мастер импорта библиотеки

Процесс импорта состоит из шагов:

#### Шаг 1: Выбор библиотеки

- Указание пути к заголовочным файлам
- Указание пути к скомпилированным библиотекам (.a, .so, .lib, .dll)
- Указание дополнительных include-путей
- Указание define-макросов

#### Шаг 2: Парсинг и предпросмотр

- Отображение дерева AST
- Список найденных функций, структур, классов
- Статистика: количество элементов каждого типа
- Предупреждения и ошибки парсинга

#### Шаг 3: Выбор элементов для импорта

- Чекбоксы для выбора конкретных функций/классов
- Фильтрация по имени, типу, namespace
- Настройка параметров инстанцирования шаблонов
- Опции: генерировать обёртки, включать static-функции, категория модулей

#### Шаг 4: Настройка модулей

- Предпросмотр генерируемых .dqmod файлов
- Возможность редактирования имён и описаний
- Предпросмотр обёрточного кода

#### Шаг 5: Генерация

- Генерация .dqmod файлов
- Генерация обёрточного кода
- Регистрация модулей в ModuleRegistry
- Отчёт о результатах

```cpp
class LibraryImportWizard : public QWizard {
    Q_OBJECT
public:
    LibraryImportWizard(ModuleRegistry* registry, QWidget* parent = nullptr);

private:
    QWizardPage* createLibrarySelectionPage();
    QWizardPage* createParseResultPage();
    QWizardPage* createSelectionPage();
    QWizardPage* createConfigurationPage();
    QWizardPage* createGenerationPage();

    LibclangParser m_parser;
    LibraryDecomposer m_decomposer;
    WrapperGenerator m_wrapperGen;
    ModuleRegistry* m_registry;
};
```

---

## AST Viewer (отладка)

Для отладки и изучения структуры библиотеки предоставляется визуализатор AST:

```cpp
class ASTViewerWidget : public QWidget {
    Q_OBJECT
public:
    explicit ASTViewerWidget(QWidget* parent = nullptr);

    void showAST(const QString& headerPath, const QStringList& args = {});

private:
    void populateTree(CXCursor cursor, QTreeWidgetItem* parentItem);
    QString cursorKindToString(CXCursorKind kind) const;
    QString typeToString(CXType type) const;

    QTreeWidget* m_tree;
    QTextEdit* m_details;
    QSplitter* m_splitter;
};
```

Отображает:
- Иерархическое дерево AST
- Тип каждого узла (FunctionDecl, ParmDecl, TypeRef и т.д.)
- Расположение в файле (строка, столбец)
- Тип данных
- Дополнительные атрибуты (static, const, virtual и т.д.)
