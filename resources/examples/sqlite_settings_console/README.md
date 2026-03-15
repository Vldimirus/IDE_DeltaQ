# SQLite Settings Console

Этот example показывает первый SQLite forcing-function для `Track B`.

Что здесь происходит:

1. граф создаёт директорию `runtime/`;
2. создаёт таблицу `kv`, если она ещё не существует;
3. записывает `profile = "deltaq"` в `runtime/profile.db`;
4. читает обратно одно scalar-значение через curated SQLite entry module;
5. печатает в stdout строку `profile=deltaq`.

Ожидаемый вывод:

```text
profile=deltaq
```

После запуска в `build/runtime/` должен появиться файл `profile.db`.
