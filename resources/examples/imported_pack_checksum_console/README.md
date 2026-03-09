# Imported Pack Checksum Console

Этот пример показывает второй checked-in showcase для imported pack-ов в DeltaQ.

Что здесь есть:

- локальный `vendor/mini_checksum_sdk`;
- curated imported pack `mini_checksum_sdk_curated` в `dqmods/`;
- скрытые raw wrapper-модули;
- видимые curated entry / adapter модули:
  - `Checksum Report`
  - `Checksum Match Report [adapter]`;
- корневой граф, который использует только curated-слой pack-а.

Сценарий выполнения:

1. `Checksum Report` создаёт checksum-context, прогоняет строку `DeltaQ` и печатает числовой checksum и hex-представление;
2. после него `Checksum Match Report` повторяет тот же путь и сравнивает hex с ожидаемым значением;
3. оба модуля корректно завершают работу через `mini_checksum_shutdown`.

Ожидаемый вывод:

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

Что важно:

- граф использует не сырые wrapper-ы, а curated imported-модули;
- `vendor/mini_checksum_sdk/src/mini_checksum_sdk.c` компилируется как часть проекта;
- include path к header-у подтягивается через metadata imported pack-а;
- это второй domain-specific imported pack рядом с `mini_sensor_sdk`, а не ещё одно расширение `core`.
