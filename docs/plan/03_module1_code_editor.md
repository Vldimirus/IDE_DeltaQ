# DeltaQ IDE - Модуль 1: Редактор кода (Code Editor)

## Обзор

Модуль Code Editor предоставляет полнофункциональный текстовый редактор для работы с исходным кодом на C/C++. Основан на компоненте QScintilla и интегрирован с Language Server Protocol (LSP) через clangd для интеллектуальных возможностей.

---

## Компоненты

```
┌─────────────────────────────────────────────────────┐
│                   Code Editor Module                 │
│                                                      │
│  ┌──────────────┐  ┌───────────┐  ┌──────────────┐ │
│  │  QScintilla   │  │ LSP Client│  │  Annotation  │ │
│  │  Editor       │  │ (clangd)  │  │  Parser      │ │
│  └──────┬───────┘  └─────┬─────┘  └──────┬───────┘ │
│         │                │                │          │
│  ┌──────┴────────────────┴────────────────┴───────┐ │
│  │              CodeEditorWidget                   │ │
│  └────────────────────┬───────────────────────────┘ │
│                       │                              │
│  ┌────────────────────┴───────────────────────────┐ │
│  │              Build Integration                  │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────┐ │ │
│  │  │  CMake   │  │ Compiler │  │   Debugger   │ │ │
│  │  │  Driver  │  │  Runner  │  │  (GDB/LLDB)  │ │ │
│  │  └──────────┘  └──────────┘  └──────────────┘ │ │
│  └────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

---

## QScintilla: Интеграция редактора

### Настройка QScintilla

```cpp
class CodeEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit CodeEditorWidget(CommandBus* commandBus, QWidget* parent = nullptr);

    void openFile(const QString& path);
    void saveFile();
    void saveFileAs(const QString& path);

    QString currentFilePath() const;
    bool isModified() const;

private:
    void setupEditor();
    void setupLexer();
    void setupAutocompletion();
    void setupMargins();

    QsciScintilla* m_editor;
    QsciLexerCPP* m_lexer;
    LSPClient* m_lspClient;
    AnnotationParser* m_annotationParser;
    CommandBus* m_commandBus;
};
```

### Конфигурация QScintilla

```cpp
void CodeEditorWidget::setupEditor() {
    m_editor = new QsciScintilla(this);

    // Кодировка
    m_editor->setUtf8(true);

    // Шрифт
    QFont font("JetBrains Mono", 12);
    font.setStyleHint(QFont::Monospace);
    m_editor->setFont(font);

    // Табуляция
    m_editor->setTabWidth(4);
    m_editor->setIndentationsUseTabs(false);
    m_editor->setAutoIndent(true);
    m_editor->setIndentationGuides(true);

    // Сворачивание кода (code folding)
    m_editor->setFolding(QsciScintilla::BoxedTreeFoldStyle);

    // Подсветка текущей строки
    m_editor->setCaretLineVisible(true);
    m_editor->setCaretLineBackgroundColor(QColor("#2d2d2d"));

    // Подсветка парных скобок
    m_editor->setBraceMatching(QsciScintilla::SloppyBraceMatch);

    // Перенос строк
    m_editor->setWrapMode(QsciScintilla::WrapWord);

    // Автозакрытие скобок
    m_editor->setAutoCompletionThreshold(2);
    m_editor->setAutoCompletionSource(QsciScintilla::AcsAll);
}

void CodeEditorWidget::setupLexer() {
    m_lexer = new QsciLexerCPP(m_editor);

    // Цветовая схема (тёмная тема)
    m_lexer->setDefaultPaper(QColor("#1e1e1e"));
    m_lexer->setDefaultColor(QColor("#d4d4d4"));

    m_lexer->setColor(QColor("#569cd6"), QsciLexerCPP::Keyword);          // Ключевые слова
    m_lexer->setColor(QColor("#4ec9b0"), QsciLexerCPP::KeywordSet2);      // Типы
    m_lexer->setColor(QColor("#ce9178"), QsciLexerCPP::DoubleQuotedString); // Строки
    m_lexer->setColor(QColor("#b5cea8"), QsciLexerCPP::Number);           // Числа
    m_lexer->setColor(QColor("#6a9955"), QsciLexerCPP::Comment);          // Комментарии
    m_lexer->setColor(QColor("#6a9955"), QsciLexerCPP::CommentLine);      // Однострочные комментарии
    m_lexer->setColor(QColor("#c586c0"), QsciLexerCPP::PreProcessor);     // Препроцессор
    m_lexer->setColor(QColor("#dcdcaa"), QsciLexerCPP::Operator);         // Операторы

    m_editor->setLexer(m_lexer);
}

void CodeEditorWidget::setupMargins() {
    // Номера строк
    m_editor->setMarginType(0, QsciScintilla::NumberMargin);
    m_editor->setMarginWidth(0, "00000");
    m_editor->setMarginLineNumbers(0, true);

    // Маркеры (breakpoints, ошибки)
    m_editor->setMarginType(1, QsciScintilla::SymbolMargin);
    m_editor->setMarginWidth(1, 20);
    m_editor->setMarginSensitivity(1, true);

    // Маркер breakpoint (красная точка)
    m_editor->markerDefine(QsciScintilla::Circle, MARKER_BREAKPOINT);
    m_editor->setMarkerBackgroundColor(QColor("#e51400"), MARKER_BREAKPOINT);

    // Маркер ошибки (красная волнистая линия)
    m_editor->markerDefine(QsciScintilla::Background, MARKER_ERROR);
    m_editor->setMarkerBackgroundColor(QColor("#5c2020"), MARKER_ERROR);

    // Маркер предупреждения
    m_editor->markerDefine(QsciScintilla::Background, MARKER_WARNING);
    m_editor->setMarkerBackgroundColor(QColor("#5c5c20"), MARKER_WARNING);

    // Маркер текущей строки отладки (жёлтая стрелка)
    m_editor->markerDefine(QsciScintilla::RightArrow, MARKER_DEBUG_LINE);
    m_editor->setMarkerBackgroundColor(QColor("#e8e800"), MARKER_DEBUG_LINE);

    // Сворачивание кода
    m_editor->setMarginType(2, QsciScintilla::SymbolMargin);
    m_editor->setMarginWidth(2, 15);
    m_editor->setFolding(QsciScintilla::BoxedTreeFoldStyle, 2);
}
```

---

## LSP-клиент (clangd)

### Архитектура LSP-клиента

```cpp
class LSPClient : public QObject {
    Q_OBJECT
public:
    explicit LSPClient(QObject* parent = nullptr);
    ~LSPClient();

    // Управление процессом
    void start(const QString& clangdPath, const QStringList& args = {});
    void stop();
    bool isRunning() const;

    // Инициализация
    void initialize(const QString& rootUri);
    void didOpen(const QString& uri, const QString& languageId, const QString& text);
    void didChange(const QString& uri, const QString& text);
    void didSave(const QString& uri);
    void didClose(const QString& uri);

    // Запросы
    void completion(const QString& uri, int line, int character);
    void hover(const QString& uri, int line, int character);
    void definition(const QString& uri, int line, int character);
    void references(const QString& uri, int line, int character);
    void rename(const QString& uri, int line, int character, const QString& newName);
    void formatting(const QString& uri);
    void signatureHelp(const QString& uri, int line, int character);
    void documentSymbols(const QString& uri);
    void codeAction(const QString& uri, int startLine, int startChar,
                    int endLine, int endChar);

signals:
    // Ответы на запросы
    void completionResult(const QList<CompletionItem>& items);
    void hoverResult(const HoverInfo& info);
    void definitionResult(const Location& location);
    void referencesResult(const QList<Location>& locations);
    void renameResult(const WorkspaceEdit& edit);
    void formattingResult(const QList<TextEdit>& edits);
    void signatureHelpResult(const SignatureHelp& help);
    void documentSymbolsResult(const QList<DocumentSymbol>& symbols);
    void codeActionResult(const QList<CodeAction>& actions);

    // Диагностика (ошибки и предупреждения)
    void diagnosticsReceived(const QString& uri, const QList<Diagnostic>& diagnostics);

    // Состояние
    void initialized();
    void error(const QString& message);

private:
    void sendRequest(const QString& method, const QJsonObject& params);
    void sendNotification(const QString& method, const QJsonObject& params);
    void handleResponse(const QJsonObject& response);
    void handleNotification(const QString& method, const QJsonObject& params);

    QProcess* m_process;
    int m_nextRequestId;
    QMap<int, QString> m_pendingRequests;  // id -> method
};
```

### Протокол обмена с clangd

Коммуникация происходит через stdin/stdout процесса clangd в формате JSON-RPC 2.0:

```
Content-Length: <длина>\r\n
\r\n
{"jsonrpc":"2.0","id":1,"method":"textDocument/completion","params":{...}}
```

### Интеграция LSP с редактором

```cpp
void CodeEditorWidget::setupAutocompletion() {
    // Подключение к LSP для автодополнения
    connect(m_lspClient, &LSPClient::completionResult,
            this, &CodeEditorWidget::showCompletions);

    // Диагностика -- подчёркивание ошибок
    connect(m_lspClient, &LSPClient::diagnosticsReceived,
            this, &CodeEditorWidget::showDiagnostics);

    // Всплывающие подсказки при наведении
    connect(m_lspClient, &LSPClient::hoverResult,
            this, &CodeEditorWidget::showHoverTooltip);

    // Переход к определению по Ctrl+Click
    connect(m_editor, &QsciScintilla::indicatorClicked,
            this, &CodeEditorWidget::handleIndicatorClick);
}

void CodeEditorWidget::showDiagnostics(const QString& uri,
                                        const QList<Diagnostic>& diagnostics) {
    // Очистить предыдущие маркеры
    m_editor->markerDeleteAll(MARKER_ERROR);
    m_editor->markerDeleteAll(MARKER_WARNING);
    m_editor->clearAnnotations();

    for (const auto& diag : diagnostics) {
        int line = diag.range.start.line;

        if (diag.severity == DiagnosticSeverity::Error) {
            m_editor->markerAdd(line, MARKER_ERROR);
        } else if (diag.severity == DiagnosticSeverity::Warning) {
            m_editor->markerAdd(line, MARKER_WARNING);
        }

        // Волнистое подчёркивание
        int startPos = m_editor->positionFromLineIndex(
            diag.range.start.line, diag.range.start.character);
        int endPos = m_editor->positionFromLineIndex(
            diag.range.end.line, diag.range.end.character);

        m_editor->fillIndicatorRange(
            diag.range.start.line, diag.range.start.character,
            diag.range.end.line, diag.range.end.character,
            diag.severity == DiagnosticSeverity::Error ? INDICATOR_ERROR : INDICATOR_WARNING);

        // Аннотация справа от строки
        m_editor->annotate(line, diag.message,
                           diag.severity == DiagnosticSeverity::Error ?
                           m_errorAnnotationStyle : m_warningAnnotationStyle);
    }
}
```

---

## Система сборки

### Поддерживаемые компиляторы

| Компилятор | Платформы | Стандарты |
|-----------|-----------|----------|
| **GCC** | Linux, Windows (MinGW) | C11, C17, C23, C++17, C++20, C++23 |
| **Clang** | Linux, macOS, Windows | C11, C17, C23, C++17, C++20, C++23 |
| **MSVC** | Windows | C11, C17, C++17, C++20, C++23 |

### BuildSystem

```cpp
class BuildSystem : public QObject {
    Q_OBJECT
public:
    explicit BuildSystem(CodeGenerator* codeGen, QObject* parent = nullptr);

    // Конфигурация
    void setCompiler(const QString& compiler);    // gcc, clang, msvc
    void setStandard(const QString& standard);    // c17, c++20, ...
    void setOptimization(const QString& level);   // -O0, -O1, -O2, -O3, -Os
    void setBuildDirectory(const QString& path);
    void addDefine(const QString& define);
    void addIncludeDir(const QString& path);
    void addLibrary(const QString& lib);

    // Сборка
    void build();
    void rebuild();
    void clean();
    void buildSingleFile(const QString& filePath);

    // CMake
    void generateCMakeLists(const Project& project);
    void configureCMake();

    // Состояние
    bool isBuilding() const;
    BuildResult lastResult() const;

signals:
    void buildStarted();
    void buildOutput(const QString& line);
    void buildError(const QString& file, int line, const QString& message);
    void buildWarning(const QString& file, int line, const QString& message);
    void buildFinished(bool success, int errors, int warnings);

private:
    void parseBuildOutput(const QString& line);

    CodeGenerator* m_codeGen;
    QProcess* m_buildProcess;
    QString m_compiler;
    QString m_standard;
    QString m_buildDir;
};
```

### Генерация CMakeLists.txt

```cpp
void BuildSystem::generateCMakeLists(const Project& project) {
    QString cmake;
    cmake += "cmake_minimum_required(VERSION 3.24)\n";
    cmake += QString("project(%1 VERSION %2)\n\n")
                 .arg(project.name(), project.version());

    cmake += QString("set(CMAKE_C_STANDARD %1)\n")
                 .arg(project.cStandard());
    cmake += QString("set(CMAKE_CXX_STANDARD %1)\n\n")
                 .arg(project.cxxStandard());

    // Исходные файлы модулей
    cmake += "# Исходные файлы модулей\n";
    cmake += "set(MODULE_SOURCES\n";
    for (const auto& module : project.modules()) {
        cmake += QString("    %1\n").arg(module->sourcePath());
    }
    cmake += ")\n\n";

    // Сгенерированный код графов
    cmake += "# Сгенерированный код\n";
    cmake += "set(GENERATED_SOURCES\n";
    cmake += "    ${CMAKE_BINARY_DIR}/generated/main.c\n";
    cmake += "    ${CMAKE_BINARY_DIR}/generated/graph_code.c\n";
    cmake += ")\n\n";

    // Исполняемый файл
    cmake += QString("add_executable(%1\n").arg(project.name());
    cmake += "    ${MODULE_SOURCES}\n";
    cmake += "    ${GENERATED_SOURCES}\n";
    cmake += ")\n\n";

    // Include директории
    cmake += QString("target_include_directories(%1 PRIVATE\n").arg(project.name());
    cmake += "    ${CMAKE_SOURCE_DIR}/include\n";
    cmake += "    ${CMAKE_SOURCE_DIR}/modules\n";
    cmake += "    ${CMAKE_BINARY_DIR}/generated\n";
    for (const auto& dir : project.includeDirs()) {
        cmake += QString("    %1\n").arg(dir);
    }
    cmake += ")\n\n";

    // Библиотеки
    if (!project.libraries().isEmpty()) {
        cmake += QString("target_link_libraries(%1 PRIVATE\n").arg(project.name());
        for (const auto& lib : project.libraries()) {
            cmake += QString("    %1\n").arg(lib);
        }
        cmake += ")\n\n";
    }

    // Флаги компиляции
    cmake += QString("target_compile_options(%1 PRIVATE\n").arg(project.name());
    cmake += "    -Wall -Wextra\n";
    cmake += QString("    %1\n").arg(project.optimization());
    cmake += ")\n";

    // Сохранение файла
    QFile file(project.buildDir() + "/CMakeLists.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(cmake.toUtf8());
}
```

---

## Аннотации @dqmodule

### Поддерживаемые аннотации

| Аннотация | Описание | Пример |
|-----------|----------|--------|
| `@dqmodule` | Объявление модуля | `@dqmodule name=add version=1.0.0` |
| `@dqport` | Определение порта | `@dqport input a:int` |
| `@dqexec` | Порт выполнения | `@dqexec output exec_true` |
| `@dqdepends` | Зависимость | `@dqdepends helper version=>=1.0` |
| `@dqcategory` | Категория | `@dqcategory math` |
| `@dqicon` | Иконка | `@dqicon icons/add.png` |
| `@dqcolor` | Цвет узла | `@dqcolor #4CAF50` |
| `@dqtag` | Тег | `@dqtag arithmetic` |

### Автоматическая генерация .dqmod

При сохранении .c/.cpp файла с аннотациями IDE автоматически:

1. Парсит аннотации через `AnnotationParser`
2. Сравнивает с существующим .dqmod (если есть)
3. Обновляет или создаёт .dqmod файл
4. Уведомляет `ModuleRegistry` об изменении

```cpp
void CodeEditorWidget::onFileSaved(const QString& path) {
    if (path.endsWith(".c") || path.endsWith(".cpp") ||
        path.endsWith(".h") || path.endsWith(".hpp")) {

        // Парсинг аннотаций
        auto modules = m_annotationParser->parse(path);

        for (const auto& moduleDef : modules) {
            // Генерация .dqmod
            QString dqmodPath = deriveModulePath(path, moduleDef.name);
            m_annotationParser->generateDqmod(moduleDef, dqmodPath);

            // Уведомление через CommandBus
            auto cmd = std::make_unique<UpdateModuleCommand>(
                m_moduleStore, moduleDef, dqmodPath);
            m_commandBus->execute(std::move(cmd));
        }
    }
}
```

### Полный пример модуля с аннотациями

```c
/**
 * @dqmodule name=string_split version=1.0.0 category=string
 * @dqmodule description="Разделяет строку по разделителю"
 *
 * @dqport input text:string description="Входная строка"
 * @dqport input delimiter:string description="Разделитель" default=","
 * @dqport output parts:array<string> description="Массив частей"
 * @dqport output count:int description="Количество частей"
 *
 * @dqtag string
 * @dqtag parsing
 * @dqcolor #FF9800
 */

#include <string.h>
#include <stdlib.h>

typedef struct {
    char** items;
    size_t count;
} StringArray;

StringArray dq_string_split(const char* text, const char* delimiter) {
    StringArray result = {NULL, 0};

    // Подсчёт количества частей
    const char* tmp = text;
    while ((tmp = strstr(tmp, delimiter)) != NULL) {
        result.count++;
        tmp += strlen(delimiter);
    }
    result.count++;

    // Выделение памяти
    result.items = (char**)malloc(result.count * sizeof(char*));

    // Разделение
    size_t i = 0;
    char* copy = strdup(text);
    char* token = strtok(copy, delimiter);
    while (token != NULL) {
        result.items[i++] = strdup(token);
        token = strtok(NULL, delimiter);
    }
    free(copy);

    return result;
}
```

---

## Тестирование модулей (Module Tester)

Модуль — это завершённый элемент, «кирпичик». **Прежде чем модуль станет доступен в палитре блоков, он должен быть протестирован.** Для этого в редакторе кода встроен пошаговый эмулятор.

### Конвейер: код → тест → палитра

```
[Написал код с @dqmodule] → [Сохранил] → .dqmod создан (статус: ⚠ не протестирован)
                                              │
                                              ▼
                                    [Модуль тестирования]
                                    Пользователь задаёт входные данные
                                    Запускает модуль
                                    Проверяет выходные данные
                                              │
                                    ┌─────────┴──────────┐
                                    ▼                    ▼
                              [PASS ✓]              [FAIL ✗]
                              Модуль доступен       Показать ошибки
                              в палитре блоков      Модуль недоступен
```

### Интерфейс тестирования

В редакторе кода появляется боковая панель:

| Элемент | Описание |
|---------|----------|
| **Входные порты** | Поля ввода для каждого `@dqport input` — пользователь задаёт тестовые значения |
| **Кнопка «Запустить»** | Компилирует и запускает модуль с заданными входами |
| **Кнопка «Пошагово»** | Пошаговое выполнение (как отладчик, но для одного модуля) |
| **Выходные порты** | Результат выполнения для каждого `@dqport output` |
| **Статус** | PASS / FAIL + сообщения об ошибках |

### Пример тестирования

Для модуля `add(a: int, b: int) → result: int`:

```
┌─────────────────────────────────────┐
│  Тест модуля: add                   │
├─────────────────────────────────────┤
│  Входы:                             │
│    a: [5     ]                      │
│    b: [3     ]                      │
├─────────────────────────────────────┤
│  [▶ Запустить]  [⏭ Пошагово]       │
├─────────────────────────────────────┤
│  Выходы:                            │
│    result: 8  ✓                     │
├─────────────────────────────────────┤
│  Статус: PASS                       │
└─────────────────────────────────────┘
```

### Обработка ошибок компилятора

При сборке модуля вывод компилятора (GCC/Clang/MSVC) парсится и ошибки маппятся на строки кода:

```cpp
class CompilerOutputParser {
public:
    struct CompilerError {
        QString file;
        int line;
        int column;
        QString severity;  // "error", "warning", "note"
        QString message;
    };

    // Универсальный парсер — определяет формат автоматически
    QList<CompilerError> parse(const QString& output);

private:
    QList<CompilerError> parseGCC(const QString& output);    // файл:строка:колонка: тип: сообщение
    QList<CompilerError> parseClang(const QString& output);  // аналогично GCC
    QList<CompilerError> parseMSVC(const QString& output);   // файл(строка): тип Cxxxx: сообщение
};
```

---

## Отладчик

### Архитектура отладчика

```cpp
class DebugManager : public QObject {
    Q_OBJECT
public:
    explicit DebugManager(QObject* parent = nullptr);

    // Управление
    void startDebug(const QString& executable, const QStringList& args = {});
    void stopDebug();
    void pauseExecution();
    void continueExecution();

    // Шаги
    void stepOver();
    void stepInto();
    void stepOut();
    void runToCursor(const QString& file, int line);

    // Точки останова
    void addBreakpoint(const QString& file, int line);
    void removeBreakpoint(const QString& file, int line);
    void toggleBreakpoint(const QString& file, int line);
    void setConditionalBreakpoint(const QString& file, int line,
                                   const QString& condition);
    QList<Breakpoint> breakpoints() const;

    // Переменные
    void evaluateExpression(const QString& expression);
    void watchVariable(const QString& name);
    QList<Variable> localVariables() const;
    QList<Variable> watchedVariables() const;

    // Стек вызовов
    QList<StackFrame> callStack() const;
    void selectFrame(int index);

signals:
    void debugStarted();
    void debugStopped();
    void breakpointHit(const QString& file, int line);
    void stepped(const QString& file, int line);
    void variablesUpdated(const QList<Variable>& locals);
    void callStackUpdated(const QList<StackFrame>& frames);
    void expressionEvaluated(const QString& expr, const QString& value);
    void debugOutput(const QString& text);

private:
    void sendGDBCommand(const QString& command);
    void parseGDBOutput(const QString& output);

    QProcess* m_debugProcess;
    QString m_debugger;  // "gdb" или "lldb"
    QList<Breakpoint> m_breakpoints;
};
```

### Интеграция с редактором

При отладке Code Editor:

- Подсвечивает текущую строку выполнения (жёлтая стрелка на полях)
- Отображает значения переменных при наведении мыши
- Позволяет добавлять breakpoints кликом по полю номеров строк
- Показывает панели: Переменные, Стек вызовов, Консоль отладки

### Панели отладки

| Панель | Содержание |
|--------|-----------|
| **Переменные** | Локальные и наблюдаемые переменные, их значения |
| **Стек вызовов** | Иерархия вызовов функций |
| **Точки останова** | Список всех breakpoints с условиями |
| **Консоль отладки** | Ввод выражений для вычисления, вывод GDB |
| **Вывод программы** | stdout/stderr отлаживаемой программы |

---

## Горячие клавиши Code Editor

| Действие | Комбинация | Описание |
|----------|-----------|----------|
| Сохранить | `Ctrl+S` | Сохранить текущий файл |
| Найти | `Ctrl+F` | Поиск в файле |
| Заменить | `Ctrl+H` | Поиск и замена |
| Перейти к строке | `Ctrl+G` | Переход по номеру строки |
| Перейти к определению | `F12` / `Ctrl+Click` | Переход к определению символа |
| Найти все ссылки | `Shift+F12` | Найти все использования символа |
| Переименовать | `F2` | Переименование символа |
| Форматировать | `Shift+Alt+F` | Форматирование кода |
| Комментировать | `Ctrl+/` | Закомментировать/раскомментировать строку |
| Дублировать | `Ctrl+D` | Дублировать строку |
| Удалить строку | `Ctrl+Shift+K` | Удалить текущую строку |
| Свернуть | `Ctrl+Shift+[` | Свернуть блок кода |
| Развернуть | `Ctrl+Shift+]` | Развернуть блок кода |
| Сборка | `Ctrl+B` | Собрать проект |
| Запуск | `F5` | Запуск с отладкой |
| Запуск без отладки | `Ctrl+F5` | Запуск без отладчика |
| Шаг через | `F10` | Step Over |
| Шаг внутрь | `F11` | Step Into |
| Шаг наружу | `Shift+F11` | Step Out |
| Breakpoint | `F9` | Переключить точку останова |
