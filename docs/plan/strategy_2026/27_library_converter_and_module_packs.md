# Library Converter And Module Packs

## Purpose

Зафиксировать стратегию, по которой DeltaQ расширяет экосистему модулей не за счёт бесконтрольного разрастания `core`, а за счёт **преобразования внешних библиотек в модульные pack-ы**.

## Why It Matters

Для DeltaQ разнообразие модулей действительно важно, но оно не должно разрушать качество standard library.

Если пытаться решать это только расширением `core`, проект быстро получит:

- перегруженную библиотеку;
- смешение baseline-модулей и узкоспециализированных обёрток;
- трудную discoverability;
- потерю product focus.

Правильный путь для DeltaQ:

- `core` остаётся небольшим curated baseline;
- внешние библиотеки становятся источником новых возможностей;
- входом в граф всегда остаются **модули**, а не сырые `.h/.so/.dll/.lib`.

## Current State

В проекте уже есть:

- идея импортировать библиотеки и строить модульные обёртки;
- `core` как стандартная библиотека;
- локальные и составные модули проекта;
- codegen/build pipeline, в который модульные pack-ы могут быть встроены.

Но пока ещё не зафиксирована продуктовая модель:

- чем `core` отличается от imported pack;
- как выглядит pipeline `library -> modules`;
- какие артефакты создаёт converter;
- как отличать сырую автоматическую обёртку от curated pack.

## Target State

DeltaQ должен поддерживать три слоя модульной экосистемы:

### 1. Core

Небольшой, очень качественный baseline.

Содержит:

- общие data/control модули;
- базовые conversion/string/io модули;
- обязательные runtime-модули главных эталонных сценариев.

Не должен разрастаться за счёт библиотек устройств, SDK и domain-specific API.

### 2. External Module Packs

Модульные pack-ы, полученные из внешних библиотек, SDK, драйверов и API.

Содержат:

- wrapper-модули для функций библиотеки;
- metadata о платформах, заголовках, бинарных зависимостях и типах;
- curated grouping по категориям и ролям;
- при необходимости hand-written adapter-модули поверх raw wrappers.

Это и есть главный источник разнообразия DeltaQ.

### 3. Project Modules

Локальные модули конкретного проекта:

- атомарные;
- составные;
- адаптерные;
- orchestration-модули, нужные только внутри одного проекта.

## Decisions

### Graph sees only modules

В граф напрямую не попадают:

- заголовки;
- `.dll/.so/.lib`;
- сырые сигнатуры внешней библиотеки;
- произвольные символы ABI.

Пользователь работает только с модулями и pack-ами модулей.

### Core is not the diversity bucket

`core` отвечает за baseline и product clarity.

Разнообразие функциональности должно приходить в основном через:

- imported library packs;
- curated extension packs;
- project-local adapters.

### Converter output is not automatically “good library”

Автоматически созданный wrapper-pack ещё не считается качественной библиотекой.

Нужны отдельные стадии:

1. import
2. wrapper generation
3. curation
4. verification
5. publication/use

### Platform specificity lives in packs, not in the graph model

Если библиотека имеет разные реализации под Linux и Windows, это не должно ломать пользовательскую модель графа.

Нужно стремиться к такой схеме:

- единый внешний контракт pack-а;
- разные platform bindings внутри pack metadata и build settings;
- при необходимости platform-specific variants на уровне pack-а, а не пользовательского graph vocabulary.

## Module Pack Model

Минимальная модель imported pack-а:

- `pack.json`
- набор `.dqmod`
- optional generated headers/source adapters
- metadata о:
  - include paths
  - binary paths
  - link flags
  - supported platforms
  - calling convention
  - type mappings
  - init/shutdown requirements

### Pack Levels

Следует различать хотя бы 3 уровня зрелости pack-а:

#### Raw Wrapper Pack

Почти прямое отображение внешней библиотеки в модули.

Нужен как быстрый технический старт, но ещё не является хорошим UX-слоем.

#### Curated Pack

Обёртки уже сгруппированы, переименованы и очищены от лишнего низкоуровневого шума.

Именно этот уровень нужен для регулярного практического использования.

#### Reference Pack

Curated pack плюс:

- документация;
- пример проекта;
- verify baseline;
- понятный onboarding.

Это лучший кандидат для демонстрации ценности DeltaQ.

## Converter Pipeline

### Stage 1. Library Intake

Пользователь указывает:

- заголовки;
- бинарные зависимости;
- include dirs;
- link settings;
- платформу или набор платформ;
- язык интерфейса (`C`, затем другие backend-ы по мере зрелости).

### Stage 2. Symbol Analysis

Converter извлекает:

- функции;
- структуры и opaque handles;
- enum/константы;
- callbacks;
- потенциальные init/shutdown точки.

### Stage 3. Wrapper Module Generation

Для выбранных символов создаются `.dqmod`:

- с контрактом портов;
- с типами DeltaQ;
- с metadata о внешней зависимости;
- с базовым `sourceCode/header/includes`.

### Stage 4. Type And Contract Mapping

Пользователь или wizard уточняет:

- какие C-типы становятся какими DeltaQ-типами;
- какие значения удобнее выражать как handles/context;
- какие функции лучше скрыть;
- какие функции должны стать более высокоуровневыми модулями.

### Stage 5. Pack Curation

На этом шаге появляются:

- категории;
- display names;
- doc strings;
- роли модулей;
- скрытие лишних low-level wrappers;
- adapter-модули поверх сырого API.

Именно здесь из raw wrapper получается нормальный pack.

### Stage 6. Verification

Pack должен проходить хотя бы минимальную проверку:

- pack собирается;
- link settings корректны;
- критические wrappers вызываются;
- ошибки подключения локализуются;
- platform metadata согласована с реальной сборкой.

### Stage 7. Use In Graph

После этого пользователь видит в IDE уже не библиотеку как набор файлов, а готовый pack модулей.

## Implementation Directions

### 1. Развести уровни модульной экосистемы

Нужно явно различать:

- `core`
- `external pack`
- `project module`

в metadata, UI и build pipeline.

### 2. Ввести metadata pack-уровня

Нужны pack-описания для:

- платформ;
- линковки;
- происхождения библиотеки;
- версии SDK;
- зрелости pack-а.

### 3. Описать minimum viable converter

Первый practical path:

- импорт `C`-заголовков;
- генерация raw wrappers;
- ручная curation через IDE;
- сохранение результата как отдельного pack-а.

### 4. Не пытаться сразу покрыть всю ABI-сложность

Для первого цикла не нужно брать всё:

- сложные callbacks;
- макросы как полноценный API;
- template-heavy C++;
- нестандартные calling conventions без доказанной необходимости.

### 5. Сделать один reference scenario

Нужен хотя бы один сильный пример:

- внешняя библиотека оборудования или SDK;
- импорт в DeltaQ;
- создание curated pack-а;
- использование pack-а в графе;
- успешная сборка и запуск.

## Acceptance Criteria

- В стратегии чётко зафиксировано, что разнообразие приходит в основном через module packs, а не через разрастание `core`.
- Есть формальная разница между `core`, `external pack` и `project module`.
- Понятен pipeline `library -> raw wrappers -> curated pack -> graph`.
- Есть минимальный design для platform metadata и build integration.
- Есть хотя бы один reference scenario для library converter.

## Current MVP Status

На текущем этапе в проекте уже есть первый practical slice для `C`-библиотек:

- `Library Import Wizard` сохраняет результат импорта как установленный extension pack;
- generated `.dqmod` получают стабильные ids и metadata imported pack-а;
- для простых `C`-функций создаётся wrapper `sourceCode`, так что импорт даёт не только контракт, но и атомарный модуль с реализацией;
- metadata о заголовках, defines и link libraries сохраняется в pack;
- `CMakeGenerator` учитывает `include_paths / defines / link_libraries` только от тех imported pack-ов, которые реально используются корневыми графами проекта;
- есть внутренний end-to-end сценарий `imported pack metadata -> graph -> generated CMake -> build -> run`.

Что ещё остаётся недоведённым:

- пользовательский showcase `external library -> curated pack -> project graph -> build/run` ещё не оформлен как отдельный example/онбординг;
- full pipeline через реальный UI-импорт и последующую curation ещё не закрыт как единый сценарий;
- `C++` wrappers по-прежнему находятся за пределами первого MVP-среза.

## Deferred / Not In This Phase

- Полноценный универсальный импорт любых C++ API без ограничений.
- Автоматическое идеальное mapping всех типов без ручной curation.
- Marketplace/registry pack-ов как отдельный крупный продукт.
- Полное снятие platform-specific различий без реальных reference cases.

Детальный исполнимый план следующего подэтапа вынесен в:

- `28_imported_pack_showcase_and_curation.md`
