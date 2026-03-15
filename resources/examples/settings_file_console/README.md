# Settings File Console

Этот example показывает первый practical Stage 4 baseline для `filesystem + config_json`.

Что здесь происходит:

1. граф создаёт директорию `runtime/`;
2. читает `runtime/settings.json`, если файл уже существует;
3. достаёт `launch_count` из JSON или берёт `0` по умолчанию;
4. увеличивает счётчик на `1`;
5. сохраняет обратно JSON с `launch_count` и `profile = "deltaq"`;
6. печатает текущее состояние в stdout.

Ожидаемый вывод на первом запуске:

```text
launch_count=1
profile=deltaq
```

На втором запуске в той же директории:

```text
launch_count=2
profile=deltaq
```

После второго запуска файл `runtime/settings.json` должен содержать оба ключа:

```json
{"launch_count":2,"profile":"deltaq"}
```
