# mini_sensor_sdk

Небольшая контролируемая `C`-библиотека-фикстура для сценария:

`external library -> import -> curation -> graph -> build -> run`

## Назначение

Эта библиотека не является реальным SDK устройства. Она нужна, чтобы в DeltaQ можно было:

- импортировать внешний `C`-header;
- получить raw wrapper pack;
- выполнить curation;
- использовать pack в графе;
- проверить сборку и runtime без внешних зависимостей.

## API

- `mini_sensor_init(const char *device_name)`
- `mini_sensor_read(mini_sensor_context *ctx)`
- `mini_sensor_scale(mini_sensor_context *ctx, int factor)`
- `mini_sensor_last_error(mini_sensor_context *ctx)`
- `mini_sensor_shutdown(mini_sensor_context *ctx)`

## Детерминированное поведение

Библиотека специально не использует:

- случайность;
- системное время;
- файловую систему;
- внешнее оборудование.

Базовое значение вычисляется только из `device_name`, а итоговое чтение равно:

`base_value * scale_factor`

Для строки `sensor-A` ожидаемое поведение такое:

- после `mini_sensor_init("sensor-A")`
  - `mini_sensor_read(...) == 48`
- после `mini_sensor_scale(..., 3)`
  - `mini_sensor_scale(...) == 144`

Для `factor <= 0`:

- `mini_sensor_scale(...) == -1`
- `mini_sensor_last_error(...) == "scale factor must be positive"`

## Локальная сборка

Библиотека собирается как обычный маленький `CMake`-проект:

```bash
cmake -S . -B build
cmake --build build
```

## Роль в стратегии DeltaQ

Это не пользовательский example-проект, а именно fixture library для следующего подэтапа:

- raw import baseline;
- curation imported pack-а;
- showcase project поверх curated pack.
