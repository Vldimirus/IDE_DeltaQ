# Core Audit Matrix

Этот файл фиксирует текущие reviewed-решения по checked-in `core` pack.

Колонки:

- `Decision`:
  - `keep` — модуль остаётся в библиотеке;
  - `legacy` — модуль оставлен ради совместимости, но не рекомендуется как новый выбор;
  - `remove` — пока не используется, в текущем срезе таких модулей нет.
- `Role`:
  - `essential`
  - `convenience`
  - `specialized`
  - `legacy`

## Control

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.control.if_branch` | `keep` | `essential` | Основное exec-ветвление baseline |
| `core.control.for_loop` | `keep` | `essential` | Базовый счётный цикл v1 |
| `core.control.sequence` | `keep` | `essential` | Базовый exec-разветвитель |
| `core.control.if_then` | `legacy` | `legacy` | Узкий int-only data-flow helper, лучше выносить в локальный helper или внешний pack |
| `core.control.delay_ms` | `legacy` | `legacy` | Блокирующий runtime helper, который лучше выносить во внешний pack |

## IO

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.io.print` | `keep` | `essential` | Базовый text output |
| `core.io.println` | `keep` | `essential` | Базовый text output с переводом строки |
| `core.io.read_line` | `keep` | `essential` | Базовый console input |
| `core.io.int_constant` | `keep` | `essential` | Базовая int-константа |
| `core.io.string_constant` | `keep` | `essential` | Базовая string-константа |
| `core.io.print_float` | `legacy` | `legacy` | Typed float-output shortcut, который не образует сильный float baseline |
| `core.io.print_int` | `legacy` | `legacy` | Дублирует `int_to_string + print/println` |

## Logic

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.logic.and` | `keep` | `essential` | Базовое булево И |
| `core.logic.or` | `keep` | `essential` | Базовое булево ИЛИ |
| `core.logic.not` | `keep` | `essential` | Базовое булево НЕ |

## Conversion

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.conversion.int_to_string` | `keep` | `essential` | Базовый путь в text output |
| `core.conversion.string_to_int` | `keep` | `essential` | Базовый console/input conversion |
| `core.conversion.float_to_int` | `legacy` | `legacy` | Partial float conversion лучше выносить в numeric pack |
| `core.conversion.int_to_float` | `legacy` | `legacy` | Partial float conversion лучше выносить в numeric pack |

## Math

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.math.add` | `keep` | `essential` | Базовая арифметика v1 |
| `core.math.subtract` | `keep` | `essential` | Базовая арифметика v1 |
| `core.math.multiply` | `keep` | `essential` | Базовая арифметика v1 |
| `core.math.divide` | `keep` | `essential` | Базовая арифметика v1 |
| `core.math.abs` | `legacy` | `legacy` | Узкий прикладной int helper, лучше выносить в numeric pack или project helper |
| `core.math.add_float` | `legacy` | `legacy` | Partial float arithmetic лучше выносить в numeric pack |
| `core.math.mod` | `legacy` | `legacy` | Узкий прикладной int helper, лучше выносить в numeric pack или project helper |
| `core.math.multiply_float` | `legacy` | `legacy` | Partial float arithmetic лучше выносить в numeric pack |
| `core.math.pow` | `legacy` | `legacy` | Thin wrapper над `libm`, лучше выносить во внешний math pack |
| `core.math.sqrt` | `legacy` | `legacy` | Thin wrapper над `libm`, лучше выносить во внешний math pack |

## String

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.string.str_compare` | `keep` | `specialized` | Прикладная работа со строками |
| `core.string.str_concat` | `keep` | `specialized` | Прикладная работа со строками |
| `core.string.str_length` | `keep` | `specialized` | Прикладная работа со строками |

## Desktop

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.desktop.sdl_init` | `keep` | `essential` | Backend lifecycle baseline |
| `core.desktop.ttf_init` | `keep` | `essential` | Backend text baseline |
| `core.desktop.create_window` | `keep` | `essential` | Desktop window baseline |
| `core.desktop.create_renderer` | `keep` | `essential` | Desktop renderer baseline |
| `core.desktop.ui_init` | `keep` | `essential` | Generated UI state baseline |
| `core.desktop.event_loop` | `keep` | `essential` | Desktop event runtime baseline |
| `core.desktop.ui_cleanup_font` | `keep` | `essential` | Generated UI cleanup baseline |
| `core.desktop.destroy_renderer` | `keep` | `essential` | Desktop shutdown baseline |
| `core.desktop.destroy_window` | `keep` | `essential` | Desktop shutdown baseline |
| `core.desktop.ttf_quit` | `keep` | `essential` | Backend text shutdown baseline |
| `core.desktop.sdl_quit` | `keep` | `essential` | Backend lifecycle shutdown baseline |

## Filesystem

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.filesystem.read_text_file` | `keep` | `essential` | Первый useful-baseline path для file-backed config/state |
| `core.filesystem.write_text_file` | `keep` | `essential` | Первый useful-baseline path для file-backed config/state |
| `core.filesystem.file_exists` | `keep` | `convenience` | Ускоряет file-backed ветвления без разрастания baseline |
| `core.filesystem.ensure_dir` | `keep` | `convenience` | Удобный helper перед сохранением runtime/config файлов |

## Config / JSON

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.config_json.json_get_int` | `keep` | `essential` | Базовый int-read path для небольших JSON-config flow |
| `core.config_json.json_set_int` | `keep` | `essential` | Базовый int-write path для небольших JSON-config flow |
| `core.config_json.json_get_string` | `keep` | `convenience` | Удобный string-read helper для settings/state scenarios |
| `core.config_json.json_set_string` | `keep` | `convenience` | Удобный string-write helper для settings/state scenarios |

## Process

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.process.run_stdout` | `keep` | `essential` | Минимальный tool-runner path с захватом stdout без shell orchestration layer |
| `core.process.run_exit_code` | `keep` | `essential` | Минимальный tool-runner status path с явным success/failure contract |

## Timers

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.timers.now_ms` | `keep` | `essential` | Monotonic timestamp для budget/elapsed checks вокруг sync step |
| `core.timers.timeout_once` | `keep` | `essential` | One-shot timeout check без возврата к blocking delay helper |
| `core.timers.elapsed_ms` | `keep` | `convenience` | Удобный elapsed helper для stdout/log/reporting path |

## TCP / UDP

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.tcp_udp.udp_bind` | `keep` | `essential` | Datagrams-first loopback bind path для compact transport probe |
| `core.tcp_udp.udp_send` | `keep` | `essential` | Минимальный UDP text send path без protocol framework |
| `core.tcp_udp.udp_receive` | `keep` | `essential` | Минимальный UDP receive path с timeout boundary |
| `core.tcp_udp.udp_local_port` | `keep` | `convenience` | Удобный helper для ephemeral bind -> send-to-self flow |
| `core.tcp_udp.udp_close` | `keep` | `convenience` | Явный cleanup helper для transport probe scenario |

## Serial

| Module | Decision | Role | Note |
| --- | --- | --- | --- |
| `core.serial.serial_open` | `keep` | `essential` | Минимальный open path для serial device bridge |
| `core.serial.serial_configure` | `keep` | `essential` | Базовый raw 8N1 configure path перед exchange |
| `core.serial.serial_write` | `keep` | `essential` | Минимальный write path для небольшого text payload |
| `core.serial.serial_read` | `keep` | `essential` | Минимальный read path с timeout boundary |
| `core.serial.serial_close` | `keep` | `essential` | Явный serial cleanup path |
| `core.serial.serial_loopback_path` | `keep` | `convenience` | Self-contained PTY helper для serial probe без реального hardware |

## Current Summary

- `essential`: 44
- `specialized`: 3
- `legacy`: 12
- `remove`: 0
- `convenience`: 8 checked-in reviewed modules на текущем срезе
