# Submodule Compilation Units

## Purpose

Зафиксировать модель, в которой составной модуль (`submodule`, `graph`-module, "матрёшка") становится не inline-развёрткой внутри родительского `main.c`, а **отдельной единицей генерации и компиляции**.

## Why It Matters

Если подмодуль остаётся только внутренней рекурсией внутри `GraphCompiler`, то:

- повторное использование выглядит неполным;
- граница между "обычным модулем" и "составным модулем" остаётся нестрогой;
- generated code плохо читается;
- сборка не отражает продуктовую идею "собираем систему из модулей".

Для DeltaQ подмодуль должен вести себя как настоящий модуль:

- иметь свой `.dqgraph` как source-of-truth структуры;
- иметь свой `.dqmod` как source-of-truth контракта;
- получать собственные generated sources;
- подключаться в родительский граф как обычный модуль через контракт.

## Current State

Сейчас в проекте уже есть:

- создание составного модуля из выделения через `SubModuleFactory`;
- хранение `graphId` в `.dqmod`;
- навигация внутрь подмодуля;
- базовая защита от циклов;
- первичная рекурсивная обработка в `GraphCompiler`.

Но этого недостаточно, потому что:

- подмодуль ещё не становится отдельным generated source unit;
- нет явной модели `submodule.h/.c`;
- родительский граф не использует подмодуль как обычный compile-time dependency;
- boundary-модель входов и выходов подмодуля не доведена до строгой реализации.

## Target State

После реализации этой стратегии подмодуль работает так:

1. Пользователь выделяет группу узлов и создаёт субмодуль.
2. IDE сохраняет:
   - отдельный `.dqgraph` с внутренней структурой;
   - отдельный `.dqmod` с внешним контрактом.
3. Pre-build генерирует для субмодуля:
   - отдельный `.h`;
   - отдельный `.c`.
4. Родительский generated source:
   - подключает заголовок субмодуля;
   - вызывает функцию субмодуля как обычный модуль.
5. CMake добавляет generated `submodule.c` в сборку проекта.

Концептуально это "подключение готового блока", близкое к `include`.
Технически правильная реализация должна быть не `#include submodule.c`, а:

- `#include "generated/submodules/<name>.h"`
- компиляция `generated/submodules/<name>.c` как отдельного translation unit

## Decisions

### 1. Source of truth for a submodule

Для составного модуля источники истины делятся так:

- `.dqgraph` — внутренняя структура и связи;
- `.dqmod` — внешний контракт;
- generated `.h/.c` — только артефакты pre-build.

Generated source не должен становиться редактируемым первоисточником.

### 2. Output shape

Для каждого составного модуля нужно генерировать минимум:

- `build/generated/submodules/<module_name>.h`
- `build/generated/submodules/<module_name>.c`

Header должен содержать:

- include guard;
- необходимые forward includes;
- сигнатуру функции подмодуля;
- при необходимости служебные типы для нескольких выходов.

Source должен содержать:

- includes;
- реализацию функции подмодуля;
- generated banner с origin-графом и предупреждением о перезаписи.

### 3. Parent graph integration

Родительский generated file не должен разворачивать тело подмодуля внутрь себя.

Он должен:

- подключать header подмодуля;
- вызывать подмодуль как обычную функцию/процедуру;
- опираться только на контракт, а не на внутреннее устройство подграфа.

### 4. Function boundary of a submodule

Подмодуль должен иметь явную C-границу.

Для `v1` принять такой минимальный контракт:

- data-inputs становятся параметрами функции;
- если data-output один, он может быть `return`;
- если data-outputs несколько, нужен generated result-struct или `out`-параметры;
- execution flow не хранится как отдельный runtime object:
  подмодуль вызывается как обычный участок последовательного кода.

Это означает:

- exec-порты управляют порядком в графе;
- на границе generated C-функции субмодуля важны прежде всего данные и факт вызова.

### 5. File naming and stability

Имена generated submodule files должны быть:

- детерминированными;
- читаемыми;
- устойчивыми при повторной сборке.

Предпочтение:

- использовать нормализованное имя модуля;
- при коллизиях добавлять устойчивый суффикс на основе `module.id`.

### 6. Build system ownership

Не `GraphCompiler`, а `PreBuildProcessor + BuildPipeline + CMakeGenerator` должны владеть списком generated submodule sources.

Это нужно, чтобы:

- generated files были видны пользователю;
- сборка была прозрачной;
- `Build Output` и project tree могли трассировать origin.

## Implementation Directions

### Workstream 1. Boundary model for submodules

Нужно расширить модель составного модуля так, чтобы было ясно:

- какие узлы внутри графа соответствуют внешним входам;
- какие узлы/порты формируют внешние выходы;
- как эти boundary points сериализуются.

Рекомендуемое решение для `v1`:

- при создании субмодуля `SubModuleFactory` не только вычисляет `inputs/outputs`,
  но и сохраняет в `GraphNode.properties` или в отдельной metadata-структуре
  соответствие:
  - `external input -> internal target node/port`
  - `external output <- internal source node/port`

Без этой карты generated function не сможет надёжно собрать boundary-код.

### Workstream 2. Generated header/source for a submodule

Нужно ввести новый генератор, например:

- `SubmoduleCodeGenerator`

или новый подрежим внутри `GraphCompiler`, который умеет генерировать:

- не только `main.c`,
- но и `module function` для составного модуля.

Минимальные обязанности генератора:

- build C signature;
- build local variable declarations;
- сгенерировать тело функции из внутреннего графа;
- вернуть один или несколько выходов в согласованной форме;
- сформировать `header + source + metadata about origin`.

### Workstream 3. Pre-build artifact model

`PreBuildProcessor` должен начать возвращать отдельный тип артефактов для субмодулей:

- `GeneratedSubmoduleHeader`
- `GeneratedSubmoduleSource`

Для каждого такого артефакта нужно хранить:

- путь;
- role;
- source graph;
- owning module;
- родительский проект.

### Workstream 4. CMake integration

`CMakeGenerator` должен автоматически добавлять generated submodule sources в проектную сборку.

Минимально:

- включать generated submodule `.c` в target sources;
- добавлять include path до папки generated headers.

Цель:

- субмодуль становится обычной единицей компиляции;
- родительский `main.c` знает только про header.

### Workstream 5. Parent codegen integration

`GraphCompiler`, компилирующий родительский граф, должен:

- распознавать составной модуль как обычный callable module;
- не разворачивать его внутренний код inline;
- использовать generated function name из submodule contract metadata.

### Workstream 6. IDE visibility

IDE должна ясно показывать:

- что у составного модуля есть собственные generated `.h/.c`;
- что эти файлы происходят из конкретного `.dqgraph`;
- что ошибки внутри submodule source ведут обратно в submodule graph.

Это продолжает уже начатую линию `generated -> source-of-truth`.

## Phase Plan

### Stage 1. Metadata and boundary mapping

Сделать структуру данных, описывающую границу субмодуля.

Нужно реализовать:

- стабильную сериализацию boundary metadata;
- сохранение этой информации при создании субмодуля;
- валидацию, что boundary metadata не рассинхронизирована с внутренним графом.

Критерий готовности:

- у каждого нового составного модуля есть не только `graphId`, но и машиночитаемая boundary-модель.

### Stage 2. Submodule code generation

Сгенерировать для одного составного модуля отдельные `.h/.c`.

Нужно реализовать:

- header emitter;
- source emitter;
- banner и origin metadata;
- naming strategy.

Критерий готовности:

- один простой submodule graph даёт корректную пару generated файлов.

### Stage 3. Parent integration

Научить родительский граф использовать generated submodule function.

Нужно реализовать:

- include header в родительский source;
- вызов generated function;
- согласование типов входов/выходов.

Критерий готовности:

- родительский `main.c` компилируется, не зная внутренностей submodule graph.

### Stage 4. Build integration

Подключить generated submodule files к сборке проекта.

Нужно реализовать:

- регистрацию артефактов в `PreBuildProcessor`;
- обновление `CMakeLists.txt`;
- диагностику в `Build Output`.

Критерий готовности:

- субмодуль реально собирается как отдельная compile unit.

### Stage 5. End-to-end reusable composition example

Нужен эталонный пример, где:

- подмодуль создан из выделения;
- используется повторно минимум в двух местах;
- генерирует отдельные `.h/.c`;
- проходит `pre-build -> build -> run`.

Критерий готовности:

- reusable composition перестаёт быть только UI-функцией и становится доказанным рабочим сценарием.

## Acceptance Criteria

- Создание субмодуля приводит к появлению отдельного `.dqgraph` и `.dqmod`.
- Pre-build создаёт для субмодуля отдельные generated `.h/.c`.
- Родительский generated source использует только header и вызов функции субмодуля.
- `CMakeLists.txt` включает submodule `.c` в сборку.
- Ошибка компиляции в generated submodule source трассируется обратно к исходному submodule graph.
- Есть хотя бы один стабильный reference example с повторным использованием субмодуля.

## Deferred / Not In This Phase

- Полноценная ABI-модель для мультиязычных составных модулей.
- Автоматическое инкрементальное кэширование generated submodule artifacts.
- Сложные варианты async/runtime orchestration на границе субмодуля.
- Автоматический экспорт одного и того же субмодуля как shared/static library вне проекта.
