# Core Useful Baseline Audit

Дата среза: `2026-03-15`

Этот файл не создаёт второй competing strategy поверх
`docs/plan/strategy_2026/21_standard_library_strategy.md`.
Он фиксирует execution-grade audit для `Stage 4: Core Module Library v1`
из `docs/plan/strategy_2026/32_product_maturity_recovery_v1.md`.

## Current Repo Facts

- checked-in `core` pack сейчас состоит только из категорий:
  - `config_json`
  - `control`
  - `conversion`
  - `desktop`
  - `filesystem`
  - `io`
  - `logic`
  - `math`
  - `process`
  - `serial`
  - `string`
  - `tcp_udp`
  - `timers`
- `core.control.delay_ms` не считается достаточным baseline для `timers`:
  он уже помечен как `legacy` и описан как blocking helper, который лучше
  уводить во внешний runtime pack.
- external/imported pack-и остаются главным каналом для domain-specific SDK и
  узких protocol adapters; Stage 4 про generic useful baseline, а не про раздувание
  `core` hardware-specific vocabulary.

## Status Matrix

| Category | Status | Essential baseline slice | Convenience / specialized boundary | Why deferred now |
| --- | --- | --- | --- | --- |
| `filesystem` | `added in this cycle` | `read_text_file`, `write_text_file` уже checked-in как `essential`; `file_exists`, `ensure_dir` добавлены как `convenience` | `append_text_file`, `list_dir` допустимы как `convenience`; binary I/O, copy/move/remove, file watching не входят в первый baseline | Первый slice сознательно ограничен small file-backed config/state flow и не превращает `core` в POSIX wrapper layer |
| `timers` | `added in this cycle` | `now_ms`, `timeout_once` уже checked-in как `essential`; `elapsed_ms` добавлен как `convenience` | recurring ticks, scheduler pools и wall-clock formatting остаются за пределами v1; blocking-delay не возвращать в новый baseline | Slice сознательно ограничен short-lived monotonic budget/elapsed checks вокруг sync runtime steps |
| `config/json` | `added in this cycle` | `json_get_int`, `json_set_int` уже checked-in как `essential`; `json_get_string`, `json_set_string` добавлены как `convenience` | path helpers/defaults допустимы как `convenience`; schema validation, rich arrays и domain schemas не часть v1 | Первый slice сознательно ограничен плоским top-level JSON object для settings/state use cases |
| `process` | `added in this cycle` | `run_stdout`, `run_exit_code` уже checked-in как `essential` | stdin/stderr/env/workdir helpers допустимы как `convenience`; shell/PTY/pipeline orchestration не часть v1 | Slice сознательно ограничен простым Linux-first tool-runner path без попытки превратить `core` в subprocess framework |
| `tcp/udp` | `added in this cycle` | `udp_bind`, `udp_send`, `udp_receive` уже checked-in как `essential`; `udp_local_port`, `udp_close` добавлены как `convenience` | TCP sessions, servers, multicast, TLS и protocol-specific wrappers остаются вне v1; datagram-first slice не должен маскироваться под полный networking layer | Slice сознательно ограничен loopback UDP probe path и оставляет richer transport semantics за пределами checked-in core |
| `serial` | `added in this cycle` | `serial_open`, `serial_configure`, `serial_write`, `serial_read`, `serial_close` уже checked-in как `essential`; `serial_loopback_path` добавлен как `convenience` | line/frame helpers допустимы как `convenience`; protocol decoders, hotplug, vendor SDK adapters не часть checked-in `core` | Slice сознательно ограничен generic raw serial bridge и self-contained PTY probe без попытки превратить `core` в hardware SDK layer |

## Role Policy For New Categories

- `essential`
  - только те модули, без которых category не образует shortest useful scenario.
- `convenience`
  - допустимые ускорители поверх already-healthy baseline, но не новые shortcut-свалки.
- `specialized`
  - прикладные helpers, которые полезны, но не должны маскироваться под обязательный стартовый набор.
- `legacy`
  - не использовать как стартовый инструмент для новых useful categories; сначала строить чистый baseline, а не плодить будущий legacy.

## Expected Docs And Verification Bar

Для каждого нового `essential`-модуля в этих категориях нужно требовать одновременно:

- checked-in `.dqmod` в `modules/core` как единственный source of truth;
- `description`, `deltaq.doc.when_to_use`, `deltaq.doc.limitations`;
- explicit curation role в `StandardLibrary`;
- compile/test verification на уровне существующего module quality bar;
- хотя бы один checked-in `build -> run` reference scenario, который использует категорию как часть реального flow.

Для `convenience`/`specialized` bar остаётся почти тем же, но без права
маскироваться под обязательный baseline.

## Reference Scenarios

Уже checked-in в текущем slice:

1. `settings_file_console`
   - `filesystem + config/json`
   - shortest path `load config -> increment state -> save config`.

2. `process_timer_console`
   - `process + timers`
   - shortest path `launch tool -> print stdout and exit code -> check elapsed/budget`.

3. `transport_probe_console`
   - `tcp/udp + timers`
   - shortest path `bind -> send -> receive -> timeout/close`.

4. `serial_probe_console`
   - `serial + timers`
   - shortest path `open -> exchange payload -> timeout/close`.

Следующие target scenarios:

5. Нужны уже не новые baseline categories, а consolidation slices:
   - docs/discoverability cleanup;
   - palette guidance;
   - возможно один cross-category scenario.

## Decision Summary

- useful Stage 4 baseline теперь развивается через explicit category slices, а не через разрозненные helper-модули;
- `filesystem`, `config/json`, `process`, `timers`, `tcp_udp` и `serial` уже переведены из purely deferred зоны в checked-in initial baseline;
- category baseline portion Stage 4 можно считать закрытым; дальше возможны только consolidation/discoverability slices.
