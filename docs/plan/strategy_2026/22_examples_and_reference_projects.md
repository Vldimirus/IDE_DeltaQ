# Examples And Reference Projects

## Context

DeltaQ нельзя убедительно объяснить только архитектурой. Нужны примеры, которые показывают, почему модульная композиция реально ускоряет разработку.

## Principles

- Пример должен демонстрировать не количество экранов, а продуктовую идею.
- Эталонный проект должен быть достаточно маленьким, чтобы его можно было понять целиком.
- Примеры должны быть полезны одновременно как onboarding, regression reference и showcase.

## Concrete Decisions

### Required examples

#### 1. Minimal console flow

Цель:

- показать базовый путь `module -> graph -> code -> build -> run`.

#### 2. Desktop/UI flow

Цель:

- показать связку модулей, графа, generated UI/code и runtime.

#### 3. Reusable composition example

Цель:

- показать подмодули, повторное использование и ценность библиотеки модулей.
- показать, что подмодуль генерируется как отдельные `.h/.c`, а не как скрытая inline-развёртка.

Текущий эталонный проект:

- `resources/examples/reusable_composition_console`
- показывает два составных модуля:
  - `echo_with_prefix`
  - `measure_text`
- показывает повторное использование `echo_with_prefix` в двух местах корневого графа;
- проходит полный путь `pre-build -> build -> run` и даёт детерминированный runtime output.

#### 4. Imported pack showcase

Цель:

- показать путь `external library -> raw import -> curation -> graph -> build -> run`;
- показать, что разнообразие DeltaQ растёт через `external module packs`, а не через раздувание `core`;
- показать pack как reusable библиотеку модулей, а не как набор случайных wrapper-ов.

Рекомендуемый первый кейс:

- `mini_sensor_sdk`

Подробный исполнимый план для этого примера вынесен в:

- `28_imported_pack_showcase_and_curation.md`

Текущий checked-in showcase:

- `resources/examples/imported_pack_sensor_console`
- использует curated imported pack `mini_sensor_sdk_curated`;
- содержит локальный `vendor/mini_sensor_sdk`, curated `.dqmod` и корневой граф;
- проходит `pre-build -> build -> run` как self-contained example;
- сопровождается walkthrough:
  - `docs/library/imported_packs/mini_sensor_sdk.md`

### Example requirements

Каждый эталонный проект должен:

- иметь понятную задачу;
- использовать стандартные модули, а не случайные ad-hoc блоки;
- быть пригодным для демонстрации;
- быть достаточно стабильным для регрессии.

### Usage of examples

Примеры используются как:

- обучающие сценарии;
- smoke/regression ориентиры;
- материал для README и внешней презентации;
- способ проверять, что стратегия проекта остаётся практичной.

## Success Criteria

- Есть минимум 3 примера, через которые можно показать смысл DeltaQ.
- Есть отдельный imported pack showcase, объясняющий идею converter/wrapping.
- Примеры покрывают основной workflow и reuse.
- Новый пользователь понимает не только "что умеет IDE", но и "зачем её подход полезен".
