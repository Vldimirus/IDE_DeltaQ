# Imported Pack Sensor SDK Fixture

Этот каталог нужен для первого демонстрационного сценария `Import And Wrapping`.

Сейчас он содержит:

- controlled fixture library:
  - `vendor/mini_sensor_sdk`
- documented raw import baseline для этой библиотеки

## Цель

На этом fixture-case DeltaQ должен пройти путь:

`external library -> raw import -> curation -> graph -> build -> run`

Пока на текущем шаге зафиксирован именно **raw import baseline**.

## Fixture Library

Используемая библиотека:

- `vendor/mini_sensor_sdk`

Это маленький детерминированный `C`-SDK с API:

- `mini_sensor_init(const char *device_name)`
- `mini_sensor_read(mini_sensor_context *ctx)`
- `mini_sensor_scale(mini_sensor_context *ctx, int factor)`
- `mini_sensor_last_error(mini_sensor_context *ctx)`
- `mini_sensor_shutdown(mini_sensor_context *ctx)`

## Raw Import Baseline

Канонический raw pack для этого SDK должен иметь:

- `packName`: `mini_sensor_sdk_raw`
- `displayName`: `Mini Sensor SDK Raw`
- `category`: `sensor_raw`
- `language`: `c`
- `standard`: `c17`
- `link_libraries`: `mini_sensor_sdk`

### Expected raw modules

#### `mini_sensor_init`

- inputs:
  - `device_name : string`
- outputs:
  - `result : pointer`

#### `mini_sensor_read`

- inputs:
  - `ctx : pointer`
- outputs:
  - `result : int`

#### `mini_sensor_scale`

- inputs:
  - `ctx : pointer`
  - `factor : int`
- outputs:
  - `result : int`

#### `mini_sensor_last_error`

- inputs:
  - `ctx : pointer`
- outputs:
  - `result : string`

#### `mini_sensor_shutdown`

- inputs:
  - `ctx : pointer`
- outputs:
  - нет

## Verification

Этот baseline считается зафиксированным, если:

- `LibraryDecomposer` даёт именно такие raw wrappers;
- `LibraryPackager` создаёт стабильный `pack.json` и `.dqmod`;
- повторный прогон даёт тот же результат без ручной правки файлов.

Автоматическая проверка:

- `tests/libProcessor/test_ImportedPackRawBaseline.cpp`

## V1 Curation Flow

После raw import для `mini_sensor_sdk` официальный `v1` flow выглядит так:

1. открыть imported-модули пакета в `Module Manager`;
2. выровнять `display name` через блок `Import/Curation`;
3. назначить `curation role`:
   - `raw_wrapper` для сырого API;
   - `curated_entry` для рекомендованных точек входа;
   - `adapter` для вспомогательных модулей поверх raw API;
   - `hidden` для низкоуровневых wrapper-ов, которые не должны попадать в палитру;
4. при необходимости скорректировать категорию и описание;
5. сохранить модуль обратно в исходный `.dqmod` imported pack-а;
6. использовать в графе только видимые curated entry / adapter модули.

### Product Rules Implemented In IDE

- imported-модуль хранит:
  - `deltaq.import.display_name`
  - `deltaq.import.curation_role`
- `Module Manager` показывает и редактирует эти поля;
- сохранение идёт обратно в исходный `.dqmod` файла imported pack-а, а не в `user_modules/`;
- модули с ролью `hidden` остаются редактируемыми в `Module Manager`, но скрываются из `Module Palette`;
- палитра использует curated `display name`, а tooltip показывает:
  - imported pack
  - исходный символ
  - роль import/curation

### Expected First Curation For `mini_sensor_sdk`

- `mini_sensor_init`:
  - `display name = Sensor Init`
  - `role = curated_entry`
- `mini_sensor_read`:
  - `display name = Sensor Read`
  - `role = curated_entry`
- `mini_sensor_scale`:
  - `display name = Sensor Scale`
  - `role = adapter`
- `mini_sensor_last_error`:
  - `display name = Last Error`
  - `role = hidden`
- `mini_sensor_shutdown`:
  - `display name = Sensor Shutdown`
  - `role = curated_entry`

## Current Verification

Сейчас curated pack для `mini_sensor_sdk` подтверждается и checked-in example-проектом,
и автоматическим integration test-ом, который делает полный путь:

`fixture SDK -> curated imported pack -> graph -> pre-build -> CMake -> build -> run`

Что проверяется:

- imported pack requirements реально попадают в `CMakeLists.txt`;
- curated entry-модули проходят `build/run` как обычные модули графа;
- runtime подтверждает полный сценарий:
  - `READ=48`
  - `SCALED=144`
  - `FAILED_SCALE=-1`
  - `LAST_ERROR=scale factor must be positive`
  - `SHUTDOWN=done`

Автоматическая проверка:

- `tests/codegen/test_PreBuildProcessor.cpp`
  - `curatedImportedPackBuildsAndRunsMiniSensorFlow()`

Важно:

- checked-in showcase project теперь лежит в
  `resources/examples/imported_pack_sensor_console`;
- путь `header + binary -> raw wrappers -> curated pack` закреплён отдельным
  reproducible raw-baseline test-ом;
- `v1` claim остаётся ограниченным:
  - Linux-first intake;
  - `C ABI` baseline;
  - missing header/binary path или unsupported ABI должны завершаться explicit
    diagnostic-ом до записи pack-а.
