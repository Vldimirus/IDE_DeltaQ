# Imported Pack Sensor Console

Этот пример показывает checked-in showcase для imported pack-ов в DeltaQ.

Что здесь есть:

- локальный `vendor/mini_sensor_sdk`;
- curated imported pack `mini_sensor_sdk_curated` в `dqmods/`;
- скрытые raw wrapper-модули;
- видимые curated entry / adapter модули:
  - `Sensor Scale Report`
  - `Sensor Error Report [adapter]`;
- корневой граф, который использует только curated-слой pack-а.

Сценарий выполнения:

1. `Sensor Scale Report` создаёт контекст датчика, читает значение, масштабирует его и печатает отчёт;
2. после него `Sensor Error Report` намеренно вызывает ошибку масштаба и печатает `LAST_ERROR`;
3. оба модуля корректно завершают работу через `mini_sensor_shutdown`.

Ожидаемый вывод:

```text
READ=48
SCALED=144
SHUTDOWN=done
FAILED_SCALE=-1
LAST_ERROR=scale factor must be positive
SHUTDOWN=done
```

Что важно:

- граф использует не сырые wrapper-ы, а curated imported-модули;
- `vendor/mini_sensor_sdk/src/mini_sensor_sdk.c` компилируется как часть проекта;
- include path к header-у подтягивается через metadata imported pack-а.
