# Standard Library Value Proof

Этот документ нужен как короткая внешняя фиксация того, **почему checked-in `core` полезен на практике**.

Речь не о количестве модулей. Речь о том, что baseline standard library уже снимает повторяющийся glue-code в первых meaningful workflow DeltaQ.

## Критерий доказательства

Standard library считается полезной не тогда, когда в ней много элементов, а когда она:

- позволяет собрать первый рабочий сценарий без написания стартовых helper-модулей;
- даёт достаточно сильные примитивы для reusable composition;
- остаётся полезной и в desktop/UI flow, а не только в console demo.

Ниже зафиксированы три checked-in сценария, которые уже это доказывают.

## 1. Minimal Console Flow

Reference project:

- `resources/examples/minimal_console_flow`

Core baseline:

- `core.io.string_constant`
- `core.io.read_line`
- `core.io.println`

Что пользователь получает сразу:

- готовый интерактивный console graph;
- zero custom modules на старте;
- прозрачный generated `src/main.c`, который легко сопоставить с графом.

Что это экономит:

- ручной стартовый ввод/вывод boilerplate;
- одноразовые helper-функции только ради первого runnable flow;
- лишний переход в код до того, как пользователь увидел ценность graph workflow.

## 2. Reusable Composition Console

Reference project:

- `resources/examples/reusable_composition_console`

Core baseline:

- `core.string.str_concat`
- `core.string.str_length`
- `core.conversion.int_to_string`
- `core.io.println`

Что пользователь получает сразу:

- составной модуль `echo_with_prefix`;
- составной модуль `measure_text`;
- повторное использование одного и того же submodule в корневом графе;
- отдельные generated compilation units для подмодулей.

Что это экономит:

- ручные helper-функции для склейки строк и форматирования длины;
- дублирование логики при повторном использовании;
- расползание примера в custom C-код вместо graph-level composition.

## 3. Desktop UI Flow

Reference project:

- `resources/examples/desktop_ui_flow`

Core baseline:

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

Что пользователь получает сразу:

- готовый graph-level SDL2 lifecycle;
- рабочую связку `graph + .dqui + generated UI runtime`;
- runtime path, который можно показать без ручной сборки event loop с нуля.

Что это экономит:

- повторяющийся SDL2 init/shutdown boilerplate;
- ручную склейку window/renderer/UI state lifecycle;
- перегрузку desktop example низкоуровневыми деталями вместо пользовательского сценария.

## Вывод

Эти три сценария уже закрывают минимальный value proof для checked-in `core`:

- standard library ускоряет первый runnable console flow;
- standard library служит хорошим baseline для reusable composition;
- standard library остаётся полезной в desktop/UI сценарии.

Именно так сейчас нужно показывать ценность `core` внешнему пользователю:

- через `example -> graph -> generated code -> run`;
- а не через абстрактное утверждение, что в репозитории есть `43` модуля.

## Граница утверждения

Этот proof не означает, что `core` должен разрастаться дальше.

Наоборот:

- `core` остаётся компактным curated baseline;
- imported pack-и расширяют экосистему наружу;
- project modules и submodules закрывают локальную прикладную логику.
