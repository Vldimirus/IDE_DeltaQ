# Core Config / JSON Modules

## Роль категории

`config_json` в checked-in `core` закрывает не "весь JSON", а очень конкретный
useful baseline:

- прочитать scalar-настройку из небольшого top-level object;
- обновить scalar-настройку;
- продолжить graph flow уже с новым JSON text.

Это baseline для settings/state сценариев, а не полноценный parser/serializer framework.

## Essential

### `core.config_json.json_get_int`

- Роль: `essential`
- Зачем нужен: shortest path для чтения int-настройки из JSON-config
- Когда брать: если graph должен восстановить счётчик, timeout или другой scalar state
- Ограничение: работает только с top-level object и простыми int values

### `core.config_json.json_set_int`

- Роль: `essential`
- Зачем нужен: shortest path для обновления int-настройки и передачи нового JSON дальше по graph flow
- Когда брать: если workflow сохраняет counters, numeric limits или small state snapshots
- Ограничение: перестраивает compact JSON заново и не сохраняет исходное форматирование

## Convenience

### `core.config_json.json_get_string`

- Роль: `convenience`
- Зачем нужен: читает string-настройку без отдельного parser state
- Когда брать: если в settings file лежит profile, label или другой текстовый scalar
- Ограничение: поддерживает только top-level string values и базовый unescape

### `core.config_json.json_set_string`

- Роль: `convenience`
- Зачем нужен: обновляет string-настройку в том же compact JSON flow
- Когда брать: если graph сохраняет profile/state labels вместе с int-настройками
- Ограничение: не является заменой полноценному JSON serializer или schema layer

## Практическое правило

Если задача укладывается в:

- `read_text_file -> json_get_*`
- `json_set_* -> write_text_file`

то `core.config_json` остаётся честным useful baseline.

Если нужен nested JSON, rich arrays, schema validation или полноценная object model,
это уже отдельный layer за пределами checked-in `core`.
