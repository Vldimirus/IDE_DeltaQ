# SQLite Curated Pack

`sqlite_curated` — первый official database pack в DeltaQ после усиления
`Module Studio`.

Это не расширение `modules/core`, а отдельный checked-in curated pack:

- raw thin wrappers дают auditable substrate;
- curated entry modules дают короткий user-facing SQLite surface;
- pack собирается без `sqlite3.h` dev headers, потому что использует
  checked-in runtime header и `dlopen("libsqlite3.so.0")`.

## Pack Layout

Файлы pack-а лежат в:

- [modules/sqlite_curated/pack.json](/home/vladimir/Prog/IDE_DeltaQ/modules/sqlite_curated/pack.json)
- [sqlite_deltaq_runtime.h](/home/vladimir/Prog/IDE_DeltaQ/modules/sqlite_curated/include/sqlite_deltaq_runtime.h)

## Raw Thin Wrappers

Эти модули intentionally скрыты из normal palette flow через
`deltaq.import.curation_role = hidden`:

- `sqlite_open`
- `sqlite_close`
- `sqlite_exec`
- `sqlite_prepare`
- `sqlite_bind_int`
- `sqlite_bind_text`
- `sqlite_step`
- `sqlite_column_int`
- `sqlite_column_text`
- `sqlite_finalize`

Они нужны как:

- auditable low-level layer;
- substrate для future authoring/verification;
- явный bridge к SQLite ABI без ad-hoc C helper-кода в проекте.

## Curated Entry Modules

Основной user-facing surface сейчас такой:

- `sqlite_exec_path`
- `sqlite_query_scalar_text`
- `sqlite_query_scalar_int`

Именно эти модули должны быть нормальным входом в palette, когда нужен короткий
SQLite workflow без ручного lifecycle `open -> prepare -> bind -> step -> finalize`.

## Limitations

- Linux-first runtime path;
- требуется `libsqlite3.so.0` или `libsqlite3.so` в системе;
- нет rich error surface: current helpers возвращают status/fallback, а не
  полную diagnostic object model;
- curated entry modules открывают и закрывают database на каждом вызове, поэтому
  pack сейчас рассчитан на короткие deterministic graph steps, а не на long-lived
  session/cache layer.

## Verification Status

Текущий checked-in proof уже есть на пяти уровнях:

- `test_CMakeGenerator` подтверждает local pack include-dir propagation;
- `test_StandardLibrary` подтверждает install of official bundled pack;
- `test_PreBuildProcessor::sqliteSettingsConsoleExampleBuildsAndRunsEndToEnd`
  подтверждает `pre-build -> CMake -> build -> run`.
- `test_PreBuildProcessor::sqliteNotesDesktopExampleBuildsAndRunsHeadless`
  подтверждает desktop-side `pre-build -> CMake -> build -> headless run`;
- `test_MainWindowEditorActions::sqliteImportedModuleCanRunSavedVerificationScenario`
  подтверждает, что `Module Studio` реально открывает curated SQLite module,
  запускает saved verification scenario и сохраняет mixed `exec + data` contract
  без дрейфа.

## Reference Examples

Checked-in scenarios сейчас такие:

- [sqlite_settings_console](/home/vladimir/Prog/IDE_DeltaQ/resources/examples/sqlite_settings_console)
- [sqlite_notes_desktop](/home/vladimir/Prog/IDE_DeltaQ/resources/examples/sqlite_notes_desktop)

`sqlite_settings_console` доказывает shortest useful flow:

- create table
- insert/update value
- select scalar text
- print result

`sqlite_notes_desktop` доказывает reuse того же curated pack-а внутри desktop flow:

- create table on startup
- persist note text in `runtime/notes.db`
- read the same note back into runtime log
- complete headless desktop run without ad-hoc glue outside the pack
