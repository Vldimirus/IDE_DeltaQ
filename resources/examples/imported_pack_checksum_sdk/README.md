# Imported Pack Checksum SDK Fixture

Этот каталог нужен как второй controlled fixture-case для `Import And Wrapping`.

Сейчас он содержит:

- controlled fixture library:
  - `vendor/mini_checksum_sdk`
- documented raw import baseline для этой библиотеки

## Цель

На этом fixture-case DeltaQ должен пройти путь:

`external library -> raw import -> curation -> graph -> build -> run`

В отличие от `mini_sensor_sdk`, этот кейс показывает не device/runtime SDK, а
алгоритмический text-processing pack.

## Fixture Library

Используемая библиотека:

- `vendor/mini_checksum_sdk`

Это маленький детерминированный `C`-SDK с API:

- `mini_checksum_init(int seed)`
- `mini_checksum_push_text(mini_checksum_context *ctx, const char *text)`
- `mini_checksum_hex(mini_checksum_context *ctx)`
- `mini_checksum_match(mini_checksum_context *ctx, const char *expected_hex)`
- `mini_checksum_shutdown(mini_checksum_context *ctx)`

## Raw Import Baseline

Канонический raw pack для этого SDK должен иметь:

- `packName`: `mini_checksum_sdk_raw`
- `displayName`: `Mini Checksum SDK Raw`
- `category`: `checksum_raw`
- `language`: `c`
- `standard`: `c17`

### Expected raw modules

#### `mini_checksum_init`

- inputs:
  - `seed : int`
- outputs:
  - `result : pointer`

#### `mini_checksum_push_text`

- inputs:
  - `ctx : pointer`
  - `text : string`
- outputs:
  - `result : int`

#### `mini_checksum_hex`

- inputs:
  - `ctx : pointer`
- outputs:
  - `result : string`

#### `mini_checksum_match`

- inputs:
  - `ctx : pointer`
  - `expected_hex : string`
- outputs:
  - `result : int`

#### `mini_checksum_shutdown`

- inputs:
  - `ctx : pointer`
- outputs:
  - нет

## V1 Curation Flow

Для `mini_checksum_sdk` curated layer должен скрывать raw API и поднимать вверх
два user-facing entry-модуля:

- `Checksum Report`
- `Checksum Match Report [adapter]`

Логика такая же, как у `mini_sensor_sdk`:

1. raw wrapper-ы сохраняются как imported-модули с pack metadata;
2. лишние низкоуровневые wrapper-ы получают роль `hidden`;
3. поверх raw API появляются curated entry / adapter-модули;
4. граф использует уже curated-слой, а не raw wrappers напрямую.

## Что этот кейс добавляет

`mini_checksum_sdk` нужен не просто как ещё один пример.

Он показывает, что imported pack-и в DeltaQ:

- не ограничиваются hardware-like SDK;
- подходят и для algorithmic / text-processing расширений;
- действительно являются главным каналом роста экосистемы вне `core`.
