# SQLite Notes Desktop

Этот example закрывает desktop-side proof для `Track B`.

Что здесь происходит:

1. граф подготавливает `runtime/notes.db`;
2. создаёт таблицу `notes`, если её ещё нет;
3. записывает одну persisted note через curated SQLite module;
4. читает её обратно и печатает `loaded_note=...` в stdout;
5. после этого запускается обычный desktop/UI runtime и окно живёт до auto-close.

Для headless/regression запуска можно задать:

```text
DQ_SQLITE_NOTES_DESKTOP_AUTOCLOSE_MS
```

Ожидаемый stdout:

```text
loaded_note=Persisted from desktop flow
```
