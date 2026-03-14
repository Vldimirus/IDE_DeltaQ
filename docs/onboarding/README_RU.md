# Onboarding DeltaQ

Эта папка является самым коротким documentation entry point для нового пользователя DeltaQ на Linux.

## С чего начать

1. [Первый запуск DeltaQ](first_run_ru.md)
   Самый быстрый walkthrough для пути `модуль -> граф -> generated C code -> build -> run`.

2. [Каталог примеров](../../resources/examples/README_RU.md)
   Рекомендуемый порядок checked-in showcase-проектов и пояснение, что именно доказывает каждый пример.

3. [Linux-first checklist для release artifacts](../release/linux_first_release_checklist_ru.md)
   Ручной путь приёмки Linux tarball/AppImage, если вы проверяете уже собранный artifact, а не source checkout.

4. [Checklist для Linux Project Export](../release/linux_project_export_checklist_ru.md)
   Путь handoff-проверки для Linux application artifact, который был собран внутри DeltaQ через `Build -> Export Linux Bundle`.

## Рекомендуемый порядок

Используйте документы в таком порядке:

1. пройдите minimal console walkthrough;
2. откройте каталог examples и выберите следующий более сильный сценарий;
3. если проверяете packaged build, используйте Linux artifact checklist.
4. если передаёте приложение, собранное внутри DeltaQ, используйте checklist для project export.

## Связанные документы

- [Центр library docs](../library/README.md)
- [Release And CI](../release/README.md)
