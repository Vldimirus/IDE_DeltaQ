# mini_checksum_sdk Imported Pack Walkthrough

## Purpose

Этот walkthrough объясняет второй полный сценарий imported pack в DeltaQ:

`external library -> raw import -> curation -> graph -> build -> run`

В отличие от `mini_sensor_sdk`, этот кейс показывает не hardware-like SDK, а
algorithmic/text-processing pack.

## Source Assets

Исходная fixture library:

- [mini_checksum_sdk.h](/home/vladimir/Prog/IDE_DeltaQ/resources/examples/imported_pack_checksum_sdk/vendor/mini_checksum_sdk/include/mini_checksum_sdk.h)
- [mini_checksum_sdk.c](/home/vladimir/Prog/IDE_DeltaQ/resources/examples/imported_pack_checksum_sdk/vendor/mini_checksum_sdk/src/mini_checksum_sdk.c)

Checked-in showcase project:

- [imported_pack_checksum_console](/home/vladimir/Prog/IDE_DeltaQ/resources/examples/imported_pack_checksum_console)

## What Raw Import Produces

Raw import для `mini_checksum_sdk` даёт wrapper-модули:

- `mini_checksum_init`
- `mini_checksum_push_text`
- `mini_checksum_hex`
- `mini_checksum_match`
- `mini_checksum_shutdown`

Это низкоуровневый слой, близкий к внешнему API. Он нужен для аудита и curation,
но не должен быть основным пользовательским interface surface.

## What Curation Changes

Для `mini_checksum_sdk` curated layer делает следующее:

1. raw wrapper-ы сохраняются как imported-модули с pack metadata;
2. низкоуровневые wrapper-ы получают роль `hidden`;
3. поверх raw API добавляются user-facing модули:
   - `Checksum Report`
   - `Checksum Match Report`;
4. граф использует уже curated-слой, а не raw wrappers напрямую.

## Showcase Project Structure

В example-проекте [imported_pack_checksum_console](/home/vladimir/Prog/IDE_DeltaQ/resources/examples/imported_pack_checksum_console) лежат:

- `.dqproj` проекта;
- `dqmods/` с curated imported-модулями;
- `graphs/main.dqgraph`;
- `vendor/mini_checksum_sdk` как локальная копия fixture SDK.

Это делает проект self-contained:

- его можно открыть как обычный DeltaQ project;
- `pre-build` генерирует `src/main.c` из графа;
- `vendor/mini_checksum_sdk/src/mini_checksum_sdk.c` компилируется как часть проекта;
- include path к `mini_checksum_sdk.h` приходит из metadata imported pack-а.

## Graph-Level Scenario

Корневой граф проекта:

1. вызывает `Checksum Report` с `seed = 7` и `text = "DeltaQ"`;
2. затем по `execution`-цепочке вызывает `Checksum Match Report` с теми же данными и `expected_hex = "00000000"`.

Первый модуль показывает calculated checksum, второй — adapter-path для сравнения
с expected value.

## Expected Runtime Output

Для checked-in example ожидается такой вывод:

```text
TEXT=DeltaQ
CHECKSUM=2115045471
HEX=7E11085F
SHUTDOWN=done
EXPECTED=00000000
ACTUAL=7E11085F
MATCH=0
SHUTDOWN=done
```

## Verification

Этот сценарий закреплён regression test-ом:

- [test_PreBuildProcessor.cpp](/home/vladimir/Prog/IDE_DeltaQ/tests/codegen/test_PreBuildProcessor.cpp)

Ключевые проверки:

- `pre-build` успешно генерирует `src/main.c`;
- `CMakeGenerator` подтягивает include path imported pack-а;
- проект собирается без ручной доводки;
- runtime output совпадает с ожидаемым, включая deterministic checksum `7E11085F`.

## V1 Limits

Этот второй walkthrough разделяет те же честные границы `v1`:

- intake path считается Linux-first;
- supported baseline — `C ABI` header + include/link metadata;
- missing headers, missing binary path и попытка прямого `C++` ABI import должны останавливаться explicit diagnostic-ом до записи pack-а;
- сложные callbacks и non-Linux packaging cases остаются следующим слоем, а не claim-ом текущего adapter-а.

## Why This Example Matters

Этот example показывает:

- imported pack-и полезны не только для hardware-like SDK, но и для algorithmic расширений;
- разнообразие модулей приходит через внешние pack-и, а не через раздувание `core`;
- curated layer остаётся главным пользовательским интерфейсом внешней библиотеки.
