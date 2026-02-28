# DeltaQ IDE - Структура проекта

## Общая структура репозитория IDE

```
IDE_DeltaQ/
├── CMakeLists.txt                  # Корневой CMake файл проекта IDE
├── README.md                       # Описание проекта
├── LICENSE                         # Лицензия
│
├── docs/                           # Документация
│   ├── plan/                       # Плановая документация (этот набор файлов)
│   │   ├── 00_general_plan.md
│   │   ├── 01_architecture.md
│   │   ├── 02_module_system.md
│   │   ├── 03_module1_code_editor.md
│   │   ├── 04_module2_block_editor.md
│   │   ├── 05_module3_ui_designer.md
│   │   ├── 06_module4_lib_processor.md
│   │   ├── 07_codegen.md
│   │   ├── 08_project_structure.md
│   │   └── 09_roadmap.md
│   ├── api/                        # API-документация (Doxygen)
│   ├── user_guide/                 # Руководство пользователя
│   └── dev_guide/                  # Руководство разработчика
│
├── src/                            # Исходный код IDE
│   ├── main.cpp                    # Точка входа приложения
│   ├── app/                        # Слой Application
│   │   ├── CMakeLists.txt
│   │   ├── SessionManager.h
│   │   ├── SessionManager.cpp
│   │   ├── ActionManager.h
│   │   ├── ActionManager.cpp
│   │   ├── CommandBus.h
│   │   ├── CommandBus.cpp
│   │   ├── UndoManager.h
│   │   ├── UndoManager.cpp
│   │   ├── Command.h              # Базовый класс команд
│   │   └── commands/               # Конкретные команды
│   │       ├── AddNodeCommand.h
│   │       ├── AddNodeCommand.cpp
│   │       ├── RemoveNodeCommand.h
│   │       ├── RemoveNodeCommand.cpp
│   │       ├── ConnectPortsCommand.h
│   │       ├── ConnectPortsCommand.cpp
│   │       ├── MoveNodeCommand.h
│   │       ├── MoveNodeCommand.cpp
│   │       ├── ChangePropertyCommand.h
│   │       ├── ChangePropertyCommand.cpp
│   │       ├── AddWidgetCommand.h
│   │       ├── AddWidgetCommand.cpp
│   │       └── ...
│   │
│   ├── core/                       # Слой Core Services
│   │   ├── CMakeLists.txt
│   │   ├── ModuleRegistry.h
│   │   ├── ModuleRegistry.cpp
│   │   ├── ProjectManager.h
│   │   ├── ProjectManager.cpp
│   │   ├── codegen/                # Генерация кода
│   │   │   ├── CodeGenerator.h
│   │   │   ├── CodeGenerator.cpp
│   │   │   ├── CodeGenBackend.h    # Интерфейс бэкенда
│   │   │   ├── ConsoleBackend.h
│   │   │   ├── ConsoleBackend.cpp
│   │   │   ├── SDL2Backend.h
│   │   │   ├── SDL2Backend.cpp
│   │   │   ├── SDL2CodeGenerator.h
│   │   │   ├── SDL2CodeGenerator.cpp
│   │   │   ├── IR.h                # Промежуточное представление
│   │   │   └── IR.cpp
│   │   ├── compiler/               # Компилятор графов
│   │   │   ├── GraphCompiler.h
│   │   │   ├── GraphCompiler.cpp
│   │   │   ├── TopologicalSort.h
│   │   │   ├── TopologicalSort.cpp
│   │   │   ├── IRGenerator.h
│   │   │   ├── IRGenerator.cpp
│   │   │   ├── CCodeEmitter.h
│   │   │   └── CCodeEmitter.cpp
│   │   ├── build/                  # Система сборки
│   │   │   ├── BuildSystem.h
│   │   │   ├── BuildSystem.cpp
│   │   │   ├── CMakeDriver.h
│   │   │   ├── CMakeDriver.cpp
│   │   │   ├── CompilerRunner.h
│   │   │   ├── CompilerRunner.cpp
│   │   │   └── BuildOutputParser.h
│   │   ├── debug/                  # Отладка
│   │   │   ├── DebugManager.h
│   │   │   ├── DebugManager.cpp
│   │   │   ├── GDBDriver.h
│   │   │   ├── GDBDriver.cpp
│   │   │   ├── LLDBDriver.h
│   │   │   ├── LLDBDriver.cpp
│   │   │   └── GraphDebugger.h
│   │   ├── lsp/                    # LSP-клиент
│   │   │   ├── LSPClient.h
│   │   │   ├── LSPClient.cpp
│   │   │   ├── LSPProtocol.h       # Типы данных протокола
│   │   │   └── LSPProtocol.cpp
│   │   ├── parser/                 # Парсеры
│   │   │   ├── AnnotationParser.h
│   │   │   ├── AnnotationParser.cpp
│   │   │   ├── LibclangParser.h
│   │   │   ├── LibclangParser.cpp
│   │   │   ├── LibraryDecomposer.h
│   │   │   ├── LibraryDecomposer.cpp
│   │   │   ├── WrapperGenerator.h
│   │   │   ├── WrapperGenerator.cpp
│   │   │   └── TemplateInstantiator.h
│   │   └── layout/                 # Система компоновки UI
│   │       ├── LayoutEngine.h
│   │       └── LayoutEngine.cpp
│   │
│   ├── data/                       # Слой Data
│   │   ├── CMakeLists.txt
│   │   ├── FileSystem.h
│   │   ├── FileSystem.cpp
│   │   ├── ModuleStore.h
│   │   ├── ModuleStore.cpp
│   │   ├── GraphStore.h
│   │   ├── GraphStore.cpp
│   │   ├── UILayoutStore.h
│   │   ├── UILayoutStore.cpp
│   │   ├── models/                 # Модели данных
│   │   │   ├── Project.h
│   │   │   ├── Project.cpp
│   │   │   ├── Module.h
│   │   │   ├── Module.cpp
│   │   │   ├── Graph.h
│   │   │   ├── Graph.cpp
│   │   │   ├── Node.h
│   │   │   ├── Node.cpp
│   │   │   ├── Connection.h
│   │   │   ├── Connection.cpp
│   │   │   ├── Port.h
│   │   │   ├── Port.cpp
│   │   │   ├── UILayout.h
│   │   │   ├── UILayout.cpp
│   │   │   ├── Widget.h
│   │   │   └── Widget.cpp
│   │   └── serialization/          # Сериализация/десериализация
│   │       ├── JsonSerializer.h
│   │       ├── JsonSerializer.cpp
│   │       ├── DqmodSerializer.h
│   │       ├── DqmodSerializer.cpp
│   │       ├── DqgraphSerializer.h
│   │       ├── DqgraphSerializer.cpp
│   │       ├── DquiSerializer.h
│   │       ├── DquiSerializer.cpp
│   │       ├── DqprojSerializer.h
│   │       └── DqprojSerializer.cpp
│   │
│   └── ui/                         # Слой Presentation
│       ├── CMakeLists.txt
│       ├── MainWindow.h
│       ├── MainWindow.cpp
│       ├── MainWindow.ui           # Qt Designer файл (опционально)
│       ├── code_editor/            # Модуль 1: Редактор кода
│       │   ├── CodeEditorWidget.h
│       │   ├── CodeEditorWidget.cpp
│       │   ├── CodeEditorTab.h
│       │   ├── CodeEditorTab.cpp
│       │   ├── SyntaxHighlighter.h  # Дополнительная подсветка (для @dqmodule)
│       │   └── SyntaxHighlighter.cpp
│       ├── block_editor/           # Модуль 2: Блочный редактор
│       │   ├── BlockEditorWidget.h
│       │   ├── BlockEditorWidget.cpp
│       │   ├── BlockScene.h
│       │   ├── BlockScene.cpp
│       │   ├── NodeItem.h
│       │   ├── NodeItem.cpp
│       │   ├── PortItem.h
│       │   ├── PortItem.cpp
│       │   ├── ConnectionItem.h
│       │   ├── ConnectionItem.cpp
│       │   ├── ModulePalette.h
│       │   ├── ModulePalette.cpp
│       │   ├── MinimapWidget.h
│       │   └── MinimapWidget.cpp
│       ├── ui_designer/            # Модуль 3: Дизайнер UI
│       │   ├── UIDesignerWidget.h
│       │   ├── UIDesignerWidget.cpp
│       │   ├── DesignCanvas.h
│       │   ├── DesignCanvas.cpp
│       │   ├── WidgetItem.h
│       │   ├── WidgetItem.cpp
│       │   ├── WidgetPalette.h
│       │   ├── WidgetPalette.cpp
│       │   ├── PropertyEditor.h
│       │   ├── PropertyEditor.cpp
│       │   ├── EventBindingDialog.h
│       │   ├── EventBindingDialog.cpp
│       │   ├── UIPreview.h
│       │   └── UIPreview.cpp
│       ├── lib_processor/          # Модуль 4: Обработчик библиотек
│       │   ├── LibraryProcessorWidget.h
│       │   ├── LibraryProcessorWidget.cpp
│       │   ├── LibraryImportWizard.h
│       │   ├── LibraryImportWizard.cpp
│       │   ├── ASTViewerWidget.h
│       │   └── ASTViewerWidget.cpp
│       ├── panels/                 # Общие панели
│       │   ├── ProjectExplorer.h
│       │   ├── ProjectExplorer.cpp
│       │   ├── OutputPanel.h
│       │   ├── OutputPanel.cpp
│       │   ├── DebugPanel.h
│       │   ├── DebugPanel.cpp
│       │   ├── VariablesPanel.h
│       │   ├── VariablesPanel.cpp
│       │   ├── CallStackPanel.h
│       │   ├── CallStackPanel.cpp
│       │   ├── BreakpointsPanel.h
│       │   └── BreakpointsPanel.cpp
│       ├── dialogs/                # Диалоговые окна
│       │   ├── NewProjectDialog.h
│       │   ├── NewProjectDialog.cpp
│       │   ├── ProjectSettingsDialog.h
│       │   ├── ProjectSettingsDialog.cpp
│       │   ├── PreferencesDialog.h
│       │   ├── PreferencesDialog.cpp
│       │   ├── AboutDialog.h
│       │   └── AboutDialog.cpp
│       └── themes/                 # Темы оформления
│           ├── ThemeManager.h
│           ├── ThemeManager.cpp
│           ├── dark_theme.qss
│           └── light_theme.qss
│
├── include/                        # Публичные заголовки (для плагинов/расширений)
│   └── deltaq/
│       ├── Plugin.h
│       ├── CommandInterface.h
│       └── ModuleInterface.h
│
├── resources/                      # Ресурсы приложения
│   ├── resources.qrc               # Qt Resource файл
│   ├── icons/                      # Иконки
│   │   ├── app_icon.png
│   │   ├── toolbar/
│   │   │   ├── new.png
│   │   │   ├── open.png
│   │   │   ├── save.png
│   │   │   ├── build.png
│   │   │   ├── run.png
│   │   │   ├── debug.png
│   │   │   ├── undo.png
│   │   │   └── redo.png
│   │   ├── nodes/                  # Иконки для категорий узлов
│   │   │   ├── math.png
│   │   │   ├── logic.png
│   │   │   ├── io.png
│   │   │   └── string.png
│   │   └── widgets/                # Иконки для виджетов UI Designer
│   │       ├── button.png
│   │       ├── textfield.png
│   │       ├── label.png
│   │       └── ...
│   ├── fonts/                      # Встроенные шрифты
│   │   ├── JetBrainsMono-Regular.ttf
│   │   └── JetBrainsMono-Bold.ttf
│   ├── templates/                  # Шаблоны проектов
│   │   ├── console_app/
│   │   │   ├── template.json
│   │   │   └── files/
│   │   ├── sdl2_app/
│   │   │   ├── template.json
│   │   │   └── files/
│   │   └── empty/
│   │       └── template.json
│   └── std_modules/                # Стандартные модули
│       ├── math/
│       │   ├── add.dqmod
│       │   ├── add.c
│       │   ├── add.h
│       │   ├── subtract.dqmod
│       │   ├── subtract.c
│       │   ├── subtract.h
│       │   └── ...
│       ├── logic/
│       │   ├── branch.dqmod
│       │   ├── branch.c
│       │   ├── branch.h
│       │   └── ...
│       ├── io/
│       │   ├── print.dqmod
│       │   ├── print.c
│       │   ├── print.h
│       │   └── ...
│       └── string/
│           ├── concat.dqmod
│           ├── concat.c
│           ├── concat.h
│           └── ...
│
├── tests/                          # Тесты
│   ├── CMakeLists.txt
│   ├── unit/                       # Модульные тесты
│   │   ├── test_module_registry.cpp
│   │   ├── test_graph_compiler.cpp
│   │   ├── test_code_generator.cpp
│   │   ├── test_annotation_parser.cpp
│   │   ├── test_libclang_parser.cpp
│   │   ├── test_layout_engine.cpp
│   │   ├── test_command_bus.cpp
│   │   ├── test_undo_manager.cpp
│   │   ├── test_serialization.cpp
│   │   └── ...
│   ├── integration/                # Интеграционные тесты
│   │   ├── test_build_pipeline.cpp
│   │   ├── test_graph_to_code.cpp
│   │   ├── test_ui_codegen.cpp
│   │   ├── test_library_import.cpp
│   │   └── ...
│   └── fixtures/                   # Тестовые данные
│       ├── sample_project/
│       │   ├── project.dqproj
│       │   ├── modules/
│       │   ├── graphs/
│       │   └── ui/
│       ├── sample_modules/
│       │   ├── add.dqmod
│       │   └── ...
│       ├── sample_graphs/
│       │   ├── simple_calc.dqgraph
│       │   └── ...
│       └── sample_libraries/
│           ├── vector2d.h
│           └── ...
│
├── third_party/                    # Сторонние зависимости
│   ├── CMakeLists.txt
│   └── nlohmann_json/              # nlohmann/json (header-only)
│       └── json.hpp
│
├── scripts/                        # Вспомогательные скрипты
│   ├── build.sh                    # Скрипт сборки (Linux/macOS)
│   ├── build.bat                   # Скрипт сборки (Windows)
│   ├── install_deps.sh             # Установка зависимостей
│   ├── package.sh                  # Создание дистрибутива
│   └── generate_docs.sh            # Генерация документации (Doxygen)
│
└── .github/                        # CI/CD (GitHub Actions)
    └── workflows/
        ├── build.yml               # Сборка на всех платформах
        ├── test.yml                # Запуск тестов
        └── release.yml             # Создание релиза
```

---

## Структура пользовательского проекта (создаваемого в IDE)

```
MyDeltaQProject/
├── project.dqproj                  # Файл проекта
│
├── modules/                        # Модули проекта
│   ├── src/                        # Исходный код модулей
│   │   ├── math/
│   │   │   ├── add.c
│   │   │   ├── add.h
│   │   │   ├── multiply.c
│   │   │   └── multiply.h
│   │   ├── logic/
│   │   │   ├── compare.c
│   │   │   └── compare.h
│   │   └── custom/
│   │       ├── my_module.c
│   │       └── my_module.h
│   └── defs/                       # Определения модулей (.dqmod)
│       ├── add.dqmod
│       ├── multiply.dqmod
│       ├── compare.dqmod
│       └── my_module.dqmod
│
├── graphs/                         # Графы (визуальные программы)
│   ├── main.dqgraph                # Основной граф
│   ├── helper_calc.dqgraph         # Вспомогательный подграф
│   └── event_handlers.dqgraph      # Обработчики событий UI
│
├── ui/                             # Макеты интерфейса
│   ├── main_window.dqui            # Главное окно
│   └── settings_dialog.dqui        # Диалог настроек
│
├── wrappers/                       # Обёрточный код (от Library Processor)
│   ├── vector2d_wrapper.h
│   ├── vector2d_wrapper.cpp
│   └── ...
│
├── include/                        # Дополнительные заголовки
│   └── common.h
│
├── resources/                      # Ресурсы приложения
│   ├── fonts/
│   │   └── default.ttf
│   ├── icons/
│   │   └── app.png
│   └── data/
│       └── config.json
│
├── build/                          # Директория сборки (генерируется)
│   ├── CMakeLists.txt              # Сгенерированный CMake
│   ├── generated/                  # Сгенерированный код
│   │   ├── main.c
│   │   ├── ui.h
│   │   ├── ui.c
│   │   ├── events.h
│   │   ├── events.c
│   │   ├── graph_logic.h
│   │   ├── graph_logic.c
│   │   ├── dq_widgets.h
│   │   └── dq_widgets.c
│   ├── debug/                      # Файлы отладки
│   │   └── main.dqmap              # Маппинг графов на строки кода
│   └── bin/                        # Скомпилированный результат
│       └── MyDeltaQProject         # Исполняемый файл
│
└── .deltaq/                        # Служебная директория IDE
    ├── session.json                # Состояние сессии (открытые вкладки, позиция)
    ├── breakpoints.json            # Сохранённые точки останова
    └── cache/                      # Кэш (индексы, скомпилированные модули)
        ├── module_index.json
        └── compile_cache/
```

---

## Описание ключевых директорий

### Исходный код IDE (`src/`)

| Директория | Слой | Описание |
|-----------|------|----------|
| `src/app/` | Application | Управление сессиями, команды, undo/redo |
| `src/app/commands/` | Application | Реализации конкретных команд |
| `src/core/` | Core Services | Бизнес-логика приложения |
| `src/core/codegen/` | Core Services | Генерация кода (бэкенды, IR) |
| `src/core/compiler/` | Core Services | Компиляция графов |
| `src/core/build/` | Core Services | Запуск внешних компиляторов |
| `src/core/debug/` | Core Services | Управление отладкой |
| `src/core/lsp/` | Core Services | Клиент Language Server Protocol |
| `src/core/parser/` | Core Services | Парсинг аннотаций и библиотек |
| `src/core/layout/` | Core Services | Движок компоновки UI |
| `src/data/` | Data | Хранилища данных |
| `src/data/models/` | Data | Модели данных (Module, Graph, Node...) |
| `src/data/serialization/` | Data | Чтение/запись форматов .dqmod, .dqgraph и др. |
| `src/ui/` | Presentation | Все виджеты пользовательского интерфейса |
| `src/ui/code_editor/` | Presentation | Модуль 1: текстовый редактор |
| `src/ui/block_editor/` | Presentation | Модуль 2: блочный редактор |
| `src/ui/ui_designer/` | Presentation | Модуль 3: дизайнер интерфейсов |
| `src/ui/lib_processor/` | Presentation | Модуль 4: обработчик библиотек |
| `src/ui/panels/` | Presentation | Вспомогательные панели (вывод, отладка) |
| `src/ui/dialogs/` | Presentation | Диалоговые окна |
| `src/ui/themes/` | Presentation | Темы оформления |

### Ресурсы (`resources/`)

| Директория | Описание |
|-----------|----------|
| `resources/icons/` | Иконки для панелей инструментов, узлов, виджетов |
| `resources/fonts/` | Моноширинные шрифты для редактора кода |
| `resources/templates/` | Шаблоны для создания новых проектов |
| `resources/std_modules/` | Стандартная библиотека модулей DeltaQ |

### Тесты (`tests/`)

| Директория | Описание |
|-----------|----------|
| `tests/unit/` | Модульные тесты (каждый компонент отдельно) |
| `tests/integration/` | Интеграционные тесты (взаимодействие компонентов) |
| `tests/fixtures/` | Тестовые данные (проекты, модули, графы) |

---

## Соглашения об именовании

### Файлы

| Тип | Паттерн | Пример |
|-----|---------|--------|
| Заголовочный файл | `PascalCase.h` | `ModuleRegistry.h` |
| Файл реализации | `PascalCase.cpp` | `ModuleRegistry.cpp` |
| Файл модуля | `snake_case.dqmod` | `add.dqmod` |
| Файл графа | `snake_case.dqgraph` | `main_graph.dqgraph` |
| Файл UI | `snake_case.dqui` | `main_window.dqui` |
| Файл проекта | `*.dqproj` | `project.dqproj` |
| Тест | `test_snake_case.cpp` | `test_module_registry.cpp` |
| Скрипт | `snake_case.sh/.bat` | `build.sh` |

### Классы и методы

| Элемент | Стиль | Пример |
|---------|-------|--------|
| Класс | PascalCase | `ModuleRegistry` |
| Метод | camelCase | `findByName()` |
| Член класса | `m_camelCase` | `m_modules` |
| Константа | `UPPER_SNAKE_CASE` | `MAX_UNDO_STACK` |
| Enum | PascalCase | `PortType::DataInput` |
| Namespace | lowercase | `deltaq` |
