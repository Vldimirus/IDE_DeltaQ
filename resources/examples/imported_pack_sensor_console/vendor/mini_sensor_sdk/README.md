# mini_sensor_sdk

Локальная копия controlled fixture library для showcase-проекта
`imported_pack_sensor_console`.

Зачем она лежит прямо внутри примера:

- проект можно копировать и собирать как self-contained DeltaQ example;
- generated `main.c` и imported-модули компилируются вместе с
  `vendor/mini_sensor_sdk/src/mini_sensor_sdk.c`;
- include path к `mini_sensor_sdk.h` подтягивается через metadata curated pack-а.

Ожидаемое поведение для `sensor-A`:

- `mini_sensor_read(...) == 48`
- `mini_sensor_scale(..., 3) == 144`
- `mini_sensor_scale(..., 0) == -1`
- `mini_sensor_last_error(...) == "scale factor must be positive"`
