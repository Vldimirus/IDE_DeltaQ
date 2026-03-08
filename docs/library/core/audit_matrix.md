# Core Audit Matrix

Этот файл фиксирует текущие reviewed-решения по checked-in `core` pack.

Колонки:

- `Decision`:
  - `keep` — модуль остаётся в библиотеке;
  - `legacy` — модуль оставлен ради совместимости, но не рекомендуется как новый выбор;
  - `remove` — пока не используется, в текущем срезе таких модулей нет.
- `Role`:
  - `essential`
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

## Current Summary

- `essential`: 28
- `specialized`: 3
- `legacy`: 12
- `remove`: 0
- `convenience`: 0 checked-in reviewed modules на текущем срезе
