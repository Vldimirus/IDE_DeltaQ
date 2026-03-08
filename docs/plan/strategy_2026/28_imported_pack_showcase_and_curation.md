# Imported Pack Showcase And Curation

## Purpose

Зафиксировать **детальный исполнимый план** следующего подэтапа `Phase 2 / Import And Wrapping`:

- как выбрать первый reference case;
- как довести imported pack от raw wrapper до curated pack;
- как оформить это не только как внутреннюю технику, но и как понятный showcase;
- в каком порядке это реализовывать, чтобы не расползтись по scope.

## Why It Matters

Сейчас у DeltaQ уже есть:

- стратегическая модель `library -> raw wrappers -> curated pack -> graph`;
- minimum viable converter для `C`-библиотек;
- build integration для `include_paths / defines / link_libraries`;
- внутренний end-to-end test, что imported pack metadata реально доходит до `CMake` и runtime.

Но пока ещё нет самого важного пользовательского доказательства:

- **какой именно pack считается хорошим**;
- **что такое curation в IDE**;
- **как пользователь проходит путь от внешней библиотеки до понятного reusable набора модулей**;
- **какой example использовать как reference/onboarding**, а не только как внутренний тест.

## Current State

На сегодня:

- импорт библиотек уже умеет сохранять extension pack;
- pack metadata уже участвует в сборке проекта;
- imported pack ещё не оформлен как законченный пользовательский сценарий;
- curation пока есть только как идея и частично как ручная работа через существующие редакторы;
- нет отдельного showcase project, который бы объяснял import/wrapping внешнему пользователю.

## Target State

Нужен один **контролируемый и демонстрационный** сценарий:

1. есть небольшая внешняя `C`-библиотека-фикстура внутри репозитория;
2. её можно импортировать в DeltaQ как raw wrapper pack;
3. поверх raw pack выполняется curation;
4. curated pack используется в графе реального примера;
5. пример проходит `import -> curation -> graph -> codegen -> build -> run`;
6. пользователь может понять этот путь по example и документации без чтения внутреннего кода IDE.

## Decisions

### 1. Первый reference case должен быть контролируемой fixture-библиотекой

Первый showcase нельзя строить на реальном vendor SDK, скачиваемом извне.

Причины:

- нестабильная среда;
- платформенные различия;
- внешние лицензии;
- риск уткнуться не в DeltaQ, а в чужую сборку.

Поэтому первый reference case должен использовать **небольшую локальную C-библиотеку в репозитории**.

### 2. Первый case должен быть ближе к SDK, чем к чистой математике

Пример только с `cos()` уже хорош как технический тест, но слаб как showcase.

Первый демонстрационный case должен показывать не только “вызвали одну функцию”, а хотя бы такой набор:

- `init`;
- `read/process`;
- `shutdown`;
- один-два параметризованных вызова;
- хотя бы один handle/context тип.

Это лучше выражает идею “библиотека/SDK превращается в pack модулей”.

### 3. Showcase нужно делать в двух слоях

#### Layer A. Internal technical reference

Контролируемая фикстура + deterministic build/run.

Назначение:

- regression;
- быстрый smoke;
- отладка converter/cmake/runtime.

#### Layer B. User-facing showcase

Отдельный example + docs + walkthrough.

Назначение:

- onboarding;
- демонстрация ценности pack curation;
- материал для README.

### 4. Curation должна быть явной стадией, а не “ручным магическим доведением”

Для первого цикла curation должна быть описана как набор конкретных действий:

- переименование display names;
- перенос в категории;
- скрытие лишних raw wrappers;
- добавление doc strings;
- настройка ролей модулей;
- создание 1-2 adapter-модулей поверх сырого API.

Если это не формализовать, imported pack так и останется технической заготовкой.

## Reference Case

### Working candidate

Рекомендуемый первый case:

- `mini_sensor_sdk`

Состав фикстуры:

- `sensor_context * sensor_init(const char *device_name);`
- `int sensor_read(sensor_context *ctx);`
- `int sensor_scale(sensor_context *ctx, int factor);`
- `void sensor_shutdown(sensor_context *ctx);`
- при желании:
  - `const char * sensor_last_error(sensor_context *ctx);`

Почему это хороший case:

- есть `init/read/shutdown`;
- есть pointer/handle;
- есть обычные входы/выходы;
- можно сделать deterministic runtime;
- это уже похоже на библиотеку оборудования, но без настоящего оборудования.

### Deliverables of the case

- fixture library в репозитории;
- imported raw pack;
- curated pack;
- example project, использующий curated pack;
- walkthrough document.

## Implementation Directions

### Stage 1. Add Fixture Library

Нужно добавить в репозиторий небольшую `C`-библиотеку-фикстуру.

Требования:

- собирается локально без внешних зависимостей;
- имеет `.h` + `.c`;
- deterministic output;
- подходит и для unit/integration tests, и для showcase.

Рекомендуемое размещение:

- `resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk`

Минимальный состав:

- `include/mini_sensor_sdk.h`
- `src/mini_sensor_sdk.c`
- `README.md`

### Stage 2. Define Raw Import Baseline

Нужно зафиксировать, что считается успешным raw import для этой библиотеки.

Обязательные результаты:

- generated `pack.json`;
- `.dqmod` для raw wrappers;
- корректные DeltaQ-типы в портах;
- корректная metadata линковки и include dirs;
- стабильные module ids.

Критерий закрытия stage:

- raw import можно повторить детерминированно;
- результат не зависит от ручной доводки в файлах после импорта.

Текущий статус:

- для `mini_sensor_sdk` baseline уже зафиксирован regression-тестом
  `test_ImportedPackRawBaseline.cpp`.

### Stage 3. Formalize Curation Flow

Нужно определить, что именно делает пользователь после raw import.

Обязательные curation-операции `v1`:

1. назначить pack title и описание;
2. выровнять display names модулей;
3. разложить модули по категориям;
4. скрыть низкоуровневые/лишние wrappers;
5. добавить doc strings;
6. при необходимости создать 1-2 adapter-модуля поверх raw функций.

Для первого цикла допускается, что часть действий выполняется через существующие `Module Manager` / редактор `.dqmod`, но это должно быть описано как официальный flow.

Текущий статус:

- `Module Manager` уже показывает и редактирует:
  - `display name`
  - `curation role`;
- imported-модуль сохраняется обратно в исходный `.dqmod` файла pack-а;
- `hidden` imported wrapper-ы скрываются из `Module Palette`, но остаются доступны в `Module Manager`;
- curation flow для `mini_sensor_sdk` описан в
  `resources/examples/imported_pack_sensor_sdk/README.md`.

### Stage 4. Add Verification For Imported Pack

Нужно ввести минимальную верификацию именно для curated pack-а.

Обязательные проверки:

- проект с curated pack успешно собирается;
- runtime output совпадает с ожидаемым;
- init/read/shutdown path проходит полностью;
- ошибка link/include локализуется в понятном месте;
- used imported pack requirements реально попадают в `CMakeLists.txt`.

Текущий статус:

- есть automated integration test
  `curatedImportedPackBuildsAndRunsMiniSensorFlow()` в
  `tests/codegen/test_PreBuildProcessor.cpp`;
- тест строит fixture SDK как внешнюю библиотеку, создаёт verified curated pack,
  затем прогоняет:
  - `graph -> pre-build -> CMake -> build -> run`;
- runtime подтверждает:
  - `READ=48`
  - `SCALED=144`
  - `FAILED_SCALE=-1`
  - `LAST_ERROR=scale factor must be positive`
  - `SHUTDOWN=done`;
- imported pack requirements реально проверяются в generated `CMakeLists.txt`
  по `pack_name`, `include_paths` и `link_libraries`.

### Stage 5. Build Showcase Project

Нужно создать отдельный example project, который использует curated pack.

Рекомендуемое размещение:

- `resources/examples/imported_pack_sensor_console`

Пример должен:

- открываться как обычный проект DeltaQ;
- использовать imported/curated pack, а не вручную написанный ad-hoc код;
- проходить `pre-build -> build -> run`;
- иметь понятный runtime output.

Текущий статус:

- added checked-in showcase project:
  - `resources/examples/imported_pack_sensor_console`;
- проект self-contained:
  - `.dqproj`
  - `dqmods/` c curated imported pack-ом
  - `graphs/main.dqgraph`
  - `vendor/mini_sensor_sdk`;
- automated regression:
  - `importedPackSensorConsoleExampleBuildsAndRunsEndToEnd()` в
    `tests/codegen/test_PreBuildProcessor.cpp`.

### Stage 6. Write Walkthrough Documentation

Нужно оформить короткий walkthrough:

1. что за библиотека;
2. как она импортируется;
3. что такое raw pack;
4. что было сделано на этапе curation;
5. как pack используется в графе;
6. что получилось на выходе.

Рекомендуемое размещение:

- `docs/library/imported_packs/mini_sensor_sdk.md`

Текущий статус:

- walkthrough added:
  - `docs/library/imported_packs/mini_sensor_sdk.md`;
- он связывает между собой:
  - fixture library
  - raw import
  - curation
  - checked-in showcase project
  - expected runtime output.

## Recommended Execution Order

Работать именно в таком порядке:

1. fixture library;
2. raw import baseline;
3. curation flow definition;
4. verification hooks;
5. showcase project;
6. walkthrough docs.

Не наоборот.

Сначала нужен контролируемый кейс и детерминированный pipeline, и только потом удобство/подача.

## Concrete Tasks

### Task Group 1. Fixture

- добавить `mini_sensor_sdk` в репозиторий;
- сделать локальную сборку и usage без внешних пакетов;
- зафиксировать ожидаемое runtime поведение.

### Task Group 2. Raw Import

- импортировать fixture в extension pack;
- проверить generated metadata;
- проверить stable ids;
- проверить `CMake` integration.

### Task Group 3. Curation

- определить финальное имя pack-а;
- назначить категории;
- пометить raw wrappers и curated wrappers;
- добавить описание и ограничения;
- при необходимости сделать adapter-модуль `sensor_read_scaled` или аналог.

### Task Group 4. Showcase

- собрать граф примера на curated pack;
- добавить example project;
- добавить walkthrough;
- привязать example к regression.

## Acceptance Criteria

- В репозитории есть контролируемая fixture library для первого imported-pack showcase.
- Есть reproducible raw import baseline для этой библиотеки.
- Есть формальный curation flow, а не неявная ручная доводка.
- Есть curated pack, пригодный для реального использования в графе.
- Есть example project `external library -> curated pack -> graph -> build -> run`.
- Есть user-facing walkthrough, объясняющий этот путь.

## Deferred / Not In This Stage

- Полноценный импорт сложных `C++` API.
- Автоматическое покрытие callbacks, templates и всех ABI-случаев.
- Marketplace/registry imported pack-ов.
- Несколько разных vendor SDK одновременно.
- Сразу native-quality UX для всех этапов curation.
