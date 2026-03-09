# Первый запуск DeltaQ

Это пошаговое руководство даёт самый короткий путь первого знакомства с DeltaQ без погружения во внутреннюю архитектуру.

## Шаг 1. Откройте эталонный проект

Откройте:

- `resources/examples/minimal_console_flow/minimal_console_flow.dqproj`

Этот проект специально маленький: один граф, только core-модули, никакой дополнительной библиотеки и никаких подмодулей.

## Шаг 2. Посмотрите на исходный граф

Откройте:

- `resources/examples/minimal_console_flow/graphs/main.dqgraph`

Что в нём происходит:

1. `string_constant` подаёт строку `Hello from DeltaQ!`;
2. первый `println` печатает приветствие;
3. `read_line` читает одну строку из stdin;
4. второй `println` печатает полученный текст.

Это и есть минимальный путь DeltaQ:

`модуль -> граф -> generated C code -> build -> run`

## Шаг 3. Соберите и запустите проект

В IDE:

1. нажмите `Build`;
2. затем `Run`;
3. введите, например, `DeltaQ`.

Ожидаемый вывод:

```text
Hello from DeltaQ!
DeltaQ
```

## Шаг 4. Проверьте generated code

После `pre-build` и сборки откройте:

- `resources/examples/minimal_console_flow/src/main.c`

Именно этот файл DeltaQ генерирует из `graphs/main.dqgraph`. На этом шаге важно увидеть, что:

- граф не скрывает runtime в проприетарном формате;
- результатом является обычный C-код;
- generated output можно сопоставить с узлами исходного графа.

## Куда идти дальше

После этого руководства переходите к более сильным showcase-примерам:

- `resources/examples/desktop_ui_flow` — показывает desktop/UI путь с generated SDL2 runtime-файлами и живыми обработчиками;
- `resources/examples/reusable_composition_console` — показывает подмодули и реальное повторное использование;
- `resources/examples/imported_pack_sensor_console` — показывает путь `external library -> curated pack -> graph -> build -> run`.

Для полного пути нового пользователя и Linux artifact handoff используйте:

- `docs/onboarding/README_RU.md`
- `resources/examples/README_RU.md`
- `docs/release/linux_first_release_checklist_ru.md`
