# Экосистема модулей DeltaQ

Этот каталог теперь служит общей точкой входа в library/docs-контур DeltaQ.

Он нужен, чтобы не искать по репозиторию отдельно:

- что считается `core` baseline;
- как устроен quality bar для модулей;
- где смотреть curated imported pack path;
- какие examples показывают ценность модульной экосистемы.

## Слои экосистемы

В `v1` у DeltaQ есть три основных слоя:

- `core` — checked-in стандартная библиотека, небольшой curated baseline;
- `imported packs` — расширения из внешних библиотек через путь `library -> raw wrappers -> curated pack -> graph`;
- `project modules` — локальные и составные модули конкретного проекта.

Именно это различие теперь должно быть видно не только в стратегии, но и в повседневной работе:

- в `Module Manager`;
- в `Module Palette`;
- в checked-in документации;
- в example-проектах.

Для imported pack-ов это теперь означает и отдельную группировку по pack-у внутри
деревьев IDE, а не только tooltip или metadata-поля leaf-модуля.

## Quality Bar И Verification

Базовая идея простая:

- `core` заслуживает доверия не количеством, а explicit curation review;
- imported pack не считается качественным только потому, что wrapper сгенерировался;
- локальный проектный модуль становится надёжным после staged verification.

Текущий минимальный verify/story для модуля:

- корректный контракт;
- наличие реализации;
- compile check;
- test check;
- допуск в композицию.

Для `core` поверх этого действует ещё product-curation:

- `essential`
- `convenience`
- `specialized`
- `legacy`

Для imported pack-ов дополнительно важна curation-роль:

- `raw_wrapper`
- `curated_entry`
- `adapter`
- `hidden`

## Куда смотреть дальше

### Стандартная библиотека

- [core/README.md](./core/README.md) — общая логика curation standard library
- [core/audit_matrix.md](./core/audit_matrix.md) — reviewed-матрица по checked-in `core`
- [core/io.md](./core/io.md)
- [core/control.md](./core/control.md)
- [core/conversion.md](./core/conversion.md)
- [core/math.md](./core/math.md)
- [core/desktop.md](./core/desktop.md)
- [core/logic.md](./core/logic.md)
- [core/string.md](./core/string.md)

### Imported Packs

- [imported_packs/mini_sensor_sdk.md](./imported_packs/mini_sensor_sdk.md) — первый полный curated imported-pack walkthrough
- [imported_packs/mini_checksum_sdk.md](./imported_packs/mini_checksum_sdk.md) — второй imported-pack walkthrough для algorithmic/text-processing case

## Связанные example-проекты

Если нужен shortest path от library docs к живому результату, сейчас есть такие reference flows:

- `resources/examples/minimal_console_flow` — базовый `module -> graph -> generated C -> build -> run`
- `resources/examples/desktop_ui_flow` — UI/backend path
- `resources/examples/reusable_composition_console` — reusable composition
- `resources/examples/imported_pack_sensor_console` — curated imported pack в реальном graph/runtime
- `resources/examples/imported_pack_checksum_console` — второй curated imported pack в реальном graph/runtime

## Value Proof Для Standard Library

Ценность checked-in `core` нужно показывать не числом модулей, а тем, насколько быстро он даёт собрать первые полезные сценарии без ad-hoc glue-code.

### 1. Minimal console flow

Reference project:

- `resources/examples/minimal_console_flow`

Какие `core`-модули участвуют:

- `core.io.string_constant`
- `core.io.read_line`
- `core.io.println`

Почему это важно:

- первый рабочий console path собирается только из checked-in baseline;
- пользователю не нужно сначала писать свои стартовые helper-модули;
- generated `src/main.c` остаётся маленьким и легко проверяемым.

### 2. Reusable composition

Reference project:

- `resources/examples/reusable_composition_console`

Какие `core`-модули дают ускорение:

- `core.string.str_concat`
- `core.string.str_length`
- `core.conversion.int_to_string`
- `core.io.println`

Почему это важно:

- составные модули `echo_with_prefix` и `measure_text` собираются поверх уже готовых string/conversion/io примитивов;
- reuse строится на стабильном baseline, а не на одноразовом C-helper коде;
- подмодули потом переиспользуются как обычные `.dqmod` с отдельными generated `.h/.c`.

### 3. Desktop/UI flow

Reference project:

- `resources/examples/desktop_ui_flow`

Какие `core`-модули дают ускорение:

- `core.desktop.sdl_init`
- `core.desktop.ttf_init`
- `core.desktop.create_window`
- `core.desktop.create_renderer`
- `core.desktop.ui_init`
- `core.desktop.event_loop`
- `core.desktop.ui_cleanup_font`
- `core.desktop.destroy_renderer`
- `core.desktop.destroy_window`
- `core.desktop.ttf_quit`
- `core.desktop.sdl_quit`

Почему это важно:

- SDL2 lifecycle уже выражен как графовый runtime-путь;
- граф описывает приложение, а не низкоуровневый init/pump/teardown boilerplate;
- UI example показывает, что `core` полезен не только в console onboarding, но и в desktop flow.

## Что это доказывает

- checked-in `core` уже ускоряет как минимум `3` эталонных сценария;
- ценность standard library видна через examples, а не только через audit-матрицу;
- `core` остаётся компактным baseline, а расширение экосистемы дальше идёт через imported pack-и и project modules.

## Imported Packs Как Supply Channel

Теперь это утверждение опирается не на один showcase, а на два разных checked-in imported pack case:

- `mini_sensor_sdk` — hardware-like/runtime SDK;
- `mini_checksum_sdk` — algorithmic/text-processing SDK.

Это важно, потому что показывает:

- новые reusable возможности приходят в DeltaQ через pack-и из внешних библиотек;
- `core` не используется как ведро для domain-specific расширений;
- imported pack-и уже работают как основной канал роста vocabulary вне curated baseline.

## Что этот раздел должен удерживать

Этот каталог не должен превращаться в архив заметок.

Его задача:

- удерживать единый narrative про module ecosystem;
- связывать `core`, imported pack-и и examples;
- делать verification story понятной без чтения всей стратегии `2026`.
