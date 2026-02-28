# DeltaQ IDE - Детальная архитектура

## Общая концепция

Архитектура DeltaQ IDE построена на принципе **четырёхслойной модели** с чётким разделением ответственности. Все слои взаимодействуют через **CommandBus** -- централизованную шину команд, обеспечивающую слабую связанность компонентов и полную поддержку отмены/повтора операций.

---

## 4-слойная модель

```
┌─────────────────────────────────────────────────────────────┐
│                      PRESENTATION                            │
│   Qt Widgets  │  QScintilla  │  Qt Graphics View             │
├─────────────────────────────────────────────────────────────┤
│                      APPLICATION                             │
│  SessionManager │ ActionManager │ CommandBus │ UndoManager   │
├─────────────────────────────────────────────────────────────┤
│                     CORE SERVICES                            │
│  ModuleRegistry │ CodeGenerator │ ProjectMgr │ BuildSystem   │
│  LanguageBackend(C) │ LanguageBackend(Py) │ ... (pluggable) │
├─────────────────────────────────────────────────────────────┤
│                         DATA                                 │
│  FileSystem │ ModuleStore │ GraphStore │ UILayoutStore        │
└─────────────────────────────────────────────────────────────┘
```

### Слой 1: Presentation (Представление)

Отвечает за отображение пользовательского интерфейса и обработку пользовательского ввода.

| Компонент | Технология | Назначение |
|-----------|-----------|-----------|
| **MainWindow** | Qt Widgets | Главное окно, панели, меню |
| **CodeEditorWidget** | QScintilla | Текстовый редактор кода |
| **BlockEditorWidget** | Qt Graphics View | Визуальный редактор графов |
| **UIDesignerWidget** | Qt Widgets + Custom | Дизайнер интерфейсов |
| **PropertyPanel** | Qt Widgets | Панель свойств выбранного элемента |
| **ProjectExplorer** | QTreeView | Дерево проекта |
| **OutputPanel** | QTextEdit | Вывод компиляции и отладки |

**Правила слоя Presentation:**

- НЕ содержит бизнес-логики
- Все действия пользователя преобразуются в команды (Command) и отправляются в CommandBus
- Подписывается на события для обновления отображения
- Виджеты не общаются друг с другом напрямую

### Слой 2: Application (Приложение)

Координирует работу приложения, управляет сессиями, командами и отменой операций.

#### SessionManager

Управляет жизненным циклом приложения и сессий пользователя.

```cpp
class SessionManager {
public:
    void openProject(const QString& path);
    void closeProject();
    void saveSession();
    void restoreSession();

    Project* currentProject() const;
    QList<EditorTab*> openTabs() const;

signals:
    void projectOpened(Project* project);
    void projectClosed();
    void sessionRestored();
};
```

#### ActionManager

Регистрирует и управляет пользовательскими действиями (actions) -- пунктами меню, горячими клавишами, кнопками панелей инструментов.

```cpp
class ActionManager {
public:
    void registerAction(const QString& id, QAction* action);
    void setShortcut(const QString& id, const QKeySequence& shortcut);
    QAction* action(const QString& id) const;

    // Группы действий для контекстного включения/выключения
    void enableGroup(const QString& group);
    void disableGroup(const QString& group);
};
```

#### CommandBus

Центральный компонент архитектуры. Реализует паттерн **Command** для всех мутирующих операций в системе.

**Потокобезопасность:** CommandBus защищён мьютексом, т.к. может вызываться из фоновых потоков (сборка, парсинг). Команды из фоновых потоков маршрутизируются в основной поток через `QMetaObject::invokeMethod`.

```cpp
class CommandBus : public QObject {
    Q_OBJECT
public:
    static CommandBus* instance();

    // Выполнить команду (добавляется в UndoManager автоматически)
    // Потокобезопасно: если вызвано из не-GUI потока, команда ставится в очередь
    void execute(std::unique_ptr<Command> cmd);

    // Выполнить без записи в историю (для read-only операций)
    void executeNoHistory(std::unique_ptr<Command> cmd);

    // Пакетное выполнение (группа команд как одна операция отмены)
    void beginMacro(const QString& description);
    void endMacro();

signals:
    void commandExecuted(const Command* cmd);
    void commandUndone(const Command* cmd);
    void commandRedone(const Command* cmd);

private:
    UndoManager* m_undoManager;
    QMutex m_mutex;  // Защита от одновременного доступа из разных потоков
};
```

#### UndoManager

Управляет стеком отмены/повтора операций.

```cpp
class UndoManager {
public:
    void push(std::unique_ptr<Command> cmd);
    void undo();
    void redo();

    bool canUndo() const;
    bool canRedo() const;

    QString undoText() const;
    QString redoText() const;

    void clear();
    void setClean();  // Отметить текущее состояние как "сохранённое"
    bool isClean() const;

    // Макро-команды (группировка)
    void beginMacro(const QString& text);
    void endMacro();

private:
    QStack<std::unique_ptr<Command>> m_undoStack;
    QStack<std::unique_ptr<Command>> m_redoStack;
    int m_cleanIndex;
};
```

### Слой 3: Core Services (Основные сервисы)

Реализует бизнес-логику -- управление модулями, генерацию кода, сборку проектов. Все компоненты проектируются **язык-агностично**: логика ядра работает с абстрактными модулями и IR, а конкретный язык подключается через интерфейс `LanguageBackend`.

| Компонент | Назначение |
|-----------|-----------|
| **ModuleRegistry** | Реестр всех загруженных .dqmod модулей (язык-агностичный) |
| **CodeGenerator** | Оркестратор генерации: делегирует работу активному LanguageBackend |
| **LanguageBackend** | **Интерфейс бэкенда языка** (C, Python, Rust и т.д.) — см. раздел ниже |
| **ProjectManager** | Управление файлами проекта (.dqproj), включая настройку целевого языка |
| **BuildSystem** | Запуск сборки через подключаемые **тулчейны** (CMake+GCC, pip, cargo...) |
| **LSPClient** | Клиент Language Server Protocol (clangd, pyright, rust-analyzer...) |
| **DebugManager** | Управление сессиями отладки через подключаемые адаптеры (GDB, LLDB, debugpy...) |
| **GraphCompiler** | Компиляция графов: Graph → **IR** (язык-агностичный) → LanguageBackend → код |
| **LibraryParser** | Парсинг библиотек через подключаемые **парсеры** (libclang, tree-sitter...) |

### Слой 4: Data (Данные)

Отвечает за хранение и персистентность данных.

| Компонент | Назначение |
|-----------|-----------|
| **FileSystem** | Абстракция файловой системы, отслеживание изменений |
| **ModuleStore** | Хранилище .dqmod модулей |
| **GraphStore** | Хранилище .dqgraph графов |
| **UILayoutStore** | Хранилище .dqui макетов интерфейса |

```cpp
class ModuleStore {
public:
    void load(const QString& path);
    void save(const Module& module);
    void remove(const QString& moduleId);

    Module* findById(const QString& id) const;
    Module* findByName(const QString& name) const;
    QList<Module*> findByCategory(const QString& category) const;
    QList<Module*> allModules() const;

signals:
    void moduleAdded(const Module* module);
    void moduleRemoved(const QString& id);
    void moduleUpdated(const Module* module);
};
```

---

## Мультиязычная архитектура

### Принцип

DeltaQ IDE проектируется как **язык-агностичная платформа**. Ядро IDE (CommandBus, ModuleRegistry, GraphCompiler, UI) не зависит от конкретного языка программирования. Вся язык-специфичная логика инкапсулирована в подключаемых бэкендах.

Первым реализуется бэкенд **C/C++**, но архитектура позволяет добавлять Python, Rust, Go и другие языки без изменения ядра.

### Матрица: Язык × Тип приложения

```
                │  Console  │  SDL2 GUI  │  Custom  │
────────────────┼───────────┼────────────┼──────────┤
 C/C++          │  ✓ (v1)   │  ✓ (v1)    │  ✓ (v1)  │
 Python         │  ✓ (v2)   │  ✓ (v2)*   │  ✓ (v2)  │
 Rust           │  ✓ (v3)   │  ✓ (v3)*   │  ✓ (v3)  │
 Go             │  ✓ (v3)   │  —         │  ✓ (v3)  │

 * SDL2 в Python — через ctypes/cffi, в Rust — через sdl2 crate
```

### Интерфейс LanguageBackend

```cpp
class LanguageBackend {
public:
    virtual ~LanguageBackend() = default;

    // Идентификация
    virtual QString id() const = 0;              // "c", "python", "rust"
    virtual QString displayName() const = 0;     // "C/C++", "Python 3", "Rust"
    virtual QString fileExtension() const = 0;   // ".c", ".py", ".rs"
    virtual QStringList supportedVersions() const = 0;

    // === Генерация кода ===

    // Генерация исходного кода функции из IR
    virtual QString emitFunction(const IRFunction& func) = 0;

    // Генерация точки входа (main)
    virtual QString emitEntryPoint(const IR& ir, const ProjectConfig& config) = 0;

    // Генерация заголовков / деклараций (для C — .h, для Python — нет, для Rust — pub fn)
    virtual QString emitDeclaration(const IRFunction& func) = 0;

    // Маппинг типов IR на типы языка
    virtual QString mapType(const IRType& type) const = 0;

    // Маппинг вызова функции
    virtual QString emitFunctionCall(const IRCall& call) const = 0;

    // Маппинг управляющих конструкций (if, for, while)
    virtual QString emitControlFlow(const IRControlFlow& cf) const = 0;

    // === Система сборки ===

    // Генерация файла сборки (CMakeLists.txt, setup.py, Cargo.toml...)
    virtual QString generateBuildFile(const Project& project) const = 0;

    // Команда сборки
    virtual QStringList buildCommands(const Project& project) const = 0;

    // Команда запуска
    virtual QStringList runCommands(const Project& project) const = 0;

    // === LSP / Тулинг ===

    // Путь к LSP-серверу для этого языка
    virtual QString lspServerPath() const = 0;
    virtual QStringList lspServerArgs() const = 0;

    // Путь к отладчику
    virtual QString debuggerPath() const = 0;
    virtual QStringList debuggerArgs(const Project& project) const = 0;

    // === Парсинг библиотек ===

    // Парсер заголовков / деклараций для импорта внешних библиотек
    virtual std::unique_ptr<LibParser> createLibParser() const = 0;
};
```

### Реестр бэкендов

```cpp
class LanguageBackendRegistry {
public:
    static LanguageBackendRegistry* instance();

    void registerBackend(std::unique_ptr<LanguageBackend> backend);
    LanguageBackend* backend(const QString& id) const;      // "c", "python", "rust"
    QStringList availableLanguages() const;

    // Язык по умолчанию (из настроек проекта)
    LanguageBackend* defaultBackend() const;
    void setDefaultBackend(const QString& id);

private:
    QMap<QString, std::unique_ptr<LanguageBackend>> m_backends;
    QString m_defaultId = "c";
};
```

### Абстрактная система типов IR

IR (промежуточное представление) не привязано к типам конкретного языка. Каждый `LanguageBackend` выполняет маппинг IR-типов на свои нативные типы.

```cpp
// IR-типы (язык-агностичные)
enum class IRTypeKind {
    Void,
    Integer,     // int8, int16, int32, int64
    Float,       // float32, float64
    Boolean,
    String,      // строковый тип (char* в C, str в Python, String в Rust)
    Array,       // массив с типом элемента
    Struct,      // пользовательская структура
    Pointer,     // указатель (в C — *, в Python/Rust — маппится иначе)
    Any          // динамический тип (для Python; в C → void*)
};

struct IRType {
    IRTypeKind kind;
    int bitWidth;              // для Integer/Float: 8/16/32/64
    IRType* elementType;       // для Array, Pointer
    QString structName;        // для Struct
};
```

Примеры маппинга:

| IR-тип | C | Python | Rust |
|--------|---|--------|------|
| `Integer(32)` | `int` | `int` | `i32` |
| `Float(64)` | `double` | `float` | `f64` |
| `Boolean` | `int` | `bool` | `bool` |
| `String` | `const char*` | `str` | `&str` |
| `Array(Integer(32))` | `int*` + len | `list[int]` | `Vec<i32>` |
| `Struct("Point")` | `struct Point` | `Point` (dataclass) | `Point` (struct) |

### Конвейер генерации (мультиязычный)

```
┌──────────────┐     ┌──────────────┐     ┌────────────────────────┐
│   .dqgraph   │────▶│  GraphCompiler│───▶│   IR (язык-агностичный) │
│   .dqui      │     │              │     │   IRFunction, IRType    │
│   .dqmod     │     │              │     │   IRControlFlow, IRCall │
└──────────────┘     └──────────────┘     └───────────┬────────────┘
                                                       │
                              ┌─────────────────────────┤
                              │                         │
                    ┌─────────▼──────┐        ┌────────▼────────┐
                    │ CLanguageBackend│        │PythonBackend    │  ...
                    │ emitFunction()  │        │emitFunction()   │
                    │ mapType()       │        │mapType()        │
                    └────────┬───────┘        └────────┬────────┘
                             │                         │
                       ┌─────▼─────┐            ┌──────▼────┐
                       │  .c / .h  │            │  .py      │
                       └─────┬─────┘            └──────┬────┘
                             │                         │
                       ┌─────▼─────┐            ┌──────▼────┐
                       │ CMake+GCC │            │ pip/python│
                       └───────────┘            └───────────┘
```

---

## Паттерн Command

Каждая мутирующая операция в системе оформляется как объект команды с методами `execute()` и `undo()`.

### Базовый класс Command

```cpp
class Command {
public:
    virtual ~Command() = default;

    virtual void execute() = 0;
    virtual void undo() = 0;

    // Описание для отображения в меню Edit -> Undo/Redo
    virtual QString description() const = 0;

    // Можно ли объединить с предыдущей командой того же типа
    // (например, последовательные вводы символов в редакторе)
    virtual bool mergeWith(const Command* other) { return false; }
    virtual int id() const { return -1; }
};
```

### Примеры команд

```cpp
// Добавление узла в граф
class AddNodeCommand : public Command {
public:
    AddNodeCommand(GraphStore* store, const QString& graphId,
                   const NodeData& nodeData)
        : m_store(store), m_graphId(graphId), m_nodeData(nodeData) {}

    void execute() override {
        m_nodeId = m_store->addNode(m_graphId, m_nodeData);
    }

    void undo() override {
        m_store->removeNode(m_graphId, m_nodeId);
    }

    QString description() const override {
        return QString("Добавить узел '%1'").arg(m_nodeData.name);
    }

private:
    GraphStore* m_store;
    QString m_graphId;
    NodeData m_nodeData;
    QString m_nodeId;  // ID созданного узла (для undo)
};

// Изменение свойства модуля
class ChangeModulePropertyCommand : public Command {
public:
    ChangeModulePropertyCommand(ModuleStore* store, const QString& moduleId,
                                 const QString& property, const QVariant& newValue)
        : m_store(store), m_moduleId(moduleId),
          m_property(property), m_newValue(newValue) {
        m_oldValue = m_store->findById(moduleId)->property(property);
    }

    void execute() override {
        m_store->setProperty(m_moduleId, m_property, m_newValue);
    }

    void undo() override {
        m_store->setProperty(m_moduleId, m_property, m_oldValue);
    }

    QString description() const override {
        return QString("Изменить %1").arg(m_property);
    }

private:
    ModuleStore* m_store;
    QString m_moduleId;
    QString m_property;
    QVariant m_oldValue;
    QVariant m_newValue;
};

// Соединение портов в графе
class ConnectPortsCommand : public Command {
public:
    ConnectPortsCommand(GraphStore* store, const QString& graphId,
                        const ConnectionData& connection)
        : m_store(store), m_graphId(graphId), m_connection(connection) {}

    void execute() override {
        m_connectionId = m_store->addConnection(m_graphId, m_connection);
    }

    void undo() override {
        m_store->removeConnection(m_graphId, m_connectionId);
    }

    QString description() const override {
        return "Соединить порты";
    }

private:
    GraphStore* m_store;
    QString m_graphId;
    ConnectionData m_connection;
    QString m_connectionId;
};
```

---

## Поток данных

### Пример: пользователь добавляет узел в граф

```
[Пользователь]
    │ drag & drop модуля на холст
    ▼
[BlockEditorWidget] (Presentation)
    │ создаёт AddNodeCommand
    ▼
[CommandBus] (Application)
    │ передаёт в UndoManager
    │ вызывает command->execute()
    ▼
[GraphStore] (Data)
    │ добавляет узел в хранилище
    │ emit nodeAdded(...)
    ▼
[BlockEditorWidget] (Presentation)
    │ получает сигнал, отрисовывает узел
    ▼
[Пользователь видит новый узел]
```

### Пример: пользователь нажимает Undo

```
[Пользователь]
    │ Ctrl+Z
    ▼
[ActionManager] (Application)
    │ вызывает CommandBus::undo()
    ▼
[UndoManager] (Application)
    │ берёт последнюю команду из стека
    │ вызывает command->undo()
    ▼
[GraphStore] (Data)
    │ удаляет узел
    │ emit nodeRemoved(...)
    ▼
[BlockEditorWidget] (Presentation)
    │ получает сигнал, удаляет узел с холста
    ▼
[Пользователь видит результат отмены]
```

### Пример: сборка проекта

```
[Пользователь]
    │ нажимает Build
    ▼
[ActionManager] (Application)
    │ создаёт BuildProjectCommand
    ▼
[CommandBus] (Application)
    │ executeNoHistory() — сборка не подлежит отмене
    ▼
[BuildSystem] (Core Services)
    │ 1. Определяет целевой язык из project.language ("c", "python", "rust")
    │ 2. Получает LanguageBackend из реестра
    │ 3. Вызывает CodeGenerator → IR → backend.emitFunction()
    │ 4. Генерирует файл сборки: backend.generateBuildFile()
    │ 5. Запускает сборку: backend.buildCommands()
    │ emit buildOutput(line)
    │ emit buildFinished(success)
    ▼
[OutputPanel] (Presentation)
    │ отображает вывод компиляции
```

---

## Взаимодействие модулей

Ключевое правило: **модули НЕ общаются друг с другом напрямую**. Вся коммуникация идёт через CommandBus и сигналы хранилищ данных.

```
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
│  Code    │     │  Block   │     │   UI     │     │  Lib     │
│  Editor  │     │  Editor  │     │ Designer │     │ Processor│
└────┬─────┘     └────┬─────┘     └────┬─────┘     └────┬─────┘
     │                │                │                │
     └────────────────┴────────┬───────┴────────────────┘
                               │
                        ┌──────┴──────┐
                        │  CommandBus │
                        └──────┬──────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
       ┌──────┴──────┐ ┌──────┴──────┐ ┌───────┴─────┐
       │ ModuleStore │ │ GraphStore  │ │UILayoutStore│
       └─────────────┘ └─────────────┘ └─────────────┘
```

### Пример межмодульного взаимодействия

Когда пользователь редактирует исходный код модуля в Code Editor и сохраняет:

1. **Code Editor** -> `SaveModuleSourceCommand` -> **CommandBus**
2. **CommandBus** -> выполняет -> **ModuleStore** обновляет файл
3. **ModuleStore** -> emit `moduleUpdated(module)`
4. **Block Editor** -> получает сигнал -> обновляет отображение узлов, использующих этот модуль
5. **UI Designer** -> получает сигнал -> обновляет связанные обработчики событий

---

## Инициализация приложения

```cpp
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // 1. Инициализация слоя Data
    FileSystem fileSystem;
    ModuleStore moduleStore(&fileSystem);
    GraphStore graphStore(&fileSystem);
    UILayoutStore uiLayoutStore(&fileSystem);

    // 2. Инициализация языковых бэкендов
    auto& langRegistry = *LanguageBackendRegistry::instance();
    langRegistry.registerBackend(std::make_unique<CLanguageBackend>());
    // В будущем:
    // langRegistry.registerBackend(std::make_unique<PythonBackend>());
    // langRegistry.registerBackend(std::make_unique<RustBackend>());

    // 3. Инициализация Core Services
    ModuleRegistry registry(&moduleStore);
    CodeGenerator codeGen(&registry, &langRegistry);
    ProjectManager projectMgr(&fileSystem);
    BuildSystem buildSystem(&codeGen, &langRegistry);

    // 4. Инициализация Application
    CommandBus commandBus;
    UndoManager undoManager;
    commandBus.setUndoManager(&undoManager);
    SessionManager sessionMgr(&projectMgr);
    ActionManager actionMgr;

    // 5. Инициализация Presentation
    MainWindow mainWindow;
    mainWindow.init(&commandBus, &actionMgr, &sessionMgr,
                    &moduleStore, &graphStore, &uiLayoutStore);

    mainWindow.show();
    return app.exec();
}
```

---

## Принципы расширяемости

1. **Новый язык программирования** -- реализовать интерфейс `LanguageBackend` и зарегистрировать в `LanguageBackendRegistry`. Ядро IDE не требует изменений
2. **Новый тип команды** -- достаточно создать класс, наследующий `Command`, без изменения существующего кода
3. **Новый виджет** -- подключается к CommandBus и слушает сигналы хранилищ
4. **Новый формат экспорта** -- реализуется как дополнительный бэкенд CodeGenerator
5. **Новый тулчейн** -- добавляется в BuildSystem через LanguageBackend
6. **Новый парсер библиотек** -- реализуется через `LanguageBackend::createLibParser()`

Такая архитектура обеспечивает:
- **Язык-агностичность**: ядро работает с абстрактным IR, конкретный язык — плагин
- Слабую связанность (loose coupling) между компонентами
- Лёгкость тестирования (каждый слой тестируется отдельно)
- Полноценную поддержку undo/redo для всех операций
- Возможность расширения без модификации существующего кода (Open/Closed Principle)

### Пример: добавление поддержки Python

Для добавления нового языка необходимо реализовать один класс:

```cpp
class PythonBackend : public LanguageBackend {
public:
    QString id() const override { return "python"; }
    QString displayName() const override { return "Python 3"; }
    QString fileExtension() const override { return ".py"; }

    QString mapType(const IRType& type) const override {
        switch (type.kind) {
            case IRTypeKind::Integer: return "int";
            case IRTypeKind::Float:   return "float";
            case IRTypeKind::String:  return "str";
            case IRTypeKind::Boolean: return "bool";
            case IRTypeKind::Array:
                return QString("list[%1]").arg(mapType(*type.elementType));
            // ...
        }
    }

    QString emitFunction(const IRFunction& func) override {
        QString code;
        code += QString("def %1(%2)").arg(func.name, emitParams(func.params));
        if (func.returnType.kind != IRTypeKind::Void)
            code += QString(" -> %1").arg(mapType(func.returnType));
        code += ":\n";
        for (const auto& stmt : func.body)
            code += "    " + emitStatement(stmt) + "\n";
        return code;
    }

    QString generateBuildFile(const Project& project) const override {
        // Генерирует requirements.txt и setup.py/pyproject.toml
    }

    QStringList buildCommands(const Project& project) const override {
        return {"pip", "install", "-r", "requirements.txt"};
    }

    QStringList runCommands(const Project& project) const override {
        return {"python3", "main.py"};
    }

    QString lspServerPath() const override { return "pyright"; }
    QString debuggerPath() const override { return "debugpy"; }

    std::unique_ptr<LibParser> createLibParser() const override {
        return std::make_unique<PythonStubParser>();  // парсит .pyi стабы
    }
};
```

После регистрации:
```cpp
langRegistry.registerBackend(std::make_unique<PythonBackend>());
```

Пользователь может выбрать Python в настройках проекта, и весь конвейер (генерация, сборка, LSP, отладка, импорт библиотек) автоматически переключается на Python-инструменты.

---

## Тестирование модулей

Каждый модуль — это завершённый элемент («кирпичик»). Прежде чем модуль можно использовать в графе, он должен быть **протестирован отдельно**. Для этого в редакторе кода предусмотрен встроенный **модуль тестирования** — пошаговый эмулятор.

### Конвейер: написал → протестировал → разрешил использовать

```
[Код модуля (.cpp)]
    │ @dqmodule аннотация
    ▼
[AnnotationParser]
    │ создаёт .dqmod (статус: "не протестирован")
    ▼
[Модуль тестирования]
    │ Пользователь задаёт входные данные
    │ Пошаговое выполнение (эмулятор)
    │ Проверка выходных данных
    ▼
[Результат: PASS / FAIL]
    │
    ├── PASS → модуль получает статус "протестирован" → доступен в палитре блоков
    └── FAIL → модуль остаётся недоступным, ошибки показаны пользователю
```

### ModuleTester

```cpp
class ModuleTester : public QObject {
    Q_OBJECT
public:
    // Установить входные значения для тестирования
    void setInput(const QString& portName, const QVariant& value);

    // Запустить модуль с заданными входами
    TestResult run(const Module& module);

    // Пошаговое выполнение (для отладки модуля)
    void stepInto(const Module& module);
    void stepOver();
    void stepOut();

    // Результаты
    QVariant output(const QString& portName) const;
    bool passed() const;
    QStringList errors() const;

signals:
    void testStarted();
    void testFinished(bool success);
    void stepCompleted(int line, const QVariantMap& variables);
};
```

---

## Многопоточность в графах

Графическая система упрощает разработку, но не ограничивает возможности. Для параллельного выполнения предусмотрены специальные блоки:

### Блок «Независимый поток» (ThreadModule)

Модуль или блок из модулей может быть помечен как выполняющийся в отдельном потоке. Обмен данными с основным потоком — через **порты синхронизации**.

```
┌──────────────────────────────────────────────────────────┐
│  Основной поток (Main Thread)                            │
│                                                           │
│  [read_data] → [process] → [display]                     │
│                    │                                      │
│              sync_port_out                                │
│                    │                                      │
│  ──────────────────┼─────────────────────────────────── │
│                    │  ThreadModule                        │
│                    ▼                                      │
│  [heavy_computation] → [result] → sync_port_in → [merge]│
│                                                           │
│  Отдельный поток (Worker Thread)                         │
└──────────────────────────────────────────────────────────┘
```

Порты синхронизации обеспечивают:
- **Передачу данных** между потоками (очередь сообщений)
- **Блокирующее ожидание** (sync_port_in ждёт результат)
- **Неблокирующую проверку** (poll — есть ли данные?)

Это аналог `std::thread` + `std::mutex` в C, но визуально на графе пользователь видит два потока выполнения и точки синхронизации.
