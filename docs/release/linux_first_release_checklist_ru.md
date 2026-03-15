# Linux-first checklist для DeltaQ release artifacts

Этот checklist является user-facing путём приёмки Linux artifact DeltaQ перед тем, как отдавать его другому пользователю или показывать публично.

Текущее framing для этого checklist:

- рассматривайте проверяемый artifact как **Linux release candidate**
- не трактуйте его как cross-platform release claim

## Выберите формат artifact

- `.tar.gz`
  Используйте его, если нужен самый прозрачный ручной review-path: layout bundle, examples, modules и generated files видны как обычные файлы.

- `.AppImage`
  Используйте его, если нужен самый чистый single-file handoff. Для глубокого ручного просмотра bundled examples и layout tarball остаётся более удобным artifact.

## 1. Проверьте checksum

Положите artifact рядом с `SHA256SUMS`, затем выполните одну из команд:

```bash
grep 'Linux-x86_64.tar.gz' SHA256SUMS | sha256sum -c
grep 'x86_64.AppImage' SHA256SUMS | sha256sum -c
```

## 2. Опционально: изолируйте writable state

Если не хотите смешивать проверку со своим обычным состоянием DeltaQ, используйте:

```bash
export DELTAQ_HOME="$(mktemp -d)"
```

После первого запуска ожидаются такие writable paths:

- `$DELTAQ_HOME/config/settings.ini`
- `$DELTAQ_HOME/modules/core/pack.json`

Без `DELTAQ_HOME` эти же файлы будут созданы в `~/.deltaq/`.

## 3. Первый запуск

### Tarball

```bash
tar xf DeltaQ-*-Linux-x86_64.tar.gz
cd DeltaQ-*-Linux-x86_64
./deltaq
```

### AppImage

```bash
chmod +x DeltaQ-*-x86_64.AppImage
./DeltaQ-*-x86_64.AppImage
```

На первом запуске проверьте:

- приложение стартует без попытки писать внутрь bundled tree;
- writable state появляется в `$DELTAQ_HOME/` или `~/.deltaq/`;
- bundled `core` pack становится доступен в user-writable modules root.

## 4. Самый короткий product proof

Для самого понятного ручного proof используйте tarball bundle и откройте:

- `examples/minimal_console_flow/minimal_console_flow.dqproj`

Дальше:

1. откройте `graphs/main.dqgraph`;
2. нажмите `Build`;
3. нажмите `Run`;
4. введите `DeltaQ`.

Ожидаемый вывод:

```text
Hello from DeltaQ!
DeltaQ
```

После этого посмотрите:

- `src/main.c`

Смысл этого шага в том, чтобы подтвердить: release artifact всё ещё выражает тот же прозрачный путь, что и source checkout:

`модуль -> граф -> generated C code -> build -> run`

## 5. Более сильный optional proof

Если после minimal console path нужен более широкий showcase:

- откройте `examples/desktop_ui_flow` для desktop/UI path;
- откройте `examples/reusable_composition_console` для submodule reuse;
- откройте `examples/imported_pack_sensor_console` или `examples/imported_pack_checksum_console` для flow `external library -> pack`.

## 6. Что значит "artifact проходит"

Linux artifact находится в хорошем user-facing состоянии, если:

- checksum verification проходит;
- первый запуск создаёт writable state вне bundle;
- bundled examples присутствуют;
- `minimal_console_flow` по-прежнему проходит путь `open -> build -> run`;
- generated `src/main.c` остаётся легко читаемым и сопоставимым с графом.
