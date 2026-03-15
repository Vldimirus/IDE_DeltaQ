# Stage 3 Blocker Shortlist

Дата среза: `2026-03-15`

Этот документ ведёт рабочий blocker-shortlist для `Stage 3: Bug Burn-Down For Trust`
из `32_product_maturity_recovery_v1.md`.

Правила:

- сюда попадают только top-level user-facing дефекты, которые реально подрывают trust;
- статус `closed` ставится только при наличии regression test или reproducible smoke-check;
- дефекты "на всякий случай" не держатся здесь как blocker без явного текущего repro.

## Critical Gate Status

- Подтверждённых открытых `crash`-блокеров для top-level Linux flow
  `create/open project -> edit -> save -> build -> run`: `нет`.
- Подтверждённых открытых `data-loss`-блокеров для того же flow: `нет`.
- Подтверждённых открытых `editor/runtime mismatch`-блокеров для того же flow: `нет`.
- На текущем срезе shortlist состоит из trust/polish дефектов, а не из flow-breaking аварий.
- Открытых blocker-item-ов в shortlist на срезе `2026-03-15`: `нет`.

## Shortlist

| ID | Severity | Surface | Status | Проблема | Доказательство | Закрытие / next action |
| --- | --- | --- | --- | --- | --- | --- |
| `S3-01` | `high` | public status / first-contact trust | `closed` (`2026-03-15`) | `docs/PROGRESS.md` выглядел как текущий source of truth с `~97%` и "всё завершено", хотя current product plan всё ещё находится в maturity recovery. | Старый верх документа и audit notes в `docs/reports/project_state_2026-03-09*.md` расходились со strategy-level acceptance. | **Reproducible smoke-check:** открыть `docs/PROGRESS.md`; expected: документ явно помечен как исторический журнал, верхний блок ведёт к `32_product_maturity_recovery_v1.md`, `33_bug_burn_down_shortlist.md` и audit docs, а не выдаёт phase snapshot за текущую продуктовую оценку. |
| `S3-02` | `medium` | public status / stale known issues | `closed` (`2026-03-15`) | В `docs/PROGRESS.md` оставался открытым уже исправленный `zoomFit` bug, что визуально создавало ложную картину активного blocker-а. | Старый раздел `Известные проблемы` противоречил уже закрытому шагу 62 и regression coverage в `tests/blockEditor/test_BlockEditorWidget.cpp`. | **Reproducible smoke-check:** открыть `docs/PROGRESS.md`; expected: старый open `zoomFit` item больше не числится активной проблемой, а текущий blocker tracking вынесен в Stage 3 shortlist. |
| `S3-03` | `medium` | desktop generated runtime / first-contact trust | `closed` (`2026-03-15`) | Generated SDL2 event source scaffold-ился через raw `TODO` handler bodies и placeholder comments. Это не ломало build/run flow, но делало fresh desktop path менее product-grade. | `src/uiDesigner/SDL2CodeGenerator.cpp` теперь генерирует one-time stub log без raw `TODO`, а `tests/uiDesigner/test_SDL2CodeGenerator.cpp` закрепляет и новый scaffold, и отсутствие placeholder comment для unbound button path. | **Automated regression test:** `QT_QPA_PLATFORM=offscreen ./build/qt-dev/tests/test_SDL2CodeGenerator` должен проходить; expected: `eventsSource` содержит `dq_ui_generated_event_stub_once(...)`, не содержит `TODO`, а `uiSource` не вставляет `TODO: onClick handler` для кнопки без event binding. |
