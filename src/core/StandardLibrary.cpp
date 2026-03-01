// Стандартная библиотека модулей DeltaQ — реализация
#include "StandardLibrary.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace DeltaQ {

// Вспомогательная функция: создание core-модуля
static Module makeCore(const QString &name, const QString &category,
                       const QString &description,
                       const QVector<Port> &inputs,
                       const QVector<Port> &outputs,
                       const QString &sourceCode,
                       const QStringList &includes = {})
{
    Module m;
    m.id = "core." + category + "." + name;
    m.name = name;
    m.version = StandardLibrary::VERSION;
    m.language = "c";
    m.description = description;
    m.category = category;
    m.origin = "core";
    m.inputs = inputs;
    m.outputs = outputs;
    m.sourceCode = sourceCode;
    m.includes = includes;
    return m;
}

QVector<Module> StandardLibrary::createAll()
{
    QVector<Module> modules;

    // ===== io =====
    modules.append(makeCore("print", "io",
        "Вывод текста",
        {{"text", "string", ""}},
        {},
        "void dq_print(const char *text) {\n"
        "    printf(\"%s\", text);\n"
        "}",
        {"stdio.h"}
    ));

    modules.append(makeCore("println", "io",
        "Вывод текста с переводом строки",
        {{"text", "string", ""}},
        {},
        "void dq_println(const char *text) {\n"
        "    printf(\"%s\\n\", text);\n"
        "}",
        {"stdio.h"}
    ));

    modules.append(makeCore("read_line", "io",
        "Чтение строки",
        {},
        {{"text", "string", ""}},
        "const char *dq_read_line(void) {\n"
        "    static char buf[1024];\n"
        "    if (fgets(buf, sizeof(buf), stdin)) {\n"
        "        buf[strcspn(buf, \"\\n\")] = '\\0';\n"
        "        return buf;\n"
        "    }\n"
        "    return \"\";\n"
        "}",
        {"stdio.h", "string.h"}
    ));

    modules.append(makeCore("print_int", "io",
        "Вывод числа",
        {{"value", "int", ""}},
        {},
        "void dq_print_int(int value) {\n"
        "    printf(\"%d\", value);\n"
        "}",
        {"stdio.h"}
    ));

    modules.append(makeCore("print_float", "io",
        "Вывод дробного числа",
        {{"value", "float", ""}},
        {},
        "void dq_print_float(float value) {\n"
        "    printf(\"%f\", value);\n"
        "}",
        {"stdio.h"}
    ));

    modules.append(makeCore("string_constant", "io",
        "Строковая константа",
        {{"value", "string", "\"Hello\""}},
        {{"out", "string", ""}},
        "const char *dq_string_constant(const char *value) {\n"
        "    return value;\n"
        "}"
    ));

    modules.append(makeCore("int_constant", "io",
        "Целочисленная константа",
        {{"value", "int", "0"}},
        {{"out", "int", ""}},
        "int dq_int_constant(int value) {\n"
        "    return value;\n"
        "}"
    ));

    // ===== math =====
    modules.append(makeCore("add", "math",
        "Сложение",
        {{"a", "int", ""}, {"b", "int", ""}},
        {{"result", "int", ""}},
        "int dq_add(int a, int b) {\n"
        "    return a + b;\n"
        "}"
    ));

    modules.append(makeCore("subtract", "math",
        "Вычитание",
        {{"a", "int", ""}, {"b", "int", ""}},
        {{"result", "int", ""}},
        "int dq_subtract(int a, int b) {\n"
        "    return a - b;\n"
        "}"
    ));

    modules.append(makeCore("multiply", "math",
        "Умножение",
        {{"a", "int", ""}, {"b", "int", ""}},
        {{"result", "int", ""}},
        "int dq_multiply(int a, int b) {\n"
        "    return a * b;\n"
        "}"
    ));

    modules.append(makeCore("divide", "math",
        "Деление",
        {{"a", "int", ""}, {"b", "int", ""}},
        {{"result", "int", ""}},
        "int dq_divide(int a, int b) {\n"
        "    if (b == 0) return 0;\n"
        "    return a / b;\n"
        "}"
    ));

    modules.append(makeCore("mod", "math",
        "Остаток от деления",
        {{"a", "int", ""}, {"b", "int", ""}},
        {{"result", "int", ""}},
        "int dq_mod(int a, int b) {\n"
        "    if (b == 0) return 0;\n"
        "    return a % b;\n"
        "}"
    ));

    modules.append(makeCore("abs", "math",
        "Модуль числа",
        {{"value", "int", ""}},
        {{"result", "int", ""}},
        "int dq_abs(int value) {\n"
        "    return value < 0 ? -value : value;\n"
        "}"
    ));

    modules.append(makeCore("add_float", "math",
        "Сложение (float)",
        {{"a", "float", ""}, {"b", "float", ""}},
        {{"result", "float", ""}},
        "float dq_add_float(float a, float b) {\n"
        "    return a + b;\n"
        "}"
    ));

    modules.append(makeCore("multiply_float", "math",
        "Умножение (float)",
        {{"a", "float", ""}, {"b", "float", ""}},
        {{"result", "float", ""}},
        "float dq_multiply_float(float a, float b) {\n"
        "    return a * b;\n"
        "}"
    ));

    modules.append(makeCore("sqrt", "math",
        "Квадратный корень",
        {{"value", "float", ""}},
        {{"result", "float", ""}},
        "float dq_sqrt(float value) {\n"
        "    return sqrtf(value);\n"
        "}",
        {"math.h"}
    ));

    modules.append(makeCore("pow", "math",
        "Возведение в степень",
        {{"base", "float", ""}, {"exp", "float", ""}},
        {{"result", "float", ""}},
        "float dq_pow(float base, float exp) {\n"
        "    return powf(base, exp);\n"
        "}",
        {"math.h"}
    ));

    // ===== string =====
    modules.append(makeCore("str_length", "string",
        "Длина строки",
        {{"text", "string", ""}},
        {{"length", "int", ""}},
        "int dq_str_length(const char *text) {\n"
        "    return (int)strlen(text);\n"
        "}",
        {"string.h"}
    ));

    modules.append(makeCore("str_concat", "string",
        "Конкатенация строк",
        {{"a", "string", ""}, {"b", "string", ""}},
        {{"result", "string", ""}},
        "const char *dq_str_concat(const char *a, const char *b) {\n"
        "    static char buf[2048];\n"
        "    snprintf(buf, sizeof(buf), \"%s%s\", a, b);\n"
        "    return buf;\n"
        "}",
        {"stdio.h", "string.h"}
    ));

    modules.append(makeCore("str_compare", "string",
        "Сравнение строк",
        {{"a", "string", ""}, {"b", "string", ""}},
        {{"equal", "bool", ""}},
        "int dq_str_compare(const char *a, const char *b) {\n"
        "    return strcmp(a, b) == 0;\n"
        "}",
        {"string.h"}
    ));

    // ===== logic =====
    modules.append(makeCore("and", "logic",
        "Логическое И",
        {{"a", "bool", ""}, {"b", "bool", ""}},
        {{"result", "bool", ""}},
        "int dq_and(int a, int b) {\n"
        "    return a && b;\n"
        "}"
    ));

    modules.append(makeCore("or", "logic",
        "Логическое ИЛИ",
        {{"a", "bool", ""}, {"b", "bool", ""}},
        {{"result", "bool", ""}},
        "int dq_or(int a, int b) {\n"
        "    return a || b;\n"
        "}"
    ));

    modules.append(makeCore("not", "logic",
        "Логическое НЕ",
        {{"value", "bool", ""}},
        {{"result", "bool", ""}},
        "int dq_not(int value) {\n"
        "    return !value;\n"
        "}"
    ));

    // ===== conversion =====
    modules.append(makeCore("int_to_string", "conversion",
        "Преобразование int в string",
        {{"value", "int", ""}},
        {{"result", "string", ""}},
        "const char *dq_int_to_string(int value) {\n"
        "    static char buf[64];\n"
        "    snprintf(buf, sizeof(buf), \"%d\", value);\n"
        "    return buf;\n"
        "}",
        {"stdio.h"}
    ));

    modules.append(makeCore("string_to_int", "conversion",
        "Преобразование string в int",
        {{"text", "string", ""}},
        {{"result", "int", ""}},
        "int dq_string_to_int(const char *text) {\n"
        "    return atoi(text);\n"
        "}",
        {"stdlib.h"}
    ));

    modules.append(makeCore("float_to_int", "conversion",
        "Преобразование float в int",
        {{"value", "float", ""}},
        {{"result", "int", ""}},
        "int dq_float_to_int(float value) {\n"
        "    return (int)value;\n"
        "}"
    ));

    modules.append(makeCore("int_to_float", "conversion",
        "Преобразование int в float",
        {{"value", "int", ""}},
        {{"result", "float", ""}},
        "float dq_int_to_float(int value) {\n"
        "    return (float)value;\n"
        "}"
    ));

    // ===== control =====
    modules.append(makeCore("if_then", "control",
        "Условный выбор",
        {{"cond", "bool", ""}, {"then_val", "int", ""}, {"else_val", "int", ""}},
        {{"result", "int", ""}},
        "int dq_if_then(int cond, int then_val, int else_val) {\n"
        "    return cond ? then_val : else_val;\n"
        "}"
    ));

    modules.append(makeCore("delay_ms", "control",
        "Задержка в миллисекундах",
        {{"ms", "int", ""}},
        {},
        "void dq_delay_ms(int ms) {\n"
        "    struct timespec ts;\n"
        "    ts.tv_sec = ms / 1000;\n"
        "    ts.tv_nsec = (ms % 1000) * 1000000L;\n"
        "    nanosleep(&ts, NULL);\n"
        "}",
        {"time.h"}
    ));

    return modules;
}

void StandardLibrary::install(const QString &coreDir)
{
    if (isUpToDate(coreDir))
        return;

    QDir().mkpath(coreDir);

    auto modules = createAll();

    // Группировка по категориям — создание подпапок
    QMap<QString, QVector<Module>> byCategory;
    for (const auto &m : modules)
        byCategory[m.category].append(m);

    for (auto it = byCategory.begin(); it != byCategory.end(); ++it) {
        QString catDir = coreDir + "/" + it.key();
        QDir().mkpath(catDir);

        for (const auto &mod : it.value()) {
            QString path = catDir + "/" + mod.name + ".dqmod";
            QFile file(path);
            if (file.open(QIODevice::WriteOnly)) {
                QJsonDocument doc(mod.toJson());
                file.write(doc.toJson(QJsonDocument::Indented));
            }
        }
    }

    // Записываем pack.json для core
    QJsonObject pack;
    pack["name"] = "DeltaQ Standard Library";
    pack["version"] = QString(VERSION);
    pack["author"] = "DeltaQ Team";
    pack["description"] = "Стандартная библиотека модулей DeltaQ IDE";

    QFile packFile(coreDir + "/pack.json");
    if (packFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(pack);
        packFile.write(doc.toJson(QJsonDocument::Indented));
    }
}

bool StandardLibrary::isUpToDate(const QString &coreDir)
{
    QFile packFile(coreDir + "/pack.json");
    if (!packFile.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(packFile.readAll(), &err);
    if (err.error != QJsonParseError::NoError)
        return false;

    return doc.object()["version"].toString() == QString(VERSION);
}

} // namespace DeltaQ
