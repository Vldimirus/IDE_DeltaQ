# Core Library Curation

## Назначение

Этот раздел фиксирует не просто список checked-in `core`-модулей, а их
продуктовую роль внутри DeltaQ.

`core` должен оставаться компактным и понятным baseline-набором, а не
превращаться в свалку shortcut-ов и частных обёрток.

## Текущие роли

- `essential` — опорные модули `v1`, на которых строится базовый workflow.
- `convenience` — допустимые ускорители графа, которые не размывают модель.
- `specialized` — полезные прикладные модули вне минимального baseline.
- `legacy` — сохранены ради совместимости, но не рекомендуются как новый выбор.

## Текущие legacy slices

Сейчас в `legacy` переведены:

- `core.io.print_int`
- `core.io.print_float`
- `core.control.if_then`
- `core.control.delay_ms`
- `core.conversion.float_to_int`
- `core.conversion.int_to_float`
- `core.math.add_float`
- `core.math.multiply_float`
- `core.math.abs`
- `core.math.mod`
- `core.math.pow`
- `core.math.sqrt`

Причина:

- `core.io.print_int` дублирует composable-путь `int_to_string + print/println`;
- typed output shortcut в таком виде стимулирует разрастание `core` за счёт
  множества узких вариаций одного и того же действия;
- `core.control.if_then` является узким `int`-only data-flow helper-ом и лучше
  выглядит как локальный submodule или внешний extension pack, чем как часть
  рекомендуемого checked-in baseline;
- `core.math.abs` и `core.math.mod` являются узкими прикладными `int`-helper-ами
  и лучше выглядят как часть внешнего numeric pack-а или локального project helper-а;
- partial float support (`print_float`, `float_to_int`, `int_to_float`, `add_float`, `multiply_float`)
  не образует достаточно сильный и цельный numeric baseline для checked-in `core`;
- `core.control.delay_ms` является блокирующим runtime helper-ом и хуже подходит
  для долгосрочной роли в `core`, чем для внешнего runtime pack-а;
- `core.math.pow` и `core.math.sqrt` являются thin wrapper-ами над `libm` и лучше
  подходят для внешнего math pack-а с явной линковкой;
- для старых графов legacy-модули остаются доступными, но в новых графах IDE должна
  подсказывать более явные альтернативы.

## Что это значит в IDE

- в `Module Palette` `specialized` и `legacy` core-модули скрыты по умолчанию;
- в `Module Palette` они показываются только по явному запросу;
- в `Module Palette` и `Module Manager` legacy-модуль помечается как `[legacy]`;
- tooltip показывает его роль и рекомендуемую замену;
- модуль не удалён физически, но уже исключён из рекомендуемого baseline.

## Следующие шаги

- продолжить audit остальных `core`-модулей;
- решать для каждого спорного модуля: `оставить`, `legacy` или `удалить`;
- не расширять `core` shortcut-ами, если ту же задачу уже решает ясная
  composable-связка существующих модулей.

Подробная матрица текущих решений хранится в [audit_matrix.md](./audit_matrix.md).

Stage 4 useful-category audit хранится отдельно в
[useful_baseline_audit.md](./useful_baseline_audit.md), чтобы execution-plan по
`filesystem / timers / config-json / process / tcp-udp / serial` не смешивался
с уже существующей reviewed-матрицей checked-in `core`.

Первые category guides:

- [io.md](./io.md)
- [control.md](./control.md)
- [filesystem.md](./filesystem.md)
- [config_json.md](./config_json.md)
- [process.md](./process.md)
- [serial.md](./serial.md)
- [tcp_udp.md](./tcp_udp.md)
- [timers.md](./timers.md)
- [conversion.md](./conversion.md)
- [math.md](./math.md)
- [desktop.md](./desktop.md)
- [logic.md](./logic.md)
- [string.md](./string.md)
