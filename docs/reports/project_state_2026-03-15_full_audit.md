# Полный аудит текущего состояния DeltaQ

Дата аудита: `2026-03-15`

Снимок репозитория:

- branch: `master`
- commit: `87aa170`
- version from `CMakeLists.txt`: `0.1.0`

Локальные файлы вне audit-среза:

- `CMakeLists.txt.user`
- `tmp/`

Этот документ нужен как подробный текущий snapshot: что уже реально реализовано,
на каком уровне зрелости находится каждая крупная подсистема и где остаются
границы текущего продукта.

## 1. Краткий вывод

На текущем срезе DeltaQ уже не выглядит как "интересный внутренний прототип".

Честная формулировка состояния сейчас такая:

- **сильный Linux-first modular IDE**
- **текущий public label: Linux release candidate**
- **не cross-platform release**

Главное:

- основной workflow `module -> graph -> generated C -> build -> run` реализован и
  подтверждён не только unit-тестами, но и checked-in templates, examples,
  export flow и Linux release artifact verification;
- desktop/UI contour больше не выглядит как слабое место уровня product-blocker:
  designer/runtime/template path выровнены и закрыты regression-ами;
- standard library больше не выглядит как случайный набор примитивов:
  есть reviewed curation, useful baseline и реальные checked-in scenarios;
- imported pack path реализован как working `v1` flow, но с честно узким scope:
  Linux-first, `C ABI`, explicit diagnostics, без обещаний "магической" поддержки
  сложного `C++`/callback ABI;
- strongest delivery story сегодня именно Linux-only:
  CI, package, AppImage, first-run smoke, example build/run smoke, export bundle.

Если смотреть не на vision, а на уже проверенный repo surface, DeltaQ сейчас
является **зрелым Linux-oriented инженерным инструментом с сильным end-to-end
workflow**, но не "полностью закрытым универсальным IDE-продуктом".

## 2. Шкала зрелости

В этом аудите используется такая шкала:

- `L4` — release-grade в текущем product scope
- `L3` — сильная, широко реализованная подсистема
- `L2` — рабочий baseline с явными ограничениями
- `L1` — частично реализовано или заметно хрупко
- `L0` — вне текущего claim / не реализовано как продуктовый scope

## 3. Проверяемые repo-facts

На текущем срезе в репозитории есть:

- `55` test source files `test_*.cpp`
- `55` зарегистрированных `ctest`-тестов в `build/qt-dev`
- `67` checked-in core modules `.dqmod`
- `13` checked-in core categories:
  - `config_json`
  - `control`
  - `conversion`
  - `desktop`
  - `filesystem`
  - `io`
  - `logic`
  - `math`
  - `process`
  - `serial`
  - `string`
  - `tcp_udp`
  - `timers`
- `9` checked-in example projects `.dqproj`
- `2` fixture SDK trees для imported-pack story:
  - `imported_pack_sensor_sdk`
  - `imported_pack_checksum_sdk`
- `6` template manifests `template.json`
- из них `4` user-facing templates:
  - `console`
  - `console_counter`
  - `desktop`
  - `desktop_text_editor`
- и `2` internal-only templates:
  - `desktop_empty`
  - `desktop_mdi`
- `2` GitHub Actions workflows:
  - `ci.yml`
  - `release.yml`
- `12` release/export verification scripts в `scripts/`
- `6` release screenshots в `docs/release/screenshots/`

Архитектурно проект уже выражен как набор отдельных IDE-подсистем:

- `dq_core`
- `dq_editor`
- `dq_block_editor`
- `dq_ui_designer`
- `dq_lib_processor`
- `dq_codegen`
- `dq_lsp`
- `dq_debug`
- `deltaq` как итоговый executable

## 4. Методика аудита

В рамках этого аудита были просмотрены:

- текущие strategy/checklist/log документы
- актуальные reports и release docs
- README/onboarding/library docs
- workflow-и CI/release
- template/example catalog
- CMake target layout по основным подсистемам

Во время аудита также были выполнены проверки:

- `ctest --test-dir build/qt-dev -N`
- `QT_QPA_PLATFORM=offscreen ctest --test-dir build/qt-dev --output-on-failure`
- `QT_QPA_PLATFORM=offscreen ./build/qt-dev/tests/test_PreBuildProcessor`
- `QT_QPA_PLATFORM=offscreen ./build/qt-dev/tests/test_LinuxProjectExporter`
  вне sandbox
- `QT_QPA_PLATFORM=offscreen ./build/qt-dev/tests/test_PreBuildProcessor transportProbeConsoleExampleBuildsAndRunsEndToEnd serialProbeConsoleExampleBuildsAndRunsEndToEnd`
  вне sandbox

Ограничения аудита:

- не делался новый clean-machine build с нуля;
- не прогонялись заново packaging scripts `build_release.sh` / `build_appimage.sh`;
- не проводился ручной визуальный UI-review поверх живой IDE-сессии;
- вывод ниже опирается на checked-in state, docs и test surface текущего дерева.

## 5. Текущий product scope

Текущий честный scope проекта:

- Linux-first IDE для модульных C/C++ workflow
- основной путь: код, графы, generated C, SDL2 UI, build/run, Linux export
- текущий release label: `Linux release candidate`

Что входит в текущий claim:

- Linux CI и Linux packaging
- checked-in examples и templates
- desktop/UI flow на SDL2 backend
- project export для Linux
- imported packs как Linux-first `C ABI` path

Что в текущий claim **не** входит:

- Windows delivery
- macOS delivery
- cross-platform packaging story
- universal imported-library wrapping для произвольного `C++` ABI
- обещание, что все optional dependencies всегда доступны

Важное ограничение build surface:

- без `QScintilla` editor деградирует на `QPlainTextEdit` fallback;
- без `libclang` отключаются parsing/import flows для library processor.

Это не ломает весь продукт, но это реальные product boundaries, а не мелкие
детали сборки.

## 6. Аудит по подсистемам

### 6.1 Общая сводная матрица

| Подсистема | Уровень | Что уже реализовано | Основные границы |
| --- | --- | --- | --- |
| Core workflow и workbench | `L4` | проектный lifecycle, source-of-truth discipline, build/run loop, generated-file navigation, project properties/toolchain UX | scope Linux-first; cross-platform workbench claims нет |
| Build/codegen/generated files | `L4` | `GraphCompiler`, `PreBuildProcessor`, `CMakeGenerator`, artifact origins, clickable diagnostics, reusable compilation units | есть harness-tail вокруг `test_PreBuildProcessor` под `ctest` |
| Block editor и reusable composition | `L3` | graph editing, palette, commands, submodules, reusable composition example | product surface сильный, но не заявляется как отдельный finished ecosystem beyond current flows |
| UI designer и desktop runtime | `L3` | `.dqui`, SDL2 backend boundary, property/mouse resize parity, desktop templates, headless runtime proofs | backend по сути SDL2-only; broader backend story нет |
| Core module library | `L3` | reviewed `core` pack, role model, useful baseline categories, checked-in scenarios | часть слабых/дублирующих модулей ещё живёт как `legacy` |
| Imported packs / library processor | `L2` | raw import baseline, curated packs, checked-in examples, build metadata flow, explicit diagnostics | Linux-first `C ABI` only; `C++` wrapper story не product-grade |
| Templates / examples / onboarding | `L4` | template catalog, recommended desktop starter, 9 checked-in examples, onboarding path, release screenshots | catalog strong for Linux-only claim, но не про cross-platform onboarding |
| Linux packaging / release / export | `L4` | CI, tarball, AppImage, checksums, first-run smoke, example smoke, export bundle/archive | Linux-only; AppImage export for user projects не заявлен |
| LSP integration | `L2` | LSP client, rename path, completion/integration surface | сравнительно тонкий verified surface по сравнению с build/codegen |
| Debugger | `L2` | `DebugManager`, GDB/MI baseline, breakpoint/debug wiring | baseline есть, но proof-set уже не такой сильный, как у build/export |
| Cross-platform delivery | `L0` | как roadmap-направление только обозначено | finished Windows/macOS story отсутствует |

### 6.2 Core Workflow и Workbench

Оценка: `L4`

Что реализовано:

- `MainWindow`, `ActionManager`, `SessionManager`, `ProjectManager`, `CommandBus`,
  `UndoManager`, `GraphStore`, `UILayoutStore` оформлены как нормальное ядро IDE;
- есть отдельный `Project -> Properties...` flow и portable/local разделение
  build intent vs machine-local paths;
- source-of-truth discipline доведён до user-visible уровня:
  generated files маркируются, открываются read-only и навигируются обратно
  к origin;
- toolchain/build UX уже не сводится к "сырым логам":
  есть `BuildGuidanceAnalyzer`, diagnostic surface, build output navigation,
  generated-file navigation.

Что это значит practically:

- DeltaQ уже ведёт себя не как редактор-демо, а как цельный workbench;
- для user-visible Linux scope базовый IDE lifecycle собран и протестирован.

Главные границы:

- всё это пока честно оформлено именно как Linux-first toolchain path;
- cross-platform IDE-distribution story отсутствует.

### 6.3 Build, Codegen и Generated Files

Оценка: `L4`

Что реализовано:

- graph/codegen pipeline в `src/codegen` оформлен как отдельный слой:
  `GraphCompiler`, `PreBuildProcessor`, `IR`, `BuildPipeline`, `ToolchainResolver`;
- generated artifacts имеют origin-tracking и осмысленный build surface;
- submodule/reusable composition больше не живёт как ad-hoc trick:
  есть отдельные generated units и checked-in reference example;
- desktop graph path и UI codegen выровнены по backend boundary.

Что подтверждает зрелость:

- Phase 1 checklist закрыт;
- reusable composition example checked-in;
- generated navigation и build diagnostics покрыты тестами.

Текущий audit-tail:

- `test_PreBuildProcessor` в прямом запуске работает по большинству сценариев,
  но в текущем `ctest`-запуске показывает harness inconsistency по поиску repo root;
- transport/serial runtime scenarios чувствительны к окружению и требуют
  unrestricted syscalls для честной проверки.

Это не выглядит как product-breaker, но это реальный engineering debt в test harness.

### 6.4 Block Editor и Reusable Composition

Оценка: `L3`

Что реализовано:

- `BlockScene`, `BlockEditorWidget`, `ModulePalette`, `GraphCommands`,
  `SubModuleFactory`, `CycleDetector`, `GraphDebugger`, `BreadcrumbBar`;
- есть unit/widget coverage по block editor surface;
- reusable composition оформлен не только как внутренняя возможность, но и как
  checked-in example `reusable_composition_console`.

Сильная сторона:

- graph composition уже выглядит как реальный product surface, а не только как
  визуальная надстройка над кодом;
- reuse доказан отдельным flow.

Граница:

- это сильная подсистема, но основной public story проекта всё равно строится
  не только на graph editor, а на whole workflow вместе с build/export/UI.

### 6.5 UI Designer и Desktop Runtime

Оценка: `L3`

Что реализовано:

- UI contract layer, `.dqui`, `UIModuleFactory`, `LayoutEngine`,
  `PropertyEditor`, `DesignScene`, `SDL2CodeGenerator`, `UIPreview`;
- runtime/UI contract alignment закрыт по Stage 1;
- desktop templates и runtime behavior parity закрыты regression-ами;
- `Desktop Text Editor` стал recommended starter;
- `Desktop UI Baseline` остался как advanced desktop baseline;
- mouse/property resize parity и save/reopen/build/run path уже доказаны.

Почему не `L4`:

- desktop/UI contour стал сильным и product-usable, но всё ещё узко завязан на
  SDL2 backend и Linux-first desktop scope;
- broader backend abstraction как внешний product claim пока не нужен и не закрыт.

Итог:

- для текущего Linux desktop claim UI designer уже не выглядит сырой зоной;
- для более широкой кросс-бэкендной истории работа ещё не начиналась как product goal.

### 6.6 Core Module Library

Оценка: `L3`

Что реализовано:

- `StandardLibrary` больше не выглядит как случайный inventory;
- есть reviewed matrix `essential / convenience / specialized / legacy`;
- checked-in baseline по useful категориям реально существует:
  `filesystem`, `config_json`, `process`, `timers`, `tcp_udp`, `serial`;
- есть реальные reference scenarios:
  - `settings_file_console`
  - `process_timer_console`
  - `transport_probe_console`
  - `serial_probe_console`
- `core.desktop.*` закрывает desktop lifecycle baseline.

Что важно:

- library теперь поддерживает не только trivial hello-world, но и несколько
  meaningful scenario classes;
- экосистема модулей выражена через docs, examples и palette filtering.

Почему не `L4`:

- audit matrix сама честно фиксирует незакрытый cleanup-tail:
  часть слабых/дублирующих модулей пока не удалена, а только переведена в `legacy`;
- `core` осознанно остаётся компактным baseline, а не "полной стандартной библиотекой".

Это хорошая зрелая позиция, но не "всё уже завершено".

### 6.7 Imported Packs и Library Processor

Оценка: `L2`

Что реализовано:

- `LibclangParser`, `LibraryDecomposer`, `LibraryPackager`,
  `LibraryImportWizard`, `WrapperGenerator`, `LibProcessorWidget`;
- reproducible raw baseline для fixture SDK;
- curated imported pack examples:
  - `imported_pack_sensor_console`
  - `imported_pack_checksum_console`
- build pipeline реально учитывает:
  - `include_paths`
  - `defines`
  - `link_libraries`
- failure UX усилен:
  - unsupported ABI
  - missing header/binary path
  - no half-written pack on rejected import

Почему это только `L2`:

- текущий v1 scope очень узкий и правильно ограничен:
  Linux-first + `C ABI`;
- `C++` ABI, сложные callbacks, arbitrary shared-library adaptation не входят
  в product claim;
- без `libclang` сам intake layer отключается.

Итог:

- это уже рабочий и честно оформленный baseline, а не мечта или прототип;
- но это ещё не универсальный external-library adapter.

### 6.8 Templates, Examples, Onboarding и Narrative

Оценка: `L4`

Что реализовано:

- template catalog очищен и классифицирован;
- user-facing desktop starter реально есть и выглядит осмысленно;
- examples оформлены как checked-in source trees, а не как internal fixtures;
- onboarding path и release docs связаны с README в один entry flow;
- screenshots и release surface подтверждают текущую product story.

Конкретно:

- user-facing templates: `console`, `console_counter`, `Desktop UI Baseline`,
  `Desktop Text Editor`;
- examples покрывают:
  - minimal console
  - useful file/process/transport/serial baselines
  - desktop/UI
  - reusable composition
  - imported packs

Это одна из самых сильных сторон текущего репозитория.

### 6.9 Linux Delivery, Packaging и Export

Оценка: `L4`

Что реализовано:

- полноценный Linux CI workflow;
- tarball packaging;
- AppImage packaging;
- checksum discipline;
- first-run smoke;
- example build/run smoke;
- Linux project export bundle и export archive;
- user-facing checklists для IDE artifact и exported project artifact.

Это уже не только infrastructure scaffolding:

- release docs и workflows описывают реальный handoff path;
- exported bundle проверяется отдельно от IDE bundle;
- AppImage проходит extraction + runtime smoke path.

Текущие границы:

- Linux-only scope;
- project export не обещает AppImage path;
- imported packs с внешними runtime `.so`/data outside built executable всё ещё
  требуют ручной проверки.

### 6.10 LSP

Оценка: `L2`

Что реализовано:

- есть `LSPClient`;
- есть tests для client и rename path;
- LSP surface интегрирован в editor/workbench.

Почему не выше:

- verified surface здесь заметно уже, чем у build/codegen/template/release path;
- нет такого же сильного checked-in product proof, как у export/UI/imported packs.

Итог:

- LSP уже существует как рабочий baseline, но не выглядит главным зрелым
  differentiator текущего продукта.

### 6.11 Debugger

Оценка: `L2`

Что реализовано:

- есть `DebugManager`;
- debugger surface интегрирован в graph/tool workflow;
- есть baseline test coverage.

Почему не выше:

- proof-set здесь тоже тоньше, чем у build/run/export surface;
- user-facing narrative проекта сейчас сильнее вокруг generated transparency,
  module reuse, UI flow и Linux handoff, чем вокруг debugging.

Итог:

- debugger — рабочая подсистема baseline-уровня, но пока не центр current product claim.

### 6.12 Cross-Platform Delivery

Оценка: `L0`

Фактическое состояние:

- Windows delivery story отсутствует как finished public path;
- macOS delivery story отсутствует как finished public path;
- release framing теперь это честно проговаривает.

Это не дефект относительно текущего scope, но это главный structural gap, если
смотреть на проект как на будущий cross-platform product.

## 7. Что реализовано уже действительно хорошо

Сильнейшие зоны проекта на текущем срезе:

1. **Цельный Linux workflow**
   От создания/открытия проекта до build/run/export есть единый осмысленный путь.

2. **Generated transparency**
   Source-of-truth discipline и generated navigation — реальный сильный продуктовый признак.

3. **Desktop/UI baseline**
   После maturity recovery desktop path уже не выглядит test-fixture surface.

4. **Module ecosystem**
   Есть curated `core`, imported packs, examples и docs, а не только набор `.dqmod`.

5. **Release discipline**
   Linux artifacts подтверждаются smoke/verifier-ами, а не только сборкой.

6. **Examples как product surface**
   Example catalog уже работает как нормальный showcase and onboarding layer.

## 8. Текущие открытые слабые места и риски

### 8.1 Cross-platform gap

Главный крупный открытый gap:

- нет finished Windows/macOS delivery story.

Пока это не противоречит current claim, но это главный барьер для любого
расширения public positioning beyond Linux.

### 8.2 Imported-pack scope остаётся узким

Imported pack story уже рабочая, но:

- Linux-first only
- `C ABI` only
- `libclang` dependency remains optional/runtime-sensitive

То есть это сильная `v1` capability, но не finished universal adapter story.

### 8.3 Тестовая среда и harness не полностью выровнены

Во время этого аудита видно два реальных QA-tail:

- `test_LinuxProjectExporter` в sandboxed `ctest` падает не по product bug, а из-за
  `bwrap` uid-map restriction; unrestricted rerun проходит;
- `test_PreBuildProcessor` при прямом запуске mostly healthy, но в `ctest`
  показывает inconsistent repo-root discovery;
- `transport_probe_console` и `serial_probe_console` чувствительны к sandboxed
  socket/PTY restrictions и честно проходят только unrestricted rerun.

Это не выглядит как user-facing blocker, но это показатель, что test environment
classification ещё можно улучшить.

### 8.4 Legacy-tail в `core`

`core` уже curated, но часть узких helper-модулей пока только переведена в `legacy`,
а не удалена или полностью вынесена.

Это не ломает текущий baseline, но поддерживает небольшой conceptual noise.

### 8.5 LSP/debugger менее доказаны, чем основной workflow

Эти подсистемы реализованы, но по сравнению с:

- build/codegen
- templates/examples
- UI/desktop
- Linux release/export

они сейчас подтверждены более узким proof-set.

Если когда-нибудь делать public push именно на "full IDE parity", эти зоны придётся
укреплять отдельно.

## 9. Результаты test-аудита в этой сессии

### 9.1 Общий discovery

- `ctest --test-dir build/qt-dev -N` видит `55` тестов.

### 9.2 Полный sandboxed `ctest`

Команда:

```bash
QT_QPA_PLATFORM=offscreen ctest --test-dir build/qt-dev --output-on-failure
```

Результат:

- `53/55` тестов passed
- failed:
  - `test_LinuxProjectExporter`
  - `test_PreBuildProcessor`

Классификация:

- `test_LinuxProjectExporter` упёрся в `bwrap: setting up uid map: Permission denied`
  и не выглядит продуктовым дефектом;
- `test_PreBuildProcessor` под `ctest` показал harness inconsistency по repo-root
  discovery.

### 9.3 Targeted reruns

Проверено отдельно:

- `QT_QPA_PLATFORM=offscreen ./build/qt-dev/tests/test_LinuxProjectExporter`
  вне sandbox — passed;
- `QT_QPA_PLATFORM=offscreen ./build/qt-dev/tests/test_PreBuildProcessor transportProbeConsoleExampleBuildsAndRunsEndToEnd serialProbeConsoleExampleBuildsAndRunsEndToEnd`
  вне sandbox — passed;
- прямой sandboxed запуск полного `test_PreBuildProcessor` показал, что `udp/serial`
  probe tests env-sensitive даже когда остальные scenarios проходят.

Итог по качеству:

- основной automated proof-set сильный;
- но часть end-to-end Linux tests всё ещё зависит от окружения сильнее, чем
  хотелось бы от совсем "идеального" release-grade harness.

## 10. Итоговая оценка

### Что уже можно утверждать уверенно

- DeltaQ реализован как **цельный Linux-first IDE/toolchain**, а не набор
  отдельных подсистем.
- Основной workflow уже **доказан end-to-end**.
- Linux packaging/export surface уже **release-grade в пределах текущего scope**.
- UI/desktop и module ecosystem уже не выглядят незрелыми блокерами.
- Imported packs реализованы как **рабочая v1 capability** с честными границами.

### Что пока нельзя утверждать честно

- Что проект уже является cross-platform release.
- Что imported-pack adapter покрывает широкий `C++`/ABI space.
- Что все подсистемы одинаково зрелы на уровне build/export surface.
- Что test harness уже полностью environment-agnostic.

## 11. Рекомендованные следующие шаги

Если смотреть pragmatically, после этого аудита следующий разумный порядок такой:

1. Починить harness-tail вокруг `test_PreBuildProcessor` под `ctest`.
2. Явно классифицировать environment-sensitive tests:
   - `bwrap`
   - UDP loopback
   - PTY/serial loopback
3. Решить, остаётся ли следующий фокус Linux RC hardening, или проект начинает
   отдельный track под Windows delivery.
4. Дочистить `legacy`-tail в `core`, если нужен ещё более чистый library story.
5. Если планируется усиливать IDE-claim beyond current workflow, отдельно поднять
   proof-set по LSP/debugger.

## 12. Финальная формулировка

На `2026-03-15` DeltaQ — это:

- **не просто инженерный прототип**
- **не cross-platform release**
- **а сильный Linux-first modular IDE/toolchain с release-candidate уровнем зрелости
  внутри Linux scope**

Именно так текущее состояние проекта выглядит наиболее честно и технически
обоснованно.
