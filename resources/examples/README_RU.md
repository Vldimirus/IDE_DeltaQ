# Каталог примеров DeltaQ

Эта директория является самой наглядной product surface DeltaQ: каждый example лежит в репозитории как checked-in дерево исходников, а не как синтетическая демка, собранная только логикой IDE.

## Рекомендуемый порядок

1. [minimal_console_flow](minimal_console_flow/)
   Начинать стоит отсюда. Это самый маленький proof для пути `модуль -> граф -> generated C code -> build -> run`.

2. [desktop_ui_flow](desktop_ui_flow/)
   Самый сильный desktop/UI showcase: graph, `.dqui`, generated SDL2 runtime-файлы и живые event handlers.

3. [reusable_composition_console](reusable_composition_console/)
   История про reuse: составные подмодули, отдельные generated units и повторное использование из одного корневого графа.

4. [imported_pack_sensor_console](imported_pack_sensor_console/)
   Первый reference path для imported pack: от внешней библиотеки к curated pack и рабочему graph runtime.

5. [imported_pack_checksum_console](imported_pack_checksum_console/)
   Второй reference path для imported pack из другой доменной зоны, показывающий, что imported pack-и являются главным каналом роста экосистемы, а не ещё одним раздуванием `core`.

## Fixture SDK Trees

Эти директории поддерживают imported-pack showcase-проекты и нужны тогда, когда вы хотите посмотреть vendor side этой истории, а не пройти самый короткий walkthrough DeltaQ:

- [imported_pack_sensor_sdk](imported_pack_sensor_sdk/)
- [imported_pack_checksum_sdk](imported_pack_checksum_sdk/)

## Какой пример отвечает на какой вопрос

- "Что такое DeltaQ за 5 минут?" -> [minimal_console_flow](minimal_console_flow/)
- "Как выглядит UI/designer/runtime path?" -> [desktop_ui_flow](desktop_ui_flow/)
- "Зачем нужны подмодули и reuse?" -> [reusable_composition_console](reusable_composition_console/)
- "Как внешняя библиотека превращается в reusable pack?" -> [imported_pack_sensor_console](imported_pack_sensor_console/) и [imported_pack_checksum_console](imported_pack_checksum_console/)

## Связанные документы

- [Onboarding index](../../docs/onboarding/README_RU.md)
- [Первый запуск DeltaQ](../../docs/onboarding/first_run_ru.md)
- [Linux-first checklist для release artifacts](../../docs/release/linux_first_release_checklist_ru.md)
