# DeltaQ IDE - Система модулей

## Концепция

Модуль (.dqmod) -- **единственная универсальная единица** повторно используемого кода в DeltaQ IDE. Каждый модуль представляет собой функциональный блок с определёнными входными и выходными портами.

### Ключевой принцип: «Модуль = Блок = Единица структуры»

Визуальный блочный редактор — это **отображение реальной архитектуры программы**, а не отдельный инструмент. Каждый блок на холсте — это модуль, а каждый модуль — это .cpp файл.

```
                    ┌──────────────────────────────────┐
                    │        МОДУЛЬ (Module)            │
                    │                                    │
                    │  Единый интерфейс:                │
                    │  - Входные порты (данные)          │
                    │  - Выходные порты (данные)         │
                    │  - Порты выполнения                │
                    │  - Описание (.dqmod)               │
                    │  - Исходный код (.c/.cpp)          │
                    └──────────┬───────────────────────┘
                               │
              ┌────────────────┼────────────────┐
              ▼                ▼                ▼
        ┌──────────┐    ┌──────────┐    ┌──────────┐
        │ Атомарный │    │Составной │    │Импортиро-│
        │ модуль    │    │ модуль   │    │ванный    │
        │           │    │(граф)    │    │модуль    │
        │ Написан   │    │          │    │          │
        │ вручную   │    │ Собран   │    │ Из внеш- │
        │ в Module 1│    │из блоков │    │ ней библ.│
        │           │    │в Module 2│    │ Module 4 │
        └──────────┘    └──────────┘    └──────────┘
```

### Три способа создания модулей

| Способ | Происхождение (`origin`) | Описание |
|--------|------------------------|----------|
| **Код** | `"user"` | Написал .cpp с `@dqmodule` → появился как блок в палитре |
| **Граф** | `"graph"` | Соединил блоки в граф → граф стал новым модулем со своими портами |
| **Библиотека** | `"library"` | Поглотил .h через Module 4 → каждая функция стала блоком |

### Принцип матрёшки (рекурсивная вложенность)

Составной модуль (граф) сам становится блоком и может использоваться внутри другого графа на любой глубине вложенности:

```
Уровень 0 (атомарные):
  add.cpp          → add.dqmod          → [add]
  multiply.cpp     → multiply.dqmod     → [multiply]

Уровень 1 (составной из атомарных):
  [add] → [multiply] = calc_graph.dqgraph
                      → calc.cpp  (сгенерирован)
                      → calc.dqmod (автоматически)
                      → [calc] (новый блок в палитре!)

Уровень 2 (составной из составных):
  [calc] → [print] = app_graph.dqgraph
                    → app.cpp  (сгенерирован)
                    → app.dqmod
                    → [app] (ещё один блок!)

  ... и так далее, без ограничения глубины
```

### Двунаправленная навигация

В блочном редакторе двойной клик на составной блок **раскрывает его** — показывает вложенный граф. Это позволяет навигировать по структуре программы как по файловой системе:

- Двойной клик на `[calc]` → видим внутри `[add] → [multiply]`
- Двойной клик на `[add]` → переход в Code Editor к файлу `add.cpp`
- Кнопка «Назад» → возврат к родительскому графу

---

## Формат .dqmod

Модуль описывается JSON-файлом с расширением `.dqmod`.

### Полная спецификация

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "add",
  "display_name": "Сложение",
  "version": "1.0.0",
  "language": "c",
  "category": "math",
  "description": "Складывает два целых числа",
  "author": "DeltaQ",
  "license": "MIT",

  "ports": {
    "input": [
      {
        "name": "a",
        "type": "int",
        "description": "Первое слагаемое",
        "default_value": "0"
      },
      {
        "name": "b",
        "type": "int",
        "description": "Второе слагаемое",
        "default_value": "0"
      }
    ],
    "output": [
      {
        "name": "result",
        "type": "int",
        "description": "Сумма a и b"
      }
    ],
    "execution": {
      "input": ["exec_in"],
      "output": ["exec_out"]
    }
  },

  "origin": "user",
  "source": "modules/math/add.c",
  "header": "modules/math/add.h",
  "graph_source": null,

  "dependencies": [
    {
      "module_id": "uuid-of-dependency",
      "name": "helper",
      "version": ">=1.0.0"
    }
  ],

  "build": {
    "compiler_flags": ["-O2"],
    "linker_flags": [],
    "include_dirs": [],
    "libraries": []
  },

  "testing": {
    "status": "passed",
    "last_tested": "2026-02-28T12:00:00Z",
    "test_inputs": {"a": 5, "b": 3},
    "test_outputs": {"result": 8}
  },

  "metadata": {
    "icon": "icons/math_add.png",
    "color": "#4CAF50",
    "tags": ["math", "arithmetic", "basic"],
    "created": "2026-01-15T10:30:00Z",
    "modified": "2026-02-20T14:00:00Z"
  }
}
```

### Описание полей

| Поле | Тип | Обязательное | Описание |
|------|-----|:------------:|----------|
| `id` | string (UUID) | Да | Уникальный идентификатор модуля |
| `name` | string | Да | Программное имя (латиница, без пробелов) |
| `display_name` | string | Нет | Отображаемое имя (может быть на любом языке) |
| `version` | string (semver) | Да | Версия модуля |
| `language` | string | Да | Язык исходного кода: `"c"`, `"cpp"`, `"mixed"` |
| `category` | string | Нет | Категория для группировки в палитре |
| `description` | string | Нет | Описание функциональности |
| `ports` | object | Да | Определение портов ввода/вывода |
| `origin` | string | Да | Происхождение: `"user"`, `"graph"`, `"library"` |
| `source` | string | Да | Путь к файлу исходного кода (относительно проекта) |
| `header` | string | Нет | Путь к заголовочному файлу |
| `graph_source` | string | Нет | Путь к .dqgraph (только для `origin: "graph"`) |
| `dependencies` | array | Нет | Зависимости от других модулей |
| `build` | object | Нет | Настройки сборки |
| `testing` | object | Нет | Статус тестирования: `"passed"`, `"failed"`, `"untested"`. Модуль доступен в палитре только со статусом `"passed"` |
| `metadata` | object | Нет | Метаданные для IDE |

### Типы портов данных

Поддерживаемые типы данных для портов:

| Тип | C эквивалент | Описание |
|-----|-------------|----------|
| `int` | `int` | Целое число (32 бит) |
| `int8` | `int8_t` | Целое число (8 бит) |
| `int16` | `int16_t` | Целое число (16 бит) |
| `int32` | `int32_t` | Целое число (32 бит) |
| `int64` | `int64_t` | Целое число (64 бит) |
| `uint` | `unsigned int` | Беззнаковое целое |
| `float` | `float` | Число с плавающей точкой (32 бит) |
| `double` | `double` | Число с плавающей точкой (64 бит) |
| `bool` | `bool` | Логическое значение |
| `string` | `const char*` | Строка |
| `pointer` | `void*` | Указатель |
| `array<T>` | `T*` + `size_t` | Массив элементов типа T |
| `struct:Name` | `struct Name` | Пользовательская структура |

### Порты выполнения (Execution Ports)

Порты выполнения определяют порядок вызова модулей в графе. В отличие от портов данных, они не передают значений, а управляют потоком выполнения.

- `exec_in` -- входной порт выполнения (когда модуль должен быть вызван)
- `exec_out` -- выходной порт выполнения (куда передать управление после)
- Для ветвлений можно иметь несколько выходных портов: `exec_true`, `exec_false`

---

## Минимальный пример модуля

### add.dqmod

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "add",
  "version": "1.0.0",
  "language": "c",
  "description": "Adds two integers",
  "ports": {
    "input": [
      {"name": "a", "type": "int"},
      {"name": "b", "type": "int"}
    ],
    "output": [
      {"name": "result", "type": "int"}
    ]
  },
  "source": "modules/math/add.c",
  "header": "modules/math/add.h",
  "dependencies": []
}
```

### add.h

```c
#ifndef DQ_MODULE_ADD_H
#define DQ_MODULE_ADD_H

// @dqmodule name=add version=1.0.0
// @dqport input a:int
// @dqport input b:int
// @dqport output result:int

int dq_add(int a, int b);

#endif
```

### add.c

```c
#include "add.h"

int dq_add(int a, int b) {
    return a + b;
}
```

---

## ModuleRegistry

`ModuleRegistry` -- центральный реестр всех модулей, доступных в проекте. Обеспечивает загрузку, поиск и валидацию модулей.

### Интерфейс

```cpp
class ModuleRegistry : public QObject {
    Q_OBJECT
public:
    explicit ModuleRegistry(ModuleStore* store, QObject* parent = nullptr);

    // Загрузка модулей
    void loadFromDirectory(const QString& path);
    void loadModule(const QString& dqmodPath);
    void unloadModule(const QString& moduleId);
    void reloadModule(const QString& moduleId);

    // Поиск
    Module* findById(const QString& id) const;
    Module* findByName(const QString& name) const;
    QList<Module*> findByCategory(const QString& category) const;
    QList<Module*> findByTag(const QString& tag) const;
    QList<Module*> search(const QString& query) const;
    QList<Module*> allModules() const;

    // Валидация
    ValidationResult validate(const Module& module) const;
    bool checkDependencies(const Module& module) const;
    QList<Module*> resolveDependencies(const Module& module) const;

    // Категории
    QStringList categories() const;
    QMap<QString, QList<Module*>> modulesByCategory() const;

signals:
    void moduleLoaded(const Module* module);
    void moduleUnloaded(const QString& id);
    void moduleUpdated(const Module* module);
    void validationError(const QString& moduleId, const QString& error);

private:
    ModuleStore* m_store;
    QMap<QString, Module*> m_modules;      // id -> Module
    QMap<QString, QString> m_nameIndex;    // name -> id
};
```

### Валидация модулей

При загрузке каждый модуль проходит проверку:

```cpp
struct ValidationResult {
    bool valid;
    QStringList errors;
    QStringList warnings;
};

ValidationResult ModuleRegistry::validate(const Module& module) const {
    ValidationResult result;
    result.valid = true;

    // Обязательные поля
    if (module.id().isEmpty()) {
        result.errors << "Отсутствует id модуля";
        result.valid = false;
    }
    if (module.name().isEmpty()) {
        result.errors << "Отсутствует name модуля";
        result.valid = false;
    }

    // Проверка уникальности id
    if (m_modules.contains(module.id())) {
        result.errors << QString("Модуль с id '%1' уже загружен").arg(module.id());
        result.valid = false;
    }

    // Проверка исходного файла
    if (!QFile::exists(module.sourcePath())) {
        result.errors << QString("Исходный файл не найден: %1").arg(module.sourcePath());
        result.valid = false;
    }

    // Проверка портов
    if (module.inputPorts().isEmpty() && module.outputPorts().isEmpty()) {
        result.warnings << "Модуль не имеет портов данных";
    }

    // Проверка типов портов
    for (const auto& port : module.allPorts()) {
        if (!isValidType(port.type)) {
            result.errors << QString("Неизвестный тип порта: %1").arg(port.type);
            result.valid = false;
        }
    }

    // Проверка зависимостей
    for (const auto& dep : module.dependencies()) {
        if (!m_modules.contains(dep.moduleId)) {
            result.warnings << QString("Зависимость '%1' не найдена").arg(dep.name);
        }
    }

    return result;
}
```

---

## Создание модулей через аннотации

DeltaQ IDE поддерживает создание .dqmod файлов автоматически из исходного кода C/C++ через специальные аннотации в комментариях.

### Синтаксис аннотаций

```c
// @dqmodule name=<имя> [version=<версия>] [category=<категория>] [description=<описание>]
// @dqport input <имя>:<тип> [description=<описание>] [default=<значение>]
// @dqport output <имя>:<тип> [description=<описание>]
// @dqexec input <имя>
// @dqexec output <имя>
// @dqdepends <имя_модуля> [version=<версия>]
```

### Пример с аннотациями

```c
// @dqmodule name=multiply version=1.0.0 category=math description="Умножает два числа"
// @dqport input a:float description="Первый множитель"
// @dqport input b:float description="Второй множитель"
// @dqport output result:float description="Произведение"

float dq_multiply(float a, float b) {
    return a * b;
}
```

### Пример: модуль с ветвлением

```c
// @dqmodule name=compare version=1.0.0 category=logic
// @dqport input a:int
// @dqport input b:int
// @dqport output is_equal:bool
// @dqexec input exec_in
// @dqexec output exec_true
// @dqexec output exec_false

void dq_compare(int a, int b, bool* is_equal,
                void (*exec_true)(void), void (*exec_false)(void)) {
    *is_equal = (a == b);
    if (a == b) {
        exec_true();
    } else {
        exec_false();
    }
}
```

### Парсер аннотаций

```cpp
class AnnotationParser {
public:
    // Парсит исходный файл и возвращает описания модулей
    QList<ModuleDefinition> parse(const QString& sourceFile) const;

    // Генерирует .dqmod файл из определения
    void generateDqmod(const ModuleDefinition& def, const QString& outputPath) const;

    // Генерирует заголовочный файл
    void generateHeader(const ModuleDefinition& def, const QString& outputPath) const;

private:
    ModuleDefinition parseAnnotations(const QStringList& comments) const;
    PortDefinition parsePort(const QString& annotation) const;
    ExecPort parseExecPort(const QString& annotation) const;
};
```

---

## Жизненный цикл модуля

### Этапы жизненного цикла

```
┌────────────┐
│  Создание  │  Пользователь пишет код или импортирует библиотеку
└─────┬──────┘
      ▼
┌────────────┐
│  Парсинг   │  Аннотации извлекаются, создаётся .dqmod файл
└─────┬──────┘
      ▼
┌────────────┐
│  Валидация │  ModuleRegistry проверяет корректность
└─────┬──────┘
      ▼
┌────────────┐
│ Регистрация│  Модуль добавляется в реестр, становится доступен
└─────┬──────┘
      ▼
┌────────────┐
│Использование│  Модуль можно размещать в графах, соединять с другими
└─────┬──────┘
      ▼
┌────────────┐
│  Компиляция│  При сборке проекта код модуля компилируется
└─────┬──────┘
      ▼
┌────────────┐
│ Обновление │  При изменении кода модуль перезагружается
└────────────┘
```

### Детальное описание этапов

#### 1. Создание

Модуль можно создать четырьмя способами:

- **Вручную** (`origin: "user"`): написать .c/.cpp файл с аннотациями и .dqmod файл
- **Через Code Editor** (`origin: "user"`): написать код с аннотациями, IDE автоматически сгенерирует .dqmod
- **Через Block Editor** (`origin: "graph"`): соединить модули в граф → граф автоматически компилируется в .cpp + .dqmod и становится новым модулем в палитре (принцип матрёшки)
- **Через Library Processor** (`origin: "library"`): импортировать функции из существующей библиотеки

#### 2. Парсинг

`AnnotationParser` сканирует исходный файл:

1. Находит комментарии с `@dqmodule`
2. Извлекает определения портов (`@dqport`)
3. Извлекает порты выполнения (`@dqexec`)
4. Извлекает зависимости (`@dqdepends`)
5. Генерирует или обновляет .dqmod файл

#### 3. Валидация

`ModuleRegistry::validate()` проверяет:

- Наличие обязательных полей (id, name, version, language)
- Уникальность id в рамках проекта
- Существование исходных файлов
- Корректность типов портов
- Доступность зависимостей

#### 4. Регистрация

После успешной валидации модуль:

- Добавляется в `ModuleRegistry`
- Появляется в палитре модулей Block Editor
- Индексируется для поиска по имени, категории, тегам

#### 5. Использование

В Block Editor модуль можно:

- Перетащить из палитры на холст (создаётся узел)
- Соединить порты с другими узлами
- Настроить значения по умолчанию для входных портов

#### 6. Компиляция

При сборке проекта:

1. `GraphCompiler` анализирует граф и определяет порядок вызова
2. `CodeGenerator` генерирует C/C++ код, вызывающий функции модулей
3. `BuildSystem` компилирует все исходники модулей и сгенерированный код

#### 7. Обновление

При изменении исходного кода модуля:

1. `FileSystem` обнаруживает изменение файла
2. `AnnotationParser` повторно парсит аннотации
3. `ModuleRegistry::reloadModule()` обновляет модуль
4. Все графы, использующие модуль, уведомляются через сигнал `moduleUpdated`

---

## Составные модули (граф → модуль)

### Автоматическая генерация модуля из графа

Когда пользователь создаёт граф в Block Editor, IDE автоматически:

1. Определяет **внешние порты** графа (незанятые входы/выходы крайних узлов)
2. Генерирует C-функцию, инкапсулирующую логику графа
3. Создаёт `.dqmod` с `origin: "graph"` и ссылкой на `.dqgraph`
4. Регистрирует новый модуль в палитре

### Пример: составной модуль `calc`

#### Граф (calc.dqgraph)

```
Входы графа: a (int), b (int)
[add(a, b)] ──result──▶ sum ──[multiply(sum, 2)]──result──▶ Выход графа: result (int)
```

#### Сгенерированный calc.cpp

```c
#include "add.h"
#include "multiply.h"

// Автоматически сгенерировано из calc.dqgraph
int dq_calc(int a, int b) {
    int sum = dq_add(a, b);
    int result = dq_multiply(sum, 2);
    return result;
}
```

#### Сгенерированный calc.dqmod

```json
{
  "id": "auto-generated-uuid",
  "name": "calc",
  "display_name": "Вычисление",
  "version": "1.0.0",
  "language": "c",
  "category": "custom",
  "origin": "graph",
  "graph_source": "graphs/calc.dqgraph",
  "source": "generated/calc.c",
  "header": "generated/calc.h",
  "ports": {
    "input": [
      {"name": "a", "type": "int"},
      {"name": "b", "type": "int"}
    ],
    "output": [
      {"name": "result", "type": "int"}
    ]
  },
  "dependencies": [
    {"module_id": "add-uuid", "name": "add"},
    {"module_id": "multiply-uuid", "name": "multiply"}
  ]
}
```

### Определение внешних портов графа

Граф определяет свои внешние порты через специальные узлы-маркеры:

| Узел | Назначение |
|------|-----------|
| `GraphInput` | Определяет входной порт составного модуля |
| `GraphOutput` | Определяет выходной порт составного модуля |

```
[GraphInput "a":int] ──▶ a ──[add]──result──▶ [GraphOutput "result":int]
[GraphInput "b":int] ──▶ b ──┘
```

Эти маркеры не генерируют код — они лишь определяют сигнатуру функции `dq_calc(int a, int b) → int`.

### Рекурсивная вложенность

Составной модуль имеет тот же интерфейс, что и атомарный. Поэтому он может использоваться внутри другого графа без ограничений:

```
[calc] — это блок. Его можно:
  ├── Перетащить в любой граф
  ├── Соединить с другими блоками (атомарными или составными)
  ├── Двойной клик → раскрыть и увидеть внутренний граф
  └── Использовать в графе, который сам станет модулем → матрёшка
```

При компиляции проекта все составные модули раскрываются в дерево вызовов функций. Рекурсия (модуль ссылается сам на себя) **запрещена** и проверяется при регистрации.

---

## Формат .dqproj (файл проекта)

Файл проекта объединяет все модули, графы и макеты.

```json
{
  "name": "MyProject",
  "version": "1.0.0",
  "description": "Описание проекта",
  "author": "Имя автора",

  "modules": ["modules/*.dqmod"],
  "graphs": ["graphs/*.dqgraph"],
  "ui_layouts": ["ui/*.dqui"],

  "build": {
    "compiler": "gcc",
    "standard": "c17",
    "output": "build/",
    "optimization": "-O2",
    "warnings": ["-Wall", "-Wextra"],
    "defines": [],
    "include_dirs": ["include/"],
    "library_dirs": [],
    "libraries": ["SDL2"]
  },

  "settings": {
    "auto_generate_dqmod": true,
    "auto_validate_on_save": true,
    "default_module_category": "custom"
  }
}
```

---

## Формат .dqgraph (граф)

```json
{
  "id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "name": "main_graph",
  "description": "Основной граф приложения",
  "entry_node": "n_entry",

  "nodes": [
    {
      "id": "n_entry",
      "type": "entry_point",
      "position": {"x": 50, "y": 200},
      "properties": {}
    },
    {
      "id": "n1",
      "module_id": "550e8400-e29b-41d4-a716-446655440000",
      "position": {"x": 300, "y": 150},
      "properties": {
        "a": {"value": "10", "connected": false},
        "b": {"value": null, "connected": true}
      }
    },
    {
      "id": "n2",
      "module_id": "660e8400-e29b-41d4-a716-446655440001",
      "position": {"x": 300, "y": 350},
      "properties": {}
    },
    {
      "id": "n3",
      "module_id": "770e8400-e29b-41d4-a716-446655440002",
      "position": {"x": 600, "y": 200},
      "properties": {}
    }
  ],

  "connections": [
    {
      "id": "c1",
      "type": "execution",
      "from": {"node": "n_entry", "port": "exec_out"},
      "to": {"node": "n1", "port": "exec_in"}
    },
    {
      "id": "c2",
      "type": "data",
      "from": {"node": "n1", "port": "result"},
      "to": {"node": "n3", "port": "value"}
    },
    {
      "id": "c3",
      "type": "data",
      "from": {"node": "n2", "port": "result"},
      "to": {"node": "n1", "port": "b"}
    },
    {
      "id": "c4",
      "type": "execution",
      "from": {"node": "n1", "port": "exec_out"},
      "to": {"node": "n3", "port": "exec_in"}
    }
  ],

  "metadata": {
    "zoom": 1.0,
    "scroll_x": 0,
    "scroll_y": 0,
    "created": "2026-01-20T08:00:00Z",
    "modified": "2026-02-25T16:30:00Z"
  }
}
```

---

## Стандартные модули

DeltaQ IDE поставляется с набором стандартных модулей:

### Категория: math

| Модуль | Входы | Выходы | Описание |
|--------|-------|--------|----------|
| `add` | a:int, b:int | result:int | Сложение |
| `subtract` | a:int, b:int | result:int | Вычитание |
| `multiply` | a:float, b:float | result:float | Умножение |
| `divide` | a:float, b:float | result:float | Деление |
| `modulo` | a:int, b:int | result:int | Остаток от деления |
| `abs` | value:int | result:int | Абсолютное значение |

### Категория: logic

| Модуль | Входы | Выходы | Описание |
|--------|-------|--------|----------|
| `and` | a:bool, b:bool | result:bool | Логическое И |
| `or` | a:bool, b:bool | result:bool | Логическое ИЛИ |
| `not` | value:bool | result:bool | Логическое НЕ |
| `compare` | a:int, b:int | is_equal:bool | Сравнение |
| `branch` | condition:bool | -- | Ветвление (exec_true/exec_false) |

### Категория: io

| Модуль | Входы | Выходы | Описание |
|--------|-------|--------|----------|
| `print` | text:string | -- | Вывод строки |
| `read_int` | -- | value:int | Чтение целого числа |
| `read_string` | -- | value:string | Чтение строки |
| `file_read` | path:string | content:string | Чтение файла |
| `file_write` | path:string, content:string | success:bool | Запись в файл |

### Категория: string

| Модуль | Входы | Выходы | Описание |
|--------|-------|--------|----------|
| `concat` | a:string, b:string | result:string | Конкатенация |
| `length` | text:string | len:int | Длина строки |
| `substring` | text:string, start:int, len:int | result:string | Подстрока |
| `to_upper` | text:string | result:string | В верхний регистр |
| `to_lower` | text:string | result:string | В нижний регистр |
