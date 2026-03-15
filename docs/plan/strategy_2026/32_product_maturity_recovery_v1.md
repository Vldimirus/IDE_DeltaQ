# Product Maturity Recovery v1

## Purpose

Зафиксировать следующий главный execution-track для DeltaQ:

- довести `desktop/UI`-контур до предсказуемого состояния;
- перестать опираться на слабые тестовые шаблоны как на "готовые стартовые проекты";
- усилить практическую ценность среды через первую полезную библиотеку модулей;
- вернуть в активную работу идею `external library -> DeltaQ module pack`, но в реалистичном `v1`-scope;
- закрыть накопившийся слой мелких, но токсичных UX/flow-дефектов;
- только после этого заново принимать решение о release framing.

Этот документ не отменяет:

- `21_standard_library_strategy.md`;
- `27_library_converter_and_module_packs.md`;
- `29_linux_release_polish.md`;

Он задаёт **порядок и практический приоритет**, в котором эти идеи должны
доводиться до продуктового состояния.

## Why Now

Сейчас DeltaQ уже умеет:

- создавать проект;
- собирать generated code;
- собирать Linux bundle самой IDE;
- экспортировать Linux bundle пользовательского приложения.

Но пользовательская картина всё ещё неровная:

- `UI Designer` даёт мелкие, но критичные расхождения между редактором и runtime;
- desktop-шаблоны выглядят как исторические тестовые заготовки, а не как product-grade starters;
- standard library пока сильнее как набор базовых примитивов, чем как библиотека для реальных прикладных задач;
- есть заметные UX-шероховатости, которые подрывают доверие к среде сильнее, чем отсутствие ещё одной новой фичи.

Коротко: у DeltaQ уже сильное ядро, но прикладная и пользовательская зрелость
отстают от инфраструктуры.

## Strategic Decision

На ближайший цикл фиксируется следующий приоритет:

1. `UI / desktop hardening`
2. `template overhaul`
3. `bug burn-down` по самым токсичным пользовательским дефектам
4. `core module library v1`
5. `external library adapter v1`
6. `release framing`

Что пока **не** является главным приоритетом:

- новые платформы;
- новый build system помимо `CMake`;
- широкий маркетинговый release narrative без продуктовой базы;
- попытка сразу поддержать любые `C++`/`DLL`-случаи без доказанного `C ABI` baseline.

## Target State

Нужен не "ещё один polished demo", а следующий реальный результат:

1. пользователь создаёт desktop-проект и получает не тестовую болванку, а
   разумный стартовый каркас;
2. размер окна, свойства окна и runtime-поведение совпадают между editor и build output;
3. в стандартной библиотеке есть не только базовые primitive-модули, но и первый
   useful baseline для прикладной разработки;
4. внешнюю `C`-библиотеку можно не вручную встраивать в произвольный `CMake`, а
   оформлять в DeltaQ как понятный module pack;
5. ключевые UX-шероховатости перестают ломать доверие к продукту;
6. после этого можно честно оценить, готов ли проект к следующему Linux release framing.

## Scope

Этот track **включает**:

- `UI Designer` hardening;
- пересборку desktop templates;
- curated baseline для useful core-модулей;
- first practical flow для external library adaptation;
- целевой bug burn-down по top-level UX и build/design flows;
- повторную оценку release positioning после закрытия этих блокеров.

Этот track **не включает**:

- Windows/macOS delivery;
- installer/package-manager frontend;
- обещание "автоматически импортировать любую DLL";
- полную переработку всего UI-среды разом;
- бесконтрольное разрастание `core` за счёт узкоспециализированных SDK.

## Definition Of Done Rules

Этот master-plan должен соответствовать правилам из `23_acceptance_criteria.md`:

- критерий закрытия stage-а должен быть измеримым или хотя бы однозначно проверяемым;
- формулировки уровня "ощущается", "выглядит", "воспринимается" допустимы только
  как motivation, но не как единственный acceptance gate;
- если stage меняет templates, `.dqui`, `core` или imported-pack flow, stage не
  считается закрытым без regression-proof для затронутых checked-in examples;
- если stage меняет формат, metadata или runtime contract, должно быть явно сказано:
  - нужен ли migration path;
  - какие legacy artifacts обязаны остаться зелёными без ручной правки.

Для этого документа фиксируются общие правила закрытия stage-а:

1. есть явный список целевых изменений;
2. есть regression coverage:
   - automated test, либо
   - reproducible smoke/checklist, если automation пока нецелесообразна;
3. нет открытых unresolved blockers, противоречащих acceptance этого stage-а;
4. затронутые reference examples остаются зелёными по матрице ниже.

Дополнительное no-regression правило для maturity-cycle:

- уже закрытые delivery tracks не считаются "вне зоны ответственности";
- если stage затрагивает desktop/UI/templates/runtime/build flow, он не считается
  закрытым без повторной проверки:
  - `Build -> Export Linux Bundle` для console и desktop;
  - Linux release smoke как минимум на уровне:
    - `first-run`;
    - `open example -> build -> run`.

## Regression And Migration Matrix

Этот track опирается на уже принятый regression/reference слой из
`22_examples_and_reference_projects.md`.

Минимальная обязательная матрица:

- `resources/examples/minimal_console_flow`
  - должен оставаться зелёным на всех stage-ах как baseline `graph -> code -> build -> run`;
- `resources/examples/desktop_ui_flow`
  - обязателен для `Stage 1`, `Stage 2`, `Stage 3` и `Stage 6`;
- `resources/examples/reusable_composition_console`
  - обязателен для `Stage 4`, чтобы усиление useful baseline не ломало reuse-story;
- `resources/examples/imported_pack_sensor_console`
  - обязателен для `Stage 5` как hardware-like imported-pack reference;
- `resources/examples/imported_pack_checksum_console`
  - обязателен для `Stage 5` как algorithmic/text-processing imported-pack reference.

Обязательные delivery regression gates:

- release bundle smoke:
  - `first-run`
  - `open example -> build -> run`
- release/export smoke:
  - `minimal_console_flow -> Export Linux Bundle`
  - `desktop_ui_flow -> Export Linux Bundle`

Эти gates обязательны минимум для `Stage 1`, `Stage 2`, `Stage 3`, `Stage 6` и
для любого изменения, которое трогает desktop runtime, templates, generated UI
или build/export wiring.

Дополнительные правила migration:

- если меняется формат `.dqui`, checked-in examples должны открываться, сохраняться
  и собираться без ручного repair-step;
- если меняются desktop templates, старые checked-in examples не должны
  "тихо устареть": либо они продолжают собираться, либо для них фиксируется явный
  migration note;
- если меняется `core` vocabulary, нужно явно указать:
  - какие legacy modules остаются;
  - что переводится в `legacy`;
  - какие examples/template-ы были обновлены под новый baseline;
- если меняется imported-pack intake/curation flow, существующие reference case-ы
  `mini_sensor_sdk` и `mini_checksum_sdk` не считаются "историей вне игры":
  хотя бы один из них должен быть revalidated новым flow, а второй должен остаться
  buildable как checked-in regression example.

## Track Structure

### Stage 1. UI Designer Hardening

#### Problem

Сейчас `UI Designer` уже usable, но ещё недостаточно надёжен как source of truth:

- размер окна в редакторе и runtime не всегда совпадают;
- resize/свойства окна ощущаются нестабильными;
- часть поведения окна не выражена пользователю явно;
- регрессии здесь особенно токсичны, потому что пользователь видит их сразу.

#### Goal

Сделать desktop/UI-контур предсказуемым:

- what you see in designer ~= what you get at runtime;
- работа со свойствами окна не ломает IDE;
- resize и базовый layout-flow стабильны;
- desktop-generated UI не выглядит как "случайно собранный runtime".

#### Work Items

- выровнять contract между `.dqui`, designer scene и runtime codegen;
- зафиксировать явную модель window size / minimum size / resizable policy;
- убрать скрытые fallback-значения, которые меняют размер окна на build/run;
- проверить сохранение/загрузку desktop window metadata;
- добавить regression cases для window resize, property editing, save/load, runtime parity;
- пройтись по самым заметным runtime/layout-mismatch багам до добавления новых UI-фич.

#### Acceptance

- `resources/examples/desktop_ui_flow` и desktop-template baseline проходят
  `open -> save -> build -> run` без расхождения window size между editor metadata
  и runtime;
- `minimum width/height` и `resizable policy` совпадают между:
  - designer properties;
  - save/load `.dqui`;
  - runtime window behavior;
- изменение window properties не приводит к падению IDE в targeted regression cases;
- resize окна мышью и через properties сохраняет одинаковые width/height values
  после reopen проекта;
- есть regression coverage минимум для:
  - property editing;
  - save/load;
  - runtime parity по window size;
  - runtime parity по `minimum size` и `resizable policy`.

#### Status on 2026-03-15

- первый implementation slice уже приземлён:
  - `DesignScene` получил единый state для `title / min_width / min_height / resizable`;
  - `.dqui` round-trip больше не теряет window-level properties при save;
  - `PropertyEditor` умеет редактировать `min size` и `resizable`;
  - `SDL2CodeGenerator` использует тот же window contract для title/init size/min size/resizable;
  - добавлены regression tests на:
    - `DesignScene` save/load round-trip;
    - geometry clamp by minimum size;
    - generated runtime window contract;
- второй implementation slice уже приземлён:
  - desktop baseline assets (`desktop`, `desktop_text_editor`, `desktop_ui_flow`)
    теперь хранят явный window contract в `.dqui`;
  - graph-generated desktop `main.c` синхронизирован с новым
    `dq_ui_backend_init(..., min_width, min_height, resizable)` contract;
  - release no-regression для `Export Linux Bundle` повторно подтверждён на:
    - `minimal_console_flow`;
    - `desktop_ui_flow`;
- третий implementation slice уже приземлён:
  - `DesignScene` теперь интерпретирует `window.geometry` как runtime client area,
    а desktop chrome рисует поверх отдельного `windowFrameRect`, чтобы designer и
    runtime использовали одну и ту же coordinate/size model;
  - root anchor/layout path в designer переведён на полный client size без старого
    скрытого `title bar`-offset;
  - `PropertyEditor` синхронизирует `width / height / min size / resizable`
    контролы сразу после window property edits и больше не держит устаревшие
    ranges/value после clamp;
  - добавлены regression tests на:
    - designer client/frame model;
    - root anchor parity по полной window height;
    - property-editor sync после minimum-size clamp;
  - release no-regression для `Export Linux Bundle` ещё раз подтверждён на:
    - `minimal_console_flow`;
    - `desktop_ui_flow`;
- четвёртый implementation slice уже приземлён:
  - добавлен explicit regression на mouse-drag resize window handles прямо на
    уровне `DesignScene`, без завязки на неустойчивые viewport-pixel tests;
  - mouse-resize path теперь проверяется не только на изменение geometry в
    памяти, но и на `.dqui` save/reopen через `UILayoutStore`;
  - добавлен parity-case, который сравнивает mouse-clamped resize и property
    resize после reload и подтверждает одинаковые `width / height` значения;
- пятый implementation slice уже приземлён:
  - `PreBuildProcessor` теперь пробрасывает primary window contract из `.dqui`
    в inline desktop graph path, чтобы graph-generated `main.c` не жил на
    отдельно зашитых fallback-значениях;
  - `GraphCompiler` умеет принимать injected desktop window contract и
    использует его для `title / width / height / min_width / min_height / resizable`
    в `dq_ui_backend_init(...)`;
  - desktop baseline и `desktop_ui_flow` получили runtime-level parity checks:
    - `desktop` template prebuild теперь проверяет exact generated init contract;
    - `desktop_ui_flow` build/run и export bundle path проверяют тот же contract
      до реального headless runtime/export verification;
- шестой implementation slice уже приземлён:
  - baseline `desktop` template получил opt-in headless autoclose hook через
    `DQ_DESKTOP_TEMPLATE_AUTOCLOSE_MS`, чтобы user-facing regression мог пройти
    реальный `run`, а не останавливаться на `build`;
  - добавлен `MainWindow` integration regression на baseline flow:
    `open project -> open window1.dqui -> edit window properties ->
    save -> reopen -> build -> run`;
  - regression идёт через реальные `PropertyEditor` controls и подтверждает, что
    baseline window contract после edit/save доходит до:
    - persisted `ui/window1.dqui`;
    - regenerated `src/main.c` с exact `dq_ui_backend_init(...)`;
    - headless runtime с clean exit;
- седьмой implementation slice уже приземлён:
  - baseline `desktop` flow получил отдельный `MainWindow` acceptance proof для
    mouse resize path: реальный drag window handle в designer ->
    save -> reopen -> build -> run;
  - runtime-level proof теперь закрывает оба user-facing resize path:
    - property edit path;
    - mouse resize path;
  - это убирает последний незакрытый runtime-behavior tail для `Stage 1`:
    desktop baseline теперь доказан end-to-end не только на init contract, но и
    на реальных resize workflows от IDE до runtime;
- `Stage 1: UI Designer Hardening` можно считать закрытым:
  - desktop baseline имеет end-to-end proof для property и mouse resize path;
  - designer/codegen/runtime используют единый window contract и regression
    coverage на save/load/resize/property edit;
  - release/export no-regression теперь снова зелёный, но должен оставаться
    обязательным gate после следующих desktop/template изменений;
  - следующий основной фокус можно смещать на `Stage 2: Desktop Template Overhaul`.

### Stage 2. Desktop Template Overhaul

#### Problem

Текущие desktop templates исторически были тестовыми заготовками. Для нового
пользователя это выглядит как слабый продуктовый baseline:

- "Text Editor" не выглядит как настоящий text editor;
- шаблоны не выражают богатые desktop-паттерны;
- они учат примитивам, но не дают сильной стартовой позиции для реальной задачи.

#### Goal

Сделать templates product-grade starters, а не internal fixtures.

#### Work Items

- пересобрать `Desktop Text Editor` под правдоподобный starter workflow;
- добавить в desktop templates реальные паттерны:
  - меню;
  - вкладки;
  - несколько окон/диалогов;
  - панели/контейнеры;
- убрать из шаблонов historical copy-paste из showcase-примеров;
- выровнять graph/UI/code structure между template intent и реальным generated flow;
- описать, какие templates являются:
  - onboarding;
  - showcase;
  - practical starter.

#### Acceptance

- весь user-facing desktop template catalog в репозитории явно классифицирован:
  - `recommended starter`
  - `advanced desktop template`
  - `internal-only / hidden from normal user flow`;
- templates `desktop`, `desktop_text_editor`, `desktop_mdi`, `desktop_empty`
  не остаются в "полуподдерживаемом" состоянии:
  - либо template остаётся user-facing и проходит `create -> pre-build -> build -> run`;
  - либо явно переводится в internal-only/hidden status с зафиксированной причиной;
- создание проекта из `desktop`, `desktop_text_editor` и любого другого template-а,
  который остаётся user-facing как starter/advanced desktop option, даёт полный
  starter layout:
  - `.dqproj`
  - `graphs/main.dqgraph`
  - как минимум один `.dqui`
  - связанные user-editable event/source files;
- каждый desktop template, оставленный user-facing, проходит `pre-build -> build -> run`
  без ручной правки;
- как минимум два desktop-pattern элемента из списка `menu / tabs / dialogs / multi-window basics`
  появляются в checked-in desktop templates, а не только в ad-hoc example code;
- README/release screenshots и onboarding больше не ссылаются на legacy-fixtures
  как на основной desktop starter.

- первый implementation slice `Stage 2` уже приземлён:
  - template catalog получил явный `catalog_role` contract;
  - `New Project` wizard теперь показывает classification для user-facing templates;
  - `desktop_empty` и `desktop_mdi` переведены в `internal_only` и скрыты из
    normal wizard flow с зафиксированной причиной, чтобы user-facing desktop
    catalog перестал показывать слабые raw-SDL fixtures как обычные starters;
  - regression tests закрепляют:
    - visible template order;
    - hidden desktop templates остаются discoverable по ID для internal/test flows;
    - `desktop` и `desktop_text_editor` имеют явную desktop-catalog classification;
- второй implementation slice `Stage 2` закрывает user-facing starter path:
  - `desktop_text_editor` пересобран в notes-workspace starter вместо узкого `Text Pad`
    fixture;
  - checked-in starter теперь содержит реальные desktop-pattern элементы:
    - `MenuBar`
    - `ToolBar`
    - `TabPanel`
    - `StatusBar`;
  - template event hooks больше не тащат legacy-copy о `UI Graph Example`, а
    сразу поддерживают sample/new/append workflow и headless autoclose через
    `DQ_DESKTOP_TEXT_EDITOR_AUTOCLOSE_MS`;
  - regression coverage теперь доказывает для `desktop_text_editor` путь
    `generate -> pre-build -> build -> run` без ручной правки и отдельно
    проверяет presence desktop shell widgets в generated template assets;
- `Stage 2` пока не закрыт полностью:
  - оставался один хвост: explicit cleanup/reframing desktop template names,
    template copy и onboarding README surfaces, чтобы user-facing catalog больше
    не смешивал recommended starter с legacy/showcase wording.
- третий implementation slice закрывает и этот хвост:
  - advanced desktop template переименован из `Desktop UI Graph Example` в
    `Desktop UI Baseline`;
  - baseline template copy больше не использует user-facing wording вида
    `Example` / `Existing` и синхронизирован с advanced-desktop role;
  - README/README_RU и onboarding index теперь явно говорят, что:
    - `Desktop Text Editor` является recommended desktop starter;
    - `Desktop UI Baseline` является advanced desktop option;
    - hidden raw-SDL fixtures не являются normal wizard options;
  - regression coverage закрепляет новый metadata/naming contract для
    user-facing desktop templates;
- `Stage 2: Desktop Template Overhaul` после этого можно считать закрытым.

### Stage 3. Bug Burn-Down For Trust

#### Problem

У DeltaQ уже есть мощные фичи, но доверие к среде разрушают мелкие и повторяемые
дефекты, прежде всего в first-contact product flow:

- неочевидные ошибки build/codegen flow;
- editor/runtime mismatches;
- несогласованные шаблоны;
- мелкие падения и mismatch-ы.

Важно: этот stage не переоткрывает уже закрытый track `31_project_properties_and_toolchain_ux_v1`
целиком. Toolbar/localization/toolchain surfaces попадают сюда только если новые
изменения снова ломают их или если обнаруживается user-facing regression.

#### Goal

Не "вычищать всё подряд", а системно убрать именно те дефекты, которые сильнее
всего подрывают ощущение зрелости.

Этот stage является **поперечным gating-lane**, а не изолированным большим этапом,
который делается только после Stage 2 и только до Stage 4.

#### Work Items

- завести отдельный shortlist блокирующих UX-багов;
- приоритизировать по правилу:
  - first-contact breakage;
  - build/design mismatches;
  - crashes/data-loss risks;
  - only then cosmetic issues;
- вести bug burn-down короткими сериями после заметных изменений в `Stage 1`, `Stage 2`,
  `Stage 4` и `Stage 5`, а не как полностью отдельный waterfall-stage;
- каждый фикс доводить до regression test или reproducible checklist.

#### Acceptance

- существует явный shortlist blocker-багов с severity и статусом;
- в shortlist не остаётся открытых `crash`, `data-loss` или `editor/runtime mismatch`
  дефектов для top-level user flow;
- каждый закрытый blocker из shortlist имеет:
  - automated regression test, либо
  - reproducible smoke-check с явно записанным expected result;
- surfaces из track `31_project_properties_and_toolchain_ux_v1` не деградировали
  в результате текущего maturity-cycle.
- release/export surfaces из закрытых tracks `29/30/31` не деградировали:
  - release bundle по-прежнему проходит `first-run` и `open example -> build -> run`;
  - `Export Linux Bundle` остаётся зелёным для console и desktop reference flow.

#### Current Stage 3 Status

Рабочий blocker-shortlist для этого stage теперь ведётся отдельно в
`33_bug_burn_down_shortlist.md`.

На срезе `2026-03-15` зафиксировано:

- после закрытия `Stage 1` и `Stage 2` в top-level Linux flow не подтверждаются
  открытые `crash / data-loss / editor-runtime mismatch` blockers;
- первые короткие Stage 3 slices закрыли три trust-blocker-а:
  - misleading current-readiness framing;
  - stale open issue про уже исправленный `zoomFit`;
  - generated SDL2 event scaffold с raw `TODO`-body и placeholder comment;
- на текущем срезе explicit shortlist не содержит open blocker-item-ов, но
  `Stage 3` остаётся активным gating-lane для следующих slices после `Stage 4`
  и `Stage 5`.

### Stage 4. Core Module Library v1

#### Problem

Сейчас standard library всё ещё сильнее как библиотека примитивов, чем как
набор модулей для реальных задач.

#### Goal

Собрать первую полезную библиотеку модулей, которая даёт practical value без
ухода в бесконтрольный рост `core`.

#### Decisions

Этот stage не создаёт второй независимый baseline поверх
`21_standard_library_strategy.md`. Он является **следующим исполняемым шагом**
того же решения:

- сначала audit и curation текущего `core`;
- затем gap-analysis относительно useful scenarios;
- только потом targeted expansion.

`v1`-приоритет категорий:

- `filesystem`
- `timers`
- `config/json`
- `process`
- `tcp/udp`
- `serial`

Что сознательно откладывается:

- `USB raw` как первоочередной baseline;
- узкоспециализированные device/SDK-модули внутри `core`.

#### Work Items

- провести gap-analysis текущего `core` относительно baseline из
  `21_standard_library_strategy.md` и useful scenario needs;
- для каждой категории определить:
  - обязательные модули;
  - роли `essential / convenience / specialized / legacy`;
  - expected docs/verification bar;
- сделать 2-3 полезных end-to-end сценария, которые опираются на эти категории;
- выровнять naming, contracts и discoverability новых модулей в IDE;
- не расширять `core` хаотично: каждый модуль должен иметь explicit reason to exist.

#### Acceptance

- `core` проходит обновлённый audit/curation cycle без появления второго
  конкурирующего baseline-документа;
- для категорий `filesystem / timers / config-json / process / tcp-udp / serial`
  явно зафиксирован статус:
  - already covered;
  - added in this cycle;
  - deferred with reason;
- как минимум два checked-in reference scenario используют новые или выровненные
  useful core-модули в реальном `build -> run` flow;
- `resources/examples/reusable_composition_console` остаётся зелёным после изменений в `core`.

#### Current Stage 4 Status

Execution-grade audit для Stage 4 зафиксирован в
`docs/library/core/useful_baseline_audit.md`.

На срезе `2026-03-15` он фиксирует:

- для Stage 4 categories `filesystem / timers / config-json / process / tcp-udp / serial`
  определены:
  - essential baseline slices;
  - граница `convenience / specialized`;
  - общий docs/verification bar;
- `filesystem`, `config-json`, `process`, `timers`, `tcp-udp` и `serial` уже получили checked-in initial baseline:
  - новые `core` modules с explicit curation review;
  - reference examples `settings_file_console`, `process_timer_console`, `transport_probe_console` и `serial_probe_console`;
  - automated `build -> run` proof;
- useful-category baseline portion `Stage 4` можно считать закрытой; дальше
  оставшиеся шаги — это уже consolidation/discoverability, а не новые category gaps.

### Stage 5. External Library Adapter v1

#### Problem

Идея `library -> DeltaQ module pack` является одной из самых сильных отличительных
фич проекта, но пока не доведена до user-facing product flow.

#### Goal

Сделать первый practical path, при котором внешнюю `C`-библиотеку можно быстро
завернуть в пригодный для DeltaQ module pack.

#### Decisions

Для `v1` scope фиксируется жёстко:

- только `C ABI` как baseline;
- Linux-first intake;
- импорт прежде всего `.h` + `.so/.a` + include/link metadata;
- converter обещает не "магически поддержать всё", а дать быстрый raw-to-curated path.

`C++`, сложные callbacks и произвольные `DLL`-сценарии не запрещаются навсегда,
но не входят в первый product claim.

#### Work Items

- не начинать с третьего нового кейса без причины:
  - первично переиспользовать `mini_sensor_sdk` как hardware-like reference;
  - вторично использовать `mini_checksum_sdk` как более простой algorithmic case;
  - новый кейс допустим только если оба существующих принципиально недостаточны
    для product proof;
- довести intake metadata до воспроизводимого flow;
- сформировать raw wrapper pack;
- добавить curation step:
  - hide low-level noise;
  - rename/group modules;
  - добавить doc/role metadata;
- доказать `library -> pack -> graph -> build/run` на одном реальном сценарии;
- покрыть failure UX:
  - unsupported ABI;
  - missing headers/binaries;
  - unresolved import metadata;
- описать ограничения честно, чтобы не обещать impossible automation.

#### Acceptance

- как минимум один из существующих reference case-ов
  `mini_sensor_sdk` или `mini_checksum_sdk` проходит обновлённый flow
  `intake -> raw wrappers -> curated pack -> graph -> build/run`;
- второй checked-in imported-pack example остаётся buildable как regression reference;
- unsupported ABI или неполный import не оставляет "полусломанный pack":
  пользователь получает явный diagnostic с причиной и следующим действием;
- ограничения `C ABI`-only и Linux-first явно видны в user-facing docs/flow.

#### Current Stage 5 Status

На срезе `2026-03-15` Stage 5 уже можно считать закрытой:

- reproducible path `header + binary -> raw wrappers -> curated pack` закреплён
  raw-baseline regression-ами и checked-in walkthrough-ами для
  `mini_sensor_sdk` / `mini_checksum_sdk`;
- checked-in example-проекты
  `imported_pack_sensor_console` и `imported_pack_checksum_console`
  проходят `pre-build -> build -> run` как regression reference;
- `LibraryPackager` и `LibraryImportWizard` теперь жёстко валидируют v1 scope
  до записи pack-а:
  - Linux-first `C ABI` baseline;
  - missing header/binary path;
  - unsupported `C++` ABI;
  - no half-written pack on rejected import;
- user-facing docs/library контур теперь явно проговаривает ограничения adapter-а,
  а не оставляет их как implicit engineering knowledge.

### Stage 6. Release Framing Re-Evaluation

#### Problem

После последних delivery-улучшений DeltaQ уже можно собирать и показывать, но
без strong templates, stable UI flow и useful module baseline релиз легко
переобещать.

#### Goal

Не форсировать "зрелый релиз" раньше времени, а после закрытия предыдущих stage-ов
честно переоценить продуктовую формулировку.

#### Work Items

- после Stage 1-5 сделать новый state review;
- сравнить фактическое состояние с `Phase 2 / Phase 3` acceptance;
- решить, что честнее:
  - `technical preview`;
  - `early usable Linux release`;
  - или полноценный Linux release candidate.

#### Acceptance

- release framing опирается на реальную зрелость продукта, а не на optimism bias;
- release docs и screenshots больше не расходятся с daily-user reality.

#### Current Stage 6 Status

На срезе `2026-03-15` Stage 6 закрыта отдельным state review
`docs/reports/project_state_2026-03-15_release_framing.md`.

Принятое решение:

- текущий честный public label — `Linux release candidate`;
- это решение распространяется только на Linux-first scope;
- Windows/macOS delivery остаются roadmap-направлениями, а не текущим release claim;
- user-facing README/release docs/checklists теперь используют этот label явно,
  вместо конкурирующих расплывчатых формулировок.

## Execution Order

Практический порядок работ фиксируется так:

1. `Stage 1: UI Designer Hardening`
2. `Stage 2: Desktop Template Overhaul`
3. `Stage 4: Core Module Library v1`
4. `Stage 5: External Library Adapter v1`
5. `Stage 6: Release Framing Re-Evaluation`

`Stage 3: Bug Burn-Down For Trust` идёт поперёк этого порядка как gating-lane:

- после каждого заметного среза в `Stage 1`, `Stage 2`, `Stage 4`, `Stage 5`
  выполняется короткий bug burn-down pass;
- `Stage 6` нельзя закрыть, пока blocker-shortlist Stage 3 не вычищен до
  non-blocking остатка.

Причина такого порядка:

- сначала нужно убрать product-visible слабые места в desktop/UI контуре;
- затем перестать показывать пользователю слабые templates;
- затем усилить прикладную ценность библиотеки и library-adaptation story;
- параллельно не позволять bug debt снова разрастаться между крупными stage-ами;
- и уже на более сильной продуктовой базе снова поднимать release framing.

## Tracking Rules

Этот файл становится **главным execution plan** для maturity-cycle.

Прогресс по нему фиксируется в трёх местах:

1. `32_product_maturity_recovery_v1.md` — подробный план и stage-level decisions.
2. `98_strategy_checklist.md` — короткий текущий статус по каждому stage.
3. `99_execution_log.md` — хронология решений и выполненных шагов.

Если какой-то stage разрастётся до самостоятельного implementation-track, для него
создаётся отдельный документ следующего номера, но он остаётся подчинённым этому
master-plan, а не заменяет его.

## Success Criteria

Track можно считать закрытым, когда выполнены все условия:

- `desktop_ui_flow` и desktop-template baseline не имеют открытых size/property
  mismatch blockers;
- checked-in desktop templates создают полный starter-project и проходят
  `pre-build -> build -> run`;
- useful core baseline зафиксирован через audit/gap-analysis и доказан минимум
  на двух checked-in scenarios;
- хотя бы один из checked-in imported-pack reference case-ов проходит полный
  updated adapter flow, а второй остаётся зелёным regression example;
- blocker-shortlist не содержит открытых `crash / data-loss / editor-runtime mismatch`
  дефектов для top-level user flow;
- выполнен отдельный state review с явным release decision:
  `technical preview`, `early usable Linux release` или `Linux release candidate`.
