# Core Serial Modules

## Роль категории

`serial` в checked-in `core` не пытается быть hardware SDK layer.
Его первая задача уже:

- открыть и настроить generic serial device path;
- отправить и получить небольшой payload;
- остаться частью compact probe/device-bridge graph.

Это useful baseline для device bridge сценариев, а не полный serial framework.

## Essential

### `core.serial.serial_open`

- Роль: `essential`
- Зачем нужен: shortest path к открытому serial fd
- Когда брать: если graph уже знает device path и хочет начать exchange
- Ограничение: не занимается discovery и возвращает только file descriptor

### `core.serial.serial_configure`

- Роль: `essential`
- Зачем нужен: приводит fd к предсказуемому raw 8N1 contract
- Когда брать: перед первым read/write в generic serial flow
- Ограничение: покрывает только небольшой набор baud rate и базовый raw-mode setup

### `core.serial.serial_write`

- Роль: `essential`
- Зачем нужен: shortest path к отправке небольшого text payload
- Когда брать: если graph пишет короткий probe/command в serial endpoint
- Ограничение: не строит framed protocol layer и не покрывает partial protocol semantics

### `core.serial.serial_read`

- Роль: `essential`
- Зачем нужен: shortest path к чтению одного text chunk с timeout boundary
- Когда брать: если graph ждёт короткий ответ/echo от serial endpoint
- Ограничение: читает один chunk в фиксированный буфер и не даёт richer device metadata

### `core.serial.serial_close`

- Роль: `essential`
- Зачем нужен: делает serial lifecycle явным и завершённым
- Когда брать: в конце serial probe/device bridge path
- Ограничение: thin cleanup helper без richer shutdown/state model

## Convenience

### `core.serial.serial_loopback_path`

- Роль: `convenience`
- Зачем нужен: даёт self-contained PTY loopback для probe/test path
- Когда брать: если нужен checked-in serial scenario без реального hardware
- Ограничение: Linux-first helper для loopback/testing и не должен маскироваться под device discovery layer

## Практическое правило

Если сценарий укладывается в:

- `serial_open -> serial_configure -> serial_write -> serial_read -> serial_close`

то `core.serial` используется по назначению.

Если появляются hotplug, protocol decoders, vendor-specific setup или hardware discovery,
это уже следующий layer или imported pack, а не compact Stage 4 baseline.
