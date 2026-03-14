# Linux Release Polish

## Purpose

Зафиксировать **рабочий план следующего этапа** после закрытия основных technical priorities.

Этот файл нужен как короткий execution anchor, если работа над Linux-first release polish
растянется на несколько сессий.

Главная цель этапа:

- довести DeltaQ до состояния цельного Linux-first open-source инструмента;
- убрать user-facing papercuts;
- выровнять release surface, onboarding и examples вокруг одного понятного входа.

## Scope

Этот этап **не** про Windows/macOS delivery.

Он про:

- Linux-first polished release candidate;
- качество первого пользовательского опыта;
- coherence между IDE, examples, docs и release artifacts.

## Work Items

### 1. Block Editor Papercuts

- `[x]` Исправить `zoomFit`, чтобы малое число узлов не давало чрезмерный auto-zoom
- `[ ]` Проверить и при необходимости дочистить pan/zoom UX после фикса

Acceptance:

- `Fit` и первый auto-fit не увеличивают маленький граф до visually oversized состояния;
- поведение закреплено automated test-ом.

### 2. Clean Linux First-Run Path

- `[x]` Пройти путь `release/AppImage -> launch -> open example -> build -> run`
- `[x]` Зафиксировать найденные papercuts и исправить их

Acceptance:

- новый пользователь может пройти shortest path без внутреннего контекста.

### 3. Public Release Surface

- `[x]` Выстроить единый narrative между `README`, onboarding, examples и release docs
- `[x]` Добавить user-facing release checklist для Linux artifacts
- `[x]` Подготовить screenshots / visual proof для README или release docs

Acceptance:

- DeltaQ читается как один продукт, а не набор подсистем;
- Linux artifact можно показать внешнему пользователю без длинных устных пояснений.

### 4. Final Phase 3 Closure

- `[x]` После polish пересверить `98_strategy_checklist.md`
- `[ ]` Закрыть пункт `Проект выглядит как целостный инструмент, а не набор несвязанных подсистем`

Acceptance:

- финальный open пункт `Phase 3 / External Trust` закрыт честно, а не декларативно.

## Recommended Order

1. `zoomFit` и другие явные UX-bugs
2. clean first-run / clean-machine smoke
3. public release surface and screenshots
4. final strategy sync

## Progress Notes

- `2026-03-09`: этап создан как отдельный рабочий план.
- `2026-03-09`: `zoomFit` bug в block editor исправлен и закреплён widget-test-ом.
- `2026-03-09`: добавлен и локально подтверждён `smoke_linux_first_run.sh`; release bundle теперь проверяется через isolated `DELTAQ_HOME`, но walkthrough `open example -> build -> run` из release artifact ещё остаётся открытым.
- `2026-03-09`: добавлен и локально подтверждён `smoke_linux_example_build_run.sh`; release bundle теперь проверяет реальный путь `copied example -> open in IDE -> build -> run`, но AppImage-specific handoff в writable workspace ещё остаётся открытым.
- `2026-03-09`: `verify_appdir.sh` усилен runtime-smoke-ами; AppDir и extracted `.AppImage` теперь проходят те же `first launch` и `example build/run` проверки, что и обычный release bundle.
- `2026-03-09`: `build_appimage.sh` исправлен по двум реальным AppImage-papercut-ам: рекурсивный `appstreamcli` wrapper больше не подвешивает `appimagetool`, а в AppImage теперь явно кладётся `libqoffscreen.so`, поэтому freshly built `.AppImage` локально проходит `verify_appimage_file.sh` вместе с post-extract first-run и example smoke.
- `2026-03-09`: добавлены `docs/onboarding/README*`, `resources/examples/README*` и `docs/release/linux_first_release_checklist*.md`; README, onboarding, examples и release docs теперь связаны единым user-facing entry path, а открытым хвостом public surface остаётся уже только visual proof.
- `2026-03-14`: screenshots / visual proof добавлены в `README.md`, `README_RU.md` и `docs/release/README.md`; public release surface теперь показывает не только claims, но и реальный IDE/export flow.
