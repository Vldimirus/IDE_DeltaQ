# Core Filesystem Modules

## Роль категории

`filesystem` в checked-in `core` не пытается быть полной POSIX-обёрткой.
Его задача в Stage 4 намного уже:

- загрузить небольшой text/config payload;
- сохранить обновлённый text/config payload;
- подготовить директорию под runtime state.

Это useful baseline для file-backed сценариев, а не универсальный file manager.

## Essential

### `core.filesystem.read_text_file`

- Роль: `essential`
- Зачем нужен: shortest path для чтения небольшого текстового или JSON payload
- Когда брать: если graph должен загрузить config/state из файла без внешнего helper-кода
- Ограничение: использует фиксированный внутренний буфер и не подходит для больших файлов

### `core.filesystem.write_text_file`

- Роль: `essential`
- Зачем нужен: shortest path для сохранения небольшого текстового или JSON payload
- Когда брать: если graph должен записать config/state обратно в обычный файл
- Ограничение: перезаписывает файл целиком и не возвращает детальную диагностику ошибки

## Convenience

### `core.filesystem.file_exists`

- Роль: `convenience`
- Зачем нужен: позволяет быстро сделать ветвление вокруг file-backed flow
- Когда брать: если нужно различить `missing file` и `existing file` в графе
- Ограничение: не различает тип filesystem entry и не даёт richer metadata

### `core.filesystem.ensure_dir`

- Роль: `convenience`
- Зачем нужен: создаёт минимальный writable directory path перед сохранением state/config
- Когда брать: если scenario пишет runtime файлы в отдельную подпапку
- Ограничение: Linux-first helper без полноценного error-reporting path

## Практическое правило

Если сценарий укладывается в:

- `ensure_dir -> read_text_file -> transform -> write_text_file`

то `core.filesystem` используется правильно.

Если хочется тащить в `core` copy/move/remove/watch/symlink-поведение, сначала
нужно доказать, что это действительно часть compact useful baseline, а не
следующий специализированный layer.
