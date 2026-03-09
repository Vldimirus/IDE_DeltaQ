# Strategy 2026 Checklist

## Purpose

Этот файл нужен как **короткая карта задач и статусов** по всему контуру `strategy_2026`.

В отличие от `99_execution_log.md`, здесь хранится не хронология, а **актуальный срез**:

- что уже выполнено;
- что сейчас в работе;
- что ещё не начато;
- какой этап стратегии закрыт, а какой нет.

## Update Rules

- После каждого заметного шага реализации обновлять:
  - этот checklist;
  - `99_execution_log.md`.
- Если задача закрыта частично, она остаётся незакрытой, но получает пометку `(частично)`.
- Если крупный этап завершён, это явно отмечается в секции фаз и стадий.

## Legend

- `[x]` выполнено
- `[ ]` не выполнено или не завершено
- `(частично)` означает, что работа начата, но критерий закрытия ещё не достигнут

## Phase Status

- `[x] Phase 1: Core Stabilization`
- `[ ] Phase 2: Module Ecosystem`
- `[ ] Phase 3: Showcase And Adoption`

## Foundation And Governance

- `[x]` Зафиксирован главный стратегический контур `strategy_2026`
- `[x]` Зафиксирован продуктовый тезис DeltaQ как среды модульной композиции
- `[x]` Зафиксирована строгая модель модуля
- `[x]` Зафиксирован главный workflow `модуль -> граф -> код -> сборка -> запуск`
- `[x]` Зафиксирована стратегия codegen/runtime
- `[x]` Зафиксированы правила scope control
- `[x]` Зафиксированы критерии acceptance по фазам
- `[x]` Зафиксированы правила разработки и русскоязычных комментариев

## Phase 1: Core Stabilization

### Source Of Truth And Generated Files

- `[x]` Добавлены banner-ы с origin для generated graph/UI files
- `[x]` Pre-build переведён на структурированные generated artifacts
- `[x]` В build output видно роль generated-файла и его origin
- `[x]` Вкладки редактора помечают generated-файлы как `[gen]`
- `[x]` Generated-файлы открываются в read-only режиме с явным source-of-truth
- `[x]` Есть переход `generated file -> origin`
- `[x]` В дереве проекта generated-файлы помечаются отдельно
- `[x]` В дереве проекта по умолчанию открывается source-of-truth, а не generated output

### Build Diagnostics And Navigation

- `[x]` Ошибки из build output кликабельны
- `[x]` Навигация работает по цепочке `build error -> generated file -> source-of-truth`
- `[x]` Есть отдельная вкладка `Build Diagnostics`
- `[x]` Editor actions учитывают разницу между generated и source-of-truth файлами

### Core Workflow Reliability

- `[x]` Зафиксирован reference flow для console template
- `[x]` Зафиксирован reference flow для desktop template
- `[x]` Есть полноценный end-to-end сценарий `build -> run` без ручных обходов

### Module Readiness And Admission

- `[x]` В `Module Manager` есть явный статус готовности модуля
- `[x]` Атомарный пользовательский модуль не допускается в композицию без `passed`
- `[x]` Недопущенные модули блокируются в палитре и при вставке в граф
- `[x]` Составной модуль проходит допуск по внутреннему графу, а не только по `graphId`
- `[x]` Модель проверки модуля полностью разведена на отдельные стадии:
  - валидность контракта
  - наличие реализации
  - compile check
  - test check

## Submodule Compilation Units

### Stage 1: Boundary Metadata

- `[x]` У составного модуля есть `boundaryInputs`
- `[x]` У составного модуля есть `boundaryOutputs`
- `[x]` Boundary metadata сериализуется в `.dqmod`
- `[x]` `SubModuleFactory` заполняет boundary metadata при создании подмодуля
- `[x]` `ModuleRegistry` валидирует boundary metadata

### Stage 2: Separate Generated Units

- `[x]` Для составного модуля генерируются отдельные `.h/.c`
- `[x]` Родительский граф подключает header подмодуля и вызывает generated-функцию
- `[x]` `PreBuildProcessor` сначала генерирует submodule units, потом корневые графы
- `[x]` Build pipeline показывает submodule artifacts как отдельные сущности
- `[x]` Есть отдельное поле `generated_file_base` в модуле
- `[x]` При создании подмодуля можно задать имя generated-файлов отдельно от имени модуля
- `[x]` Если имя generated-файлов не задано, используется fallback `submodule_01`, `submodule_02`, ...

### Stage 3: Reusable Composition

- `[x]` Корректно выражена boundary-модель для `execution`-входов и `execution`-выходов подмодуля
- `[x]` Есть полноценный reusable composition scenario с несколькими подмодулями
- `[x]` Есть reference example, где подмодуль показывает реальную ценность reuse

## Phase 2: Module Ecosystem

### Standard Library Quality

- `[x]` Определены обязательные категории стандартной библиотеки для `v1`
- `[x]` Введён `v1`-audit стандартной библиотеки
- `[x]` Выровнены naming и интерфейсы ключевых модулей
- `[x]` `Module Manager` и `Module Palette` показывают core-библиотеку в одном `v1`-порядке
- `[x]` Введена product-curation модель core-модулей: `essential / convenience / specialized`
- `[x]` Для checked-in `core` pack есть явная reviewed-матрица curation
- `[x]` Checked-in `modules/core` является единым source-of-truth для core pack-а
- `[x]` Specialized core-модули выведены из default palette flow, но доступны по явному запросу
- `[x]` Legacy core-модули выведены из default palette flow, но доступны по явному запросу
- `[x]` `Module Manager` использует тот же baseline-filter для `specialized` и `legacy`, что и `Module Palette`
- `[ ]` Убраны слабые и дублирующие модули `(частично: в legacy уже переведены core.io.print_int, core.control.if_then, core.control.delay_ms, узкие math int-helper-ы и слабый partial-float slice)`
- `[ ]` Библиотека даёт заметное ускорение в 2-3 эталонных сценариях

### Module Verification

- `[x]` Унифицирован минимальный verify pipeline для модулей
- `[x]` Чётко различается качественный модуль и сырой модуль
- `[x]` Статус проверки модуля понятен во всех ключевых местах IDE
- `[x]` `Module Manager` явно показывает слой экосистемы и quality bar текущего модуля

### Module Documentation

- `[x]` Для ключевых модулей описаны назначение, контракт и ограничения
- `[x]` В IDE улучшена discoverability ролей core-модулей
- `[x]` Для значимых модулей есть документационный стандарт, видимый пользователю
- `[x]` Есть единая точка входа в library docs для `core`, imported pack-ов и verification story

### Import And Wrapping

- `[x]` Зафиксировано различие `core / external pack / project module`
- `[x]` Зафиксирован pipeline `library -> raw wrappers -> curated pack -> graph`
- `[x]` Зафиксирована минимальная модель imported module pack
- `[x]` Imported pack-и читаются в `Module Manager` и `Module Palette` как отдельные pack-boundaries, а не только как россыпь extension-модулей
- `[x]` Есть minimum viable converter для `C`-библиотек
- `[x]` Build pipeline учитывает `include_paths / defines / link_libraries` от реально используемых imported pack-ов
- `[x]` Есть детальный исполнимый план `imported pack showcase + curation`
- `[x]` В репозитории есть controlled fixture library для первого imported-pack showcase
- `[x]` Есть reproducible raw import baseline для fixture library
- `[x]` Есть формализованный `v1` flow curation imported pack-а
- `[x]` Есть curated pack, пригодный для реального использования в графе
- `[ ]` Импорт библиотек используется как основной supply channel для расширения экосистемы модулей
- `[x]` Есть reference scenario `external library -> curated pack -> project graph -> build/run`

## Examples And Reference Projects

- `[x]` Есть внутренний reference flow для minimal console scenario
- `[x]` Есть внутренний reference flow для desktop/UI scenario
- `[x]` Есть оформленный reusable composition example
- `[x]` Эталонные проекты готовы не только как регрессия, но и как showcase/onboarding

## UI Contract And Backends

### Stage 1: UI Contract Definition

- `[x]` В `Module` появились metadata для backend-независимого UI contract
- `[x]` В `UILayout` зафиксирован backend-agnostic contract baseline
- `[x]` `UIModuleFactory` создаёт contract-модули, а не SDL2-реализации
- `[x]` `GraphCompiler` различает UI contract по metadata, а не только по legacy-именам
- `[x]` `SDL2CodeGenerator` явно помечен как backend `SDL2`

### Stage 2: SDL2 As Backend v1

- `[x]` В generated UI введены `DQ_UIBackend*` типы и `dq_ui_backend_*` функции
- `[x]` Окно, event pump и frame lifecycle вынесены в SDL2 backend helpers внутри generated `ui.c`
- `[x]` `core.desktop.*` модули помечены как backend runtime слоя `sdl2`
- `[x]` Inline desktop codegen в `GraphCompiler` использует ту же backend boundary
- `[x]` Выделена backend boundary для событий, рендера и окна
- `[x]` SDL2 runtime-модули выровнены с UI contract layer
- `[x]` `GroupBox`, `ComboBox` и `Image` имеют явные SDL2 runtime/render branches, а не generic `TODO` fallback
- `[x]` `RadioButton`, `ScrollPanel` и `TabPanel` имеют явные SDL2 runtime/render branches, а `RadioButton` включён в runtime toggle flow
- `[x]` Весь словарь `UIContract` имеет явные SDL2 runtime/render branches без пропусков по contract-типам

### Stage 3: Runtime Model Consolidation

- `[x]` UI runtime model отделена от SDL2-специфики
- `[x]` Layout/state/event routing выражены как единая внутренняя модель

### Stage 4: Module And `.dqui` Alignment

- `[x]` UI-модули и `.dqui` используют единый словарь контрактов
- `[x]` Нет концептуального расхождения между UI Designer и модульной UI-моделью

## Phase 3: Showcase And Adoption

### Public Narrative

- `[x]` Обновлён верхнеуровневый README под реальную философию DeltaQ
- `[x]` Позиционирование проекта очищено от слишком широких обещаний

### Demo Projects

- `[x]` Подготовлены 2-3 сильных публичных demo-потока
- `[x]` Примеры показывают преимущества модульной композиции на реальных задачах

### Onboarding

- `[x]` Есть короткий путь первого знакомства с продуктом
- `[x]` Новый пользователь быстро понимает роль модуля, графа и generated code

### External Trust

- `[x]` Внешнему пользователю легко показать прозрачность generated code
- `[ ]` Внешнему пользователю легко показать ценность стандартной библиотеки
- `[ ]` Проект выглядит как целостный инструмент, а не набор несвязанных подсистем

## Acceptance Gates

### Phase 1

- `[x]` Generated code быстро сопоставляется с источником
- `[x]` Ошибки build/codegen локализуются в понятных местах
- `[x]` Пользователь понимает роль generated files и source-of-truth
- `[x]` Один эталонный проект стабильно проходит полный цикл `build -> run`
- `[x]` Основной workflow проходит end-to-end без скрытых ручных шагов

### Phase 2

- `[x]` Стандартная библиотека позволяет собирать несколько meaningful сценариев
- `[x]` Для ключевых модулей понятен контракт и статус проверки
- `[x]` Reuse ощущается как практическое преимущество

### Phase 3

- `[x]` Новый пользователь понимает идею проекта по README и примерам
- `[x]` Есть 2-3 сильных demo-потока
- `[ ]` DeltaQ можно показать как целостный open-source инструмент

## Delivery And Release

### Linux CI Baseline

- `[x]` В репозитории есть checked-in GitHub Actions workflow для Linux
- `[x]` Workflow делает полный `configure -> build -> ctest`, а не частичный smoke-build
- `[x]` Full-suite тесты проходят в headless-режиме через `QT_QPA_PLATFORM=offscreen`

### Packaging Baseline

- `[x]` Writable state вынесен из bundled app tree в пользовательский root `~/.deltaq`
- `[x]` Есть install-layout, совместимый с текущим runtime-ожиданием `applicationDirPath()`
- `[x]` Release bundle включает не только `deltaq` и `modules`, но и `templates` с `examples`
- `[x]` CI выполняет install/package smoke для self-contained Linux bundle
- `[x]` CI выполняет AppDir smoke на каждом Linux workflow run
- `[x]` Bundle включает translation payload и проходит единый release-layout verification script
- `[x]` CI публикует Linux package artifact и `SHA256SUMS` для каждого workflow run
- `[x]` Есть tag-based release workflow, публикующий Linux tarball и checksum как GitHub Release assets
- `[x]` Есть публичный packaged artifact beyond Linux tarball: AppImage для tagged releases
- `[x]` AppImage реально собирается и проходит локальную офлайн-safe verification, а не только infrastructure scaffolding
- `[ ]` Есть полноценная cross-platform delivery story для Windows и macOS
