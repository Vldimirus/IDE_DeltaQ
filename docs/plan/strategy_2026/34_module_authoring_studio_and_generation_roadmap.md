# Module Authoring Studio And Generation Roadmap

## Purpose

Зафиксировать следующий крупный development-track DeltaQ после закрытия
`32_product_maturity_recovery_v1.md`.

Этот документ объединяет три направления, которые логически связаны между собой:

1. новый полноценный редактор модулей;
2. расширение полезной библиотеки модулей через domain-pack-и;
3. развитие автоматической генерации модулей из исходников, библиотек и,
   в дальнем горизонте, из машинного кода.

## Why Now

После maturity recovery у DeltaQ уже есть сильный Linux-first workflow:

- `module -> graph -> generated C -> build -> run`;
- desktop/UI path;
- useful `core` baseline;
- imported pack `v1`;
- Linux release candidate framing.

Следующий multiplier должен усиливать не ещё один isolated showcase, а саму
центральную инженерную единицу DeltaQ: **модуль**.

Если новый цикл не даст сильный authoring/verification path для модуля, то:

- библиотека будет расти быстрее, чем способность её качественно создавать;
- imported/generate flow будет порождать "сырые" модули без понятного trust-path;
- учебная ценность DeltaQ останется недоиспользованной.

## Position In Overall Strategy

Этот roadmap не должен читаться как isolated plan.

Его место в общем execution-order сейчас такое:

1. `29_linux_release_polish.md` — закрыт
2. `30_linux_project_export_v1.md` — закрыт
3. `31_project_properties_and_toolchain_ux_v1.md` — закрыт
4. `32_product_maturity_recovery_v1.md` — закрыт
5. `33_bug_burn_down_shortlist.md` — закрыт как trust/burn-down cycle текущего этапа
6. `34_module_authoring_studio_and_generation_roadmap.md` — **текущий активный track**

Текущая позиция внутри самого `34_*`:

- закрыт `A0`;
- закрыт базовый `A1/A2-lite` layer;
- закрыт настоящий `A2`:
  - richer contract editing
  - diagnostics/LSP-grade workflow
- закрыт `A3 verification workspace`.
- закрыт `A4.0 trace event schema`.
- закрыт `A4.1 trace viewer`.
- закрыт `A4.2 stepping controls`.
- закрыт `B1/B2`:
  - minimum useful SQLite wrapper set checked in
  - curated official SQLite surface added
  - first forcing-function example added
- закрыт `B3/B4`:
  - desktop-side SQLite example
  - reuse of Module Studio verification path for SQLite authoring
- закрыт `A5`:
  - fragment scratchpad поверх уже существующих authoring/verification/trace слоёв
  - promotion в normal `.dqmod` через тот же Module Studio surface
- текущая ближайшая цель — `Track C / Stage C1.1`:
  - reproducible `C` source intake через manifest / `compile_commands.json`

Где смотреть статус:

- общий порядок треков: `00_master_strategy.md`
- краткий cross-track checklist: `98_strategy_checklist.md`
- хронология шагов: `99_execution_log.md`
- локальный stage-by-stage checklist: `Execution Checklist` ниже

## Core Decisions

### 1. First Priority Is Module Authoring Studio

Главный следующий приоритет:

- не database pack;
- не binary reverse pipeline;
- а **отдельная система authoring/edit/verify/trace для модулей**.

Текущий `Module Manager` должен остаться менеджером каталога, состояния и curation,
но не быть главным рабочим местом для написания сложного модуля.

### 2. Nursery Is Not A Debugger Clone

`Ясли` (`Nursery`) — это не просто ещё один thin wrapper над runtime-debugger.

Это отдельный режим:

- для одного модуля или code fragment;
- с явным вводом входных параметров;
- с пошаговым показом хода алгоритма;
- с отображением значений переменных, ветвлений и итоговых выходов;
- пригодный и для module verification, и для обучения.

То есть это **algorithm trace / execution explanation workspace**, а не обычный
`debug session` всей программы.

### 3. SQLite Should Not Bloat `core`

SQLite как направление выбрано правильно, но не как расширение `modules/core`.

Правильная модель для него:

- **official curated pack** или official imported-pack based pack;
- checked-in, документированный, verified;
- но не часть компактного `core` baseline.

Это важно, чтобы не разрушить уже выстроенную границу:

- `core` — компактный checked-in baseline;
- domain-specific vocabulary — через curated pack-и.

### 4. Automatic Generation Must Grow Slower Than Verification

Нельзя наращивать auto-generation быстрее, чем развивается trust model.

Любой модуль, полученный автоматически, не должен сразу восприниматься как
"нормальный production module".

Для generated/imported module path нужна явная лестница статусов:

- `draft`
- `generated_raw`
- `generated_unverified`
- `smoke_passed`
- `curated`
- `verified`

Техническое имя статусов может отличаться, но сама модель должна быть именно такой:
**происхождение и уровень доверия должны быть user-visible**.

### 5. Source/Library Pipelines Are Product Track; Machine-Code Pipeline Is Research Track

Нужно жёстко разделять:

- practical product track:
  - source-to-modules;
  - library-to-modules;
- research track:
  - machine-code -> decompile -> module.

Третье направление не должно ломать ближайший roadmap и не должно мешать first
usable slices.

## Definition Of Done Rules

Любой stage в этом roadmap считается закрытым только если одновременно выполнены
все условия:

- есть user-visible surface, а не только internal helper code;
- есть explicit acceptance proof, а не только "ручное впечатление";
- нет silent schema drift для `.dqmod`;
- сохранены existing checked-in examples/templates, которые не должны ломаться
  от нового stage;
- если stage environment-sensitive, это явно отмечено в tests/docs, а не
  скрыто внутри flaky harness.

Для planning-цикла вокруг authoring/generation это особенно важно, потому что:

- новые metadata-поля легко расползаются в ad-hoc `metadata`;
- auto-generation легко производит сырые артефакты быстрее, чем проект успевает
  ввести quality bar;
- trace/verification surface может быть эффектным, но не интегрированным в
  реальный module lifecycle.

## Regression And Migration Matrix

Любой stage этого roadmap не должен ломать следующие checked-in поверхности:

### Existing `.dqmod` Classes

- checked-in `modules/core/**/*.dqmod`
- local project modules в `project/dqmods/`
- imported pack modules
- composite modules
- UI contract modules

### Existing Product Proofs

- user-facing templates:
  - `console`
  - `console_counter`
  - `desktop`
  - `desktop_text_editor`
- checked-in examples:
  - `minimal_console_flow`
  - `settings_file_console`
  - `process_timer_console`
  - `transport_probe_console`
  - `serial_probe_console`
  - `desktop_ui_flow`
  - `reusable_composition_console`
  - `imported_pack_sensor_console`
  - `imported_pack_checksum_console`

### Migration Rules

- schema change в `.dqmod` требует:
  - backward read compatibility;
  - stable save behavior;
  - explicit migration rules;
  - regression coverage хотя бы на load/save round-trip.
- новый trust/verification surface не должен жить только в новом editor:
  минимум краткое состояние должно читаться и в `Module Manager`.

## Shared Intake Manifest Principle

Tracks `C1` и `C2` не должны развиваться как два разных мира.

Для обоих нужен общий repeatable artifact:

- `intake manifest`

Минимум он должен уметь выражать:

- source files / headers / libraries;
- include dirs;
- defines;
- language standard;
- file filters / symbol filters;
- output pack naming;
- provenance root;
- import timestamp / tool version.

Для source-intake допустимый shortcut:

- `compile_commands.json`, если он может быть детерминированно свёрнут в такой manifest.

Для library-intake:

- wizard может собирать manifest интерактивно,
  но повторный import должен идти уже из saved manifest-а.

## Execution Checklist

Этот roadmap должен обновляться не только narrative-статусами, но и явными
checkbox-ами по execution-пунктам.

Текущее правило:

- после каждого закрытого slice обновлять и `Slice Status`, и checklist ниже;
- если stage начат, но acceptance закрыт не полностью, пункт остаётся `[ ]`,
  а фактический partial-progress описывается в скобках.

### Track A. Module Authoring Studio

- `[x]` Stage A0: canonical `.dqmod` schema v2 для `provenance / trust_state / verification`
- `[x]` Dedicated `.dqmod` open path больше не уводит пользователя в graph redirect
- `[x]` Есть отдельный `ModuleEditorWidget` как новый authoring surface
- `[x]` Для `.dqmod` surface уже есть большой source area и базовая editor parity `(modified tab title, undo/redo, find/replace, go to line)`
- `[x]` Source-derived contract preview виден прямо в module editor
- `[x]` Есть rich contract editing, а не только preview из текущей сигнатуры
- `[x]` Есть diagnostics/LSP-grade workflow parity для `ModuleEditorWidget`
- `[x]` Stage A3: есть verification workspace с saved scenarios и explicit rerun path
- `[x]` Stage A4.0: trace event schema для Nursery
- `[x]` Stage A4.1: trace viewer
- `[x]` Stage A4.2: stepping controls
- `[x]` Stage A5: fragment scratchpad

### Track B. Official SQLite Pack

- `[x]` Stage B1: определён и реализован minimum useful SQLite module set
- `[x]` Stage B2: SQLite pack оформлен как curated official surface
- `[x]` Stage B3: есть checked-in example scenarios для console и desktop flows
- `[x]` Stage B4: Module Studio / Verification path реально используется для SQLite authoring

### Track C. Automatic Module Generation Expansion

- `[ ]` Stage C1.1: reproducible `C` source intake через manifest / `compile_commands.json`
- `[ ]` Stage C1.2: путь `source tree -> raw modules -> curated pack`
- `[ ]` Stage C1.3: ограниченный practical `C++` intake
- `[ ]` Stage C2.1: repeatable `header + binary` intake `v2`
- `[ ]` Stage C2.2: binary metadata assist без over-claim
- `[ ]` Stage C3.1: отдельный feasibility memo для machine-code research track

## Track A. Module Authoring Studio

### Problem

Сейчас модуль можно редактировать только внутри `ModuleManagerWidget`, где:

- кодовый редактор тесный и вторичен по отношению к catalog/review UI;
- verification path ограничен `compile` / `test harness`;
- нет нормального authoring workflow для нетривиального модуля;
- нет отдельного trace-представления того, как модуль работает внутри.
- `.dqmod` сейчас вообще не открывается как отдельный editor surface:
  `MainWindow` пытается найти граф, где этот модуль используется, и уводит
  пользователя в graph tab вместо нормального module editor-а.

Это делает module authoring слабее, чем уже достигнутая зрелость graph/UI/release path.

### Goal

Создать отдельную полноценную authoring-систему для модулей:

- удобную для написания кода;
- удобную для редактирования контракта и metadata;
- удобную для compile/test/trace;
- пригодную как engineering workspace и как учебный режим.

### Stage A0. Module Metadata / Trust Schema v2

Нужно:

- перестать опираться только на:
  - `compileStatus`
  - `testStatus`
  - ad-hoc `metadata`;
- ввести явную schema v2 для `.dqmod`, где разнесены три разные оси:
  1. `origin / provenance`
  2. `trust_state`
  3. `verification results`

Минимальная модель:

- `provenance`
  - source kind: `manual`, `generated_from_source`, `generated_from_library`,
    `generated_from_binary`, `core`, `imported_pack`, `graph`, `ui_contract`
  - source files / symbol / line range / manifest id
- `trust_state`
  - `draft`
  - `generated_raw`
  - `generated_unverified`
  - `smoke_passed`
  - `curated`
  - `verified`
- `verification`
  - compile result
  - test result
  - scenario refs
  - `last_verified_at`
  - optional trace artifact refs

Migration rules:

- existing `.dqmod` with only `compileStatus` / `testStatus` must still load;
- old fields may remain as compatibility layer during transition,
  but canonical editor/runtime path must read/write schema v2;
- migration must not break checked-in `core`, imported packs or project-local modules.

Acceptance:

- `.dqmod` schema can express saved scenarios, provenance, trust state and verification history without hiding everything in free-form metadata;
- old `.dqmod` files continue to load correctly;
- `Module Manager` and future `Module Studio` read the same trust/provenance model.

### Stage A1. Separate Module Studio Shell

Нужно:

- выделить новый top-level surface `Module Studio` / `Module Editor`;
- оставить `Module Manager` как catalog/discovery/curation view;
- дать открытие `.dqmod` в отдельном редакторе по двойному клику/команде;
- поддержать editing tabs для нескольких модулей.

Expected UI structure:

- `Overview`
- `Contract`
- `Source`
- `Includes / Metadata`
- `Verification`
- `Nursery`

Acceptance:

- пользователь редактирует модуль не в "узком поле внутри менеджера", а в отдельном рабочем пространстве;
- `.dqmod` открывается и сохраняется через новый editor flow;
- `Module Manager` остаётся центром каталога, но перестаёт быть главным authoring surface.

### Stage A2. Full Source Editor For Modules

Нужно:

- переиспользовать полноценный code editor stack вместо маленького preview-edit поля;
- поддержать normal editing workflow:
  - font/indent/search;
  - syntax highlight;
  - diagnostics summary;
  - generated function signature hints;
- развести:
  - code body;
  - contract/ports;
  - documentation metadata;
  - verification state.

Acceptance:

- сложный atomic module можно комфортно написать и читать внутри DeltaQ;
- source editor по качеству ближе к обычному code workspace, а не к properties panel;
- модульные metadata не смешиваются хаотично с кодом.

### Stage A3. Verification Workspace v1

Нужно:

- сделать явный экран verify path для одного модуля;
- ввести сценарии входных данных, а не один ad-hoc запуск;
- показывать:
  - compile result;
  - test scenarios;
  - expected vs actual outputs;
  - last verification timestamp;
  - trust level.

Минимальный scope:

- ручной ввод входных параметров;
- сохранение 2-3 test scenarios рядом с модулем;
- re-run compile/test без ухода из module editor.

Acceptance:

- отдельный модуль можно проверить локально, не вставляя его в граф;
- verification result остаётся привязан к модулю, а не теряется как временный run;
- user-facing trust model читается прямо в editor.

### Stage A4. Nursery / Algorithm Trace v1

Этот stage сознательно разбит на три подэтапа, чтобы не смешивать
infrastructure, viewer и UX-контролы в один слишком большой slice.

#### Stage A4.0. Trace Event Schema

Нужно:

- ввести минимальную trace event model для одного module execution:
  - step index
  - source location
  - event kind: `enter`, `assign`, `branch`, `loop_iter`, `call`, `return`
  - variable snapshot delta
  - output snapshot
- определить формат trace artifacts и связь с scenario/verification.

Scope:

- только deterministic single-module trace;
- unsupported constructs должны давать explicit "trace unavailable for this construct",
  а не молчаливую симуляцию.

Acceptance:

- trace engine выдаёт устойчивую последовательность step events;
- schema годится и для viewer, и для saved review artifacts.

#### Stage A4.1. Trace Viewer

Нужно:

- построить viewer поверх trace events;
- показывать:
  - текущий шаг;
  - текущую ветку;
  - локальные переменные;
  - входы;
  - выходы/return.

Acceptance:

- пользователь может просмотреть trace module execution даже до добавления rich stepping controls;
- viewer уже пригоден для explanatory/teaching mode.

#### Stage A4.2. Stepping Controls

Нужно:

- добавить controls:
  - `Step`
  - `Step Over`
  - `Run To End`
  - `Restart`
- связать их с saved scenarios из verification workspace.
- для single-module flow дать по-настоящему usable stepping UX,
  а не только статический trace log.

Важное scope-решение:

- `v1` Nursery не обязана понимать весь язык C;
- первый practical scope:
  - одна функция модуля;
  - locals;
  - assignments;
  - arithmetic;
  - `if/else`;
  - `for/while`;
  - calls к известным helper-функциям или к explicit supported subset.

Реализационный подход для `v1`:

- не строить сразу "полный C interpreter";
- сначала сделать trace engine через instrumentation / transformed execution,
  который даёт deterministic step events;
- unsupported constructs показывать явно, а не симулировать их молча.

Acceptance:

- пользователь может ввести входы модуля и пройти алгоритм по шагам;
- IDE показывает, как меняются локальные значения и почему получается конкретный output;
- режим реально полезен и для debugging-like понимания, и для обучения.

### Stage A5. Fragment Scratchpad

Этот stage идёт **после** первого real-world forcing function на Module Studio
через SQLite pack, а не раньше него.

Причина:

- сначала Studio должен доказать ценность на реальном pack-authoring сценарии;
- только потом имеет смысл строить более свободный sketch-mode.

Нужно:

- дать режим для временного кода без немедленного оформления в полноценный модуль;
- пользователь может набросать функцию/процедуру и прогнать её через тот же Nursery;
- после удачного результата fragment можно превратить в нормальный `.dqmod`.

Это важный шаг, потому что инженер часто сначала думает "кусочком алгоритма", а не
готовым модулем.

Acceptance:

- fragment запускается в trace-среде без создания полного project module;
- successful fragment можно promoted в normal module;
- учебный режим и engineering sketch-mode используют один и тот же trace engine.

## Track B. Official SQLite Pack

### Problem

После усиления `core` следующая полезная библиотечная ценность должна приходить не
через раздувание baseline, а через сильные domain-pack-и.

SQLite — хороший первый кандидат:

- практически полезен;
- хорошо тестируется;
- детерминирован;
- подходит и для console, и для desktop scenarios.

### Goal

Сделать первый official database pack на базе SQLite с clear verification story.

### Scope Decision

SQLite не добавляется в `modules/core`.

Правильный target:

- checked-in official pack;
- с docs, examples и verification;
- возможно построенный через improved imported-pack path, если это укрепляет
  adapter story.

User-facing surface decision:

- checked-in pack должен включать оба слоя:
  - thin wrappers как audit/authoring substrate;
  - curated adapters / entry modules как основной user-facing palette surface;
- raw/thin wrappers не должны быть главным пользовательским входом в palette.

### Stage B1. SQLite Pack Baseline

Нужно:

- зафиксировать minimum useful module set:
  - `sqlite_open`
  - `sqlite_close`
  - `sqlite_exec`
  - `sqlite_prepare`
  - `sqlite_bind_int`
  - `sqlite_bind_text`
  - `sqlite_step`
  - `sqlite_column_int`
  - `sqlite_column_text`
  - `sqlite_finalize`

Плюс нужно явно решить, какие модули являются:

- thin wrappers;
- curated entry modules;
- optional helper/adapters.

Acceptance:

- можно собрать shortest useful scenario `open -> create table -> insert -> select -> print`.

### Stage B2. Verification And Curation

Нужно:

- оформить docs/roles/limitations;
- скрыть сырой низкоуровневый noise, если он есть;
- пометить recommended entry modules и helper modules;
- добавить статус verification для pack-а и отдельных modules.

Acceptance:

- pack читается как curated official database surface, а не как россыпь thin wrappers.

### Stage B3. Example Scenarios

Нужно:

- минимум два checked-in examples:
  - `sqlite_settings_console`
  - `sqlite_notes_desktop` или аналогичный desktop flow;
- examples должны подтверждать:
  - schema creation;
  - CRUD baseline;
  - deterministic build/run.

Acceptance:

- SQLite pack доказывает практическую ценность не на одном synthetic test, а на
  двух разных workflows.

### Stage B4. Nursery/Verification Reuse

Нужно:

- использовать новый Module Studio для локальной проверки SQLite adapter-модулей;
- показать, что новый authoring/verification path реально помогает строить
  domain-pack, а не существует отдельно.

Acceptance:

- новый editor и новый pack усиливают друг друга, а не развиваются как два
  несвязанных трека.

## Track C. Automatic Module Generation Expansion

## C1. Source-To-Modules

### Goal

Уметь брать source tree и извлекать из него module candidates.

### Scope Order

Порядок строго такой:

1. `C`
2. ограниченный `C++`
3. затем только при отдельном решении:
   - Python
   - assembler

### Stage C1.1. C Source Intake v1

Нужно:

- intake `.c/.h` source set;
- выделение public/free functions;
- extraction contract из signature;
- repeatable build context:
  - include dirs;
  - defines;
  - language standard;
  - generated headers;
  - file/symbol filters;
- обязательный reproducibility input:
  - либо `compile_commands.json`,
  - либо saved intake manifest;
- сохранение provenance:
  - source file
  - symbol
  - line range

Acceptance:

- из обычного C source tree можно получить reproducible raw module set;
- повторный прогон даёт стабильный result;
- generated modules получают статус `generated_unverified`.

### Stage C1.2. C Source To Curated Pack

Нужно:

- поверх raw extraction дать rename/group/hide flow;
- добавить docs metadata и role assignment;
- позволить сохранить curated pack.

Acceptance:

- путь `source tree -> raw modules -> curated pack` работает как user-facing flow.

### Stage C1.3. Limited C++ Intake

Нужно:

- поддержать ограниченный practical subset:
  - free functions;
  - `extern "C"` entrypoints;
  - простой public class API через thin adapter;
- явно reject-ить unsupported constructs:
  - templates как public surface;
  - overloaded maze;
  - callbacks/lifetime-heavy API без adapter-а.

Acceptance:

- `C++` path существует честно, но не обещает "поддержать всё".

### Deferred From C1

Пока не включать в practical roadmap:

- Python source decomposition как primary flow;
- assembler source decomposition как normal user-facing flow.

Они могут появиться позже как отдельные specialized tracks, но не должны
размывать первый source-to-modules baseline.

## C2. Library-To-Modules

### Goal

Усилить уже существующий imported-pack path от `header + binary` к более зрелой
library adaptation story.

### Stage C2.1. Header + Binary Intake v2

Нужно:

- улучшить ABI classification;
- различать:
  - static lib
  - shared lib
  - source-backed imported pack
- явно показывать missing metadata cases;
- уметь сохранять repeatable import manifest;
- использовать ту же manifest vocabulary, что и в `C1.1`,
  чтобы source-intake и library-intake не расходились концептуально.

Acceptance:

- import можно повторить без ручного reconstruct всего wizard flow.

### Stage C2.2. Binary Metadata Assist

Нужно:

- использовать exported symbols, ABI hints и debug metadata, если они есть;
- но не pretending, что без заголовков система понимает library полноценно.

Acceptance:

- библиотечный import становится сильнее, но product claim остаётся честным.

## C3. Machine-Code -> C -> Module

### Position

Это не ближайший product-stage, а **research track**.

### Why It Is Valuable

Идея очень сильная:

- дать инженеру вход из legacy binary;
- восстановить C-like representation;
- затем превратить её в module candidate.

### Why It Must Stay Separate

Это требует отдельного сложного pipeline:

- disassembly;
- lifting;
- decompilation;
- recovery of signatures/types;
- cleanup and curation;
- trust/verification barrier.

Это уже не "ещё один import wizard", а отдельная toolchain/IR/decompiler problem.

### Practical Stage C3.1. Feasibility Study

Нужно только:

- отдельный R&D note;
- survey tooling;
- честно описать:
  - какие symbol/type preconditions нужны;
  - где без debug info результат будет слишком шумным;
  - какой verification bar потребуется.

Acceptance:

- есть decision memo, начинать ли вообще этот track в product horizon `6-12 месяцев`.

## Verification Model For Generated Modules

Для всех Tracks B/C нужна общая policy:

- generated module не становится `verified` автоматически;
- provenance и trust level must be visible in UI.

Три оси не должны сливаться в одну:

1. `origin / provenance`
2. `trust_state`
3. `verification result`

Например:

- модуль может быть `generated_from_library`, но уже `curated`;
- модуль может быть `manual`, но ещё только в `draft`;
- модуль может быть `verified` по trust_state, но иметь свежий `modified`
  verification status после новых правок.

Проверка может состоять из:

- compile;
- saved test scenarios;
- Nursery trace review;
- example-level build/run proof;
- manual curation sign-off.

## Recommended Execution Order

Практический порядок такой:

1. `Track A / Stage A0`
   - schema/trust/provenance foundation for `.dqmod`.
2. `Track A / Stage A1-A2`
   - отдельный `Module Studio` и полноценный source editor.
3. `Track A / Stage A3`
   - verification workspace.
4. `Track B / Stage B1-B2`
   - SQLite baseline and curation as first real forcing function.
5. `Track A / Stage A4.0-A4.2`
   - `Nursery` foundation, viewer and stepping.
6. `Track B / Stage B3-B4`
   - SQLite examples and Studio/Nursery reuse proof.
7. `Track A / Stage A5`
   - fragment scratchpad.
8. `Track C / Stage C1.1-C1.2`
   - source-to-modules for `C`.
9. `Track C / Stage C2.1`
   - stronger library intake.
10. `Track C / Stage C1.3`
   - limited `C++` intake.
11. `Track C3`
   - separate feasibility study only if earlier tracks are stable.

## Immediate First Slice

Если начинать сразу следующий implementation slice, он должен быть именно таким:

### Slice 0. Module Trust Schema v2 + Dedicated `.dqmod` Open Path

Нужно:

- заложить schema foundation для:
  - provenance;
  - trust state;
  - verification state;
- перестать открывать `.dqmod` через косвенный graph-redirect;
- создать отдельный editor surface для `.dqmod`;
- вынести кодовый editor из тесного layout `ModuleManagerWidget`;
- разделить:
  - catalog view;
  - authoring view;
  - verification view.

Почему именно это first slice:

- без schema foundation дальше придётся размазывать scenarios/trust/provenance по ad-hoc metadata;
- без dedicated `.dqmod` open path новый editor вообще не станет реальной surface-точкой входа;
- это фундамент и для `Nursery`;
- и для SQLite/official packs;
- и для generated module review;
- и для учебного use case.

Acceptance:

- `.dqmod` открывается в отдельном editor page, а не уводит пользователя в graph tab;
- новый `.dqmod` editor уже понимает canonical schema direction для trust/provenance;
- кодовый editor уже не ограничен маленьким preview-block;
- `Module Manager` остаётся точкой discoverability и curation, но не primary authoring tool.

### Slice 0 Status

`2026-03-15`: foundation, весь `A2` и `A3 verification workspace` уже закрыты.

Что уже сделано:

- `Module` получил canonical save/load direction для:
  - `schema_version`
  - `provenance`
  - `trust_state`
  - expanded `verification`;
- старые `.dqmod` продолжают читаться через migration fallback;
- `.dqmod` больше не открывается через graph redirect в `MainWindow`;
- появился отдельный `ModuleEditorWidget` с большим source area и save/reload flow;
- `ModuleEditorWidget` уже показывает live contract preview из текущей `dq_*` сигнатуры;
- contract table стал editable source-of-truth:
  - add/remove ports
  - sync from source
  - apply back to source signature
  - validation для single-output atomic source contract;
- базовая editor parity для `.dqmod` surface уже есть:
  - modified tab title
  - `undo/redo`
  - `find/replace`
  - `go to line`;
- `ModuleEditorWidget` больше не выпадает из diagnostics/editor-action path:
  - rename/format actions теперь доступны для editable `.dqmod`
  - есть resolved LSP document path для embedded source
  - diagnostics surface теперь приходит прямо в code area module editor;
  - есть live LSP proof через fake server для:
    - `didOpen`
    - `didChange`
    - `didSave`
    - `didClose`
    - formatting response;
- `Module Manager` читает те же trust/provenance axes в summary surface;
- в `ModuleEditorWidget` появился explicit verification workspace:
  - saved scenarios в `.dqmod`
  - compile-only path
  - verify scenario path
  - expected vs actual outputs
  - last verification timestamp
  - trust refresh после verify;
- `A4.0` теперь закрыт на schema/lifecycle уровне:
  - `.dqmod` хранит explicit `trace_artifacts` и `trace_artifact_refs`
  - verification-run сохраняет deterministic boundary trace для supported single-return modules
  - unsupported control-flow сохраняется как explicit `trace_unavailable`, а не симулируется молча
  - verification summary показывает trace count и последний trace status;
- `A4.1` теперь закрыт на viewer-уровне:
  - в `ModuleEditorWidget` появился trace viewer поверх saved artifacts
  - viewer показывает step list, current step, current flow, source snippet
  - viewer показывает trace inputs, accumulated locals и outputs/return
  - saved trace поднимается после reopen и при переключении verification scenario;
- `A4.2` теперь закрыт на playback-уровне:
  - viewer получил `Restart`, `Step`, `Step Over`, `Run To End`
  - кнопки двигают selection внутри saved trace artifact, а не живут отдельно от viewer-state
  - unsupported `trace_unavailable` path честно выключает stepping controls;
- regression proof добавлен на:
  - `Module` schema round-trip/migration;
  - `MainWindow` `.dqmod` open/save path;
  - `Module Manager` trust/provenance visibility;
  - contract rewrite path;
  - diagnostics delivery в embedded module source surface;
  - live LSP lifecycle для embedded module source;
  - saved scenario verify/save/reopen/rerun path;
  - trace artifact round-trip и explicit `trace_unavailable` path;
  - trace viewer surface after verify and after reopen;
  - stepping controls and disabled-state for unavailable traces.

Что ещё осталось до следующего slice:

- следующий рабочий slice уже не про Nursery, а про `Track B / Stage B1-B2`:
  - minimum useful SQLite module set
  - curated official surface
  - первый real-world forcing function для `Module Studio`.

## Non-Goals For The First Cycle

В этот цикл не входят:

- universal full-language module extraction for Python/C++/ASM at once;
- decompiler-grade binary recovery как "готовая фича";
- раздувание `core` database/network/vendor vocabulary;
- подмена `Nursery` обычным runtime debugger-ом;
- автоматическое присвоение generated module-ам статуса `verified` без trace/test/curation.
