# Strategy 2026 Execution Log

## Purpose

Этот файл ведётся как хронологический журнал выполнения стратегии `strategy_2026`.

Он нужен для того, чтобы по датам видеть:

- что именно было реализовано;
- что было исправлено;
- к какой фазе стратегии относится изменение;
- чем изменение было проверено.

## Update Rules

При каждом заметном шаге реализации в журнал добавляется новая запись.

Формат записи:

- дата;
- фаза стратегии;
- краткий смысл изменения;
- что добавлено или исправлено;
- чем проверено;
- если нужно, что осталось открытым.

Записи ведутся в хронологическом порядке.

---

## 2026-03-08

### Шаг 1 — создан стратегический контур `strategy_2026`

**Фаза:** подготовка стратегии перед `Phase 1`

**Что сделано:**

- создан новый набор стратегических документов в `docs/plan/strategy_2026/`;
- добавлен главный документ `00_master_strategy.md`;
- добавлены базовые документы:
  - `01_product_thesis.md`
  - `02_module_model.md`
  - `03_core_workflow.md`
  - `04_codegen_and_runtime.md`
- добавлены фазовые документы:
  - `10_phase_1_core_stabilization.md`
  - `11_phase_2_module_ecosystem.md`
  - `12_phase_3_showcase_and_adoption.md`
- добавлены supporting-документы:
  - `20_scope_control.md`
  - `21_standard_library_strategy.md`
  - `22_examples_and_reference_projects.md`
  - `23_acceptance_criteria.md`

**Зачем это сделано:**

- зафиксировать DeltaQ как среду модульной композиции ПО;
- убрать размытое позиционирование;
- построить реалистичную стратегию на `6-12 месяцев` вокруг главного workflow.

**Проверка:**

- проверена структура каталога `docs/plan/strategy_2026`;
- проверена связность документов и наличие всех файлов.

**Открыто:**

- перевести стратегию в последовательные практические шаги `Phase 1`.

### Шаг 2 — начато исполнение `Phase 1` через прозрачность generated code

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в `src/codegen/GraphCompiler.cpp` добавлен banner в generated `.c` для графов;
- в banner теперь указывается:
  - что файл сгенерирован DeltaQ;
  - из какого графа он получен;
  - что это generated source;
  - что файл может быть перезаписан;
- в `src/uiDesigner/SDL2CodeGenerator.cpp` добавлены аналогичные banner-ы во все generated UI-файлы:
  - `main.c`
  - `ui.h`
  - `ui.c`
  - `events.h`
  - `events.c`
- в тесты добавлены проверки на наличие информации о происхождении generated-файлов:
  - `tests/codegen/test_GraphCompiler.cpp`
  - `tests/uiDesigner/test_SDL2CodeGenerator.cpp`

**Зачем это сделано:**

- сделать generated code самодокументируемым;
- уменьшить ощущение "магии" в codegen;
- усилить доверие к основному workflow `graph/ui -> code -> build`.

**Проверка:**

- `cmake --build build --parallel --target test_GraphCompiler test_SDL2CodeGenerator`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SDL2CodeGenerator`

**Результат проверки:**

- оба тестовых набора прошли успешно.

**Открыто:**

- следующим шагом сделать более явным происхождение generated-артефактов в `pre-build/build` выводе и навигации по ним внутри IDE.

### Шаг 3 — происхождение generated-артефактов доведено до pipeline и навигации

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- `PreBuildProcessor` переведён с простого списка путей на структурированное описание generated-артефактов;
- для каждого generated-файла теперь хранится:
  - путь;
  - тип артефакта;
  - имя источника;
  - `id` источника;
- артефакты различаются по ролям:
  - graph C source;
  - UI header;
  - UI implementation;
  - UI event header;
  - UI event implementation;
- `BuildPipeline` теперь печатает generated-файлы в понятном виде:
  - относительный путь;
  - роль артефакта;
  - происхождение из графа или UI layout;
- в `CodeEditorWidget` generated-файлы помечаются во вкладках как `[gen]`;
- tooltip вкладки generated-файла теперь показывает:
  - что файл generated;
  - source line;
  - file role;
  - предупреждение о возможной перезаписи при `pre-build`.

**Зачем это сделано:**

- сделать происхождение generated-артефактов видимым не только внутри файлов, но и в runtime workflow IDE;
- улучшить навигацию по generated-коду;
- снизить путаницу между исходниками пользователя и артефактами pre-build.

**Технические изменения:**

- обновлены:
  - `src/codegen/PreBuildProcessor.h`
  - `src/codegen/PreBuildProcessor.cpp`
  - `src/codegen/BuildPipeline.cpp`
  - `src/editor/CodeEditorWidget.cpp`
- добавлены тесты:
  - `tests/codegen/test_PreBuildProcessor.cpp`
  - `tests/editor/test_GeneratedFileNavigation.cpp`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target test_PreBuildProcessor`
- `cmake --build build --parallel --target test_GeneratedFileNavigation`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GeneratedFileNavigation`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SDL2CodeGenerator`

**Результат проверки:**

- все перечисленные тесты прошли успешно.

**Открыто:**

- следующим шагом усилить связь между generated-артефактами и их первоисточниками в UI IDE:
  - быстрый переход от generated-файла к графу или layout;
  - более явное различение source-of-truth и generated outputs в проектной навигации.

### Шаг 4 — добавлен переход от generated-файла к source-of-truth

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в `CodeEditorWidget` расширен парсинг banner-а generated-файла:
  - выделяется `source kind`;
  - выделяется `source name`;
  - при наличии читается `source id`;
- metadata происхождения generated-файла теперь хранится в свойствах вкладки редактора;
- добавлено новое действие `Open Generated Origin`;
- действие встроено в `Edit` menu;
- действие автоматически включается только тогда, когда активна generated-вкладка с распознанным источником;
- из generated-файла теперь можно открыть:
  - исходный `.dqgraph`, если файл сгенерирован из графа;
  - исходный `.dqui`, если файл сгенерирован из UI layout.

**Зачем это сделано:**

- убрать тупиковый сценарий просмотра generated-кода;
- связать generated output с source-of-truth;
- сделать workflow `graph/layout -> generated code -> return to source` быстрым и понятным.

**Технические изменения:**

- обновлены:
  - `src/editor/CodeEditorWidget.cpp`
  - `src/core/ActionManager.cpp`
  - `src/core/MainWindow.h`
  - `src/core/MainWindow.cpp`
- расширен тест:
  - `tests/editor/test_GeneratedFileNavigation.cpp`

**Проверка:**

- `cmake --build build --parallel --target deltaq`
- `cmake --build build --parallel --target test_GeneratedFileNavigation`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GeneratedFileNavigation`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SDL2CodeGenerator`

**Результат проверки:**

- основное приложение `deltaq` успешно собрано;
- все перечисленные тесты прошли успешно.

**Открыто:**

- следующим шагом усилить различение `source-of-truth` и generated outputs в дереве проекта и/или в представлении открытых вкладок:
  - более явный UX для generated paths;
  - возможно, отдельный способ быстро увидеть, что открытый файл не является редактируемым первоисточником логики.

### Шаг 5 — generated outputs выделены в дереве проекта

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- `ProjectTreeView` теперь умеет распознавать generated-файлы по banner-у;
- generated-файлы кешируются внутри дерева проекта как отдельный класс файлов;
- в `DiagnosticDelegate` добавлен `gen`-бейдж для generated-файлов в дереве проекта;
- дерево проекта теперь различает generated outputs без чтения файлов в момент каждой отрисовки;
- при открытии проекта дерево автоматически сканирует исходники и восстанавливает generated-маркеры;
- при `pre-build` дерево обновляется инкрементально через сигнал `PreBuildProcessor::fileGenerated`;
- в контекстное меню файла добавлен пункт `Open Generated Origin`, если для generated-файла удаётся определить source-of-truth;
- origin-resolve работает для:
  - `graph -> graphs/<name>.dqgraph`
  - `UI layout -> ui/<name>.dqui`

**Зачем это сделано:**

- сделать distinction между generated outputs и первоисточниками видимым уже в проектной навигации;
- убрать ситуацию, когда generated `.c/.h` выглядят как обычные редактируемые первоисточники;
- ускорить возврат от generated output обратно к графу или layout прямо из дерева проекта.

**Технические изменения:**

- обновлены:
  - `src/editor/ProjectTreeView.h`
  - `src/editor/ProjectTreeView.cpp`
  - `src/core/MainWindow.cpp`
- добавлен тест:
  - `tests/editor/test_ProjectTreeView.cpp`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target deltaq`
- `cmake --build build --parallel --target test_ProjectTreeView`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ProjectTreeView`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GeneratedFileNavigation`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`

**Результат проверки:**

- `deltaq` успешно собран;
- все перечисленные тесты прошли успешно.

**Открыто:**

- следующим шагом усилить UX вокруг source-of-truth на уровне редактирования:
  - ограничить или явно предупреждать о прямом редактировании generated-файлов;
  - сделать переход к origin ещё ближе к месту работы пользователя, например через toolbar/inline-action в редакторе.

### Шаг 6 — source-of-truth вынесен прямо в редактор generated-файлов

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в `CodeEditorWidget` добавлена отдельная панель `generated/origin` над вкладками;
- панель показывается только для generated-файлов;
- панель явно сообщает, что файл открыт в `read-only` режиме;
- на панели теперь отображается `source-of-truth` в человекочитаемом виде:
  - тип источника;
  - имя источника;
  - при наличии `id`;
- в панель добавлена кнопка `Open Origin`;
- кнопка использует тот же переход к первоисточнику, что и действие `Edit -> Open Generated Origin`;
- UX вокруг generated-файлов теперь строится не только через tab tooltip и project tree, но и прямо в рабочем месте редактора.

**Зачем это сделано:**

- убрать лишний шаг поиска origin через меню;
- сделать статус generated output очевидным в момент чтения файла;
- усилить модель `source-of-truth -> generated output`, не оставляя generated-код похожим на обычный редактируемый исходник.

**Технические изменения:**

- обновлены:
  - `src/editor/CodeEditorWidget.h`
  - `src/editor/CodeEditorWidget.cpp`
  - `src/core/MainWindow.cpp`
- расширен тест:
  - `tests/editor/test_GeneratedFileNavigation.cpp`

**Проверка:**

- `cmake --build build --parallel --target deltaq`
- `cmake --build build --parallel --target test_GeneratedFileNavigation`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GeneratedFileNavigation`

**Результат проверки:**

- основное приложение успешно собрано;
- тест навигации по generated-файлам прошёл успешно.

**Открыто:**

- следующим шагом можно усилить source-of-truth модель ещё на уровне действий редактора:
  - отключать нерелевантные команды для generated-вкладок более явно;
  - добавить быстрый origin-переход в toolbar или tab context menu, если это окажется полезнее текущего inline-баннера.

### Шаг 7 — действия редактора стали зависеть от source-of-truth состояния

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в `MainWindow` добавлен единый пересчёт editor-actions для активной вкладки;
- `Save` теперь включается только там, где действительно есть редактируемый первоисточник:
  - обычный текстовый файл;
  - custom graph tab;
  - custom UI layout tab;
- `Save` отключается для generated text outputs;
- `Rename Symbol` и `Format Document` теперь включаются только для обычных редактируемых текстовых вкладок;
- `Open Generated Origin` продолжает включаться только для generated-файлов с распознанным источником;
- в `CodeEditorWidget` добавлен повторный `currentTabChanged` после применения generated/read-only metadata, чтобы `MainWindow` видел финальное состояние вкладки, а не промежуточное;
- добавлен отдельный integration-style тест для action-состояний главного окна.

**Зачем это сделано:**

- убрать ситуацию, когда generated output выглядит не только как исходник, но и как редактируемый рабочий объект IDE;
- сделать `source-of-truth` заметным не только визуально, но и на уровне доступных действий;
- закрепить один из ключевых принципов `Phase 1`: generated output можно читать и трассировать, но редактируемым первоисточником остаётся граф, layout или обычный текстовый файл.

**Технические изменения:**

- обновлены:
  - `src/core/MainWindow.h`
  - `src/core/MainWindow.cpp`
  - `src/editor/CodeEditorWidget.cpp`
- добавлен тест:
  - `tests/core/test_MainWindowEditorActions.cpp`
- обновлён:
  - `tests/CMakeLists.txt`

**Проверка:**

- `cmake --build build --parallel --target deltaq test_MainWindowEditorActions test_GeneratedFileNavigation`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_MainWindowEditorActions`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GeneratedFileNavigation`

**Результат проверки:**

- `deltaq` успешно собран;
- новый тест `test_MainWindowEditorActions` прошёл успешно;
- `test_GeneratedFileNavigation` повторно прошёл успешно.

**Открыто:**

- следующим шагом логично перенести distinction между `source-of-truth` и generated output ещё глубже в проектную навигацию и build UX:
  - быстрый jump-to-origin из build output;
  - явная маркировка generated outputs в more places, где пользователь выбирает файл для редактирования.

### Шаг 8 — зафиксировано правило русскоязычных комментариев в коде

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в стратегический набор добавлен отдельный документ `24_development_conventions.md`;
- в документе зафиксировано правило:
  - для новых и изменяемых функций, процедур и методов нужно писать комментарии на русском языке;
  - нетривиальная внутренняя логика тоже должна сопровождаться короткими русскоязычными комментариями;
- правило привязано к основному набору `strategy_2026` через `00_master_strategy.md`.

**Зачем это сделано:**

- превратить договорённость по стилю сопровождения кода в явное правило проекта;
- уменьшить зависимость понимания кода от контекста автора;
- сделать дальнейшую реализацию и ревью более последовательными.

**Технические изменения:**

- добавлен:
  - `docs/plan/strategy_2026/24_development_conventions.md`
- обновлён:
  - `docs/plan/strategy_2026/00_master_strategy.md`

**Проверка:**

- проверена доступность нового документа из master strategy;
- проверено, что правило явно сформулировано и не смешано с другими планами.

**Результат проверки:**

- правило разработки зафиксировано в стратегическом контуре проекта и может использоваться как обязательное при дальнейшей реализации.

### Шаг 9 — русскоязычные комментарии добавлены в уже внесённый код

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в ранее добавленные функции и процедуры внесены короткие русскоязычные комментарии;
- комментарии добавлены в код, связанный с:
  - generated/source-of-truth логикой редактора;
  - навигацией `generated -> origin`;
  - pre-build артефактами;
  - build pipeline output;
  - маркировкой generated-файлов в дереве проекта;
  - banner-ами generated graph/UI source files.

**Зачем это сделано:**

- привести уже написанный код в соответствие с новым правилом разработки;
- сделать назначение новых функций понятным без обращения к истории изменений;
- снизить зависимость сопровождения от контекста текущего автора.

**Технические изменения:**

- обновлены комментариями:
  - `src/editor/CodeEditorWidget.cpp`
  - `src/core/MainWindow.cpp`
  - `src/editor/ProjectTreeView.cpp`
  - `src/codegen/PreBuildProcessor.cpp`
  - `src/codegen/BuildPipeline.cpp`
  - `src/codegen/GraphCompiler.cpp`
  - `src/uiDesigner/SDL2CodeGenerator.cpp`

**Проверка:**

- проверено вручную, что новые и ключевые изменённые функции имеют русскоязычные комментарии;
- поведение кода не менялось, поэтому отдельный прогон тестов для этого шага не выполнялся.

**Результат проверки:**

- ранее внесённый код частично приведён к новому стандарту комментирования;
- дальнейшие изменения должны продолжать этот стиль без накопления новых "немых" участков.

### Шаг 10 — замкнута навигация `build error -> source-of-truth`

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в `MainWindow` добавлена навигация по двойному щелчку на строке `Build Output`;
- строка build output теперь разбирается через `CompilerOutputParser`;
- для обычных файлов IDE:
  - нормализует путь ошибки;
  - открывает файл;
  - ставит курсор в строку и колонку из сообщения компилятора;
- для generated-файлов DeltaQ IDE:
  - определяет их `source-of-truth` через banner и проектную навигацию;
  - открывает первоисточник вместо generated output;
- для надёжности build-navigation source-of-truth открывается как текстовый файл, даже если это `.dqgraph` или `.dqui`;
- в парсер добавлена отдельная проверка относительных путей вида `../src/...`, характерных для сборки из `build/`.

**Зачем это сделано:**

- замкнуть основной путь `build error -> generated file -> source-of-truth`;
- убрать тупиковый сценарий, когда пользователь видит только generated `.c/.h`, но не может быстро вернуться к исходному графу или layout;
- сделать build diagnostics частью прозрачного workflow, а не отдельным текстовым логом.

**Технические изменения:**

- обновлены:
  - `src/core/MainWindow.h`
  - `src/core/MainWindow.cpp`
  - `tests/editor/test_CompilerOutputParser.cpp`
  - `tests/CMakeLists.txt`
- добавлен тест:
  - `tests/core/test_MainWindowBuildOutputNavigation.cpp`

**Проверка:**

- `cmake --build build --parallel --target deltaq test_MainWindowBuildOutputNavigation test_CompilerOutputParser`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_MainWindowBuildOutputNavigation`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_CompilerOutputParser`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_MainWindowEditorActions`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GeneratedFileNavigation`

**Результат проверки:**

- `test_MainWindowBuildOutputNavigation` прошёл успешно;
- `test_CompilerOutputParser` прошёл успешно;
- регрессионные тесты `test_MainWindowEditorActions` и `test_GeneratedFileNavigation` также прошли успешно.

**Открыто:**

- следующим подшагом можно улучшить именно presentation-layer этой навигации:
  - подсветить в build output строки, по которым можно перейти;
  - при желании позже вернуть открытие `.dqgraph/.dqui` в специализированных редакторах именно из build-navigation, если это можно сделать без нестабильности.

### Шаг 11 — навигационные строки в Build Output стали визуально различимы

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в `MainWindow` добавлен отдельный путь вставки текста в `Build Output`;
- build output теперь вставляется построчно, а не только через обычный `append`;
- строки, которые распознаются как сообщения компилятора с путём/строкой/колонкой:
  - выделяются отдельным форматированием;
  - получают подчёркивание;
  - окрашиваются как навигационные;
- при наведении на такую строку курсор меняется, а tooltip подсказывает, что строка поддерживает переход по двойному щелчку;
- тот же форматированный вывод используется и для других build-related сообщений, которые попадают в `Build Output`.

**Зачем это сделано:**

- сделать переход по ошибкам не скрытой возможностью, а видимым элементом интерфейса;
- уменьшить трение в сценарии `build log -> ошибка -> source-of-truth`;
- закрепить distinction между обычным логом и строками, по которым можно перейти.

**Технические изменения:**

- обновлены:
  - `src/core/MainWindow.h`
  - `src/core/MainWindow.cpp`
- расширен тест:
  - `tests/core/test_MainWindowBuildOutputNavigation.cpp`

**Проверка:**

- `cmake --build build --parallel --target deltaq test_MainWindowBuildOutputNavigation test_CompilerOutputParser`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_MainWindowBuildOutputNavigation`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_CompilerOutputParser`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_MainWindowEditorActions`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GeneratedFileNavigation`

**Результат проверки:**

- `test_MainWindowBuildOutputNavigation` прошёл успешно, включая проверку выделения навигационных строк;
- `test_CompilerOutputParser` прошёл успешно;
- регрессионные тесты `test_MainWindowEditorActions` и `test_GeneratedFileNavigation` также прошли успешно.

**Открыто:**

- следующим шагом можно усилить UX ещё на уровне структуры build diagnostics:
  - показывать краткий summary ошибок как отдельный список/панель;
  - добавлять более явную связь между сообщением компилятора и generated/source-of-truth ролями файла.

### Шаг 12 — Build Diagnostics вынесены в отдельный навигационный список

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в `MainWindow` завершена отдельная вкладка `Build Diagnostics`;
- ошибки и предупреждения сборки теперь добавляются не только в текстовый `Build Output`, но и в структурированный список;
- каждая запись diagnostics хранит:
  - нормализованный путь к файлу ошибки;
  - строку и колонку;
  - при наличии — путь к `source-of-truth` для generated-файла;
- список diagnostics показывает:
  - severity;
  - location;
  - source;
  - message;
- двойной щелчок по записи diagnostics теперь:
  - открывает точное место ошибки для обычного файла;
  - открывает `source-of-truth` для generated-файла DeltaQ;
- во время новой сборки список diagnostics очищается отдельно от текстового build log;
- текстовый `Build Output` больше не перехватывает активную вкладку после каждой новой строки, поэтому diagnostics не теряют фокус при накоплении ошибок.

**Зачем это сделано:**

- уйти от единственного текстового лога к более структурированному представлению ошибок;
- сократить путь от сообщения компилятора до нужного первоисточника;
- сделать generated/source-of-truth связь видимой не только в редакторе, но и в build diagnostics workflow.

**Технические изменения:**

- обновлены:
  - `src/core/MainWindow.cpp`
  - `tests/core/test_MainWindowBuildOutputNavigation.cpp`

**Проверка:**

- `cmake --build build --parallel --target deltaq test_MainWindowBuildOutputNavigation test_CompilerOutputParser test_MainWindowEditorActions test_GeneratedFileNavigation`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_MainWindowBuildOutputNavigation`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_CompilerOutputParser`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_MainWindowEditorActions`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GeneratedFileNavigation`

**Результат проверки:**

- `test_MainWindowBuildOutputNavigation` прошёл успешно, включая новый сценарий с `Build Diagnostics`;
- регрессионные тесты на parser, editor actions и generated navigation также прошли успешно.

**Открыто:**

- следующим логичным шагом можно уже не усиливать presentation build log, а поднимать уровень:
  - сделать более явное различение `source-of-truth` и generated файлов при открытии/редактировании;
  - либо перейти к следующему крупному подэтапу `Phase 1`, связанному с доказательством основного end-to-end сценария.

### Шаг 13 — дерево проекта по умолчанию открывает source-of-truth для generated-файлов

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в `ProjectTreeView` изменено поведение двойного щелчка по generated-файлу;
- теперь двойной щелчок по generated `.c/.h` из дерева проекта открывает не сам generated output, а его `source-of-truth`;
- для generated-файлов в контекстном меню дерево проекта теперь явно разделяет два сценария:
  - `Open Source of Truth`
  - `Open Generated File`
- для обычных файлов поведение `Open` не изменилось.

**Зачем это сделано:**

- сделать `source-of-truth` главным путём открытия, а generated output — вторичным режимом просмотра;
- уменьшить вероятность случайного редактирования generated-артефактов;
- усилить продуктовую модель, в которой граф/layout/модуль являются первичными, а generated код — производным результатом.

**Технические изменения:**

- обновлены:
  - `src/editor/ProjectTreeView.cpp`
  - `tests/editor/test_ProjectTreeView.cpp`

**Проверка:**

- `cmake --build build --parallel --target deltaq test_ProjectTreeView test_MainWindowBuildOutputNavigation`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ProjectTreeView`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_MainWindowBuildOutputNavigation`

**Результат проверки:**

- `test_ProjectTreeView` прошёл успешно, включая новый сценарий `generated -> source-of-truth by default`;
- `test_MainWindowBuildOutputNavigation` повторно прошёл успешно, что подтвердило отсутствие регрессии в навигации.

**Открыто:**

- следующим шагом уже логично переходить от навигационного UX к более системному признаку `source-of-truth`:
  - например, сделать generated-файлы менее доступными в сценариях ручного выбора/открытия;
  - либо переключиться на следующий крупный подэтап `Phase 1`, где доказывается основной end-to-end workflow на эталонном проекте.

### Шаг 14 — эталонный desktop template закреплён как end-to-end pre-build regression

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в `test_PreBuildProcessor` добавлен интеграционный сценарий для шаблонного desktop-проекта;
- тест теперь:
  - копирует реальный `resources/templates/desktop` в temporary project;
  - загружает core-модули из библиотеки `modules/`;
  - загружает граф и UI layout в `GraphStore` и `UILayoutStore`;
  - прогоняет `PreBuildProcessor` по шаблонному проекту;
  - проверяет generated-артефакты, их происхождение и ключевые runtime-фрагменты;
- дополнительно подтверждено, что generated `src/main.c` действительно содержит SDL/TTF/window/renderer и формируется из графа `main`.

**Зачем это сделано:**

- перейти от частных UX-улучшений к доказательству главного workflow на эталонном проекте;
- зафиксировать `desktop/UI flow` как регрессионный ориентир `Phase 1`;
- получить ранний сигнал, если шаблонный демонстрационный проект перестанет проходить pre-build pipeline.

**Технические изменения:**

- обновлён:
  - `tests/codegen/test_PreBuildProcessor.cpp`

**Проверка:**

- `cmake --build build --parallel --target test_PreBuildProcessor test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`

**Результат проверки:**

- `test_PreBuildProcessor` прошёл успешно, включая новый desktop template scenario;
- `test_GraphCompiler` повторно прошёл успешно, что подтвердило стабильность desktop runtime codegen.

**Открыто:**

- следующим шагом уже логично поднимать уровень ещё выше:
  - либо закрепить аналогичный reference flow для minimal console example;
  - либо перейти к более явному пользовательскому представлению статуса модуля и его проверки внутри IDE.

### Шаг 15 — minimal console template закреплён как второй reference workflow

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в `test_PreBuildProcessor` добавлен интеграционный сценарий для шаблонного console-проекта;
- тест теперь:
  - копирует реальный `resources/templates/console` в temporary project;
  - загружает core-модули из `modules/`;
  - загружает шаблонный граф `main` в `GraphStore`;
  - прогоняет `PreBuildProcessor`;
  - проверяет, что generated `src/main.c` получен из графа и содержит ключевые части console flow;
- дополнительно проверяется, что для console template pre-build генерирует только один артефакт `GraphSource`, без лишних UI-файлов.

**Зачем это сделано:**

- закрыть оба обязательных reference scenario из стратегии примеров:
  - `minimal console flow`
  - `desktop/UI flow`
- превратить главный workflow DeltaQ в измеримый regression baseline, а не только в продуктовую формулировку;
- получить ранний сигнал, если базовый graph-to-code pipeline перестанет работать даже на минимальном шаблоне.

**Технические изменения:**

- обновлён:
  - `tests/codegen/test_PreBuildProcessor.cpp`

**Проверка:**

- `cmake --build build --parallel --target test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`

**Результат проверки:**

- `test_PreBuildProcessor` прошёл успешно, включая:
  - `processesDesktopTemplateEndToEnd`
  - `processesConsoleTemplateEndToEnd`

**Открыто:**

- следующим шагом логично переходить уже от reference flows к состоянию самих модулей:
  - показать в IDE более явный статус проверки/готовности модуля;
  - либо начать готовить третий reference scenario про reusable composition и подмодули.

### Шаг 16 — в Module Manager добавлен явный статус готовности модуля

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в `ModuleManagerWidget` добавлена явная панель состояния модуля;
- панель теперь показывает:
  - итоговый статус (`Неготов`, `Требует проверки`, `Проверен`, `Изменён`, `Ошибка проверки`, `Библиотечный`);
  - подробную строку с четырьмя признаками:
    - корректность контракта;
    - наличие реализации;
    - тестовый статус;
    - режим редактирования;
- состояние модуля теперь синхронизируется не только при выборе, но и:
  - после компиляции;
  - после тестирования;
  - после изменения кода/свойств;
  - после сохранения;
- исправлен скрытый дефект, при котором already checked модуль мог получить `modified` уже во время простого открытия из-за сигналов UI при загрузке;
- состояние дерева модулей стало согласовано с новой summary:
  - tooltip листового элемента теперь раскрывает причину текущего статуса;
  - некорректный модуль больше не выглядит как нейтральный `untested`.

**Зачем это сделано:**

- сделать статус модуля понятным без чтения логов и без догадок по маленькой иконке;
- приблизить IDE к продуктовой модели, где модуль рассматривается как проверяемая единица, а не просто текст функции;
- усилить следующий шаг стратегии после reference workflows: видимость готовности самого строительного блока.

**Технические изменения:**

- обновлены:
  - `src/editor/ModuleManagerWidget.h`
  - `src/editor/ModuleManagerWidget.cpp`
  - `tests/CMakeLists.txt`
- добавлен тест:
  - `tests/editor/test_ModuleManagerWidget.cpp`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target test_ModuleManagerWidget test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModuleManagerWidget`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`

**Результат проверки:**

- `test_ModuleManagerWidget` прошёл успешно, включая сценарии:
  - `Неготов` для модуля без реализации;
  - `Библиотечный` для core-модуля;
  - отсутствие ложного `modified` при простом открытии проверенного модуля;
  - переход в `Изменён` только после реальной правки;
- `test_PreBuildProcessor` повторно прошёл успешно, что подтвердило отсутствие регрессии в reference workflows.

**Открыто:**

- следующим шагом уже логично идти либо в третий reference scenario про reusable composition и подмодули;
- либо дальше усиливать модель модуля, например показывать отдельно статус контракта, реализации и тестов в библиотеке модулей и палитре блоков.

### Шаг 17 — введён жёсткий допуск модулей к композиции и codegen

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- в `ModuleRegistry` добавлено явное правило допуска модуля к композиции;
- теперь для атомарного пользовательского модуля проверяется:
  - валидность контракта;
  - наличие реализации;
  - прохождение тестов (`testStatus == passed`);
- `core` и `ui` модули считаются доверенными библиотечными;
- для графовых составных модулей пока допускается использование по валидному интерфейсу и внутренней компиляции графа;
- `GraphCompiler` теперь отказывает в codegen, если узел ссылается на модуль, который ещё не допущен к использованию;
- это правило покрыто отдельным тестом: сырой `untested` модуль больше не попадает в сборку даже если текст функции уже существует.

**Зачем это сделано:**

- довести до продукта основную идею DeltaQ: в композиции участвуют не любые куски текста, а только допущенные модули;
- убрать разрыв между философией "проверенный модуль" и фактическим codegen pipeline;
- сделать IDE строже и ближе к модульной дисциплине, а не просто к визуальному конструктору.

**Технические изменения:**

- обновлены:
  - `src/core/ModuleRegistry.h`
  - `src/core/ModuleRegistry.cpp`
  - `src/codegen/GraphCompiler.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`

**Проверка:**

- `cmake --build build --parallel --target test_GraphCompiler test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`

**Результат проверки:**

- `test_GraphCompiler` прошёл успешно, включая новый сценарий отказа для `untested` модуля;
- `test_PreBuildProcessor` повторно прошёл успешно, что подтвердило сохранение рабочих reference flows.

**Открыто:**

- следующим шагом уже логично усиливать ту же дисциплину на уровне составных модулей и reuse;
- либо показать статус допуска модуля не только в `Module Manager`, но и в палитре/графе до попытки сборки.

### Шаг 18 — допуск модулей вынесен в палитру и вставку в граф

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- `BlockScene` теперь умеет заранее отвечать, можно ли вставить модуль в текущий граф;
- при `drop` больше нельзя добавить модуль, который:
  - не найден в реестре;
  - ещё не допущен к использованию;
  - создаёт циклическую зависимость;
- `ModulePalette` теперь показывает статус допуска прямо в дереве модулей:
  - недопущенный модуль получает суффикс `[не готов]`;
  - tooltip показывает причину недопуска;
  - такой модуль нельзя перетаскивать из палитры;
- готовый модуль остаётся обычным drag&drop-элементом без лишних ограничений.

**Зачем это сделано:**

- перенести правило "в граф попадают только допущенные модули" на более ранний этап, а не ждать отказа уже в `GraphCompiler`;
- сделать статус модуля видимым в том месте, где пользователь реально выбирает строительные блоки;
- убрать ситуацию, когда черновой модуль выглядит доступным, но рушит workflow только на codegen.

**Технические изменения:**

- обновлены:
  - `src/blockEditor/BlockScene.h`
  - `src/blockEditor/BlockScene.cpp`
  - `src/blockEditor/ModulePalette.cpp`
  - `tests/blockEditor/test_BlockScene.cpp`
  - `tests/CMakeLists.txt`
- добавлен тест:
  - `tests/blockEditor/test_ModulePalette.cpp`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target test_BlockScene test_ModulePalette`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_BlockScene`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModulePalette`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`

**Результат проверки:**

- `test_BlockScene` прошёл успешно, включая новый сценарий запрета для `untested` модуля;
- `test_ModulePalette` прошёл успешно и подтвердил:
  - маркировку `[не готов]`;
  - отключение drag для недопущенного модуля;
  - показ причины в tooltip;
- `test_GraphCompiler` повторно прошёл успешно, что подтвердило согласованность правила допуска между UI и codegen.

**Открыто:**

- следующим шагом уже логично переносить такую же строгость на составные модули и reusable composition;
- либо показывать более детальный статус допуска в библиотеке модулей: контракт, реализация, проверка, составной/атомарный режим.

### Шаг 19 — составные модули получили реальный допуск по внутреннему графу

**Фаза:** `Phase 1: Core Stabilization`

**Что сделано:**

- `ModuleRegistry` теперь проверяет `graph`-модуль не только по `graphId`, а по реальному состоянию внутреннего графа;
- составной модуль считается допущенным только если:
  - к `ModuleRegistry` подключён `GraphStore`;
  - внутренний граф найден;
  - внутренний граф валиден;
  - внутренний граф не принадлежит другому модулю;
  - все узлы внутреннего графа ссылаются на существующие и уже допущенные модули;
  - во внутреннем графе нет рекурсивной петли по зависимостям составных модулей;
  - все внутренние соединения ссылаются на реальные узлы и порты;
- `MainWindow` теперь сразу связывает `ModuleRegistry` с `GraphStore`, чтобы правило работало во всей IDE;
- `Module Manager` получил более точную сводку:
  - показывает `Тип: атомарный/составной`;
  - показывает `Допуск: есть/нет`;
  - для составного модуля без внутреннего графа явно показывает `Неготов`.

**Зачем это сделано:**

- убрать ложное ощущение готовности у составных модулей, которые раньше считались допустимыми уже по одному заполненному `graphId`;
- выровнять модель атомарного и составного модуля через одно правило допуска;
- перенести дисциплину "используем только реально готовые строительные блоки" на reuse и подмодули.

**Технические изменения:**

- обновлены:
  - `src/core/ModuleRegistry.h`
  - `src/core/ModuleRegistry.cpp`
  - `src/core/MainWindow.cpp`
  - `src/editor/ModuleManagerWidget.cpp`
  - `tests/blockEditor/test_ModulePalette.cpp`
  - `tests/editor/test_ModuleManagerWidget.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`

**Проверка:**

- `cmake --build build --parallel --target test_ModulePalette test_ModuleManagerWidget test_GraphCompiler test_BlockScene`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModulePalette`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModuleManagerWidget`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_BlockScene`

**Результат проверки:**

- `test_ModulePalette` прошёл успешно, включая новый сценарий с недоступным составным модулем;
- `test_ModuleManagerWidget` прошёл успешно и подтвердил явный статус `Неготов` для сломанного `graph`-модуля;
- `test_GraphCompiler` прошёл успешно, включая новый отказ для составного модуля без внутреннего графа;
- `test_BlockScene` повторно прошёл успешно, что подтвердило отсутствие регрессии в предыдущем шаге.

**Открыто:**

- следующим шагом уже логично переходить от допуска составного модуля к его полноценной boundary-модели:
  - как именно внешние входы/выходы подмодуля отображаются во внутренний граф;
  - как этот reusable module проходит полноценный end-to-end codegen без скрытых заглушек.

### Планирование — добавлен отдельный стратегический план по субмодулям как compile units

**Что сделано:**

- добавлен новый документ `25_submodule_compilation_units.md`;
- в нём зафиксирована модель, где подмодуль:
  - хранится как отдельный `.dqgraph`;
  - имеет отдельный `.dqmod`;
  - генерирует собственные `.h/.c`;
  - подключается в родительский граф как обычный модуль через header и вызов функции;
- отдельно зафиксировано, что концептуально это "подключаемый готовый блок", близкий к `include`,
  но технически правильная реализация должна идти через отдельную единицу компиляции;
- `00_master_strategy.md` обновлён ссылкой на новый документ;
- `22_examples_and_reference_projects.md` уточнён требованием показывать reusable composition
  именно через отдельные generated source units.

**Зачем это сделано:**

- зафиксировать следующий крупный шаг стратегии до начала реализации;
- убрать двусмысленность вокруг того, что такое подмодуль в generated/build pipeline;
- перевести разговор о "матрёшках" в конкретный implementation plan.

### Шаг 20 — реализован Stage 1: boundary metadata для субмодулей

**Фаза:** `Submodule Compilation Units / Stage 1`

**Что сделано:**

- в модель `Module` добавлены явные boundary mappings для составного модуля:
  - `boundaryInputs`
  - `boundaryOutputs`
- каждый boundary binding теперь хранит:
  - имя внешнего порта;
  - ID внутреннего узла;
  - имя внутреннего порта;
  - kind (`data` / `execution`);
- `.dqmod` сериализация расширена секцией `boundary`, поэтому эта информация теперь является частью source-of-truth контракта составного модуля;
- `SubModuleFactory` при создании подмодуля теперь не только вычисляет `inputs/outputs`, но и сразу заполняет boundary metadata;
- `ModuleRegistry` начал валидировать boundary metadata у `graph`-модулей:
  - количество bindings должно совпадать с количеством внешних портов;
  - каждый binding должен ссылаться на реальный внутренний узел и порт;
  - типы и `kind` внешнего и внутреннего порта должны совпадать;
  - внешний порт не может быть без boundary mapping;
- из-за этого составной модуль с заполненным интерфейсом, но без boundary metadata, больше не считается допущенным к композиции.

**Зачем это сделано:**

- создать машиночитаемую модель границы субмодуля до начала генерации отдельных `.h/.c`;
- убрать скрытое знание о том, как внешний контракт связан с внутренним графом;
- подготовить базу для следующего этапа, где субмодуль будет генерироваться как отдельная единица компиляции.

**Технические изменения:**

- обновлены:
  - `include/deltaq/Module.h`
  - `src/blockEditor/SubModuleFactory.cpp`
  - `src/core/ModuleRegistry.cpp`
  - `tests/core/test_Module.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`
  - `tests/CMakeLists.txt`
- добавлен тест:
  - `tests/blockEditor/test_SubModuleFactory.cpp`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target test_Module test_SubModuleFactory test_GraphCompiler test_ModuleManagerWidget`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_Module`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SubModuleFactory`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModuleManagerWidget`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModulePalette`

**Результат проверки:**

- `test_Module` прошёл успешно, включая roundtrip boundary metadata;
- `test_SubModuleFactory` прошёл успешно и подтвердил, что новый подмодуль получает корректные boundary bindings;
- `test_GraphCompiler` прошёл успешно, включая новый отказ для составного модуля без boundary metadata;
- `test_ModuleManagerWidget` и `test_ModulePalette` повторно прошли успешно, что подтвердило совместимость нового admission rule с UI.

**Открыто:**

- следующим шагом уже логично переходить к `Stage 2`:
  - генерация отдельных `submodule.h/.c`;
  - naming strategy;
  - origin metadata для generated submodule files.

### Шаг 21 — реализован Stage 2: отдельные `submodule.h/.c` для составных модулей

**Фаза:** `Submodule Compilation Units / Stage 2`

**Что сделано:**

- `GraphCompiler` научился генерировать отдельную единицу компиляции для `graph`-модуля:
  - отдельный generated header;
  - отдельный generated source;
  - стабильное имя файла;
  - стабильное имя callable-функции;
  - отдельный result type для подмодулей с несколькими data-выходами;
- `IR` расширен методом `emitBodyCode()`, чтобы одно и то же промежуточное представление можно было использовать не только для `main()`, но и для тела generated функции подмодуля;
- при компиляции родительского графа составной модуль теперь подключается как отдельный unit:
  - через `#include "generated/submodules/<name>.h"`;
  - через вызов generated-функции подмодуля;
- `PreBuildProcessor` получил отдельный этап `processSubmodules()`:
  - сначала генерируются `.h/.c` для всех составных модулей;
  - затем компилируются только корневые графы проекта;
  - внутренние графы подмодулей больше не пишутся как обычные корневые `src/<graph>.c`;
- `BuildPipeline` обновлён новыми типами артефактов:
  - `submodule header`
  - `submodule implementation`

**Зачем это сделано:**

- перевести составной модуль из "внутренней матрёшки" в полноценную единицу сборки;
- сделать поведение подмодуля ближе к обычному reusable module:
  - у него есть собственный source-of-truth граф;
  - у него есть собственные generated исходники;
  - родительский граф использует его через явный интерфейс, а не через скрытое разворачивание;
- подготовить базу для следующего этапа, где reusable composition будет проверяться уже как полноценный end-to-end build scenario.

**Технические изменения:**

- обновлены:
  - `src/codegen/IR.h`
  - `src/codegen/IR.cpp`
  - `src/codegen/GraphCompiler.h`
  - `src/codegen/GraphCompiler.cpp`
  - `src/codegen/PreBuildProcessor.h`
  - `src/codegen/PreBuildProcessor.cpp`
  - `src/codegen/BuildPipeline.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`
  - `tests/codegen/test_PreBuildProcessor.cpp`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target test_IR test_GraphCompiler test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_IR`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`

**Результат проверки:**

- `test_IR` прошёл успешно и подтвердил, что новое `emitBodyCode()` не сломало прежнюю генерацию `main()`;
- `test_GraphCompiler` прошёл успешно, включая новый сценарий отдельной генерации `submodule.h/.c`;
- `test_PreBuildProcessor` прошёл успешно, включая новый интеграционный сценарий:
  - generated `main.c`;
  - generated `src/generated/submodules/*.h`;
  - generated `src/generated/submodules/*.c`.

**Открыто:**

- следующим шагом логично усиливать уже не сам факт генерации unit-файлов, а boundary execution model для подмодулей:
  - как выражать `execution`-входы и `execution`-выходы в generated callable submodule;
  - нужен ли отдельный pattern для подмодулей без data-return, но с выраженной exec-цепочкой;
  - как закрепить reusable composition в полноценном reference scenario c несколькими подмодулями.

### Шаг 22 — для подмодуля добавлено отдельное имя generated-файлов

**Фаза:** `Submodule Compilation Units / Naming`

**Что сделано:**

- в модель `Module` добавлено отдельное поле `generatedFileBaseName`;
- это поле сериализуется в `.dqmod` как `generated_file_base`;
- `GraphCompiler` при генерации `submodule.h/.c` теперь в первую очередь использует именно это имя;
- если поле не задано, для обратной совместимости остаётся старый fallback на основе имени модуля и его ID;
- в `BlockEditorWidget` создание подмодуля переведено с одного `QInputDialog` на отдельный диалог с двумя полями:
  - имя подмодуля;
  - имя generated-файла без расширения;
- если поле generated-файла оставить пустым, IDE автоматически подставляет и сохраняет следующее свободное имя:
  - `submodule_01`
  - `submodule_02`
  - и так далее;
- добавлена проверка уникальности generated file base среди существующих составных модулей.

**Зачем это сделано:**

- отделить отображаемое имя подмодуля от имени файлов, которые реально участвуют в сборке;
- дать пользователю предсказуемый контроль над generated `.h/.c`;
- избежать случайных имён и скрытой логики в naming strategy для reusable submodules.

**Технические изменения:**

- обновлены:
  - `include/deltaq/Module.h`
  - `src/codegen/GraphCompiler.cpp`
  - `src/blockEditor/BlockEditorWidget.cpp`
  - `tests/core/test_Module.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`
  - `tests/codegen/test_PreBuildProcessor.cpp`

**Проверка:**

- `cmake --build build --parallel --target test_Module test_GraphCompiler test_PreBuildProcessor test_BlockScene`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_Module`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_BlockScene`

**Результат проверки:**

- `test_Module` прошёл успешно и подтвердил сериализацию `generated_file_base`;
- `test_GraphCompiler` прошёл успешно и подтвердил использование явного имени `submodule_01` при codegen подмодуля;
- `test_PreBuildProcessor` прошёл успешно и подтвердил запись `submodule_01.h/.c` на диск;
- `test_BlockScene` повторно прошёл успешно, что подтвердило отсутствие регрессии в блочном редакторе.

### Шаг 23 — добавлен сводный checklist статусов по всей стратегии

**Фаза:** организационное сопровождение `strategy_2026`

**Что сделано:**

- добавлен новый файл `98_strategy_checklist.md`;
- в нём собран короткий список задач по всему набору планов:
  - foundation;
  - `Phase 1`;
  - `Submodule Compilation Units`;
  - `Phase 2`;
  - examples;
  - `Phase 3`;
  - acceptance gates;
- для каждого пункта введён явный статус:
  - выполнено;
  - не выполнено;
  - частично;
- отдельно вынесен верхний блок `Phase Status`, чтобы было видно, какой этап стратегии уже закрыт, а какой нет;
- `00_master_strategy.md` обновлён ссылкой на новый checklist.

**Зачем это сделано:**

- дополнить хронологический журнал неисторическим, а актуальным видом состояния стратегии;
- дать короткий список задач без необходимости читать весь execution log;
- упростить контроль: что уже закрыто, что осталось и где реально находится проект.

**Технические изменения:**

- добавлены/обновлены:
  - `docs/plan/strategy_2026/98_strategy_checklist.md`
  - `docs/plan/strategy_2026/00_master_strategy.md`

**Проверка:**

- проверена структура `strategy_2026`;
- проверена связность нового checklist с `00_master_strategy.md`;
- проверено, что checklist отражает уже выполненные шаги из `99_execution_log.md`.

### Шаг 24 — Stage 3 начат через exec-boundary и reuse подмодуля

**Фаза:** `Submodule Compilation Units / Stage 3`

**Что сделано:**

- `SubModuleFactory` теперь формирует не только `module + innerGraph`, но и обновлённый родительский граф:
  - выделенные узлы заменяются одним узлом подмодуля;
  - внешние `data`-связи переподключаются на новый узел;
  - внешние `execution`-связи тоже переподключаются на новый узел;
- `BlockEditorWidget` переведён на эту модель:
  - после создания подмодуля не теряет внешние связи группы;
  - подставляет в текущий граф уже готовый `updatedParentGraph`;
- добавлен regression test на boundary-поведение:
  - после сворачивания выделения подмодуль сохраняет внешний `flow_in`, `flow_out`, `value`, `result`;
  - родительский граф содержит корректные переподключённые `exec/data` связи;
- добавлен `GraphCompiler` сценарий `execAwareCompositeModuleCanBeReused`:
  - подмодуль с `flow_in/flow_out` и data-портами компилируется в отдельный unit;
  - один и тот же подмодуль вызывается в корневом графе несколько раз;
  - родительский generated `main.c` показывает явные повторные вызовы одной и той же generated-функции;
- добавлен `PreBuildProcessor` сценарий `reusesExecAwareCompositeSubmoduleInRootGraph`:
  - генерируются `submodule_exec_01.h/.c`;
  - корневой `main.c` подключает header;
  - reuse подтверждается двумя вызовами одного generated submodule function.

**Зачем это сделано:**

- закрыть реальный пробел между "подмодуль существует" и "подмодуль пригоден для reuse";
- сделать boundary-модель подмодуля корректной не только для данных, но и для execution flow;
- подготовить основу для следующего шага, где reusable composition станет уже полноценным reference example.

**Технические изменения:**

- обновлены:
  - `src/blockEditor/SubModuleFactory.h`
  - `src/blockEditor/SubModuleFactory.cpp`
  - `src/blockEditor/BlockEditorWidget.cpp`
  - `tests/blockEditor/test_SubModuleFactory.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`
  - `tests/codegen/test_PreBuildProcessor.cpp`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_SubModuleFactory test_GraphCompiler test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SubModuleFactory`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`
- дополнительно выполнена принудительная пересборка тестов:
  - `cmake --build build --parallel --clean-first --target test_GraphCompiler test_PreBuildProcessor`

**Результат проверки:**

- `test_SubModuleFactory` прошёл успешно и подтвердил автопереподключение внешних `exec/data` связей;
- `test_GraphCompiler` прошёл успешно, включая новый сценарий `execAwareCompositeModuleCanBeReused`;
- `test_PreBuildProcessor` прошёл успешно, включая новый сценарий `reusesExecAwareCompositeSubmoduleInRootGraph`;
- после `clean-first` подтверждено, что новые `QTest` case действительно вошли в итоговые бинарники.

**Открыто:**

- следующим шагом логично переходить от regression-сценариев reuse к более крупному reference example:
  - несколько подмодулей;
  - демонстрация практической ценности reuse;
  - затем оставшийся `Phase 1` acceptance: стабильный `build -> run` smoke.

### Шаг 25 — закрыт `build -> run` smoke и оформлен reference example для reusable composition

**Фаза:** `Phase 1 / Core Workflow Reliability` и `Submodule Compilation Units / Stage 5`

**Что сделано:**

- исправлен `consoleTemplateBuildsAndRunsEndToEnd`:
  - вывод `QProcess` теперь считывается один раз после завершения процесса;
  - тест стал стабильно проверять реальный runtime output, а не пустой буфер после повторного чтения;
- убрана хрупкая привязка составного модуля к `origin == "graph"`:
  - теперь признак составного модуля определяется по наличию `graphId`;
  - это позволяет файловым подмодулям из `dqmods/`, загруженным как `local`, вести себя как настоящие составные модули;
- обновлены рабочие пути IDE и codegen:
  - `ModuleRegistry`
  - `PreBuildProcessor`
  - `GraphCompiler`
  - `BlockEditorWidget`
  - `ModuleManagerWidget`
- добавлен полноценный reference project:
  - `resources/examples/reusable_composition_console`
  - включает:
    - корневой `main.dqgraph`
    - два составных модуля в `dqmods/`
    - два внутренних графа подмодулей
    - `README.md` с описанием сценария и ожидаемого вывода;
- в example-проекте показан практический reuse:
  - `echo_with_prefix` используется дважды;
  - `measure_text` подготавливает данные для второго вызова;
  - при `pre-build` появляются отдельные `echo_with_prefix.h/.c` и `measure_text.h/.c`;
- добавлен новый end-to-end тест `reusableCompositionExampleBuildsAndRunsEndToEnd`:
  - копирует reference project;
  - загружает глобальные и локальные модули;
  - выполняет `pre-build`;
  - запускает `cmake configure`;
  - собирает проект;
  - запускает готовый бинарник;
  - проверяет вывод:
    - `Hello, DeltaQ`
    - `Length: 6`;
- `deltaq` теперь копирует reference examples рядом с исполняемым файлом в каталог `examples/`.

**Зачем это сделано:**

- закрыть оставшийся `Phase 1` acceptance на реальном `build -> run`;
- доказать, что reusable composition работает не только в in-memory/regression сценариях, но и как обычный файловый проект;
- выровнять поведение "только что созданного" и "загруженного с диска" составного модуля.

**Технические изменения:**

- обновлены:
  - `include/deltaq/Module.h`
  - `src/core/ModuleRegistry.cpp`
  - `src/codegen/PreBuildProcessor.cpp`
  - `src/codegen/GraphCompiler.cpp`
  - `src/blockEditor/BlockEditorWidget.cpp`
  - `src/editor/ModuleManagerWidget.cpp`
  - `src/CMakeLists.txt`
  - `tests/codegen/test_PreBuildProcessor.cpp`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`
  - `docs/plan/strategy_2026/22_examples_and_reference_projects.md`
- добавлены:
  - `resources/examples/reusable_composition_console/README.md`
  - `resources/examples/reusable_composition_console/reusable_composition_console.dqproj`
  - `resources/examples/reusable_composition_console/src/main.c`
  - `resources/examples/reusable_composition_console/dqmods/example.graph.echo_with_prefix.dqmod`
  - `resources/examples/reusable_composition_console/dqmods/example.graph.measure_text.dqmod`
  - `resources/examples/reusable_composition_console/graphs/echo_with_prefix_graph.dqgraph`
  - `resources/examples/reusable_composition_console/graphs/measure_text_graph.dqgraph`
  - `resources/examples/reusable_composition_console/graphs/main.dqgraph`

**Проверка:**

- `cmake --build build --parallel --target test_PreBuildProcessor test_GraphCompiler test_ModuleManagerWidget`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModuleManagerWidget`
- `cmake --build build --parallel --target deltaq`

**Результат проверки:**

- `test_PreBuildProcessor` прошёл полностью, включая:
  - `consoleTemplateBuildsAndRunsEndToEnd`
  - `reusableCompositionExampleBuildsAndRunsEndToEnd`;
- `test_GraphCompiler` прошёл полностью после перехода на `graphId` как признак составного модуля;
- `test_ModuleManagerWidget` прошёл полностью и подтвердил отсутствие регрессии в статусах модулей;
- `deltaq` пересобран успешно и теперь копирует `examples/` в build output.

**Открыто:**

- `Phase 1` по-прежнему не закрыт полностью, потому что остаётся незавершённой более строгая модель верификации модуля:
  - отдельная валидность контракта;
  - compile check;
  - test check;
- следующий крупный смысловой шаг логично брать уже не в reusable composition, а в сторону `Phase 2 / Module Verification`.

### Шаг 26 — staged verification для модуля разведён на `contract / implementation / compile / test`

**Фаза:** завершение `Phase 1 / Module Readiness And Admission` и старт `Phase 2 / Module Verification`

**Что сделано:**

- модель модуля расширена отдельным `compileStatus`:
  - `passed`
  - `failed`
  - `unknown`
  - `modified`;
- сериализация `.dqmod` переведена на новое поле `verification`:
  - `compile_status`
  - `test_status`;
- сохранена обратная совместимость:
  - старое поле `testing.status` продолжает читаться;
  - для старых модулей успешный legacy test status автоматически поднимает `compileStatus`;
- допуск атомарного пользовательского модуля в композицию теперь требует двух независимых стадий:
  - `compileStatus == passed`
  - `testStatus == passed`;
- `Module Manager` теперь показывает staged verification как отдельные этапы:
  - валидность контракта;
  - наличие реализации;
  - compile check;
  - test check;
- badges и итоговый статус атомарного модуля стали точнее:
  - `Неготов`
  - `Ошибка компиляции`
  - `Ошибка проверки`
  - `Требует компиляции`
  - `Требует проверки`
  - `Изменён`
  - `Проверен`;
- `onCodeChanged` теперь инвалидирует обе стадии проверки:
  - успешную/ошибочную компиляцию переводит в `modified`;
  - успешный/ошибочный тест переводит в `modified`;
- `ModuleTestRunner::runTest()` теперь тоже пробрасывает `compilationFinished`, чтобы IDE видела compile stage и во время тестового запуска;
- staged verification встроен в ключевые места IDE:
  - `Module Manager`
  - `GraphCompiler`
  - `PreBuildProcessor`
  - `BlockScene`
  - `ModulePalette`;
- добавлен отдельный regression test на отказ модуля без compile check;
- добавлен regression test на состояние `compiled but untested`.

**Зачем это сделано:**

- перестать схлопывать качество модуля в один `testStatus`;
- реализовать твою исходную идею "в сборку попадает только проверенный модуль" уже как дисциплину IDE;
- закрыть пробел между `есть текст функции` и `модуль реально допущен к использованию`.

**Технические изменения:**

- обновлены:
  - `include/deltaq/Module.h`
  - `src/core/ModuleRegistry.cpp`
  - `src/editor/ModuleManagerWidget.h`
  - `src/editor/ModuleManagerWidget.cpp`
  - `src/editor/ModuleTestRunner.cpp`
  - `tests/core/test_Module.cpp`
  - `tests/editor/test_ModuleManagerWidget.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`
  - `tests/codegen/test_PreBuildProcessor.cpp`
  - `tests/blockEditor/test_BlockScene.cpp`
  - `tests/blockEditor/test_ModulePalette.cpp`
  - `tests/blockEditor/test_SubModuleFactory.cpp`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_Module test_ModuleManagerWidget test_GraphCompiler test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_Module`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModuleManagerWidget`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`
- `cmake --build build --parallel --target test_BlockScene test_ModulePalette`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_BlockScene`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModulePalette`

**Результат проверки:**

- `test_Module` прошёл успешно и подтвердил новую сериализацию verification + legacy fallback;
- `test_ModuleManagerWidget` прошёл успешно и подтвердил staged verification в UI;
- `test_GraphCompiler` прошёл успешно, включая отдельный отказ для отсутствующего compile check;
- `test_PreBuildProcessor` прошёл успешно и подтвердил отсутствие регрессии в reference flows;
- `test_BlockScene` и `test_ModulePalette` прошли успешно, что подтвердило сохранение gating-логики в редакторе графов.

**Итог:**

- `Phase 1: Core Stabilization` можно считать закрытой;
- следующий крупный шаг уже не про admission, а про содержательную часть `Phase 2`:
  - стандарт качества библиотеки модулей;
  - выравнивание naming и интерфейсов;
  - документацию модулей.

### Шаг 27 — добавлен стратегический план `UI как модули + SDL2 как backend v1`

**Фаза:** подготовка архитектурного направления после закрытия `Phase 1`

**Что сделано:**

- добавлен новый стратегический документ:
  - `26_ui_contract_and_backends.md`;
- в документе зафиксирована позиция:
  - пользовательский UI должен быть выражен backend-независимыми UI-модулями;
  - SDL2 не нужно выбрасывать сейчас;
  - SDL2 должен рассматриваться как backend v1, а не как продуктовая сущность UI;
- зафиксирована трёхслойная модель UI:
  - `UI Contract Layer`
  - `UI Runtime Model`
  - `UI Backend Layer`;
- отдельно зафиксировано правило:
  - нельзя допускать Windows/Linux-specific пользовательские UI-графы;
  - различия между ОС должны жить в backend-пакетах, а не в пользовательских `ui.button/ui.label`;
- описаны workstreams и stages:
  - определение UI contract;
  - выделение SDL2 backend boundary;
  - консолидация UI runtime model;
  - выравнивание `.dqui` и UI-модулей;
  - возможные native backend-ы позже;
- `00_master_strategy.md` обновлён ссылкой на новый документ.

**Зачем это сделано:**

- зафиксировать архитектурный ответ на вопрос "делать ли UI своими модулями или оставаться на SDL2";
- не сломать текущий рабочий UI path, но и не зацементировать SDL2 как окончательную сущность UI;
- подготовить основу для следующего большого обсуждения и реализации UI subsystem.

**Технические изменения:**

- добавлены/обновлены:
  - `docs/plan/strategy_2026/26_ui_contract_and_backends.md`
  - `docs/plan/strategy_2026/00_master_strategy.md`

**Проверка:**

- проверена структура `strategy_2026`;
- проверена связность нового документа с master strategy;
- проверено, что новый документ согласован с общей философией:
  - модульность
  - прозрачность
  - backend-независимый пользовательский уровень.

### Шаг 28 — реализован `UI Stage 1`: backend-независимый UI contract baseline

**Фаза:** `UI Contract And Backends / Stage 1`

**Что сделано:**

- в `Module` добавлены расширяемые `metadata`, чтобы UI-контракт описывался явно, а не угадывался по SDL2-коду;
- в `UILayout::create()` добавлен baseline metadata для backend-независимого UI layout contract;
- `UIModuleFactory` переведён с SDL2-реализаций на contract-модули:
  - UI-модули теперь маркируются как `ui_contract`;
  - для них сохраняются `widget_type`, `properties`, `events`, `state_outputs`;
  - preview/source внутри модуля теперь объясняет контракт и backend boundary, а не содержит SDL2 API;
- `GraphCompiler` теперь распознаёт UI contract по metadata и использует её как основной путь, сохраняя legacy fallback по старым `id/name`;
- `SDL2CodeGenerator` теперь явно помечает generated UI files как слой `contract` + backend `SDL2`;
- добавлены тесты на UI contract metadata и на новый статус `SDL2` как backend.

**Зачем это сделано:**

- закрепить первое практическое разделение между:
  - пользовательским UI contract;
  - backend-реализацией SDL2;
- перестать представлять SDL2-специфичный код как сам пользовательский UI-модуль;
- подготовить основу для следующих стадий:
  - backend boundary;
  - alignment `.dqui` и UI-модулей;
  - выделение общего UI runtime model.

**Технические изменения:**

- добавлены/обновлены:
  - `include/deltaq/Module.h`
  - `include/deltaq/UILayout.h`
  - `src/uiDesigner/UIModuleFactory.h`
  - `src/uiDesigner/UIModuleFactory.cpp`
  - `src/uiDesigner/SDL2CodeGenerator.cpp`
  - `src/codegen/GraphCompiler.cpp`
  - `tests/core/test_Module.cpp`
  - `tests/core/test_UILayout.cpp`
  - `tests/uiDesigner/test_SDL2CodeGenerator.cpp`
  - `tests/uiDesigner/test_UIModuleFactory.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`
  - `tests/CMakeLists.txt`
  - `docs/plan/strategy_2026/26_ui_contract_and_backends.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target test_Module test_UILayout test_UIModuleFactory test_SDL2CodeGenerator test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_Module`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_UILayout`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_UIModuleFactory`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SDL2CodeGenerator`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `cmake --build build --parallel --target deltaq`

**Результат проверки:**

- все целевые тесты прошли;
- `deltaq` собирается без регрессий;
- UI contract metadata сериализуется и читается корректно;
- `UIModuleFactory` подтверждён как источник backend-независимых contract-модулей;
- `SDL2CodeGenerator` по-прежнему работает, но теперь явно маркируется как backend.

**Итог:**

- `UI Stage 1` можно считать выполненным;
- следующий логичный шаг по UI-ветке:
  - `Stage 2: SDL2 as backend v1`;
  - то есть выделение явной backend boundary для окна, событий и рендера.

### Шаг 29 — начат `UI Stage 2`: SDL2 вынесен в явную backend boundary для generated UI

**Фаза:** `UI Contract And Backends / Stage 2`

**Что сделано:**

- в generated UI-коде введены явные backend-типы:
  - `DQ_UIBackendContext`
  - `DQ_UIBackendRenderer`
  - `DQ_UIBackendEvent`
  - `DQ_UIBackendFont`;
- в generated `ui.h` и `ui.c` введён backend API:
  - `dq_ui_backend_init`
  - `dq_ui_backend_shutdown`
  - `dq_ui_backend_poll_event`
  - `dq_ui_backend_event_is_quit`
  - `dq_ui_backend_ticks`
  - `dq_ui_backend_begin_frame`
  - `dq_ui_backend_end_frame`
  - функции чтения mouse state/event coordinates;
- generated `main.c` для `.dqui` больше не содержит прямой orchestration через `SDL_Init`, `SDL_CreateWindow`, `SDL_CreateRenderer`, `SDL_PollEvent`, `SDL_RenderPresent`;
- orchestration окна, event pump и кадра теперь идёт через backend helper functions, а SDL2 остаётся их реализацией в generated `ui.c`;
- `core.desktop.*` модули получили metadata:
  - `deltaq.kind = ui_backend_runtime`
  - `deltaq.ui.layer = backend`
  - `deltaq.ui.backend = sdl2`
  - `deltaq.ui.backend_role = ...`;
- интеграционные тесты обновлены так, чтобы проверять не только сам SDL2 codegen, но и наличие backend boundary в generated artifacts и metadata у desktop runtime модулей.

**Зачем это сделано:**

- сделать SDL2 backend-слоем не только концептуально, но и технически;
- убрать прямое протекание low-level window/event/frame orchestration в generated `main.c`;
- подготовить следующий этап, где backend boundary можно будет использовать и для дальнейшего выравнивания desktop runtime modules.

**Технические изменения:**

- добавлены/обновлены:
  - `src/uiDesigner/SDL2CodeGenerator.cpp`
  - `modules/core/desktop/sdl_init.dqmod`
  - `modules/core/desktop/ttf_init.dqmod`
  - `modules/core/desktop/create_window.dqmod`
  - `modules/core/desktop/create_renderer.dqmod`
  - `modules/core/desktop/ui_init.dqmod`
  - `modules/core/desktop/event_loop.dqmod`
  - `modules/core/desktop/ui_cleanup_font.dqmod`
  - `modules/core/desktop/destroy_renderer.dqmod`
  - `modules/core/desktop/destroy_window.dqmod`
  - `modules/core/desktop/ttf_quit.dqmod`
  - `modules/core/desktop/sdl_quit.dqmod`
  - `tests/uiDesigner/test_SDL2CodeGenerator.cpp`
  - `tests/codegen/test_PreBuildProcessor.cpp`
  - `docs/plan/strategy_2026/26_ui_contract_and_backends.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_SDL2CodeGenerator test_PreBuildProcessor test_GraphCompiler deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SDL2CodeGenerator`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`

**Результат проверки:**

- все целевые тесты прошли;
- `deltaq` собирается без регрессий;
- generated UI артефакты теперь содержат явную backend boundary;
- desktop runtime modules загружаются с metadata backend-слоя `sdl2`.

**Итог:**

- `Stage 2` продвинут вперёд, но ещё не закрыт полностью;
- следующий логичный шаг:
  - выровнять inline desktop runtime codegen с тем же backend boundary;
  - затем двигаться к `Stage 3: Runtime Model Consolidation`.

### Шаг 30 — inline desktop runtime codegen переведён на `dq_ui_backend_*`

**Фаза:** `UI Contract And Backends / Stage 2`

**Что сделано:**

- `GraphCompiler` теперь заводит единый inline desktop backend context:
  - `DQ_UIBackendContext dq_ui_backend_ctx`;
- inline desktop-модули переведены с прямых `SDL_*` вызовов на backend boundary:
  - `create_window` -> `dq_ui_backend_init` + `dq_ui_backend_window`
  - `create_renderer` -> `dq_ui_backend_renderer`
  - `event_loop` -> `dq_ui_backend_poll_event`, `dq_ui_backend_event_is_quit`, `dq_ui_backend_begin_frame`, `dq_ui_backend_end_frame`
  - `destroy_renderer` -> `dq_ui_backend_release_renderer`
  - `destroy_window` -> `dq_ui_backend_release_window`
  - `sdl_quit` -> `dq_ui_backend_shutdown`;
- `sdl_init` и `ttf_init` теперь явно трактуются как шаги backend lifecycle, а не как место, где inline codegen напрямую пишет `SDL_Init/TTF_Init`;
- в generated UI backend добавлены helper functions для desktop pipeline:
  - `dq_ui_backend_window`
  - `dq_ui_backend_renderer`
  - `dq_ui_backend_release_renderer`
  - `dq_ui_backend_release_window`.

**Зачем это сделано:**

- убрать последнее крупное концептуальное расхождение между:
  - generated UI из `.dqui`;
  - inline desktop runtime codegen из `dqgraph`;
- добиться, чтобы оба пути использовали одну и ту же backend boundary вместо двух разных моделей SDL orchestration;
- подготовить базу для дальнейшей консолидации UI runtime model.

**Технические изменения:**

- добавлены/обновлены:
  - `src/codegen/GraphCompiler.h`
  - `src/codegen/GraphCompiler.cpp`
  - `src/uiDesigner/SDL2CodeGenerator.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`
  - `tests/codegen/test_PreBuildProcessor.cpp`
  - `docs/plan/strategy_2026/26_ui_contract_and_backends.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_GraphCompiler test_PreBuildProcessor test_SDL2CodeGenerator deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SDL2CodeGenerator`

**Результат проверки:**

- все целевые тесты прошли;
- desktop template продолжает проходить `pre-build -> build -> run`;
- `GraphCompiler` больше не требует прямых `SDL_CreateWindow/SDL_PollEvent` в generated `main.c`;
- backend boundary теперь реально общая для двух путей генерации UI/desktop.

**Итог:**

- backend boundary для окна, событий и рендера теперь можно считать выделенной;
- следующий логичный шаг по UI-ветке:
  - `Stage 3: Runtime Model Consolidation`;
  - то есть вынос общего event/state/layout поведения из SDL2-ориентированной generated логики.

### Шаг 31 — UI event handling переведён на `DQ_UIRuntimeEvent`

**Фаза:** `UI Contract And Backends / Stage 3`

**Что сделано:**

- в generated UI header/source введена нормализованная runtime-модель события:
  - `DQ_UIRuntimeEventKind`
  - `DQ_UIRuntimeEvent`;
- SDL2 backend теперь явно переводит low-level события через:
  - `dq_ui_backend_translate_event(const DQ_UIBackendEvent *, DQ_UIRuntimeEvent *)`;
- `ui_handle_event(...)` больше не зависит от `SDL_Event` и работает только с `DQ_UIRuntimeEvent`;
- устаревший helper `dq_ui_backend_event_is_quit(...)` удалён, потому что завершение теперь выражается через `DQ_UIRuntimeEvent_Quit`;
- generated `main.c` для `.dqui` теперь обрабатывает события по цепочке:
  - `dq_ui_backend_poll_event`
  - `dq_ui_backend_translate_event`
  - `ui_handle_event`;
- inline desktop runtime codegen в `GraphCompiler` переведён на ту же схему, поэтому и `.dqui`, и `dqgraph` desktop pipeline используют одну и ту же event boundary.

**Зачем это сделано:**

- убрать прямую зависимость UI runtime layer от SDL2 event API;
- сделать `ui_handle_event(...)` частью внутренней UI runtime-модели, а не SDL2-обработчиком;
- подготовить следующий подэтап, где можно будет отдельно консолидировать layout/state/event routing без смешивания с backend-структурами SDL2.

**Технические изменения:**

- добавлены/обновлены:
  - `src/uiDesigner/SDL2CodeGenerator.cpp`
  - `src/codegen/GraphCompiler.cpp`
  - `tests/uiDesigner/test_SDL2CodeGenerator.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`
  - `tests/codegen/test_PreBuildProcessor.cpp`
  - `docs/plan/strategy_2026/26_ui_contract_and_backends.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_SDL2CodeGenerator test_GraphCompiler test_PreBuildProcessor deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SDL2CodeGenerator`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`

**Результат проверки:**

- все целевые тесты прошли;
- `deltaq` собирается без регрессий;
- generated UI больше не опирается на прямой `SDL_Event` внутри `ui_handle_event(...)`;
- desktop `dqgraph` pipeline и `.dqui` pipeline теперь разделяют одну и ту же runtime event boundary.

**Итог:**

- `Stage 3` начат и получил первый рабочий срез;
- следующий логичный шаг:
  - вынести общий layout/state/event routing в более явную runtime-модель;
  - затем перейти к `Stage 4: Module And .dqui Alignment`.

### Шаг 32 — введён `DQ_UIRuntimeContext` и централизованный runtime routing

**Фаза:** `UI Contract And Backends / Stage 3`

**Что сделано:**

- в generated UI header добавлены:
  - `DQ_UIWidgetId`
  - `DQ_UIRuntimeContext`;
- `UIState` теперь хранит `runtime`-секцию с общим состоянием:
  - `mouse_x`
  - `mouse_y`
  - `hovered_widget`
  - `focused_widget`
  - `active_widget`;
- в generated `ui.c` введены runtime helper functions:
  - `dq_ui_runtime_hit_test`
  - `dq_ui_runtime_sync_state`
  - `dq_ui_runtime_dispatch_click`
  - `dq_ui_runtime_toggle_checkbox`
  - `dq_ui_runtime_append_text`
  - `dq_ui_runtime_backspace`
  - `dq_ui_runtime_apply_wheel`
  - `dq_ui_runtime_update_slider`;
- `ui_handle_event(...)` больше не обновляет состояние кнопок, полей и слайдеров по месту через разрозненные widget-specific ветки;
- вместо этого обработчик обновляет общий runtime state, после чего синхронизирует render-state конкретных виджетов через `dq_ui_runtime_sync_state(...)`.

**Зачем это сделано:**

- вынести `hover/focus/active` из локальной логики отдельных виджетов в общую внутреннюю модель UI;
- сделать event routing и hit-test отдельным слоем runtime, а не набором дублирующихся веток по каждому widget type;
- подготовить следующий подэтап, где можно будет двигаться к более явной общей layout/state model поверх уже существующего runtime state.

**Технические изменения:**

- добавлены/обновлены:
  - `src/uiDesigner/SDL2CodeGenerator.cpp`
  - `tests/uiDesigner/test_SDL2CodeGenerator.cpp`
  - `docs/plan/strategy_2026/26_ui_contract_and_backends.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_SDL2CodeGenerator test_PreBuildProcessor test_GraphCompiler deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SDL2CodeGenerator`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`

**Результат проверки:**

- все целевые тесты прошли;
- `deltaq` собирается без регрессий;
- generated UI содержит явный runtime state layer поверх backend boundary;
- `Stage 3` теперь покрывает не только нормализацию событий, но и базовое внутреннее состояние/маршрутизацию UI.

**Итог:**

- `Stage 3` продвинут ещё на один законченный срез;
- следующий логичный шаг:
  - либо продолжать консолидацию runtime model в сторону layout/state semantics;
  - либо переходить к `Stage 4`, если приоритетнее выровнять `.dqui` и модульную UI-модель одним словарём контрактов.

### Шаг 33 — generated UI получил runtime layout pass для `children/layout/anchors`

**Фаза:** `UI Contract And Backends / Stage 3`

**Что сделано:**

- во все generated widget structs добавлен `base_rect`, который хранит исходную геометрию как source-of-truth для runtime layout;
- в backend boundary добавлена функция:
  - `dq_ui_backend_window_size(DQ_UIBackendContext *, int *, int *)`;
- generated UI получил отдельную runtime-функцию:
  - `ui_apply_layout(UIState *ui, int window_width, int window_height)`;
- `ui_apply_layout(...)` теперь:
  - пересчитывает абсолютные `rect` из `base_rect`;
  - учитывает иерархию `children`;
  - применяет `layout = HBox/VBox/Grid/Flow` для контейнеров;
  - применяет `anchors` для веток без auto-layout;
- generated `main.c` и inline desktop runtime в `GraphCompiler` перед рендером вызывают:
  - `dq_ui_backend_window_size(...)`
  - `ui_apply_layout(...)`.

**Зачем это сделано:**

- перестать трактовать `UILayout` как набор уже готовых абсолютных координат;
- сделать `layout` и `anchors` частью реального runtime-поведения, а не только данных дизайнера;
- связать event/runtime state и runtime geometry в одну внутреннюю модель UI поверх backend boundary.

**Технические изменения:**

- добавлены/обновлены:
  - `src/uiDesigner/SDL2CodeGenerator.cpp`
  - `src/codegen/GraphCompiler.cpp`
  - `tests/uiDesigner/test_SDL2CodeGenerator.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`
  - `tests/codegen/test_PreBuildProcessor.cpp`
  - `docs/plan/strategy_2026/26_ui_contract_and_backends.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_SDL2CodeGenerator test_GraphCompiler test_PreBuildProcessor deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SDL2CodeGenerator`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`

**Результат проверки:**

- все целевые тесты прошли;
- `deltaq` собирается без регрессий;
- generated UI теперь содержит не только runtime events/state, но и runtime geometry/layout pass;
- `Stage 3` закрывает уже три слоя:
  - runtime event boundary
  - runtime state/routing
  - runtime layout application.

**Итог:**

- `Stage 3` дожат ещё ближе к завершению;
- следующий логичный шаг:
  - либо продолжать унификацию layout semantics и contract vocabulary;
  - либо переходить к `Stage 4: Module And .dqui Alignment`, потому что следующая крупная незакрытая проблема уже больше про модель контрактов, чем про backend/runtime boundary.

### Шаг 34 — `Stage 3` закрыт через explicit runtime layout/anchor specs

**Фаза:** `UI Contract And Backends / Stage 3`

**Что сделано:**

- в generated UI header добавлены явные типы внутренней layout-модели:
  - `DQ_UIRuntimeLayoutKind`
  - `DQ_UIRuntimeLayoutSpec`
  - `DQ_UIRuntimeAnchorSpec`;
- в generated `ui.c` теперь создаются статические runtime spec-объекты:
  - `dq_ui_layoutspec_root`
  - `dq_ui_layoutspec_<widget>`
  - `dq_ui_anchorspec_<widget>`;
- runtime layout helpers переведены на эти spec-объекты:
  - `dq_ui_runtime_apply_anchor_spec(...)`
  - `dq_ui_runtime_apply_layout_spec(...)`;
- `ui_apply_layout(...)` больше не шьёт anchor/layout semantics прямо literal-ами по месту, а использует явную runtime-модель размещения;
- после этого `Stage 3` покрывает полный внутренний UI runtime layer:
  - runtime event boundary
  - runtime state/routing
  - runtime geometry/layout model.

**Зачем это сделано:**

- довести `Stage 3` до законченного состояния, а не оставлять layout semantics в виде промежуточного generated-паттерна;
- сделать внутреннюю модель UI достаточно явной, чтобы следующий этап уже честно был про выравнивание контрактов, а не про недоделанный runtime;
- закрепить идею, что SDL2 здесь уже только backend, а не хозяин UI-семантики.

**Технические изменения:**

- добавлены/обновлены:
  - `src/uiDesigner/SDL2CodeGenerator.cpp`
  - `src/codegen/GraphCompiler.cpp`
  - `tests/uiDesigner/test_SDL2CodeGenerator.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`
  - `tests/codegen/test_PreBuildProcessor.cpp`
  - `docs/plan/strategy_2026/26_ui_contract_and_backends.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_SDL2CodeGenerator test_GraphCompiler test_PreBuildProcessor deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SDL2CodeGenerator`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`

**Результат проверки:**

- все целевые тесты прошли;
- `deltaq` собирается без регрессий;
- generated UI и graph-based desktop runtime используют одну и ту же внутреннюю модель событий, состояния и layout;
- `Stage 3` можно считать закрытым.

**Итог:**

- хвост `Stage 3` добит до конца;
- следующий большой шаг по UI-ветке:
  - `Stage 4: Module And .dqui Alignment`.

### Шаг 35 — `Stage 4` начат через единый UI contract vocabulary

**Фаза:** `UI Contract And Backends / Stage 4`

**Что сделано:**

- добавлен новый общий словарь UI-контрактов в `include/deltaq/UIContract.h`;
- в словаре зафиксированы:
  - канонические contract types;
  - legacy widget types для `.dqui` и UI Designer;
  - display names и palette categories;
  - default sizes и default events;
  - designer properties;
  - module ports для UI-контрактов, уже представленных как графовые модули;
- `UIWidget` получил `metadata` и теперь синхронизирует contract metadata через единый каталог;
- старые `.dqui` без metadata теперь автоматически восстанавливают contract type по legacy widget type;
- `WidgetPalette` перестала хранить свой отдельный список виджетов и собирается из общего UI-каталога;
- `DesignScene` переведена на общий словарь для:
  - определения container types;
  - размеров drag preview;
  - display names в preview;
- `UIDesignerWidget` переведён на общий словарь для:
  - default event на double click;
  - default size вставляемого виджета;
  - первичного заполнения designer properties;
- `UIModuleFactory` теперь собирает UI contract-модули из того же каталога, а не из собственного набора `createButton/createSlider/...`;
- `GraphCompiler` переведён на более общий разбор UI contract metadata:
  - signal/event outputs заполняются через `deltaq.ui.events`;
  - state outputs выводятся из одноимённых входов или из `text -> value`, если это текстовый контракт.

**Зачем это сделано:**

- убрать расхождение между тремя разными представлениями одного и того же UI:
  - палитра виджетов;
  - `.dqui`;
  - UI contract-модули;
- подготовить почву для полного закрытия `Stage 4`, где UI Designer и модульная UI-модель должны стать одной системой, а не похожими подсистемами;
- сделать следующий шаг по UI уже не через локальные таблицы/ветвления, а через единый продуктовый словарь контрактов.

**Технические изменения:**

- добавлены/обновлены:
  - `include/deltaq/UIContract.h`
  - `include/deltaq/UILayout.h`
  - `src/uiDesigner/UIModuleFactory.h`
  - `src/uiDesigner/UIModuleFactory.cpp`
  - `src/uiDesigner/WidgetPalette.cpp`
  - `src/uiDesigner/DesignScene.cpp`
  - `src/uiDesigner/UIDesignerWidget.cpp`
  - `src/codegen/GraphCompiler.cpp`
  - `tests/core/test_UILayout.cpp`
  - `tests/uiDesigner/test_UIModuleFactory.cpp`
  - `tests/uiDesigner/test_DesignScene.cpp`
  - `tests/codegen/test_GraphCompiler.cpp`
  - `docs/plan/strategy_2026/26_ui_contract_and_backends.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_UILayout test_UIModuleFactory test_DesignScene test_GraphCompiler`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_UILayout`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_UIModuleFactory`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_DesignScene`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`
- `cmake --build build --parallel --target deltaq test_SDL2CodeGenerator`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SDL2CodeGenerator`

**Результат проверки:**

- все целевые тесты прошли;
- `deltaq` собирается без регрессий;
- палитра, `.dqui`, `UIWidget` metadata и `UIModuleFactory` теперь действительно опираются на один словарь UI-контрактов;
- `Stage 4` начат и закрыт его первый практический срез, но сам этап ещё не завершён полностью.

**Итог:**

- в проекте появился единый UI contract vocabulary;
- `Stage 4` переведён из абстрактной идеи в работающий код;
- следующий логичный шаг:
  - дожать выравнивание generated UI/runtime modules и пользовательски видимой contract model в IDE, чтобы закрыть `Stage 4` полностью.

### Шаг 36 — `Stage 4` закрыт через generated runtime и user-visible contract model

**Фаза:** `UI Contract And Backends / Stage 4`

**Что сделано:**

- `SDL2CodeGenerator` переведён на `contractType` при выборе runtime/render логики;
- generated UI теперь не зависит от legacy `w->type` как от единственного источника истины:
  - логика text field,
  - checkbox,
  - slider,
  - progress bar,
  - panel
  определяется через contract metadata;
- `PropertyEditor` переведён на единый UI-каталог:
  - показывает `Type` как display name;
  - показывает `Contract` как канонический contract type;
  - строит type-specific свойства по `UIContract.h`, а не по отдельной локальной таблице;
- это устранило реальное расхождение, где, например, `ComboBox` в дизайнере раньше показывался через legacy `text`, хотя контракт уже задавался через `items` и `selected`;
- `ObjectTreeWidget` теперь показывает display name и contract type;
- `ModuleManagerWidget` в секции UI-модулей тоже показывает display name и contract type вместо только внутренних `UI_Button`-подобных имён;
- `WidgetItem::paintComboBox(...)` подтянут к контрактной модели и умеет показывать выбранный элемент из `items/selected`, а не только legacy `text`;
- устранён warning в `PropertyEditor::clearLayout()`.

**Зачем это сделано:**

- полностью закрыть `Stage 4`, а не оставить его на уровне “словарь добавлен, но старые слои ещё живут отдельно”;
- сделать UI contract видимым и одинаковым:
  - в `.dqui`,
  - в generated UI/runtime,
  - в дизайнере,
  - в дереве объектов,
  - в библиотеке UI-модулей;
- довести UI-подсистему до состояния, где следующий этап уже честно не про выравнивание терминов, а про следующие продуктовые задачи.

**Технические изменения:**

- добавлены/обновлены:
  - `include/deltaq/UIContract.h`
  - `src/uiDesigner/SDL2CodeGenerator.cpp`
  - `src/uiDesigner/PropertyEditor.cpp`
  - `src/uiDesigner/ObjectTreeWidget.cpp`
  - `src/uiDesigner/WidgetItem.h`
  - `src/uiDesigner/WidgetItem.cpp`
  - `src/editor/ModuleManagerWidget.cpp`
  - `tests/uiDesigner/test_UIContractAlignment.cpp`
  - `tests/uiDesigner/test_SDL2CodeGenerator.cpp`
  - `tests/editor/test_ModuleManagerWidget.cpp`
  - `tests/CMakeLists.txt`
  - `docs/plan/strategy_2026/26_ui_contract_and_backends.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target test_UIContractAlignment test_SDL2CodeGenerator test_ModuleManagerWidget deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_UILayout`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_UIModuleFactory`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_DesignScene`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_UIContractAlignment`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SDL2CodeGenerator`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModuleManagerWidget`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_GraphCompiler`

**Результат проверки:**

- все целевые тесты прошли;
- `deltaq` собирается без регрессий;
- generated runtime теперь опирается на contract metadata;
- пользователь в IDE видит display name и contract type согласованно в ключевых UI-подсистемах;
- `Stage 4` можно считать закрытым.

**Итог:**

- `UI Contract And Backends / Stage 4` завершён;
- `.dqui`, UI-модули, generated runtime и IDE-представление UI теперь используют одну и ту же contract model;
- следующий крупный шаг по UI-ветке уже лежит за пределами выравнивания контрактов.

### Шаг 37 — старт `Phase 2 / Standard Library Quality` через audit и выравнивание core-библиотеки

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- в `StandardLibrary` зафиксированы обязательные категории `v1`:
  - `control`
  - `io`
  - `string`
  - `conversion`
  - `logic`
  - `math`
  - `desktop`;
- в `StandardLibrary` добавлен явный `audit` стандартной библиотеки;
- audit проверяет:
  - наличие обязательных категорий;
  - наличие ключевых модулей `v1`;
  - snake_case naming;
  - согласованность `module.id` со схемой `core.<category>.<name>`;
  - дубли id и конфликтующие имена внутри категории;
  - ожидаемые сигнатуры у ключевых модулей;
- checked-in core pack синхронизирован с тем, что уже считалось частью стандартной библиотеки в коде:
  - добавлены `if_branch`
  - добавлены `for_loop`
  - добавлены `sequence`;
- версия `modules/core/pack.json` поднята до `1.3`;
- `Module Manager` теперь показывает core-категории в фиксированном порядке `v1`, а не в случайном алфавитно-зависимом порядке.

**Зачем это сделано:**

- начать `Phase 2` не с разрастания библиотеки, а с наведения порядка в уже существующем core-наборе;
- перевести разговор о “качественной стандартной библиотеке” из общих слов в проверяемые правила;
- сделать библиотеку более предсказуемой и для IDE, и для пользователя;
- зафиксировать минимальный baseline, относительно которого дальше можно чистить слабые модули и улучшать документацию.

**Технические изменения:**

- добавлены/обновлены:
  - `src/core/StandardLibrary.h`
  - `src/core/StandardLibrary.cpp`
  - `src/editor/ModuleManagerWidget.cpp`
  - `tests/core/test_StandardLibrary.cpp`
  - `tests/editor/test_ModuleManagerWidget.cpp`
  - `tests/CMakeLists.txt`
  - `modules/core/control/if_branch.dqmod`
  - `modules/core/control/for_loop.dqmod`
  - `modules/core/control/sequence.dqmod`
  - `modules/core/pack.json`
  - `docs/plan/strategy_2026/21_standard_library_strategy.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary test_ModuleManagerWidget deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModuleManagerWidget`

**Результат проверки:**

- `test_StandardLibrary` прошёл;
- `test_ModuleManagerWidget` прошёл;
- `deltaq` собирается без регрессий;
- обязательные категории `v1` и базовый audit зафиксированы кодом и тестами;
- core pack приведён к более согласованному состоянию относительно реального `StandardLibrary`.

**Итог:**

- `Phase 2 / Standard Library Quality` начат с формального baseline, а не с произвольного наращивания модулей;
- в проекте появился проверяемый критерий качества для core-библиотеки;
- следующий логичный шаг:
  - найти слабые и дублирующие модули;
  - затем усилить discoverability и документацию ключевых модулей.

### Шаг 38 — `Standard Library Quality` доведён до единого display-порядка в IDE

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- в `StandardLibrary` добавлены общие helper-правила для display-порядка:
  - `orderCategoriesForDisplay(...)`
  - `orderModulesForDisplay(...)`;
- порядок категорий теперь задаётся не локальной логикой виджета, а единой `v1`-моделью standard library;
- внутри категории ключевые `v1` модули теперь идут раньше второстепенного алфавитного хвоста;
- `Module Palette` переведена на этот общий порядок;
- `Module Manager` тоже переведён на этот же общий порядок, чтобы библиотека читалась одинаково в обеих точках входа.

**Зачем это сделано:**

- standard library должна быть не просто валидной, а ещё и curated для пользователя;
- одинаковое представление библиотеки в IDE снижает когнитивный шум;
- это превращает `StandardLibrary` из набора helper-методов в реальный источник продуктовых правил для core-библиотеки.

**Технические изменения:**

- добавлены/обновлены:
  - `src/core/StandardLibrary.h`
  - `src/core/StandardLibrary.cpp`
  - `src/blockEditor/ModulePalette.cpp`
  - `src/editor/ModuleManagerWidget.cpp`
  - `tests/core/test_StandardLibrary.cpp`
  - `tests/blockEditor/test_ModulePalette.cpp`
  - `tests/editor/test_ModuleManagerWidget.cpp`
  - `docs/plan/strategy_2026/21_standard_library_strategy.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary test_ModulePalette test_ModuleManagerWidget deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModulePalette`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModuleManagerWidget`

**Результат проверки:**

- все три тестовых набора прошли;
- `deltaq` собирается без регрессий;
- порядок core-категорий и ключевых core-модулей теперь одинаков в audit, `Module Manager` и `Module Palette`.

**Итог:**

- `Phase 2 / Standard Library Quality` получил уже не только формальный audit, но и единый пользовательский порядок отображения библиотеки;
- следующий логичный шаг:
  - разбирать слабые и дублирующие core-модули;
  - затем переходить к документации и discoverability.

### Шаг 39 — для core-библиотеки введена product-curation модель ролей

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- в `StandardLibrary` добавлена явная product-curation классификация core-модулей:
  - `essential`
  - `convenience`
  - `specialized`;
- `requiredV1` модули автоматически считаются `essential`;
- дополнительные core-модули теперь получают понятную роль вместо статуса “просто ещё один файл в библиотеке”;
- `Module Palette` показывает эту роль в tooltip;
- `Module Manager` тоже показывает роль библиотеки в tooltip элементов дерева;
- display-порядок внутри категории теперь учитывает не только `requiredV1` baseline, но и tier:
  - сначала `essential`
  - затем `convenience`
  - затем `specialized`.

**Зачем это сделано:**

- не удалять модули вслепую, а сначала явно развести:
  - что является опорным baseline,
  - что является shortcut,
  - что является специализированным дополнением;
- улучшить discoverability стандартной библиотеки;
- подготовить почву для следующего шага, где уже можно осмысленно чистить слабые или спорные модули.

**Технические изменения:**

- добавлены/обновлены:
  - `src/core/StandardLibrary.h`
  - `src/core/StandardLibrary.cpp`
  - `src/blockEditor/ModulePalette.cpp`
  - `src/editor/ModuleManagerWidget.cpp`
  - `tests/core/test_StandardLibrary.cpp`
  - `tests/blockEditor/test_ModulePalette.cpp`
  - `tests/editor/test_ModuleManagerWidget.cpp`
  - `docs/plan/strategy_2026/21_standard_library_strategy.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary test_ModulePalette test_ModuleManagerWidget deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModulePalette`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModuleManagerWidget`

**Результат проверки:**

- все тесты прошли;
- `deltaq` собирается без регрессий;
- пользователь теперь видит не только имя core-модуля, но и его роль внутри curated standard library.

**Итог:**

- `Phase 2 / Standard Library Quality` перешёл от чисто технического audit к продуктовой модели библиотеки;
- следующий логичный шаг:
  - решать, какие core-модули действительно стоит считать слабыми или спорными;
  - затем развивать документацию значимых модулей уже поверх этой классификации.

### Шаг 40 — стратегия расширения экосистемы вынесена в `library converter + module packs`

**Фаза:** `Phase 2 / Import And Wrapping`

**Что сделано:**

- зафиксирована отдельная стратегия `Library Converter And Module Packs`;
- явно разведены три слоя экосистемы:
  - `core`
  - `external module packs`
  - `project modules`;
- зафиксирован принцип, что разнообразие DeltaQ должно приходить в основном через pack-ы внешних библиотек, а не через разрастание `core`;
- описан pipeline:
  - `library intake`
  - `symbol analysis`
  - `raw wrapper generation`
  - `type/contract mapping`
  - `pack curation`
  - `verification`
  - `use in graph`;
- зафиксирована минимальная модель imported pack-а:
  - `pack.json`
  - `.dqmod`
  - metadata линковки, платформ, типов и зрелости pack-а.

**Зачем это сделано:**

- привязать идею DeltaQ к реальному сценарию автора:
  - внешняя библиотека оборудования/SDK не используется напрямую,
  - сначала она превращается в модули,
  - потом пользователь работает уже модульной моделью;
- не смешивать curated baseline `core` и domain-specific wrappers;
- сделать import/wrapping не побочной фичей, а системным каналом роста экосистемы.

**Документы:**

- добавлен:
  - `docs/plan/strategy_2026/27_library_converter_and_module_packs.md`
- обновлены:
  - `docs/plan/strategy_2026/00_master_strategy.md`
  - `docs/plan/strategy_2026/11_phase_2_module_ecosystem.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- изменения только в Markdown-документах;
- код проекта не менялся;
- тесты не запускались.

**Итог:**

- в стратегии DeltaQ появился отдельный и ясный путь расширения через module packs;
- следующий логичный шаг:
  - спроектировать minimum viable converter для `C`-библиотек;
  - затем выбрать один reference case и реализовать его end-to-end.

### Шаг 41 — реализован первый `minimum viable converter` для `C`-библиотек

**Фаза:** `Phase 2 / Import And Wrapping`

**Что сделано:**

- добавлен `LibraryPackager`, который сохраняет импортированную библиотеку как extension pack:
  - `pack.json`
  - `.dqmod`
  - optional `wrappers/*.h/*.cpp`;
- `Library Import Wizard` теперь не заканчивается на временном `registerModule(...)`, а пишет установленный pack в глобальную папку модулей;
- imported modules получают:
  - стабильные ids вида `ext.<pack>.<module>`
  - metadata imported pack-а
  - сохранённые `header_paths / include_paths / defines / link_libraries`;
- для простых `C`-функций generated `.dqmod` теперь получают собственный wrapper `sourceCode`, то есть импорт даёт не только контракт, но и атомарный модуль с реализацией;
- `LibraryDecomposer` теперь приводит сырые C-типы к DeltaQ-типам в портах;
- `LibProcessorWidget` показывает уже imported pack-модули, а не только старые transient imports;
- `pointer` добавлен в базовый type mapping codegen/test pipeline как `void *`.

**Зачем это сделано:**

- перевести существующий `Library Processor` от “парсер + временная регистрация” к реальному supply channel для внешних модульных pack-ов;
- сделать первый working slice именно для `C`-библиотек, которые лучше всего подходят под MVP converter-а;
- подготовить основу для следующего шага, где pack metadata будет использована уже и в build integration.

**Технические изменения:**

- добавлены/обновлены:
  - `src/libProcessor/LibraryPackager.h`
  - `src/libProcessor/LibraryPackager.cpp`
  - `src/libProcessor/LibraryDecomposer.cpp`
  - `src/libProcessor/LibraryImportWizard.h`
  - `src/libProcessor/LibraryImportWizard.cpp`
  - `src/libProcessor/LibProcessorWidget.cpp`
  - `src/libProcessor/CMakeLists.txt`
  - `src/codegen/GraphCompiler.cpp`
  - `src/editor/ModuleTestRunner.cpp`
  - `tests/libProcessor/test_LibraryDecomposer.cpp`
  - `tests/libProcessor/test_LibraryPackager.cpp`
  - `tests/CMakeLists.txt`
  - `docs/plan/strategy_2026/27_library_converter_and_module_packs.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target test_LibraryDecomposer test_LibraryPackager deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_LibraryDecomposer`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_LibraryPackager`

**Результат проверки:**

- `test_LibraryDecomposer` прошёл;
- `test_LibraryPackager` прошёл;
- `deltaq` собирается без регрессий;
- `minimum viable converter` для `C`-библиотек уже сохраняет реальные extension pack-ы, а не только временные модули в памяти.

**Ограничения текущего среза:**

- `link_libraries` metadata imported pack-а пока только сохраняется, но ещё не проходит автоматически в `CMake`;
- reference scenario `external library -> curated pack -> project graph -> build/run` ещё не закрыт;
- `C++` wrappers остаются за пределами первого MVP.

**Итог:**

- `Import And Wrapping` перешёл из чисто стратегического плана в работающий кодовый pipeline;
- следующий логичный шаг:
  - протащить imported pack build metadata в `CMake`/build pipeline;
  - затем сделать один честный reference case end-to-end.

### Шаг 42 — imported pack metadata доведена до реального `CMake` и runtime

**Фаза:** `Phase 2 / Import And Wrapping`

**Что сделано:**

- `CMakeGenerator` теперь привязывается к `ModuleRegistry` и `GraphStore`;
- при генерации `CMakeLists.txt` он рекурсивно анализирует только корневые графы проекта и их составные модули;
- для реально используемых imported pack-ов в `CMake` теперь автоматически добавляются:
  - `target_include_directories(...)`
  - `target_compile_definitions(...)`
  - `target_link_libraries(...)`;
- `BuildManager` получил проксирующие setter-ы и теперь пробрасывает эти зависимости генератору `CMake`;
- `MainWindow` подключает текущий реестр модулей и хранилище графов к build-системе сразу при старте;
- добавлен end-to-end сценарий, где imported module c `cos()` требует `-lm`, а проект реально:
  - проходит pre-build,
  - генерирует корректный `CMakeLists.txt`,
  - успешно собирается,
  - успешно запускается.

**Зачем это сделано:**

- закрыть разрыв между сохранённой metadata imported pack-а и настоящей сборкой проекта;
- не тащить в `CMake` все установленные extension pack-ы подряд, а учитывать только реально используемые графом зависимости;
- подтвердить не только текстовую генерацию `CMakeLists.txt`, но и рабочий runtime-результат.

**Технические изменения:**

- обновлены:
  - `src/editor/CMakeGenerator.h`
  - `src/editor/CMakeGenerator.cpp`
  - `src/editor/BuildManager.h`
  - `src/editor/BuildManager.cpp`
  - `src/core/MainWindow.cpp`
  - `tests/editor/test_CMakeGenerator.cpp`
  - `tests/codegen/test_PreBuildProcessor.cpp`
  - `docs/plan/strategy_2026/27_library_converter_and_module_packs.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_CMakeGenerator test_PreBuildProcessor deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_CMakeGenerator`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`

**Результат проверки:**

- `test_CMakeGenerator` прошёл;
- `test_PreBuildProcessor` прошёл;
- новый сценарий `importedPackBuildRequirementsReachCMakeAndRuntime()` прошёл;
- `deltaq` собирается без регрессий.

**Итог:**

- build metadata imported pack-а больше не остаётся “декорацией” в `.dqmod` и `pack.json`, а реально доходит до `CMake`;
- у `Import And Wrapping` появился честный внутренний end-to-end path `metadata -> graph -> build -> run`;
- следующим логичным шагом остаётся уже не build integration, а пользовательский showcase:
  - реальный import через UI,
  - curation pack-а,
  - отдельный reference example/onboarding.

### Шаг 43 — оформлен детальный план `imported pack showcase + curation`

**Фаза:** `Phase 2 / Import And Wrapping`

**Что сделано:**

- добавлен отдельный детальный план:
  - `docs/plan/strategy_2026/28_imported_pack_showcase_and_curation.md`;
- в этом плане зафиксированы:
  - рекомендуемый первый reference case `mini_sensor_sdk`;
  - двухслойная модель:
    - internal technical reference
    - user-facing showcase;
  - обязательные стадии:
    - fixture library
    - raw import baseline
    - curation flow
    - verification
    - showcase project
    - walkthrough docs;
  - конкретный порядок исполнения по шагам;
  - acceptance criteria и что сознательно откладывается.

**Зачем это сделано:**

- перевести следующий этап `Import And Wrapping` из “общей идеи” в чёткий исполнимый маршрут;
- не перепрыгивать сразу к произвольным UI-улучшениям или внешним SDK;
- сначала закрыть controlled fixture library и reproducible raw import, а уже потом делать showcase.

**Документы:**

- добавлен:
  - `docs/plan/strategy_2026/28_imported_pack_showcase_and_curation.md`
- обновлены:
  - `docs/plan/strategy_2026/00_master_strategy.md`
  - `docs/plan/strategy_2026/22_examples_and_reference_projects.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- изменения только в Markdown-документах;
- код проекта не менялся;
- тесты не запускались.

**Итог:**

- для imported pack showcase теперь есть не только стратегический контур, но и пошаговый исполнительный план;
- следующий практический шаг зафиксирован однозначно:
  - добавить controlled fixture library `mini_sensor_sdk`.

### Шаг 44 — добавлена controlled fixture library `mini_sensor_sdk`

**Фаза:** `Phase 2 / Import And Wrapping`

**Что сделано:**

- в репозиторий добавлена локальная fixture library:
  - `resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk/include/mini_sensor_sdk.h`
  - `resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk/src/mini_sensor_sdk.c`
  - `resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk/CMakeLists.txt`
  - `resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk/README.md`;
- библиотека моделирует небольшой внешний SDK с жизненным циклом:
  - `mini_sensor_init`
  - `mini_sensor_read`
  - `mini_sensor_scale`
  - `mini_sensor_last_error`
  - `mini_sensor_shutdown`;
- поведение специально сделано детерминированным и не зависит от:
  - случайности
  - системного времени
  - файловой системы
  - внешнего оборудования;
- добавлен автоматический тест:
  - `tests/libProcessor/test_ImportedPackFixture.cpp`;
- тест:
  - копирует fixture library во временную директорию;
  - собирает её как отдельный `CMake`-проект;
  - компилирует маленькую harness-программу как внешний потребитель SDK;
  - проверяет стабильный runtime output.

**Зачем это сделано:**

- закрыть `Stage 1` нового плана `imported pack showcase + curation`;
- получить контролируемую библиотеку для следующего шага:
  - reproducible raw import baseline;
- не зависеть на первом showcase от реального vendor SDK и внешней среды.

**Технические изменения:**

- добавлены:
  - `resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk/include/mini_sensor_sdk.h`
  - `resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk/src/mini_sensor_sdk.c`
  - `resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk/CMakeLists.txt`
  - `resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk/README.md`
  - `tests/libProcessor/test_ImportedPackFixture.cpp`
- обновлены:
  - `tests/CMakeLists.txt`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target test_ImportedPackFixture`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ImportedPackFixture`

**Результат проверки:**

- `test_ImportedPackFixture` прошёл;
- fixture library собирается как отдельный внешний SDK;
- harness подтверждает детерминированные значения:
  - `READ=48`
  - `SCALED=144`
  - `ERROR=-1`
  - `LAST_ERROR=scale factor must be positive`.

**Итог:**

- `Stage 1` по imported pack showcase закрыт;
- следующий практический шаг теперь уже технически готов:
  - зафиксировать reproducible raw import baseline для `mini_sensor_sdk`.

### Шаг 45 — зафиксирован reproducible raw import baseline для `mini_sensor_sdk`

**Фаза:** `Phase 2 / Import And Wrapping`

**Что сделано:**

- добавлен верхнеуровневый документ примера:
  - `resources/examples/imported_pack_sensor_sdk/README.md`;
- в этом документе зафиксирован канонический raw import baseline:
  - `packName = mini_sensor_sdk_raw`
  - `category = sensor_raw`
  - `language = c`
  - `standard = c17`
  - `link_libraries = mini_sensor_sdk`;
- добавлен регрессионный тест:
  - `tests/libProcessor/test_ImportedPackRawBaseline.cpp`;
- тест строит фиксированный `ParseResult` для API `mini_sensor_sdk`, затем прогоняет:
  - `LibraryDecomposer`
  - `LibraryPackager`;
- тест проверяет:
  - состав raw wrapper-модулей;
  - DeltaQ-типы портов;
  - стабильные ids;
  - `pack.json`;
  - metadata линковки и include dirs;
  - детерминированность результата при повторном прогоне.

**Зачем это сделано:**

- закрыть `Stage 2` нового плана `imported pack showcase + curation`;
- получить не только fixture library, но и формально зафиксированный raw import result;
- сделать следующий шаг `curation flow` независимым от догадок о том, что именно считается корректным raw wrapper pack-ом.

**Технические изменения:**

- добавлены:
  - `resources/examples/imported_pack_sensor_sdk/README.md`
  - `tests/libProcessor/test_ImportedPackRawBaseline.cpp`
- обновлены:
  - `tests/CMakeLists.txt`
  - `docs/plan/strategy_2026/28_imported_pack_showcase_and_curation.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target test_ImportedPackRawBaseline`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ImportedPackRawBaseline`

**Результат проверки:**

- `test_ImportedPackRawBaseline` прошёл;
- raw import baseline для `mini_sensor_sdk` детерминирован;
- pack metadata и `.dqmod` формируются стабильно без ручной доводки.

**Итог:**

- `Stage 2` по imported pack showcase закрыт;
- следующий практический шаг по плану:
  - формализовать `v1` flow curation imported pack-а.

### Шаг 46 — формализован `v1` flow curation imported pack-а

**Фаза:** `Phase 2 / Import And Wrapping`

**Что сделано:**

- imported-модули получили явные metadata поля curation:
  - `deltaq.import.display_name`
  - `deltaq.import.curation_role`;
- `LibraryPackager` теперь создаёт raw wrapper pack уже с дефолтной ролью
  `raw_wrapper` и каноническим `display_name`;
- `ModuleRegistry` сохраняет `storagePath` для loaded `.dqmod`, чтобы imported pack
  можно было редактировать без потери исходного пути;
- в `Module Manager` добавлен блок `Import/Curation`:
  - редактирование `display name`
  - выбор роли `raw_wrapper / curated_entry / adapter / hidden`
  - показ pack и исходного символа;
- сохранение imported-модуля теперь идёт обратно в исходный `.dqmod` pack-а,
  а не в fallback `user_modules/`;
- `Module Palette`:
  - использует curated `display name`
  - показывает import metadata в tooltip
  - скрывает wrapper-ы с ролью `hidden`.

**Зачем это сделано:**

- закрыть `Stage 3` плана `imported pack showcase + curation`;
- превратить curation из неформальной идеи в явный `v1` workflow внутри IDE;
- подготовить переход к следующему шагу:
  - curated pack, пригодный для реального использования в графе.

**Технические изменения:**

- обновлены:
  - `include/deltaq/Module.h`
  - `src/libProcessor/LibraryPackager.cpp`
  - `src/core/ModuleRegistry.cpp`
  - `src/editor/ModuleManagerWidget.h`
  - `src/editor/ModuleManagerWidget.cpp`
  - `src/blockEditor/ModulePalette.cpp`
  - `resources/examples/imported_pack_sensor_sdk/README.md`
- расширены тесты:
  - `tests/libProcessor/test_LibraryPackager.cpp`
  - `tests/blockEditor/test_ModulePalette.cpp`
  - `tests/editor/test_ModuleManagerWidget.cpp`
- обновлены документы стратегии:
  - `docs/plan/strategy_2026/28_imported_pack_showcase_and_curation.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target test_LibraryPackager test_ModulePalette test_ModuleManagerWidget`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_LibraryPackager`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModulePalette`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModuleManagerWidget`

**Результат проверки:**

- `test_LibraryPackager` прошёл;
- `test_ModulePalette` прошёл;
- `test_ModuleManagerWidget` прошёл;
- formalized curation flow теперь закреплён и в коде, и в example-документации.

**Итог:**

- `Stage 3` по imported pack showcase закрыт;
- следующий практический шаг по плану:
  - ввести verification для curated pack-а и собрать реальный curated example.

### Шаг 47 — added verification для verified curated pack `mini_sensor_sdk`

**Фаза:** `Phase 2 / Import And Wrapping`

**Что сделано:**

- в `test_PreBuildProcessor.cpp` добавлен полный integration scenario
  `curatedImportedPackBuildsAndRunsMiniSensorFlow()`;
- тест:
  - собирает fixture SDK `mini_sensor_sdk` как внешнюю статическую библиотеку;
  - создаёт imported pack `mini_sensor_sdk_curated` через
    `LibraryDecomposer + LibraryPackager`;
  - переводит raw wrapper-ы в verified hidden-модули;
  - добавляет curated entry adapter-модули:
    - `sensor_scale_report`
    - `sensor_error_report`;
  - сохраняет pack на диск;
  - загружает его через `ModuleRegistry`;
  - собирает граф проекта;
  - прогоняет `pre-build -> CMake -> build -> run`.

**Что именно проверяет новый сценарий:**

- imported pack requirements попадают в `CMakeLists.txt`;
- curated pack реально пригоден для использования в графе;
- runtime подтверждает ожидаемое поведение fixture SDK:
  - `READ=48`
  - `SCALED=144`
  - `FAILED_SCALE=-1`
  - `LAST_ERROR=scale factor must be positive`
  - `SHUTDOWN=done`.

**Технические изменения:**

- обновлены:
  - `tests/codegen/test_PreBuildProcessor.cpp`
  - `tests/CMakeLists.txt`
  - `resources/examples/imported_pack_sensor_sdk/README.md`
  - `docs/plan/strategy_2026/28_imported_pack_showcase_and_curation.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake -S . -B build`
- `cmake --build build --parallel --target test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`

**Результат проверки:**

- `test_PreBuildProcessor` прошёл полностью;
- новый curated pack scenario прошёл end-to-end;
- verification для `Stage 4` теперь есть не только как план, но и как рабочий regression.

**Итог:**

- `Stage 4` по imported pack showcase закрыт на уровне internal verification;
- следующий практический шаг по плану:
  - сделать checked-in showcase project и walkthrough для curated pack-а (`Stage 5`).

### Шаг 48 — добавлен checked-in showcase project для curated pack `mini_sensor_sdk`

**Фаза:** `Phase 2 / Import And Wrapping`

**Что сделано:**

- добавлен self-contained example project:
  - `resources/examples/imported_pack_sensor_console`;
- в проекте есть:
  - `.dqproj`
  - `graphs/main.dqgraph`
  - curated imported `.dqmod`
  - локальный `vendor/mini_sensor_sdk`
  - placeholder `src/main.c` для pre-build;
- curated imported pack внутри example включает:
  - hidden raw wrappers
  - visible modules:
    - `Sensor Scale Report`
    - `Sensor Error Report [adapter]`;
- добавлен walkthrough:
  - `docs/library/imported_packs/mini_sensor_sdk.md`.

**Что проверяет новый example:**

- проект копируется как обычный DeltaQ example;
- `PreBuildProcessor` генерирует `src/main.c` из `graphs/main.dqgraph`;
- `CMakeGenerator` подтягивает include path imported pack-а;
- проект собирается вместе с `vendor/mini_sensor_sdk/src/mini_sensor_sdk.c`;
- runtime выдаёт ожидаемый детерминированный вывод.

**Технические изменения:**

- добавлены:
  - `resources/examples/imported_pack_sensor_console/...`
  - `docs/library/imported_packs/mini_sensor_sdk.md`;
- расширен regression:
  - `importedPackSensorConsoleExampleBuildsAndRunsEndToEnd()` в
    `tests/codegen/test_PreBuildProcessor.cpp`;
- обновлены:
  - `docs/plan/strategy_2026/22_examples_and_reference_projects.md`
  - `docs/plan/strategy_2026/28_imported_pack_showcase_and_curation.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`

**Результат проверки:**

- `importedPackSensorConsoleExampleBuildsAndRunsEndToEnd()` прошёл;
- checked-in showcase project проходит `pre-build -> build -> run`;
- curated pack теперь закреплён не только integration test-ом, но и публичным example.

**Итог:**

- `Stage 5` и `Stage 6` imported pack showcase закрыты;
- следующий логичный шаг:
  - решить, продолжаем ли `Phase 2` через documentation/discoverability imported pack-ов
    в самой IDE, или переходим к `Phase 3` и упаковке README/demo narrative.

### Шаг 49 — введён минимальный documentation standard, видимый в IDE

**Фаза:** `Phase 2 / Module Documentation`

**Что сделано:**

- в `Module` добавлены helper-ы для user-visible документации:
  - `documentationSummary()`
  - `documentationWhenToUse()`
  - `documentationLimitations()`;
- `LibraryPackager` теперь автоматически даёт imported raw wrapper-ам минимальную
  автодокументацию;
- в `Module Manager` добавлен явный блок `Документация`:
  - `Когда использовать`
  - `Ограничения`;
- эти поля сохраняются в `.dqmod` через metadata:
  - `deltaq.doc.when_to_use`
  - `deltaq.doc.limitations`;
- `Module Palette` и дерево модулей теперь показывают documentation block в tooltip.

**Зачем это сделано:**

- закрыть следующий практический кусок `Phase 2 / Module Documentation`;
- сделать модуль объяснимым прямо внутри IDE, а не только через внешний README;
- дать imported pack-ам и showcase-примерам user-visible documentation standard.

**Технические изменения:**

- обновлены:
  - `include/deltaq/Module.h`
  - `src/libProcessor/LibraryPackager.cpp`
  - `src/blockEditor/ModulePalette.cpp`
  - `src/editor/ModuleManagerWidget.h`
  - `src/editor/ModuleManagerWidget.cpp`
  - `resources/examples/imported_pack_sensor_console/dqmods/...sensor_scale_report.dqmod`
  - `resources/examples/imported_pack_sensor_console/dqmods/...sensor_error_report.dqmod`
- расширены тесты:
  - `tests/libProcessor/test_LibraryPackager.cpp`
  - `tests/blockEditor/test_ModulePalette.cpp`
  - `tests/editor/test_ModuleManagerWidget.cpp`
- обновлены документы стратегии:
  - `docs/plan/strategy_2026/21_standard_library_strategy.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_LibraryPackager test_ModulePalette test_ModuleManagerWidget test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_LibraryPackager`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModulePalette`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModuleManagerWidget`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_PreBuildProcessor`

**Результат проверки:**

- все целевые тесты прошли;
- документация imported pack-ов и curated example видна прямо в IDE;
- format `description + when_to_use + limitations` теперь закреплён как `v1` standard.

**Итог:**

- документационный стандарт теперь существует и виден пользователю;
- следующий логичный шаг:
  - либо продолжать `Phase 2` и переносить этот standard на ключевые core-модули,
  - либо переходить к `Phase 3` и собирать README/demo narrative поверх уже готовых примеров.

### Шаг 50 — documentation standard перенесён на checked-in core pack

**Фаза:** `Phase 2 / Module Documentation`

**Что сделано:**

- для всего текущего `modules/core` заполнены user-visible поля документации:
  - `description`
  - `deltaq.doc.when_to_use`
  - `deltaq.doc.limitations`;
- документация добавлена не только в `required v1` baseline, но и в остальные checked-in
  `core` модули, включая `string`, `math`, `conversion` и `desktop` backend runtime;
- уточнены human-readable summary у модулей `core.io.print_int` и `core.control.if_then`;
- в `test_StandardLibrary` добавлена отдельная регрессия, которая требует, чтобы
  каждый core-модуль из installed pack имел summary, `when_to_use` и `limitations`.

**Зачем это сделано:**

- довести documentation standard до уровня всего checked-in `core` pack;
- убрать разрыв между imported/showcase модулями и стандартной библиотекой;
- закрепить, что core-библиотека должна быть не только технически валидной,
  но и объяснимой пользователю прямо внутри IDE.

**Технические изменения:**

- обновлены `modules/core/**/*.dqmod`;
- расширен `tests/core/test_StandardLibrary.cpp`;
- синхронизированы документы:
  - `docs/plan/strategy_2026/21_standard_library_strategy.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`.

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary test_ModulePalette`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModulePalette`

**Итог:**

- documentation standard теперь применён ко всему checked-in `core` pack;
- следующая логичная тема внутри `Phase 2`:
  - либо чистка слабых/дублирующих модулей,
  - либо расширение module docs в сторону внешних `docs/library/core/*`.

### Шаг 51 — начата реальная чистка `core` через первый `legacy`-slice

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- выполнен первый продуктовый audit-slice по спорным shortcut-модулям `core`;
- `core.io.print_int` переведён из рекомендуемого shortcut-а в `legacy`;
- в `Module Palette` и `Module Manager` legacy core-модули теперь помечаются
  суффиксом `[legacy]`;
- добавлена человекочитаемая фиксация решения в `docs/library/core/README.md`.

**Зачем это сделано:**

- начать реальную чистку `core`, не ломая обратную совместимость;
- показать пользователю, что не каждый checked-in модуль является частью
  рекомендуемого baseline;
- зафиксировать принцип: если shortcut дублирует ясную composable-связку,
  его нужно хотя бы вывести из baseline через `legacy`.

**Технические изменения:**

- обновлены:
  - `src/core/StandardLibrary.cpp`
  - `src/blockEditor/ModulePalette.cpp`
  - `src/editor/ModuleManagerWidget.cpp`
  - `tests/core/test_StandardLibrary.cpp`
  - `tests/blockEditor/test_ModulePalette.cpp`
  - `tests/editor/test_ModuleManagerWidget.cpp`
  - `docs/library/core/README.md`
  - `docs/plan/strategy_2026/21_standard_library_strategy.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary test_ModulePalette test_ModuleManagerWidget`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModulePalette`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModuleManagerWidget`

**Итог:**

- чистка слабых/дублирующих модулей начата не теоретически, а через видимый
  curation-механизм;
- следующий логичный шаг:
  - продолжить audit остальных спорных core-модулей,
  - либо сделать для `core` отдельную таблицу решений `keep / legacy / remove`.

### Шаг 52 — checked-in `core` pack переведён на explicit curation review

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- в `StandardLibrary` введена явная reviewed-таблица для всей non-essential части
  checked-in `core` pack;
- `audit()` теперь считает ошибкой любой checked-in `core`-модуль, который не
  прошёл explicit curation review;
- добавлена полная матрица решений по текущему `core` pack:
  - `docs/library/core/audit_matrix.md`;
- `docs/library/core/README.md` теперь ссылается на эту матрицу как на живой
  reference по решениям `keep / legacy / remove`.

**Зачем это сделано:**

- убрать неявные “провалы” в generic fallback tiers;
- превратить curation standard library в проверяемое правило, а не в набор
  разрозненных `if`-веток;
- зафиксировать, что каждый checked-in `core`-модуль уже прошёл хотя бы базовый
  продуктовый разбор.

**Технические изменения:**

- обновлены:
  - `src/core/StandardLibrary.cpp`
  - `docs/library/core/README.md`
  - `docs/library/core/audit_matrix.md`
  - `docs/plan/strategy_2026/21_standard_library_strategy.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`

**Итог:**

- checked-in `core` pack теперь проходит через explicit curation review;
- следующий логичный шаг:
  - выбирать следующие реальные кандидаты на `legacy/remove`,
  - либо переходить к внешней библиотечной документации по категориям.

### Шаг 53 — начата внешняя библиотечная документация по категориям `core`

**Фаза:** `Phase 2 / Module Documentation`

**Что сделано:**

- в `test_StandardLibrary` добавлен отдельный барьер на распределение ролей
  внутри installed `core` pack:
  - `essential = 28`
  - `specialized = 14`
  - `legacy = 1`
  - `convenience = 0`;
- добавлены первые category guides:
  - `docs/library/core/io.md`
  - `docs/library/core/control.md`;
- `docs/library/core/README.md` теперь ссылается не только на audit-матрицу, но и
  на первые пользовательские guides по категориям.

**Зачем это сделано:**

- закрепить reviewed-матрицу не только в `audit()`, но и в явном regression test;
- начать внешнюю библиотечную документацию по `core` с самых важных категорий:
  `io` и `control`;
- сделать решения `essential / specialized / legacy` понятными не только из кода
  и tooltip, но и из обычной docs-навигации.

**Технические изменения:**

- обновлены:
  - `tests/core/test_StandardLibrary.cpp`
  - `docs/library/core/README.md`
  - `docs/library/core/io.md`
  - `docs/library/core/control.md`

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`

**Итог:**

- reviewed-распределение ролей теперь закреплено отдельным тестом;
- библиотечная документация `core` начала выходить из уровня внутренних заметок
  в нормальный пользовательский формат;
- следующий логичный шаг:
  - продолжить category guides для `conversion / math / desktop`,
  - либо вернуться к выбору следующего `legacy/remove` кандидата.

### Шаг 54 — расширена внешняя библиотечная документация `core`

**Фаза:** `Phase 2 / Module Documentation`

**Что сделано:**

- добавлены category guides:
  - `docs/library/core/conversion.md`
  - `docs/library/core/math.md`
  - `docs/library/core/desktop.md`;
- `docs/library/core/README.md` теперь собирает почти весь базовый набор guides
  для `core`-категорий.

**Зачем это сделано:**

- довести documentation standard от tooltip-уровня до нормальной внешней docs-навигации;
- объяснить пользователю не только отдельные модули, но и смысл категорий:
  - где baseline;
  - где specialized-хвост;
  - где backend-runtime слой;
- подготовить `core`-библиотеку к более внятному `Phase 3` narrative.

**Технические изменения:**

- добавлены:
  - `docs/library/core/conversion.md`
  - `docs/library/core/math.md`
  - `docs/library/core/desktop.md`
- обновлён:
  - `docs/library/core/README.md`

**Проверка:**

- тесты не запускались, потому что изменения только в Markdown.

**Итог:**

- внешний docs-контур `core` теперь покрывает `io`, `control`, `conversion`, `math`, `desktop`;
- следующий логичный шаг:
  - дописать оставшиеся guides `logic` и `string`,
  - либо вернуться к следующему кандидату на `legacy/remove`.

### Шаг 55 — docs-контур `core` закрыт для базовых категорий

**Фаза:** `Phase 2 / Module Documentation`

**Что сделано:**

- добавлены оставшиеся guides:
  - `docs/library/core/logic.md`
  - `docs/library/core/string.md`;
- `docs/library/core/README.md` теперь собирает полный базовый набор category guides
  для checked-in `core` pack.

**Зачем это сделано:**

- завершить первый нормальный внешний docs-контур по `core`-категориям;
- сделать `logic` и `string` такими же объяснимыми, как `io`, `control`, `conversion`,
  `math` и `desktop`;
- подготовить библиотеку к следующему уровню narrative без провалов по категориям.

**Технические изменения:**

- добавлены:
  - `docs/library/core/logic.md`
  - `docs/library/core/string.md`
- обновлён:
  - `docs/library/core/README.md`

**Проверка:**

- тесты не запускались, потому что изменения только в Markdown.

**Итог:**

- внешний docs-контур `core` теперь покрывает все основные checked-in категории;
- следующий логичный шаг:
  - вернуться к audit и выбрать следующий реальный кандидат на `legacy/remove`,
  - либо собирать уже верхнеуровневый README/demo narrative для `Phase 3`.

### Шаг 56 — legacy-модули убраны из default palette flow

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- в `Module Palette` появился переключатель `Показывать legacy`;
- legacy core-модули теперь скрыты по умолчанию и не навязываются в рабочем baseline;
- при явном включении переключателя legacy-модуль снова появляется в палитре с
  пометкой `[legacy]` и обычным tooltip с ролью и replacement hint.

**Зачем это сделано:**

- сделать curation стандартной библиотеки не только описанной, но и реально
  влияющей на повседневный UX палитры;
- исключить legacy-модули из рекомендуемого default-flow без разрушения
  обратной совместимости;
- сохранить доступ к старым модулям, но только по явному решению пользователя.

**Технические изменения:**

- обновлены:
  - `src/blockEditor/ModulePalette.h`
  - `src/blockEditor/ModulePalette.cpp`
  - `tests/blockEditor/test_ModulePalette.cpp`
  - `docs/library/core/README.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_ModulePalette`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModulePalette`

**Итог:**

- legacy-статус теперь влияет не только на текст tooltip, но и на видимость в палитре;
- следующий логичный шаг:
  - выбирать следующий реальный кандидат на `legacy/remove`,
  - либо переходить к верхнеуровневому README/demo narrative.

### Шаг 58 — `core.control.delay_ms` переведён в `legacy`

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- следующий честный кандидат из usage-аудита переведён из `specialized` в `legacy`:
  - `core.control.delay_ms`;
- пересчитана reviewed-матрица ролей:
  - `essential = 28`
  - `specialized = 13`
  - `legacy = 2`;
- обновлены docs по `control` и общая `core`-матрица решений.

**Зачем это сделано:**

- `delay_ms` не входит в baseline;
- модуль нигде осмысленно не используется в reference-сценариях;
- блокирующая пауза хуже подходит для роли checked-in `core` helper-а, чем для внешнего runtime pack-а;
- поэтому для него честнее статус совместимости, а не рекомендуемого прикладного модуля.

**Технические изменения:**

- обновлены:
  - `src/core/StandardLibrary.cpp`
  - `tests/core/test_StandardLibrary.cpp`
  - `docs/library/core/control.md`
  - `docs/library/core/README.md`
  - `docs/library/core/audit_matrix.md`

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`

**Итог:**

- `legacy`-слой больше не ограничивается одним `print_int`;
- следующий логичный шаг:
  - либо искать следующий кандидат на `legacy/remove`,
  - либо считать `Phase 2` достаточно укреплённой и переключаться на `Phase 3`.

### Шаг 59 — `core.math.pow` и `core.math.sqrt` переведены в `legacy`

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- в `legacy` переведены ещё два checked-in `core`-модуля:
  - `core.math.pow`
  - `core.math.sqrt`;
- reviewed-матрица ролей пересчитана:
  - `essential = 28`
  - `specialized = 11`
  - `legacy = 4`.

**Зачем это сделано:**

- оба модуля являются thin wrapper-ами над `libm`;
- core-пайплайн не выражает для них явную `link_libraries` механику так, как это уже
  умеют imported pack-ы;
- как часть checked-in `core` baseline они слабее, чем как кандидаты для внешнего
  math pack-а с явной линковкой и более полной math-моделью.

**Технические изменения:**

- обновлены:
  - `src/core/StandardLibrary.cpp`
  - `tests/core/test_StandardLibrary.cpp`
  - `docs/library/core/math.md`
  - `docs/library/core/README.md`
  - `docs/library/core/audit_matrix.md`

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`

**Итог:**

- `legacy`-слой теперь включает не только shortcut- и runtime-helper-ы, но и слабые
  thin wrapper-ы над внешней math-библиотекой;
- следующий логичный шаг:
  - либо продолжать curation-аудит дальше,
  - либо переключаться на `Phase 3`.

### Шаг 60 — partial float support переведён в `legacy`

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- в `legacy` переведён следующий цельный curation-slice:
  - `core.io.print_float`
  - `core.conversion.float_to_int`
  - `core.conversion.int_to_float`
  - `core.math.add_float`
  - `core.math.multiply_float`;
- reviewed-матрица ролей пересчитана:
  - `essential = 28`
  - `specialized = 6`
  - `legacy = 9`.

**Зачем это сделано:**

- этот набор образует неполный float-хвост внутри `core`, но не даёт цельного
  numeric baseline;
- он нигде не используется в эталонных сценариях проекта;
- такой слой лучше развивать как внешний numeric pack, а не как checked-in core-библиотеку.

**Технические изменения:**

- обновлены:
  - `src/core/StandardLibrary.cpp`
  - `tests/core/test_StandardLibrary.cpp`
  - `docs/library/core/io.md`
  - `docs/library/core/conversion.md`
  - `docs/library/core/math.md`
  - `docs/library/core/README.md`
  - `docs/library/core/audit_matrix.md`

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`

**Итог:**

- legacy-слой теперь покрывает весь слабый partial float slice внутри `core`;
- следующий логичный шаг:
  - либо продолжать curation-аудит уже по оставшимся `specialized`,
  - либо считать `Phase 2` достаточно укреплённой и переходить к `Phase 3`.

### Шаг 57 — specialized core-модули убраны из default palette flow

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- в `Module Palette` появился переключатель `Показывать specialized`;
- specialized core-модули и чисто specialized core-категории теперь скрыты по умолчанию;
- default core-flow палитры теперь показывает рекомендуемый baseline:
  - essential
  - convenience
- specialized и legacy остаются доступны, но только по явному запросу.

**Зачем это сделано:**

- сделать curated standard library реально читаемой в повседневной работе;
- убрать прикладной хвост `core` из первого слоя выбора модулей;
- приблизить палитру к идее “сначала baseline, потом расширения”.

**Технические изменения:**

- обновлены:
  - `src/blockEditor/ModulePalette.h`
  - `src/blockEditor/ModulePalette.cpp`
  - `tests/blockEditor/test_ModulePalette.cpp`
  - `docs/library/core/README.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_ModulePalette`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_ModulePalette`

**Итог:**

- default palette flow теперь стал заметно ближе к рекомендованному baseline;
- следующий логичный шаг:
  - либо возвращаться к выбору следующего кандидата на `legacy/remove`,
  - либо уже переключаться на README/demo narrative для `Phase 3`.

### Шаг 61 — `core.control.if_then` переведён в legacy

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- `core.control.if_then` переведён из `specialized` в `legacy`;
- reviewed-матрица ролей пересчитана:
  - `essential = 28`
  - `specialized = 5`
  - `legacy = 10`;
- обновлены библиотечные docs и checklist по текущему curation-срезу.

**Зачем это сделано:**

- `if_then` остаётся слишком узким `int`-only data-flow helper-ом для checked-in `core`;
- он не усиливает базовую модель `exec`-графа так, как это делает `if_branch`;
- более честное место для такого паттерна:
  - локальный submodule/project helper;
  - внешний extension pack.

**Технические изменения:**

- обновлены:
  - `src/core/StandardLibrary.cpp`
  - `tests/core/test_StandardLibrary.cpp`
  - `docs/library/core/control.md`
  - `docs/library/core/README.md`
  - `docs/library/core/audit_matrix.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`

**Итог:**

- control-category теперь содержит только `essential` baseline и `legacy`-хвост;
- следующий логичный шаг:
  - либо продолжать audit уже по оставшимся `specialized` (`abs`, `mod`, `str_*`),
  - либо остановить curation на этом срезе и перейти к `Phase 3`.

### Шаг 62 — `core.math.abs` и `core.math.mod` переведены в legacy

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- `core.math.abs` и `core.math.mod` переведены из `specialized` в `legacy`;
- reviewed-матрица ролей пересчитана:
  - `essential = 28`
  - `specialized = 3`
  - `legacy = 12`;
- обновлены библиотечные docs и checklist по math-category.

**Зачем это сделано:**

- это узкие прикладные `int`-helper-ы, которые не усиливают checked-in baseline;
- они не участвуют в эталонных сценариях проекта;
- более честное место для такого слоя:
  - внешний numeric pack;
  - локальный project helper.

**Технические изменения:**

- обновлены:
  - `src/core/StandardLibrary.cpp`
  - `tests/core/test_StandardLibrary.cpp`
  - `docs/library/core/math.md`
  - `docs/library/core/README.md`
  - `docs/library/core/audit_matrix.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`

**Итог:**

- в `specialized` checked-in core остался только string-slice:
  - `core.string.str_compare`
  - `core.string.str_concat`
  - `core.string.str_length`;
- следующий логичный шаг:
  - либо перевести и string-slice в `legacy`,
  - либо оставить его как последний прикладной specialized-хвост checked-in core.

### Шаг 63 — `modules/core` закреплён как единый source-of-truth для standard library

**Фаза:** `Phase 2 / Standard Library Quality`

**Что сделано:**

- `StandardLibrary::createAll()` перестал собирать core pack из hardcoded C++-описаний;
- `StandardLibrary::createAll()` теперь читает checked-in `.dqmod` из source-of-truth pack-а на диске;
- `StandardLibrary::install()` теперь копирует source-of-truth pack, а не генерирует второй вариант core-библиотеки;
- `StandardLibrary::isUpToDate()` сравнивает установленный pack с source-of-truth pack-ом;
- добавлены регрессии на `createAll()` и `install()`.

**Зачем это сделано:**

- устранить раздвоение source-of-truth между `modules/core` и `StandardLibrary.cpp`;
- закрепить модель, где `StandardLibrary` отвечает за:
  - audit;
  - ordering;
  - curation;
  - copy/sync pack-а;
- но не за второе независимое определение checked-in core-модулей.

**Технические изменения:**

- обновлены:
  - `src/core/StandardLibrary.h`
  - `src/core/StandardLibrary.cpp`
  - `tests/core/test_StandardLibrary.cpp`
  - `docs/plan/strategy_2026/21_standard_library_strategy.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target test_StandardLibrary`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_StandardLibrary`

**Итог:**

- для checked-in core pack остался один источник истины: файловая библиотека в `modules/core`;
- следующий логичный шаг:
  - либо закрывать curation на текущем срезе и считать string-slice последним честным `specialized`,
  - либо отдельно обсуждать судьбу string-модулей.

### Шаг 64 — верхнеуровневый README выровнен под Linux-first public narrative

**Фаза:** `Phase 3 / Public Narrative`

**Что сделано:**

- `README.md` и `README_RU.md` больше не подают DeltaQ как слишком широкую "универсальную IDE";
- в README добавлены явные секции `What DeltaQ Is / Что такое DeltaQ` и `Current Scope / Текущий scope`;
- верхнеуровневая подача теперь прямо фиксирует главный workflow:
  - `module -> graph -> generated C code -> build -> run`;
- README теперь явно называют Linux текущей целевой платформой и отделяют:
  - уже закрытый Linux delivery;
  - ещё не закрытые Windows/macOS claims;
- Quick Start теперь тоже прямо помечен как Linux-first build/release path;
- в `98_strategy_checklist.md` закрыт пункт про очистку позиционирования от слишком широких обещаний.

**Зачем это сделано:**

- выровнять public-facing narrative с реальным состоянием репозитория;
- убрать разрыв между сильной Linux delivery readiness и незавершённой cross-platform story;
- сделать DeltaQ более цельным продуктом в подаче, а не набором технических подсистем.

**Технические изменения:**

- обновлены:
  - `README.md`
  - `README_RU.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- выполнено ручное ревью текстов и roadmap-секций;
- тесты не запускались, так как изменения только документационные.

**Итог:**

- верхнеуровневый narrative теперь честно фиксирует DeltaQ как Linux-first modular C/C++ workflow IDE;
- следующий логичный шаг:
  - либо дожимать внешний narrative вокруг ценности standard library,
  - либо закрывать пункт про "целостный open-source инструмент" уже через финальную polishing-подачу Linux release surface.

### Шаг 65 — standard library value proof зафиксирован через три checked-in сценария

**Фаза:** `Phase 3 / External Trust`

**Что сделано:**

- в `README.md` и `README_RU.md` добавлен отдельный блок про то, почему checked-in `core` полезен на практике;
- в `docs/library/README.md` добавлен явный section `Value Proof Для Standard Library`;
- создан отдельный документ `docs/library/value_proof.md`, который фиксирует три сценария, через которые нужно показывать ценность `core`;
- value proof привязан не к числу модулей, а к уже существующим checked-in example-проектам:
  - `minimal_console_flow`
  - `reusable_composition_console`
  - `desktop_ui_flow`;
- в `98_strategy_checklist.md` закрыты пункты:
  - `Библиотека даёт заметное ускорение в 2-3 эталонных сценариях`;
  - `Внешнему пользователю легко показать ценность стандартной библиотеки`.

**Зачем это сделано:**

- убрать слабое место в external trust narrative;
- показать standard library как практический baseline, а не как набор абстрактных модулей;
- привязать ценность `core` к живым example-flow, которые уже можно открыть, собрать и показать внешнему пользователю.

**Технические изменения:**

- обновлены:
  - `README.md`
  - `README_RU.md`
  - `docs/library/README.md`
  - `docs/library/value_proof.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- выполнено ручное ревью сценариев against checked-in examples и их graph/module composition;
- тесты не запускались, так как изменения только документационные.

**Итог:**

- standard library теперь можно показывать через три конкретных runnable flow, а не через общий тезис "в core есть 43 модуля";
- следующий логичный шаг:
  - либо дожимать последний open пункт Phase 3 про целостность продукта,
  - либо полировать Linux release surface уже как внешний open-source инструмент.

### Шаг 66 — imported pack-и закреплены как основной supply channel экосистемы

**Фаза:** `Phase 2 / Import And Wrapping`

**Что сделано:**

- добавлен второй controlled fixture-case `mini_checksum_sdk` рядом с `mini_sensor_sdk`;
- добавлен второй checked-in imported-pack example:
  - `resources/examples/imported_pack_checksum_console`;
- новый пример содержит:
  - локальный `vendor/mini_checksum_sdk`;
  - curated imported pack `mini_checksum_sdk_curated` в `dqmods/`;
  - корневой граф, использующий только curated layer;
- для нового pack-а добавлен отдельный walkthrough:
  - `docs/library/imported_packs/mini_checksum_sdk.md`;
- `docs/library/README.md` теперь явно фиксирует два imported-pack reference case из разных доменных зон:
  - hardware-like/runtime;
  - algorithmic/text-processing;
- в `98_strategy_checklist.md` закрыт пункт про imported pack-ы как основной supply channel расширения модульной экосистемы.

**Зачем это сделано:**

- убрать ощущение, что imported pack path — это один специальный showcase для `mini_sensor_sdk`;
- показать, что расширение vocabulary DeltaQ реально идёт через external pack-и, а не через раздувание `core`;
- закрепить imported pack-и как повторяемый продуктовый путь, а не разовую инженерную capability.

**Технические изменения:**

- добавлены и обновлены:
  - `resources/examples/imported_pack_checksum_sdk/...`
  - `resources/examples/imported_pack_checksum_console/...`
  - `docs/library/imported_packs/mini_checksum_sdk.md`
  - `docs/library/README.md`
  - `docs/plan/strategy_2026/22_examples_and_reference_projects.md`
  - `docs/plan/strategy_2026/27_library_converter_and_module_packs.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`
  - `tests/codegen/test_PreBuildProcessor.cpp`

**Проверка:**

- новый example закреплён e2e regression-тестом, который делает:
  - `graph -> pre-build -> CMake -> build -> run`;
- runtime проверяет:
  - checksum report;
  - expected/actual comparison;
  - корректный shutdown path.

**Итог:**

- imported pack-и теперь подтверждены не одним, а двумя checked-in reference case;
- следующий логичный шаг:
  - либо дожимать последний open narrative-пункт про целостность продукта,
  - либо уже полировать Linux release surface как внешний open-source инструмент.

### Шаг 67 — создан рабочий план Linux release polish и закрыт первый UX-bug `zoomFit`

**Фаза:** `Linux-first release polish`

**Что сделано:**

- добавлен отдельный рабочий план этапа:
  - `docs/plan/strategy_2026/29_linux_release_polish.md`;
- `BlockEditorWidget::zoomFit()` перестал раздувать маленький граф выше естественного масштаба `1:1`;
- логика auto-fit вынесена в отдельные helper-методы:
  - `fitTargetRect()`
  - `applyAutoFitTransform()`;
- fit теперь:
  - добавляет комфортный padding;
  - расширяет слишком маленький scene-rect до размеров viewport;
  - ограничивает auto-fit сверху значением `AutoFitMaxZoom = 1.0`;
- добавлен отдельный widget-test:
  - `tests/blockEditor/test_BlockEditorWidget.cpp`;
- test покрывает два сценария:
  - single-node graph не overscale-ится;
  - wide graph по-прежнему реально zoom-out-ится.

**Зачем это сделано:**

- убрать первый явный UX-papercut из Linux polish backlog;
- зафиксировать рабочий план этапа в репозитории, чтобы он не терялся между сессиями;
- закрепить поведение `zoomFit` regression-тестом, а не только визуальной проверкой.

**Технические изменения:**

- обновлены:
  - `src/blockEditor/BlockEditorWidget.h`
  - `src/blockEditor/BlockEditorWidget.cpp`
  - `tests/CMakeLists.txt`
  - `tests/blockEditor/test_BlockEditorWidget.cpp`
  - `docs/plan/strategy_2026/29_linux_release_polish.md`

**Проверка:**

- `cmake --build build --parallel --target test_BlockEditorWidget test_BlockScene`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_BlockEditorWidget`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_BlockScene`

**Итог:**

- Linux polish этап теперь зафиксирован как отдельный рабочий план;
- первый tangible UX-fix уже сделан и покрыт тестом;
- следующий логичный шаг:
  - либо продолжать clean first-run / clean-machine smoke,
  - либо собирать public release surface и screenshots.

### Шаг 68 — release bundle получил automated first-run smoke через isolated `DELTAQ_HOME`

**Фаза:** `Linux-first release polish`

**Что сделано:**

- `SessionManager` получил единый статический путь чтения языка:
  - `storedLanguage()`;
- startup больше не читает язык из отдельного `QSettings("DeltaQ", "IDE")`, а использует тот же writable-root, что и остальная сессия IDE;
- в `main.cpp` добавлена поддержка `DELTAQ_SMOKE_EXIT_MS`, чтобы automation-path мог завершать GUI-процесс без ручного взаимодействия;
- добавлен shell smoke:
  - `scripts/smoke_linux_first_run.sh`;
- smoke запускает `deltaq` из готового bundle с изолированным `DELTAQ_HOME` и проверяет, что на первом запуске создаются:
  - `config/settings.ini`;
  - `modules/core/pack.json`;
- `build_release.sh` теперь автоматически прогоняет этот smoke после сборки release bundle;
- `.github/workflows/ci.yml` тоже запускает этот smoke на install-layout `build/install-smoke`;
- для детерминированности smoke по умолчанию форсирует `QT_QPA_PLATFORM=offscreen`, если пользователь сам не задал другой backend.

**Зачем это сделано:**

- проверить не только layout release bundle, но и реальный первый запуск IDE;
- зафиксировать Linux-first runtime-path как воспроизводимую часть release discipline;
- убрать зависимость automation-проверки от случайного состояния GUI-сессии.

**Технические изменения:**

- обновлены:
  - `src/core/SessionManager.h`
  - `src/core/SessionManager.cpp`
  - `src/main.cpp`
  - `tests/core/test_SessionManager.cpp`
  - `scripts/smoke_linux_first_run.sh`
  - `scripts/build_release.sh`
  - `.github/workflows/ci.yml`
  - `docs/release/README.md`
  - `docs/plan/strategy_2026/29_linux_release_polish.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake -S . -B build -DDQ_BUILD_TESTS=ON`
- `cmake --build build --parallel --target test_SessionManager deltaq`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_SessionManager`
- `./scripts/build_release.sh --no-tests`
- `./scripts/smoke_linux_first_run.sh build/release/DeltaQ`

**Итог:**

- Linux release bundle теперь проходит не только static layout verification, но и реальный first-run smoke;
- tracking этапа больше не теряется:
  - high-level статус виден в `98_strategy_checklist.md`;
  - подробный план живёт в `29_linux_release_polish.md`;
  - факт выполнения зафиксирован в `99_execution_log.md`.

### Шаг 69 — release bundle получил automated `open example -> build -> run` smoke

**Фаза:** `Linux-first release polish`

**Что сделано:**

- в `MainWindow` добавлен внутренний startup automation path:
  - открыть `.dqproj`;
  - выполнить `build`;
  - выполнить `run`;
  - опционально подать stdin;
  - проверить expected stdout;
  - завершить IDE с детерминированным exit code;
- `main.cpp` получил automation CLI-опции:
  - `--automation-project`
  - `--automation-build`
  - `--automation-run`
  - `--automation-stdin`
  - `--automation-expect-output`
  - `--automation-quit`;
- `BuildPipeline` теперь честно завершает pipeline после compile phase, а не зависает без `pipelineFinished`;
- `CMakeGenerator` начал нормализовывать project-facing dialect labels вроде `c17` и `c++20` в значения, которые реально понимает CMake;
- добавлен shell smoke:
  - `scripts/smoke_linux_example_build_run.sh`;
- smoke:
  - копирует checked-in example из bundled `examples/` в writable temp-workspace;
  - запускает `deltaq` из release bundle с automation-аргументами;
  - проходит путь `open project -> build -> run -> quit`;
  - проверяет expected runtime output для `minimal_console_flow`;
- `build_release.sh` и `.github/workflows/ci.yml` теперь прогоняют этот smoke как часть Linux release discipline.

**Зачем это сделано:**

- проверить не только запуск IDE, но и реальный продуктовый workflow из готового release bundle;
- сделать regression-path для shortest example loop reproducible в CI и локально;
- закрыть реальный найденный баг в build workflow: `c17` из `.dqproj` ломал generated `CMakeLists.txt`.

**Технические изменения:**

- обновлены:
  - `src/codegen/BuildPipeline.cpp`
  - `src/core/MainWindow.h`
  - `src/core/MainWindow.cpp`
  - `src/main.cpp`
  - `src/editor/CMakeGenerator.cpp`
  - `tests/editor/test_CMakeGenerator.cpp`
  - `scripts/smoke_linux_example_build_run.sh`
  - `scripts/build_release.sh`
  - `.github/workflows/ci.yml`
  - `docs/release/README.md`
  - `docs/plan/strategy_2026/29_linux_release_polish.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `cmake --build build --parallel --target deltaq test_CMakeGenerator`
- `QT_QPA_PLATFORM=offscreen ./build/tests/test_CMakeGenerator`
- ручной automation-run:
  - `./build/src/deltaq --automation-project ... --automation-build --automation-run --automation-stdin 'DeltaQ\\n' --automation-expect-output 'Hello from DeltaQ!\\nDeltaQ' --automation-quit`
- `./scripts/build_release.sh --no-tests`
- `./scripts/smoke_linux_example_build_run.sh build/release/DeltaQ`

**Итог:**

- Linux release bundle теперь проходит:
  - layout verification;
  - isolated first-run smoke;
  - real example `open -> build -> run` smoke;
- clean Linux first-run path заметно продвинут, но AppImage-specific user handoff в writable workspace остаётся следующим открытым кусочком.

### Шаг 70 — AppDir и extracted AppImage переведены с layout-check на runtime-smoke verification

**Фаза:** `Linux-first release polish`

**Что сделано:**

- `verify_appdir.sh` больше не ограничивается static layout-check;
- после проверки desktop metadata и bundle layout он теперь запускает:
  - `smoke_linux_first_run.sh` на `AppDir/usr/bin`;
  - `smoke_linux_example_build_run.sh` на `AppDir/usr/bin`;
- `verify_appimage_file.sh` автоматически наследует этот же уровень проверки, потому что после `--appimage-extract` повторно вызывает `verify_appdir.sh` на unpacked image.

**Зачем это сделано:**

- сделать AppDir / AppImage verification сопоставимой по строгости с обычным Linux release bundle;
- проверить, что AppImage contents не только правильно упакованы, но и реально поднимают shortest DeltaQ workflow;
- приблизить пункт `release/AppImage -> launch -> open example -> build -> run` к честному закрытию.

**Технические изменения:**

- обновлены:
  - `scripts/verify_appdir.sh`
  - `docs/release/README.md`
  - `docs/plan/strategy_2026/29_linux_release_polish.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `./scripts/build_appimage.sh --no-tests --appdir-only`
- при наличии готового `.AppImage`: `./scripts/verify_appimage_file.sh <file.AppImage> <SHA256SUMS>`

**Итог:**

- AppDir verification теперь проверяет runtime, а не только структуру папок;
- extracted `.AppImage` наследует тот же runtime-smoke уровень через `verify_appimage_file.sh`;
- следующий открытый пласт Linux polish уже смещается из packaging в public release surface и user-facing proof.

### Шаг 71 — свежесобранный `.AppImage` доведён до полностью локально подтверждённого runtime-artifact

**Фаза:** `Linux-first release polish`

**Что сделано:**

- в `build_appimage.sh` исправлен реальный packaging-bug: `appstreamcli` теперь резолвится до подмены `PATH`, поэтому локальный wrapper больше не рекурсирует сам в себя и не подвешивает `appimagetool`;
- в тот же скрипт добавлено явное включение `Qt` platform plugin `libqoffscreen.so` в `AppDir/usr/plugins/platforms`;
- после этого `build_appimage.sh` снова выполняет полный цикл:
  - release bundle build;
  - AppDir verification;
  - real `.AppImage` build;
  - `verify_appimage_file.sh` на freshly built artifact.

**Зачем это сделано:**

- закрыть AppImage не как infrastructure scaffolding, а как реально собранный и локально проверенный Linux artifact;
- убрать расхождение между "AppImage собирается" и "extracted AppImage реально запускает DeltaQ в headless/runtime smoke";
- добить delivery/runtime slice Linux polish до честного `done`.

**Технические изменения:**

- обновлены:
  - `scripts/build_appimage.sh`
  - `docs/release/README.md`
  - `docs/plan/strategy_2026/29_linux_release_polish.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`

**Проверка:**

- `./scripts/build_appimage.sh --no-tests --linuxdeploy build/tools/linuxdeploy --linuxdeploy-qt-plugin build/tools/linuxdeploy-plugin-qt --appimagetool build/tools/appimagetool --runtime-file build/tools/runtime-x86_64`

**Итог:**

- новый `.AppImage` реально собирается локально;
- `verify_appimage_file.sh` проходит на свежем artifact вместе с checksum-check, post-extract first-run smoke и post-extract example `open -> build -> run` smoke;
- для `Linux-first release polish` открытым остаётся уже не packaging/runtime, а public release surface и visual proof.

### Шаг 72 — Public release surface получил единый user-facing entry path

**Фаза:** `Linux-first release polish`

**Что сделано:**

- добавлены:
  - `docs/onboarding/README.md`
  - `docs/onboarding/README_RU.md`
  - `resources/examples/README.md`
  - `resources/examples/README_RU.md`
  - `docs/release/linux_first_release_checklist.md`
  - `docs/release/linux_first_release_checklist_ru.md`;
- `README.md`, `README_RU.md`, `docs/onboarding/first_run*.md` и `docs/release/README.md` теперь ссылаются друг на друга как на один user-facing маршрут;
- manual Linux artifact handoff теперь описан не только через internal packaging docs, но и через отдельный human-readable checklist для tarball/AppImage.

**Зачем это сделано:**

- убрать разрыв между "коротким first-run", "каталогом examples" и "release artifact validation";
- сделать README entry-point не только продуктовым тезисом, но и навигацией по реальному первому пользовательскому пути;
- приблизить DeltaQ к состоянию цельного Linux-first инструмента без длинных устных пояснений.

**Технические изменения:**

- обновлены:
  - `README.md`
  - `README_RU.md`
  - `docs/onboarding/first_run.md`
  - `docs/onboarding/first_run_ru.md`
  - `docs/release/README.md`
  - `docs/plan/strategy_2026/29_linux_release_polish.md`
  - `docs/plan/strategy_2026/98_strategy_checklist.md`
- добавлены:
  - `docs/onboarding/README.md`
  - `docs/onboarding/README_RU.md`
  - `resources/examples/README.md`
  - `resources/examples/README_RU.md`
  - `docs/release/linux_first_release_checklist.md`
  - `docs/release/linux_first_release_checklist_ru.md`

**Проверка:**

- sanity-check ссылок и entry-point paths через `rg` по `README`, `docs/` и `resources/examples/`.

**Итог:**

- narrative между `README`, onboarding, examples и release docs теперь собран в единый путь;
- user-facing Linux artifact checklist закрыт;
- открытым куском public release surface остаётся уже не базовая навигация, а screenshots / visual proof.

### Шаг 73 — запущен execution track `Linux Project Export v1`

**Фаза:** `Linux-first delivery follow-up`

**Что сделано:**

- зафиксировано, что packaging самой `DeltaQ IDE` и export пользовательского
  приложения являются разными delivery-контурами;
- создан новый план
  `docs/plan/strategy_2026/30_linux_project_export_v1.md`;
- `98_strategy_checklist.md` обновлён:
  - новый active execution track теперь явно выделен;
  - добавлен отдельный delivery-block для `Linux Project Export v1`.

**Зачем это сделано:**

- убрать стратегическую путаницу между "IDE собирается как Linux artifact" и
  "IDE выпускает готовый Linux artifact для пользовательского проекта";
- перенести главный implementation focus с остаточного public polish на
  закрытие product-critical path:
  `build -> export -> runnable Linux bundle`;
- зафиксировать scope `v1` так, чтобы не расползтись сразу в AppImage export,
  Windows/macOS и universal portability claims.

**Что зафиксировано в плане:**

- `v1` export строится как directory bundle в `dist/<ProjectName>/`, а не сразу
  как AppImage;
- сначала делается backend export service и automated smoke, потом UI action;
- acceptance для `v1` закрывается минимум на `console` и `desktop`;
- imported pack runtime payload считается отдельным явно контролируемым куском,
  а не "магией по умолчанию".

**Проверка:**

- сверены текущие repo facts:
  - `test_PreBuildProcessor.cpp` уже подтверждает
    `pre-build -> CMake -> build -> run` для `console` и `desktop`;
  - `CMakeGenerator.cpp` по desktop-пути всё ещё опирается на системные
    `SDL2`/`SDL2_ttf`;
  - отдельного export pipeline для пользовательского проекта в репозитории пока нет.

**Итог:**

- `strategy_2026` теперь содержит не только Linux packaging story для IDE, но и
  отдельный source-of-truth для следующего product-critical этапа;
- дальнейшая реализация может идти последовательно по плану
  `30_linux_project_export_v1.md`, без создания второго несвязанного набора
  планов.

### Шаг 74 — execution spec для `Linux Project Export v1` доведён до `v2`

**Фаза:** `Linux-first delivery follow-up`

**Что сделано:**

- скорректирован план `30_linux_project_export_v1.md` по четырём критическим
  execution-gap-ам, выявленным при review;
- убрана ошибочная опора на `BuildManager::expectedBuildArtifact(...)` как на
  способ найти runtime binary проекта;
- desktop font/data dependency поднята из "later asset story" в основной desktop
  export contract;
- в verification добавлен clean-environment gate вместо проверки только на
  developer machine;
- зафиксирована semantics свежести сборки:
  user-facing export по умолчанию делает rebuild перед assemble/export.

**Зачем это сделано:**

- убрать риск, что реализация export начнёт паковать `Makefile`/`build.ninja`
  вместо реального executable;
- не допустить ложного закрытия desktop export только упаковкой `.so`, когда UI
  runtime всё ещё зависит от системных шрифтов;
- сделать self-contained claim проверяемым честно, а не только на машине, где
  уже установлены все нужные runtime pieces;
- исключить silent packaging stale build-а.

**На что теперь опирается план:**

- `BuildManager::expectedBuildArtifact(...)` трактуется только как marker
  сконфигурированного build tree;
- export должен получить отдельный executable resolver поверх текущих heuristic
  путей (`MainWindow::resolveProjectExecutable()` / test helper
  `findBuiltExecutable(...)`);
- desktop export acceptance требует bundled font baseline и явного runtime lookup
  order;
- delivery-claim требует не только local smoke, но и isolated verification.

**Итог:**

- `30_linux_project_export_v1.md` больше не является только strategy outline;
- документ стал ближе к реальному execution spec, на который уже можно
  последовательно опираться при реализации export backend.

### Шаг 75 — реализован первый кодовый срез `Linux Project Export v1` для console-path

**Фаза:** `Linux-first delivery follow-up`

**Что сделано:**

- добавлен `src/editor/ProjectExecutableResolver.{h,cpp}`;
- добавлен `src/editor/LinuxProjectExporter.{h,cpp}`;
- `MainWindow` переведён на `ProjectExecutableResolver` для run/debug пути вместо
  ручного поиска по нескольким hardcoded candidate-path;
- добавлены autotest-ы:
  - `tests/editor/test_ProjectExecutableResolver.cpp`
  - `tests/editor/test_LinuxProjectExporter.cpp`;
- `98_strategy_checklist.md` обновлён так, чтобы console export был отмечен как
  реально закрытый slice, а общий Linux export track остался честно частичным.

**Зачем это сделано:**

- закрыть первый реальный implementation step из нового плана, а не оставлять
  `Linux Project Export v1` только на уровне документа;
- развести build-system marker и runtime executable уже в коде, а не только в spec;
- получить минимальный handoff-ready artifact хотя бы для console-пути;
- закрепить regression path `build -> export -> run exported bundle`.

**Технические детали:**

- `ProjectExecutableResolver` ищет runnable executable в `build/` и подкаталогах,
  вместо misuse `BuildManager::expectedBuildArtifact(...)`;
- `LinuxProjectExporter` пока честно поддерживает только `console` и собирает
  directory bundle:
  - launcher в корне bundle;
  - `bin/<ProjectName>.bin`;
  - `lib/`;
  - `assets/fonts/`;
  - `EXPORT_INFO.txt`;
- desktop export в текущем slice явно отклоняется с диагностикой, чтобы не
  создавать ложное впечатление завершённости.

**Проверка:**

- `cmake -S . -B build -DDQ_BUILD_TESTS=ON`
- `cmake --build build --parallel --target deltaq test_BuildManager test_ProjectExecutableResolver test_LinuxProjectExporter`
- `ctest --test-dir build --output-on-failure -R 'test_(BuildManager|ProjectExecutableResolver|LinuxProjectExporter)$'`

**Результат проверки:**

- все 3 целевых теста прошли успешно;
- `test_LinuxProjectExporter` подтверждает, что console-bundle можно запустить из
  handoff directory после удаления исходного project tree.

**Итог:**

- `Linux Project Export v1` вышел из состояния "только план";
- console-path теперь закрыт как реальная рабочая вертикаль;
- следующим implementation target остаётся desktop runtime bundling с font/data
  baseline и isolated verification.

### Шаг 76 — добавлен user-facing `Build -> Export Linux Bundle` и закрыт local desktop bundle smoke

**Фаза:** `Linux-first delivery follow-up`

**Что сделано:**

- в `ActionManager` добавлено действие `build.exportLinuxBundle`;
- в `MainWindow` добавлен user-facing путь `Build -> Export Linux Bundle`;
- export теперь запускается только через связку `pre-build -> build -> export`, а
  не пакует последний случайный output из `build/`;
- build/output wiring в `MainWindow` выровнен так, чтобы pre-build log больше не
  стирался на событии `BuildManager::buildStarted`;
- autotest `test_MainWindowEditorActions` расширен проверкой регистрации и wiring
  нового action;
- `test_LinuxProjectExporter` уже подтверждает runnable desktop bundle после
  удаления исходного project tree.

**Зачем это сделано:**

- закрыть важный user-facing slice плана, а не оставлять export только backend-функцией;
- жёстко зафиксировать semantics "сначала собрать, потом экспортировать";
- приблизить Linux export к реальному UX внутри IDE;
- перевести desktop export из состояния "архитектурно задуман" в состояние
  "локально runnable и проверенный autotest-ом".

**Технические детали:**

- `MainWindow::runBuildPipeline(...)` стал общим helper-ом для обычного build и
  export-coupled build;
- `MainWindow::exportCurrentProjectLinuxBundle(...)` использует
  `LinuxProjectExporter` и пишет результат в `Build Output`;
- новое action подключено в `Build` menu и использует дефолтный output path
  `dist/<ProjectName>/`;
- desktop export по-прежнему не объявлен полностью завершённым: runtime payload и
  local smoke уже есть, но clean-environment / isolated verification gate ещё открыт.

**Проверка:**

- `cmake --build build --parallel --target deltaq test_ActionManager test_MainWindowEditorActions test_MainWindowBuildOutputNavigation test_LinuxProjectExporter`
- `QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure -R 'test_(ActionManager|MainWindowEditorActions|MainWindowBuildOutputNavigation|LinuxProjectExporter)$'`

**Результат проверки:**

- все 4 целевых теста прошли успешно;
- `MainWindow` user-facing wiring подтверждён GUI-тестом;
- regression по build output navigation не появилась;
- desktop export smoke остаётся зелёным после добавления user-facing export flow.

**Итог:**

- внутри IDE появился честный путь `Build -> Export Linux Bundle`;
- export больше не требует ручного backend-вызова;
- текущий основной незакрытый technical gap сместился на isolated verification и
  packaging/handoff polishing exported bundle.

### Шаг 77 — добавлены project-export archive packaging и loader-level verification scripts

**Фаза:** `Linux-first delivery follow-up`

**Что сделано:**

- `LinuxProjectExporter` расширен поддержкой `.tar.gz` архива из того же bundle;
- `MainWindow` export flow теперь по умолчанию создаёт не только `dist/<ProjectName>/`,
  но и archive рядом с ним;
- добавлены shell-скрипты:
  - `scripts/verify_project_export_bundle.sh`
  - `scripts/verify_project_export_archive.sh`;
- `test_LinuxProjectExporter` расширен так, чтобы проверять:
  - bundle verification script;
  - archive verification script;
  - извлечение `.tar.gz`;
  - запуск extracted artifact после удаления исходного bundle directory.

**Зачем это сделано:**

- закрыть handoff gap между "есть directory bundle" и "есть реальный передаваемый artifact";
- получить loader-level proof, что non-allowlisted runtime libs у desktop export
  берутся из bundle, а не молча из host system;
- закрепить regression path не только для bundle, но и для packaged archive.

**Технические детали:**

- `LinuxExportOptions` получил `packageArchive`, а `LinuxExportResult` — `archivePath`;
- archive собирается через `tar -czf` из уже подготовленного bundle directory;
- `verify_project_export_bundle.sh` валидирует layout, desktop font baseline и
  через `ldd` + `LD_LIBRARY_PATH` проверяет, что non-allowlisted libs резолвятся
  из `bundle/lib`;
- `verify_project_export_archive.sh` распаковывает архив и прогоняет bundle verifier
  на extracted payload.
- launcher больше не зависит от внешнего `dirname`, поэтому extracted bundle можно
  запускать в почти пустом environment без скрытой зависимости от host `PATH`.

**Проверка:**

- `cmake --build build --parallel --target test_LinuxProjectExporter test_MainWindowEditorActions`
- `QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure -R 'test_(LinuxProjectExporter|MainWindowEditorActions)$'`

**Результат проверки:**

- оба теста прошли успешно;
- exporter теперь regression-covered не только по directory bundle, но и по `.tar.gz`;
- extracted archive запускается после удаления исходного bundle;
- extracted archive дополнительно запускается в near-clean environment;
- loader-level verification для desktop export автоматизирован хотя бы на уровне
  текущего Linux host.

**Итог:**

- `Linux Project Export v1` получил handoff-ready archive path;
- закрыт acceptance про упаковку exported bundle без отдельного packaging flow;
- следующий незакрытый technical gate теперь уже уже сузился до clean-environment
  verification и user-visible documentation/limitations.
