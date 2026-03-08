# mini_sensor_sdk Imported Pack Walkthrough

## Purpose

Этот walkthrough объясняет первый полный сценарий imported pack в DeltaQ:

`external library -> raw import -> curation -> graph -> build -> run`

Он опирается на controlled fixture library `mini_sensor_sdk`, чтобы весь путь был
детерминированным и повторяемым.

## Source Assets

Исходная fixture library:

- [mini_sensor_sdk.h](/home/vladimir/Prog/IDE_DeltaQ/resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk/include/mini_sensor_sdk.h)
- [mini_sensor_sdk.c](/home/vladimir/Prog/IDE_DeltaQ/resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk/src/mini_sensor_sdk.c)

Checked-in showcase project:

- [imported_pack_sensor_console](/home/vladimir/Prog/IDE_DeltaQ/resources/examples/imported_pack_sensor_console)

## What Raw Import Produces

Raw import для `mini_sensor_sdk` даёт wrapper-модули:

- `mini_sensor_init`
- `mini_sensor_read`
- `mini_sensor_scale`
- `mini_sensor_last_error`
- `mini_sensor_shutdown`

Это низкоуровневый слой, близкий к внешнему API. Он полезен для аудита и точечной
обёртки, но не должен быть главным пользовательским интерфейсом библиотеки.

## What Curation Changes

В `v1` curation flow выполняются такие действия:

1. raw wrapper-ы сохраняются как imported-модули с pack metadata;
2. для них задаются `display name` и `curation role`;
3. лишние низкоуровневые wrapper-ы получают роль `hidden`;
4. поверх raw API добавляются более полезные curated entry / adapter модули;
5. граф использует уже curated-слой, а не raw wrappers напрямую.

Для `mini_sensor_sdk` в checked-in showcase используются:

- hidden raw wrappers:
  - `mini_sensor_init`
  - `mini_sensor_read`
  - `mini_sensor_scale`
  - `mini_sensor_last_error`
  - `mini_sensor_shutdown`
- visible curated modules:
  - `Sensor Scale Report`
  - `Sensor Error Report [adapter]`

## Showcase Project Structure

В example-проекте [imported_pack_sensor_console](/home/vladimir/Prog/IDE_DeltaQ/resources/examples/imported_pack_sensor_console) лежат:

- `.dqproj` проекта;
- `dqmods/` с curated imported-модулями;
- `graphs/main.dqgraph`;
- `vendor/mini_sensor_sdk` как локальная копия fixture SDK.

Это сделано специально, чтобы проект был self-contained:

- его можно открыть как обычный DeltaQ project;
- `pre-build` генерирует `src/main.c` из графа;
- `vendor/mini_sensor_sdk/src/mini_sensor_sdk.c` компилируется как часть проекта;
- include path к `mini_sensor_sdk.h` приходит из metadata imported pack-а.

## Graph-Level Scenario

Корневой граф проекта:

1. вызывает `Sensor Scale Report` с `device_name = "sensor-A"` и `factor = 3`;
2. затем по `execution`-цепочке вызывает `Sensor Error Report` с тем же
   `device_name` и `factor = 0`.

Первый модуль демонстрирует успешный путь SDK, второй показывает failure path и
работу `LAST_ERROR`.

## Expected Runtime Output

Для checked-in example ожидается такой вывод:

```text
READ=48
SCALED=144
SHUTDOWN=done
FAILED_SCALE=-1
LAST_ERROR=scale factor must be positive
SHUTDOWN=done
```

## Verification

Этот сценарий закреплён regression test-ом:

- [test_PreBuildProcessor.cpp](/home/vladimir/Prog/IDE_DeltaQ/tests/codegen/test_PreBuildProcessor.cpp)

Ключевые проверки:

- `pre-build` успешно генерирует `src/main.c`;
- `CMakeGenerator` подтягивает include path imported pack-а;
- проект собирается без ручной доводки;
- runtime output совпадает с ожидаемым.

## Why This Example Matters

Этот example показывает три принципа DeltaQ:

- разнообразие модулей приходит не через раздувание `core`, а через imported pack-ы;
- raw import сам по себе недостаточен, ценность появляется после curation;
- итоговый проект всё равно остаётся прозрачным: граф, `.dqmod`, generated `main.c`
  и обычная сборка видны пользователю.
