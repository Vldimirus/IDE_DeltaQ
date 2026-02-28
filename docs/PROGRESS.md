# DeltaQ IDE — Прогресс разработки

> Последнее обновление: 2026-02-28 (Фаза 5.5 — Маршрутизация файлов + вкладки для графов/UI + удаление связей)

---

## Общий прогресс

```
Фаза 0: Планирование         [████████████████████] 100%  ✓ утверждено
Фаза 1: Ядро + Редактор кода  [████████████████████] 100%  ✓ завершена
Фаза 2: Блочный редактор      [████████████████████] 100%  ✓ завершена
Фаза 3: Дизайнер UI + Библ.    [████████████████████] 100%  ✓ завершена
Фаза 4: Интеграция             [█████░░░░░░░░░░░░░░░]  25%
─────────────────────────────────────────────────────
Общий прогресс проекта:                                ~78%
```

**Текущая фаза:** 4 — Интеграция (следующая)

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
| 17 | Фаза 2.5: Отладчик (GDB/MI) | DebugManager: полная реализация GDB/MI протокола (запуск/остановка GDB, MI-команды с токенами, парсинг MI-вывода). Точки останова: add/remove/toggle/conditional, синхронизация с GDB. Stepping: stepOver (-exec-next), stepInto (-exec-step), stepOut (-exec-finish), runToCursor. Переменные: requestLocalVariables (-stack-list-variables), evaluateExpression. Стек вызовов: requestCallStack (-stack-list-frames), selectFrame. MainWindow: меню Debug (F5 Start, Shift+F5 Stop, F10/F11/Shift+F11 Step, F9 Breakpoint), панель Variables (QTreeWidget), панель Call Stack с навигацией, Debug Console. ActionManager: 7 новых действий debug.*. 1 новый тест (~20 тест-кейсов), всего 16 тестов — все проходят. |
| 18 | Фаза 2.6: Исправление LSPClient + диагностика + hover | Критический баг: handleResponse принимал QJsonObject — терял массивы. Исправлено на QJsonValue. Definition и references теперь корректно парсят Location[]. Рефакторинг highlightCurrentLine → updateExtraSelections (единая точка: текущая строка + скобки + диагностика + debug). setDiagnostics → WaveUnderline (красный=error, оранжевый=warning, синий=info) с tooltip. CodePlainTextEdit: mouseMoveEvent + QTimer 500ms → hoverRequested → LSPClient::hover → QToolTip::showText. |
| 19 | Фаза 2.7: Маркеры отладки + breakpoints в margin | LineNumberArea расширена на 16px для маркеров. Красный кружок — breakpoint, жёлтая стрелка — текущая строка отладки. Клик в margin → toggle breakpoint → DebugManager. Зелёный фон строки при останове. breakpointAdded/Removed → синхронизация маркеров. debugStopped → clearDebugLineInAllTabs. |
| 20 | Фаза 2.8: Popup автодополнения | CompletionPopup: QFrame + QListWidget, цветные иконки по kind (F=функция, V=переменная, C=класс, E=enum, K=keyword, S=snippet). Фильтрация по prefix (case-insensitive). EventFilter: Enter/Tab=вставка, Esc=скрыть, ↑↓=навигация. Триггеры: «.», «->», «::», Ctrl+Space (через completionRequested). |
| 21 | Фаза 2.9: Rename + Formatting + Undo/Redo + ProjectTreeView | LSPClient: rename() и formatting() запросы, парсинг WorkspaceEdit и TextEdit[]. LSPTypes: LSPTextEdit + WorkspaceEdit структуры. CodeEditorWidget: renameSymbol (QInputDialog + LSP rename), formatDocument (LSP formatting), применение edits в обратном порядке. ActionManager: edit.rename (F2), edit.format (Ctrl+Shift+I). Undo/redo: делегирование — фокус в CodeEditor → QPlainTextEdit, иначе → UndoManager. ProjectTreeView: DiagnosticDelegate с цветными бейджами ошибок/предупреждений. 4 новых теста (~30 тест-кейсов), всего 20 тестов — все проходят. |
| 22 | Фаза 3: Блочный редактор (полностью) | **3.1 Графовый редактор (ядро):** BlockScene (QGraphicsScene — узлы, соединения, drag&drop, валидация), NodeItem (QGraphicsObject — прямоугольник с заголовком, цвет по категории, адаптивная высота по числу портов), PortItem (QGraphicsEllipseItem — кружок с подписью, цвет по типу данных, hover-эффект), ConnectionItem (QGraphicsPathItem — кривая Безье, расширенная зона клика). BlockEditorWidget: QGraphicsView + toolbar (зум +/-, fit), Ctrl+колесо масштабирование, Ctrl+0 fitInView, Ctrl+A выделить всё. **3.2 Палитра модулей:** ModulePalette (QTreeWidget по категориям из ModuleRegistry, QLineEdit поиск, drag&drop MIME "application/x-dqmodule", цветные иконки по категории, tooltip с описанием и портами). QSplitter: палитра слева, холст справа. Автообновление при moduleRegistered/Updated/Unregistered. **3.3 Команды графа (Undo/Redo):** AddNodeCommand, RemoveNodeCommand (с сохранением и восстановлением соединений), MoveNodeCommand (с mergeWith для объединения последовательных перемещений), ConnectCommand, DisconnectCommand, ChangePropertyCommand. Все операции через CommandBus. **3.4 Компилятор графов:** IR (IRInstruction: Call, Assign, TypeConvert, DeclareVar, Comment, Return; фабричные методы; emitCCode). GraphCompiler: валидация узлов/портов/типов, топологическая сортировка (алгоритм Кана, обнаружение циклов), генерация IR, неявные преобразования типов (int→float/double, float→double, bool→int), CompilationResult с sourceMap (строка→nodeId). **3.5 Интеграция со сборкой:** onBuild: если активен BlockEditor → GraphCompiler::compile → сохранение .c в generated/ → CMakeGenerator учитывает generated/. Подсветка ошибочных узлов на холсте. **3.6 Визуальная отладка:** GraphDebugger: маппинг строк кода→узлов через sourceMap, onBreakpointHit→highlightNode, onStepped→completed+highlight, onDebugStopped→clearDebugState, onVariablesUpdated→tooltip на портах с текущими значениями. Визуальные состояния: серая рамка (обычный), жёлтая (текущий), зелёная (выполненный), красная (ошибка). 4 новых теста (~34 тест-кейса), всего 24 теста — все проходят. |
| 23 | Фаза 4: Дизайнер UI + Обработчик библиотек (полностью) | UI Designer + SDL2 Code Generator + LibProcessor + UIPreview. 29 новых файлов, 7 модифицированных, 8 тестов (~52 тест-кейса), всего 32 теста. |
| 24 | Фаза 5.1: NewProjectWizard | Мастер создания проекта (QWizard, 3 страницы: тип/имя/сводка). ProjectTemplates: генерация шаблонных файлов (Console — hello world, Desktop — SDL2 + UILayout + 5 C-файлов). Project.h: поле projectType. ProjectManager: перегрузка createProject(name, dir, type). MainWindow: onNewProject → NewProjectWizard + автооткрытие main.c. 4 новых файла, 5 модифицированных, все 32 теста проходят. |
| 25 | Фаза 5.2: Close Project + Recent Projects + пустой запуск | restoreSession/saveSession: убрано автооткрытие проекта и вкладок (пустой запуск). onCloseProject: закрытие проекта + вкладок + очистка дерева. File → Recent Projects: подменю из SessionManager::recentProjects(), клик → открытие, Clear History. updateRecentProjectsMenu() вызывается при New/Open/Recent. CodeEditorWidget::closeAllTabs(). 4 файла изменены, все 32 теста проходят. |
| 26 | Фаза 5.3: UI Designer — окно по умолчанию | DesignScene: рамка окна 800×600 в drawBackground() (тень, тело, title bar с кнопками close/min/max, заголовок). Поля m_windowRect, m_windowTitle, TitleBarHeight, геттеры/сеттеры. UIDesignerWidget: showEvent + centerOnWindow (fitInView при первом показе). Вид отцентрирован на рамке окна. 4 файла изменены, все 32 теста проходят. |
| 27 | Фаза 5.4: Система шаблонов — собираемые проекты | **Часть A (SDL2 сборка):** CMakeGenerator — параметр projectType, генерация find_package(PkgConfig)+pkg_check_modules(SDL2)+target_include_directories+target_link_libraries для desktop. BuildManager — проброс projectType через build(). MainWindow — передача projectType при сборке. **Часть B (Шаблоны модулей/графов):** ProjectTemplates — generateConsoleModulesAndGraphs (модуль hello category=io + граф main с 1 узлом), generateDesktopModulesAndGraphs (модули event_handler+update_label + граф main с 2 узлами и соединением action→action). MainWindow::onNewProject — загрузка moduleRegistry и graphStore после генерации шаблонов. NewProjectWizard — сводка включает .dqmod и .dqgraph файлы. 8 файлов изменены, все 32 теста проходят. |
| 28 | Фаза 5.5: Маршрутизация + вкладки + удаление связей | **Маршрутизация:** MainWindow::onFileActivated() — .dqgraph/.dqmod/.dqui открываются как вкладки в едином таб-баре (не переключают QStackedWidget). **include/:** убрана из ensureDirectories(). **Delete:** BlockEditorWidget — удаление выделенных узлов и соединений (Delete). ConnectionItem — selectable + красная подсветка. **UI Designer:** connectUIDesignerSignals() — сигналы drag&drop/move/resize/GenerateCode/Preview подключаются к каждому экземпляру. **Generate Code** → src/ вместо generated/. 7 файлов изменены, все 32 теста проходят. |

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
- [x] Фаза 2.5: Отладчик (DebugManager с GDB/MI, breakpoints, stepping, переменные, стек, панели)
- [x] Фаза 2.6: Исправление LSPClient + диагностика + hover
- [x] Фаза 2.7: Маркеры отладки + breakpoints в margin
- [x] Фаза 2.8: Popup автодополнения
- [x] Фаза 2.9: Rename + Formatting + Undo/Redo + ProjectTreeView
- [x] Фаза 3: Блочный редактор (полностью — 6 подэтапов, 18 новых файлов, 7 модифицированных)
- [x] Фаза 4: Дизайнер UI + Обработчик библиотек (полностью — 8 подэтапов, 29 новых файлов, 7 модифицированных)
- [x] Фаза 5.1: NewProjectWizard (мастер создания проекта с шаблонами Console/Desktop)
- [x] Фаза 5.2: Close Project + Recent Projects + пустой запуск
- [x] Фаза 5.3: UI Designer — окно по умолчанию при открытии
- [x] Фаза 5.4: Система шаблонов — собираемые проекты (SDL2 в CMake, .dqmod + .dqgraph генерация)
- [x] Фаза 5.5: Маршрутизация файлов + вкладки для графов/UI + удаление связей

### Известные проблемы (Фаза 5.5)

- [ ] Блочный редактор (.dqgraph): узлы слишком приближены при открытии — zoomFit масштабирует чрезмерно для малого числа узлов
- [ ] UI Designer (.dqui): drag&drop виджетов из палитры на холст может не работать корректно во вкладке
- [ ] UI Designer: нет дерева виджетов (object tree) в боковой панели
- [ ] UI Designer: выделение виджета и просмотр свойств в правой панели — требует проверки

---

## Что дальше

**Фазы 1–3 — завершены на 100%.**

**Фаза 4 (Дизайнер UI + Обработчик библиотек) — завершена на 100%.**

**Фаза 5 (Интеграция) — начата (25%).**

Реализовано:
- UI Designer: DesignScene, WidgetItem (12 типов), WidgetPalette (23 типа, 4 категории), PropertyEditor
- UICommands: 7 команд undo/redo (Add/Remove/Move/Resize/ChangeProperty/ChangeLayout/BindEvent)
- LayoutEngine: HBox, VBox, Grid, Flow компоновки
- SDL2 Code Generator: 5 файлов C-кода из UILayout
- EventBindingDialog: привязка событий (Module Function / Graph Trigger / Custom)
- UIPreview: генерация → gcc → запуск SDL2-приложения
- LibclangParser: парсинг C/C++ заголовков через libclang (#ifdef DQ_HAS_LIBCLANG)
- LibraryDecomposer: функции→модули, классы→модули (create/destroy/methods)
- WrapperGenerator: extern "C" обёртки для C++ классов
- LibraryImportWizard: 5-шаговый мастер импорта
- 8 новых тестов (~52 тест-кейса), всего 32 теста

**Фаза 5 (Интеграция)** — см. детальные планы в `docs/plan/09_roadmap.md`

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
