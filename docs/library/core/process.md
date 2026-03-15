# Core Process Modules

## Роль категории

`process` в checked-in `core` не пытается быть shell framework или PTY toolkit.
Его задача в Stage 4 уже:

- запустить небольшой внешний tool/script;
- получить stdout или exit code;
- остаться прозрачным частью обычного graph runtime.

Это useful baseline для tool-runner и integration сценариев, а не общий subprocess layer.

## Essential

### `core.process.run_stdout`

- Роль: `essential`
- Зачем нужен: shortest path для запуска внешнего шага и получения stdout как string
- Когда брать: если graph должен вызвать tool/script и использовать его текстовый результат дальше
- Ограничение: Linux-first POSIX helper, блокирует выполнение до конца процесса и хранит stdout в фиксированном буфере

### `core.process.run_exit_code`

- Роль: `essential`
- Зачем нужен: shortest path для явной проверки success/failure после запуска внешнего шага
- Когда брать: если graph должен убедиться, что tool/script завершился кодом `0`
- Ограничение: не отдаёт stderr/stdout в graph и поддерживает только program + несколько позиционных аргументов

## Практическое правило

Если сценарий укладывается в:

- `run_stdout`
- `run_exit_code`
- `print result / timeout status`

то `core.process` используется по назначению.

Если появляется потребность в shell pipelines, env/workdir management, stdin streaming
или PTY, это уже не compact core baseline, а следующий слой runtime/integration pack-ов.
