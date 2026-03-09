# Отчёт о состоянии проекта DeltaQ

Дата аудита: `2026-03-09`

## 1. Краткий вывод

На текущем срезе DeltaQ выглядит как **технически сильно продвинутый инженерный проект**, в котором уже собран основной workflow:

`модуль -> граф -> codegen -> pre-build/build -> запуск`

Ключевые редакторы, codegen, execution flow, подмодули, импорт библиотек, UI designer и build/debug infrastructure в репозитории **реально присутствуют и покрыты тестами**. По этой шкале проект находится на высокой стадии готовности.

При этом DeltaQ пока заметно слабее по второй шкале: **продуктовая и стратегическая зрелость**. Репозиторий ещё не доведён до состояния, где внешний пользователь за первые 10-15 минут получает ясную картину продукта, проходит onboarding без путаницы и видит завершённый showcase-набор.

Итоговая консервативная оценка:

- **Техническая реализованность:** `~88%`
- **Продуктовая/стратегическая готовность:** `~63%`

Если опираться только на `docs/PROGRESS.md`, проект выглядит почти завершённым. Если опираться на стратегические acceptance-критерии и текущее состояние документации/examples, до зрелого v1 ещё остаётся существенный объём работы.

## 2. Методика оценки

Оценка в этом отчёте строится по **двойной шкале**:

1. **Техническая реализованность**
   - есть ли подсистема в `src/`;
   - интегрирована ли она в основной workflow;
   - есть ли тесты и checked-in examples;
   - видна ли реальная инженерная завершённость, а не только каркас.

2. **Продуктовая/стратегическая готовность**
   - закрывает ли проект acceptance-критерии `strategy_2026`;
   - понятен ли он новому пользователю;
   - хватает ли примеров, документации и объяснимости;
   - согласованы ли README, progress-документы и реальное состояние repo.

Источники для оценки:

- `docs/PROGRESS.md`
- `docs/plan/09_roadmap.md`
- `docs/plan/strategy_2026/*`
- `README.md`, `README_RU.md`
- текущее дерево `src/`, `tests/`, `modules/core/`, `resources/examples/`

Важное замечание:

- `docs/PROGRESS.md` отражает в первую очередь **implementation progress** и даёт очень высокую формальную оценку (`~97%`).
- Этот отчёт использует более строгую audit-логику и потому намеренно даёт **ниже продуктовую оценку**, даже если кодовая база богата функциональностью.

## 3. Что реально реализовано

### Основной workflow

На уровне репозитория уже прослеживается полноценный сквозной путь:

- модульная модель (`.dqmod`) существует и используется как основной уровень композиции;
- графовая модель (`.dqgraph`) интегрирована с палитрой, редактором и codegen;
- pre-build pipeline генерирует исходники из графов и UI layout;
- build pipeline собирает проект через CMake;
- generated-файлы помечаются как generated и умеют вести обратно к source-of-truth;
- есть self-contained reference examples, проходящие end-to-end сценарии.

### Реализованные крупные подсистемы

- **Core infrastructure:** project/session/action/undo/registry/store services, templates, project tree, diagnostics.
- **Code editor:** базовый редактор, LSP client, completion, rename/format, diagnostics, find/replace, goto line, build error navigation.
- **Debugger:** GDB/MI integration, breakpoints, stepping, variables, call stack, graph highlighting.
- **Block editor:** node scene, typed ports, execution/data flow, palette, undo/redo, submodules, breadcrumbs, cycle detection.
- **Codegen/build pipeline:** IR, graph compiler, pre-build processor, compiler detection, two-phase build.
- **UI designer:** visual scene, widget hierarchy, property editing, layout engine, anchors, SDL2 code generation, preview.
- **Library processor:** libclang parser, decomposition, wrapper generation, import wizard, curated imported-pack path.
- **Module ecosystem:** checked-in core pack, audit matrix, staged verification statuses, local modules, imported packs.

### Что это означает practically

Проект уже нельзя считать “только набором планов” или “черновым прототипом”. В репозитории есть реальная инженерная система, пригодная для внутренней разработки, демонстрации и дальнейшей стабилизации.

Слабое место не в отсутствии подсистем как таковых, а в том, что **верхний продуктовый слой отстаёт от нижнего инженерного слоя**.

## 4. Оценка по подсистемам

| Подсистема | Тех. готовность | Продукт. готовность | Состояние |
| --- | ---: | ---: | --- |
| Core + project infrastructure | 95% | 80% | Сильный фундамент: project lifecycle, stores, templates, navigation, diagnostics, session flow реализованы и связаны между собой. |
| Code editor + LSP + debugger | 90% | 75% | Подсистема богата функциями, но продуктовая зрелость зависит от окружения (`clangd`, `gdb`, optional `QScintilla`) и ещё не доведена до polished UX. |
| Block editor + graph composition | 92% | 82% | Один из самых зрелых блоков проекта: execution flow, palette, undo/redo, submodules, graph debugging уже выглядят цельно. |
| Codegen + pre-build/build pipeline | 90% | 80% | Pipeline прозрачен и хорошо оформлен, generated origin хорошо прослеживается, но внешний пользовательский narrative вокруг него ещё недооформлен. |
| UI Designer + SDL2 backend | 82% | 62% | Визуальный дизайнер и backend есть, но в `src/uiDesigner/SDL2CodeGenerator.cpp` остаются явные `TODO`-ветки по rendering/fields/handlers. |
| Library processor / imported packs | 78% | 65% | База и showcase есть, но подсистема чувствительна к наличию `libclang` и пока сильнее как engineering capability, чем как polished user flow. |
| Standard library / module ecosystem | 84% | 68% | Библиотека уже существенная и curated, но до fully trusted ecosystem ещё не хватает более жёсткой quality discipline и stronger module storytelling. |
| Showcase / onboarding / public readiness | 45% | 35% | Это самая слабая зона: examples и narrative пока не соответствуют уровню глубины внутренних подсистем. |

### Общий баланс

Сильнее всего проект закрыт там, где нужен именно инженерный костяк:

- core workflow;
- graph/codegen/build;
- блочная композиция;
- внутренняя инфраструктура разработки.

Слабее всего проект закрыт там, где нужна уже не реализация функций, а **доказательство зрелости продукта**:

- внешний onboarding;
- showcase;
- согласованная документация;
- публичная подача и package/distribution story.

## 5. Критические незавершённые моменты

### Этап A — критично для целостности продукта

Статус: `частично закрыто, но ещё является главным блокером зрелого v1`

1. **Showcase и onboarding не доведены**
   - стратегия требует минимум `3` сильных эталонных примера;
   - в `resources/examples` сейчас только `2` checked-in `.dqproj` примера;
   - README пока не объясняет DeltaQ через короткий reproducible путь первого знакомства.

2. **Публичная документация отстаёт от репозитория**
   - README и README_RU содержат устаревшие количественные claims;
   - в README остаются TODO на screenshots;
   - внешний пользователь получает менее зрелую картину, чем реально есть в коде.

3. **Нет единого честного статуса готовности**
   - `docs/PROGRESS.md` фактически объявляет проект почти завершённым;
   - стратегические документы и acceptance-логика задают существенно более строгую планку;
   - без синхронизации этих документов легко получить ложное ощущение финальной готовности.

### Этап B — критично для инженерного качества v1

Статус: `реализовано частично, требует дожатия`

1. **Module verification и quality bar**
   - staged verification и статусность модулей уже есть;
   - но стратегически эта область ещё не выглядит окончательно закрытой как центральный механизм доверия к reusable ecosystem.

2. **UI codegen не полностью production-complete**
   - `src/uiDesigner/SDL2CodeGenerator.cpp` всё ещё содержит явные `TODO`-ветки;
   - это означает, что UI backend уже силён как система, но ещё не выглядит полностью дочищенным для уверенного v1.

3. **Есть зафиксированные known issues**
   - в `docs/PROGRESS.md` остаётся отмеченная проблема с zoom-fit в block editor для малого числа узлов;
   - это не ломает архитектуру, но мешает считать UX полностью стабилизированным.

### Этап C — важно, но не блокирует основной сценарий

Статус: `следующий горизонт`

- cross-platform maturity beyond Linux;
- packaging and distribution;
- CI/CD;
- plugin system;
- performance work for large graphs;
- дополнительные language backends.

Это важные направления, но они не должны идти раньше синхронизации документации, showcase и quality bar основного modular workflow.

## 6. Расхождения между документацией и фактическим состоянием

Это отдельная проблема проекта, а не просто косметика.

### `docs/PROGRESS.md` слишком оптимистичен как единый источник истины

Сейчас документ декларирует:

- почти полное завершение фаз;
- общий прогресс около `97%`.

Проблема в том, что это число хорошо описывает **объём уже написанной инженерной работы**, но плохо описывает **реальную продуктовую готовность** по критериям `strategy_2026`.

### README и README_RU устарели количественно

Сейчас в верхнеуровневой документации по-прежнему фигурируют:

- `32` test suites;
- `~26` core modules.

По фактическому состоянию репозитория на дату аудита:

- `48` test source-файлов `test_*.cpp`;
- `43` checked-in core-модуля в `modules/core`.

### В README остаются видимые признаки незавершённости

- TODO на скриншоты главного окна, block editor и UI designer;
- нет полноценного короткого onboarding path;
- нет упаковки narrative вокруг реальных showcase examples.

### Strategy и repo facts пока не сведены в единый public-facing слой

Стратегические документы уже хорошо формулируют, что DeltaQ должен быть:

- средой модульной композиции;
- системой reuse-проверенных модулей;
- инструментом с прозрачным generated code.

Но этот нарратив ещё не доведён до уровня README, examples и first-run experience.

## 7. Что делать дальше по приоритету

### Приоритет 1

**Синхронизировать документацию состояния проекта**

Обновить:

- `README.md`
- `README_RU.md`
- `docs/PROGRESS.md`

Цель:

- убрать противоречия в цифрах;
- отделить implementation progress от product readiness;
- привести верхнеуровневое описание к реальному состоянию repo.

### Приоритет 2

**Закрыть showcase/onboarding phase**

Нужно:

- довести набор reference examples минимум до `3` сильных сценариев;
- сделать короткий first-run path;
- показать reusable composition и generated transparency не только в коде, но и в документации.

### Приоритет 3

**Дожать module ecosystem как продуктовую ценность**

Нужно:

- укрепить quality bar для модулей;
- сделать verification story более однозначной;
- усилить documentation/discoverability для core pack и curated imported packs.

### Приоритет 4

**Закрыть оставшиеся TODO в UI/backend зоне**

Нужно:

- пройтись по `SDL2CodeGenerator`;
- убрать placeholder-ветки;
- зафиксировать, какие UI-сценарии поддерживаются полностью, а какие ещё нет.

### Приоритет 5

**Только после этого двигаться в delivery и external adoption**

Сюда относятся:

- packaging;
- CI/CD;
- broader platform support;
- public release discipline.

## Приложение: фактические индикаторы репозитория

### Тесты

- `48` test source-файлов `test_*.cpp`
- покрытие по направлениям:
  - `core`: `15`
  - `editor`: `10`
  - `uiDesigner`: `7`
  - `libProcessor`: `6`
  - `blockEditor`: `4`
  - `codegen`: `3`
  - `lsp`: `2`
  - `debug`: `1`

### Core library

- `43` checked-in core-модуля в `modules/core`
- распределение по категориям:
  - `control`: `5`
  - `conversion`: `4`
  - `desktop`: `11`
  - `io`: `7`
  - `logic`: `3`
  - `math`: `10`
  - `string`: `3`

### Examples

- `2` checked-in example projects с `.dqproj`:
  - `resources/examples/reusable_composition_console`
  - `resources/examples/imported_pack_sensor_console`

### Итог по состоянию

DeltaQ уже не находится в стадии “идеи” или “каркаса”. Это **сильная инженерная база**, у которой:

- уже есть реальный modular workflow;
- уже есть большой объём функциональности;
- уже есть внутренние признаки зрелой архитектуры.

Но до состояния **убедительного, честно завершённого v1 продукта** проекту всё ещё критически нужны:

- синхронизированная документация;
- полноценный showcase-набор;
- cleaned-up onboarding;
- более жёсткая связка между implementation progress и strategic acceptance.
