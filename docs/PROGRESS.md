# DeltaQ IDE — Прогресс разработки

> Последнее обновление: 2026-02-28 (Фаза 2.4)

---

## Общий прогресс

```
Фаза 0: Планирование         [████████████████████] 100%  ✓ утверждено
Фаза 1: Ядро + Редактор кода  [███████████████████░]  95%  (2.4 завершена)
Фаза 2: Блочный редактор      [░░░░░░░░░░░░░░░░░░░░]   0%
Фаза 3: Дизайнер UI           [░░░░░░░░░░░░░░░░░░░░]   0%
Фаза 4: Обработчик библиотек  [░░░░░░░░░░░░░░░░░░░░]   0%
Фаза 5: Интеграция             [░░░░░░░░░░░░░░░░░░░░]   0%
─────────────────────────────────────────────────────
Общий прогресс проекта:                                ~32%
```

**Текущая фаза:** 1 — Ядро + Редактор кода

---

## Выполненное

### 2026-02-28

| # | Что сделано | Описание |
|---|------------|----------|
| 1 | Документация: все планы (00–09) | 10 файлов в `docs/plan/` — архитектура, модули, генерация кода, роадмап |
| 2 | Концепция матрёшки | Граф = модуль = .cpp. Рекурсивная вложенность на любую глубину. Обновлены: 02, 04, 07 |
| 3 | Мультиязычная архитектура | LanguageBackend интерфейс, IR с абстрактными типами, примеры C/Python/Rust. Обновлены: 01, 07 |
| 4 | Ревью планов: принятые решения | Потокобезопасность CommandBus (мьютексы), тестирование модулей (эмулятор), многопоточные блоки (ThreadModule + порты синхронизации), обработка ошибок компилятора, Copy/Paste без связей, упрощённый визуал блоков. Обновлены: 01, 02, 03, 04 |
| 5 | Каркас IDE (черновик) | CMakeLists.txt, структура каталогов, MainWindow, CommandBus, UndoManager, ActionManager, SessionManager, ModuleRegistry, ProjectManager |
| 5 | Модели данных (черновик) | Module.h, Project.h, Graph.h — базовые структуры с JSON-сериализацией |
| 6 | Редактор кода (черновик) | CodeEditorWidget (вкладки), CodeEditorTab (текстовый редактор с нумерацией строк) |
| 7 | Первая успешная сборка | Исправлены CMakeLists.txt (заглушки → INTERFACE), CommandBus (vector вместо QStack), CodeEditorTab (подкласс QPlainTextEdit), AnnotationParser (поля Module). Проект компилируется и линкуется. |
| 8 | Локализация (i18n) | QTranslator, загрузка переводов в main.cpp, меню Settings > Language (English/Русский), SessionManager хранит язык, CMake-цели lupdate/lrelease, полный русский перевод (60 строк). Смена языка с перезапуском. |
| 9 | Фаза 1.1: Доработка ядра + тесты | CommandBus: QMutex, MacroCommand, executeNoHistory, beginMacro/endMacro. UndoManager: clean state (setClean/isClean/clear), cleanChanged сигнал. ActionManager: группы действий (enableGroup/disableGroup/addToGroup). SessionManager: openTabs, lastOpenedProject, тестовый конструктор. Тестовая инфраструктура Qt6::Test, 4 теста (~41 тест-кейсов), все проходят. |
| 10 | Фаза 1.2: Модели данных + тесты | Module: operator==, findInput/findOutput, hasInput/hasOutput, isValid(). Project: operator==, isValid(). Graph: operator==, findNode, addNode, removeNode (каскадное удаление соединений), addConnection, removeConnection, connectionsForNode, isValid, nodeCount/connectionCount. UILayout.h (новый): UIWidget и UILayout — модель UI-макета (.dqui) с toJson/fromJson, вложенными виджетами. 4 новых теста (~39 тест-кейсов), всего 8 тестов — все проходят. |
| 11 | Фаза 1.3: Хранилища данных + тесты | GraphStore: register/unregister/find/findByName, load/save .dqgraph, loadFromDirectory, saveAll. UILayoutStore: аналогичный API для .dqui. ProjectManager: интеграция с GraphStore и UILayoutStore (загрузка/сохранение/очистка при open/save/close). MainWindow: инициализация всех stores. 3 новых теста (~28 тест-кейсов), всего 11 тестов — все проходят. |
| 12 | Фаза 1.4: Базовый UI | BuildManager → Output dock (вывод сборки, блокировка кнопки, Clean). onRun: поиск и запуск бинарника → Application Output. StatusBar: Ln/Col при перемещении курсора. ProjectTreeView: контекстное меню (Open, New File/Folder, Delete, Show in FM), фильтр CMakeLists.txt. Сессия: сохранение/восстановление открытых вкладок и последнего проекта. |
| 13 | Фаза 2.1: Доработка редактора кода | FindReplaceBar (Ctrl+F поиск, Ctrl+H замена, подсветка совпадений, счётчик, опции Case/Words). Go to Line (Ctrl+G). Подсветка парных скобок ((){}\[\], вперёд/назад, красная подсветка при отсутствии пары). SyntaxHighlighter: имена функций, @dqmodule/@dqport аннотации, параметры аннотаций. AnnotationParser: парсинг @dqport (direction, name, type, default). ActionManager: edit.replace (Ctrl+H), edit.goToLine (Ctrl+G). |
| 14 | Фаза 2.2: LSP-клиент (clangd) | LSPClient (JSON-RPC 2.0 через stdin/stdout, Content-Length framing). LSPTypes: Position, Range, Location, Diagnostic, CompletionItem, HoverInfo, DocumentSymbol. Синхронизация документов: didOpen/didChange/didSave/didClose с CodeEditorWidget. Запросы: completion, hover, definition, references. Автозапуск clangd при открытии проекта. Навигация: Go to Definition (F12), Find References (Shift+F12). Диагностика: отображение счётчика ошибок/предупреждений. 1 новый тест (~16 тест-кейсов), всего 12 тестов — все проходят. |
| 15 | Фаза 2.3: Система сборки | CompilerOutputParser: парсинг вывода GCC/Clang/CMake (error/warning/note), regex для формата файл:строка:столбец, сигнал errorFound. CMakeGenerator: генерация CMakeLists.txt из исходников проекта (рекурсивный сбор .c/.cpp/.cxx/.cc, пропуск build/, стандарты C/C++, флаги компиляции). BuildManager: рефакторинг с интеграцией парсера и генератора, двухэтапная сборка (configure + build), проверка CMakeCache.txt. Навигация к ошибкам: buildError сигнал с файлом/строкой/столбцом. 2 новых теста (~19 тест-кейсов), всего 14 тестов — все проходят. |
| 16 | Фаза 2.4: Аннотации @dqmodule | AnnotationParser: переписан с построчным парсингом, поддержка множественных @dqmodule в одном файле, многострочных комментариев (/* */), парсинг key=value (в кавычках и без), стабильные ID (SHA256 от путь+имя), сигналы ошибок и moduleParsed. Module.h: добавлено поле category. Автогенерация .dqmod при сохранении файла (CodeEditorWidget → AnnotationParser → .dqmod + ModuleRegistry). ModuleRegistry: findBySourcePath, modulesByCategory, categories, validateModule, hasDuplicateName. Загрузка реестра при открытии проекта, уведомления в статусбар. 1 новый тест (~14 тест-кейсов), всего 15 тестов — все проходят. |

> **Черновики** (п. 5–6) были созданы до утверждения планов и доработаны при первой сборке.

---

## Текущие задачи

- [x] Ревью и доработка планов совместно с разработчиком
- [x] Утверждение планов (2026-02-28)
- [x] Верификация и доработка черновиков кода под утверждённые планы (первая сборка)
- [x] Фаза 1.1: Доработка каркаса ядра + тесты (CommandBus, UndoManager, ActionManager, SessionManager)
- [x] Фаза 1.2: Модели данных (Module, Project, Graph, UILayout — operator==, isValid, helper-методы, тесты)
- [x] Фаза 1.3: Хранилища данных (GraphStore, UILayoutStore, ProjectManager — интеграция со stores, тесты)
- [x] Фаза 1.4: Базовый UI (BuildManager→Output, StatusBar Ln/Col, контекстное меню дерева, сессия вкладок)
- [x] Фаза 2.1: Доработка редактора (Find/Replace, Go to Line, парные скобки, SyntaxHighlighter, AnnotationParser)
- [x] Фаза 2.2: LSP-клиент (JSON-RPC, didOpen/didChange/didSave, completion, hover, definition, references, диагностика)
- [x] Фаза 2.3: Система сборки (CompilerOutputParser, CMakeGenerator, BuildManager рефакторинг, навигация к ошибкам)
- [x] Фаза 2.4: Аннотации @dqmodule (AnnotationParser, автогенерация .dqmod, ModuleRegistry доработка, интеграция)

---

## Что дальше

**Фаза 1 — оставшиеся задачи:**

| Блок | Задачи | Статус |
|------|--------|--------|
| Редактор кода | QScintilla интеграция, доработка ProjectTreeView | WIP |
| LSP-клиент | Виджет автодополнения, подчёркивание ошибок в редакторе, hover-подсказки | WIP |
| Система сборки | Генерация CMakeLists, запуск компиляции, парсинг ошибок, панель вывода | DONE |
| Отладчик | GDB/LLDB драйвер, breakpoints, step, панель переменных, стек вызовов | TODO |

**Фазы 2–5** — см. детальные планы в `docs/plan/09_roadmap.md`

---

## Документация

| Файл | Описание |
|------|----------|
| `docs/plan/00_general_plan.md` | Общий план проекта |
| `docs/plan/01_architecture.md` | 4-слойная архитектура, CommandBus, мультиязычность |
| `docs/plan/02_module_system.md` | Система модулей, .dqmod, матрёшка |
| `docs/plan/03_module1_code_editor.md` | Модуль 1: Редактор кода |
| `docs/plan/04_module2_block_editor.md` | Модуль 2: Визуальный блочный редактор |
| `docs/plan/05_module3_ui_designer.md` | Модуль 3: Дизайнер UI |
| `docs/plan/06_module4_lib_processor.md` | Модуль 4: Обработчик библиотек |
| `docs/plan/07_codegen.md` | Генерация кода, IR, бэкенды |
| `docs/plan/08_project_structure.md` | Структура каталогов |
| `docs/plan/09_roadmap.md` | Дорожная карта и фазы |

---

## Обозначения

- **TODO** — не начато
- **WIP** — в работе
- **DONE** — завершено
- **BLOCKED** — заблокировано
- **Черновик** — создано, но требует ревью после утверждения планов
