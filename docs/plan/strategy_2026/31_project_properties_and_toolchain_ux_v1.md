# Project Properties And Toolchain UX v1

## Purpose

Зафиксировать **исполняемый план** для следующего product-grade слоя DeltaQ:

- дать пользователю явную точку входа `Project -> Properties...`;
- превратить текущие скрытые build-предположения в понятную и сохраняемую модель проекта;
- ввести нормальную систему обнаружения toolchain-ов и builder-ов;
- довести quick-access toolbar до состояния продуктового UI:
  - иконки;
  - tooltip/status-tip;
  - понятный hover feedback;
- закрыть самые заметные дыры русской локализации на верхнем уровне IDE.

Этот этап не заменяет `Linux Project Export v1`, а делает его честно настраиваемым
и понятным пользователю.

## Why It Matters

Сейчас в DeltaQ уже есть рабочий путь:

- `graph/ui -> generated code -> CMake -> build -> run`;
- `build -> export Linux bundle`.

Но пользовательский слой вокруг этого workflow ещё сырой:

- toolbar в основном текстовый и без иконок;
- при наведении нет цельной action-description модели;
- в главном UI отсутствует `Project Properties`;
- `.dqproj` уже хранит build-блок, но реальная сборка использует его лишь частично;
- auto-detection toolchain-ов существует, но пока это только простой `PATH`-scan;
- русская локализация закрыта не полностью и выглядит неровно;
- для пользователя неочевидно:
  - какой компилятор используется;
  - какой builder используется;
  - как это изменить;
  - где увидеть проблему, если toolchain отсутствует.

Пока этот слой не доведён, DeltaQ может собирать и экспортировать проект, но ещё
не выглядит как цельный инструмент для настройки и сопровождения реальной сборки.

## Current State

На сегодня уже есть:

- `BuildConfig` внутри `.dqproj`;
- сериализация project-level build settings;
- базовый `CompilerDetector`, который ищет `gcc/clang/g++/clang++`, `cmake`, `make`, `ninja`;
- генерация пользовательского `CMakeLists.txt`;
- `BuildManager`, `BuildPipeline` и `Linux export`;
- общие IDE-настройки в `SettingsDialog`;
- инфраструктура `lupdate/lrelease` и `deltaq_ru.ts`.

На сегодня ещё нет:

- отдельного user-facing диалога `Project Properties`;
- различия между portable project settings и machine-local tool paths;
- реального использования выбранного toolchain-а в `BuildManager`;
- понятной модели `generator / build type / compiler / cmake path / builder path`;
- project-level diagnostics для missing toolchain;
- action metadata для toolbar/status-tip UX;
- полного top-level RU coverage для меню, toolbar и новых build/export surfaces.

## Target State

Нужен следующий пользовательский результат:

1. пользователь открывает проект;
2. видит `Project -> Properties...`;
3. может настроить:
   - build profile;
   - C/C++ standard;
   - generator (`Ninja` / `Unix Makefiles`);
   - auto/manual toolchain mode;
   - export-related project defaults;
4. IDE показывает, какие toolchain-и найдены на машине и какой kit сейчас выбран;
5. build/export используют именно выбранную конфигурацию;
6. если toolchain или runtime dependency отсутствуют, IDE показывает понятный
   guided diagnostic, а не только сырой compiler log;
7. toolbar имеет иконки, tooltip и status-bar description;
8. в русской локали top-level UI выглядит цельно и без явных англоязычных дыр.

## Scope

Этот этап **включает**:

- `Project Properties` dialog;
- persistence project build settings;
- separate local-user toolchain override layer;
- toolchain scanning/rescan;
- `BuildManager` integration с выбранным toolchain/generator;
- guided diagnostics для missing build dependencies;
- toolbar icons + hover descriptions;
- top-level Russian localization cleanup.

Этот этап **не включает**:

- полноценную установку компиляторов из IDE с root-правами;
- поддержку нескольких build systems кроме `CMake`;
- Windows/MSVC kit management;
- macOS/Xcode toolchain flow;
- package-manager frontend внутри IDE;
- полную локализацию всех глубинных диагностических строк во всём проекте.

## Decisions

### 1. `CMake` остаётся единственным build orchestrator для `v1`

DeltaQ уже построена вокруг генерации `CMakeLists.txt`, `cmake configure` и
`cmake --build`.

Поэтому для `v1` фиксируется:

- project build system = `CMake`;
- пользователь выбирает не альтернативный build system, а параметры запуска `CMake`;
- `make/direct` не развиваются как равноправные product modes.

`BuildConfig.buildTool` в текущем виде считается историческим и должен быть
либо переосмыслен как `generator`, либо выведен из product-facing модели.

### 2. Нужно жёстко разделить portable и machine-local настройки

В `.dqproj` должны жить только project-facing, переносимые настройки:

- build profile;
- generator preference;
- C/C++ standard;
- extra flags;
- export defaults;
- policy `auto/manual`.

Абсолютные пути и machine-local executable selection не должны быть обязательной
частью `.dqproj`, иначе проект станет непереносимым.

Для этого нужен отдельный local layer:

- `.dqproj.user`, либо
- другой явно локальный companion-file с игнорированием в VCS.

В нём хранятся:

- `cmake` path;
- `c compiler` path;
- `c++ compiler` path;
- `ninja/make` path;
- local override выбранного kit-а.

### 3. `Install compilers from IDE` для `v1` заменяется на guided install

Для первой зрелой версии не стоит встраивать package-management frontend с
root-операциями.

Вместо этого нужен:

- scanner состояния toolchain-а;
- понятный missing-dependency report;
- install hints по дистрибутиву;
- кнопка `Rescan`.

То есть `v1` решает discoverability и onboarding, а не роль системного package manager.

### 4. Project Properties важнее, чем расширение общего Settings dialog

`Settings` — это IDE-level preferences.

`Project Properties` — это project-level configuration surface.

Смешивать их нельзя, иначе:

- непонятно, что переносится вместе с проектом;
- непонятно, что относится к конкретной машине;
- пользователь теряет доверие к build semantics.

### 5. Toolbar должен перейти на action metadata model

Недостаточно просто навесить PNG/SVG на несколько кнопок.

Нужна единая action-модель:

- icon;
- text;
- tooltip;
- status-tip;
- optional `whatsThis`/short description.

Только тогда quick-access bar, меню и status-bar будут говорить одним языком.

### 6. Toolchain selection должен реально влиять на build/export

Нельзя добавлять диалог настроек проекта, если `BuildManager` продолжит всегда делать:

- `cmake ..`
- `cmake --build . --parallel`

без учёта выбранного generator/compiler path.

Иначе появится "декоративный" UI без product truth.

### 7. Configurable build output dir выводится из `v1` scope

Хотя в текущем `BuildConfig` уже есть поле `output`, кодовая база пока жёстко
завязана на `build/` во многих местах:

- `BuildManager`;
- executable resolver;
- export pipeline;
- project templates;
- autotests и smoke-path.

Поэтому для `v1` фиксируется:

- build root остаётся `build/`;
- поле `output` можно сохранять только как compatibility detail, если это нужно для migration;
- `Project Properties` не должен показывать editable output dir;
- отдельная configurable build-root story возможна только после специального
  cross-cutting audit всех `build/`-зависимых путей.

### 8. Toolchain UX должен опираться на formal kit model

Недостаточно хранить набор разрозненных путей.

Нужна явная модель:

- scanner формирует список kit-ов;
- у каждого kit-а есть стабильный `kit_id`;
- пользователь выбирает либо `selected_kit_id`, либо manual override mode;
- local persistence хранит не только пути, но и режим выбора.

Правило приоритета фиксируется сразу:

1. `manual override`, если он включён и валиден;
2. `selected_kit_id`, если такой kit всё ещё разрешается после rescan;
3. `auto best available kit`.

### 9. Project Properties требует metadata-only save semantics

Текущий `ProjectManager::saveProject()` сохраняет не только `.dqproj`, но и:

- module registry;
- graphs;
- UI layouts.

Для `Project Properties` этого недостаточно.

Нужен отдельный metadata-only путь:

- `saveProjectMetadataOnly()` или эквивалентный save mode;
- отдельное сохранение local-user toolchain файла;
- отсутствие побочного сохранения unrelated graph/UI/module changes.

Иначе диалог свойств проекта будет иметь скрытые side effects.

### 10. Toolchain/generator changes должны иметь configure fingerprint

Смена хотя бы одного из параметров ниже должна принудительно инвалидировать старый
configure state:

- generator;
- profile;
- `cmake` executable;
- `C` compiler;
- `C++` compiler;
- toolchain mode.

Для этого нужен явный fingerprint/resolved-config signature, который можно сравнивать
с прошлым configure state до запуска build.

## Proposed Data Model

### `.dqproj`

В `.dqproj` рекомендуется хранить:

```json
{
  "build": {
    "system": "cmake",
    "generator": "auto",
    "profile": "Debug",
    "c_standard": "c17",
    "cxx_standard": "c++20",
    "extra_c_flags": [],
    "extra_cxx_flags": [],
    "toolchain_mode": "auto"
  }
}
```

Смысл:

- это intent проекта;
- это можно коммитить в репозиторий;
- это одинаково читается на разных машинах.

Для `v1` важно:

- build root остаётся фиксированным `build/`;
- editable output dir в product UI не появляется;
- если compatibility-требование заставляет временно сохранить legacy-поле `output`,
  оно не должно рассматриваться как активная пользовательская настройка.

### `.dqproj.user`

В локальном companion-файле рекомендуется хранить:

```json
{
  "toolchain": {
    "selection_mode": "kit",
    "selected_kit_id": "linux-gcc-ninja",
    "manual_override": {
      "enabled": false,
      "cmake_path": "",
      "c_compiler_path": "",
      "cxx_compiler_path": "",
      "builder_path": "",
      "generator": ""
    },
    "last_resolved_fingerprint": "sha256:..."
  }
}
```

Смысл:

- это local override;
- это не обязано переноситься между машинами;
- это не должно ломать клонирование проекта на другом Linux host.

При этом:

- scanner inventory не обязательно сериализовать полностью;
- сериализуется именно выбор пользователя и local override;
- precedence определяется правилами из решения про formal kit model.

## Proposed UI Surface

### Project Menu

Нужен отдельный `Project` menu между `File` и `Edit`.

Минимум:

- `Project Properties...`
- `Rescan Toolchains`
- `Open Build Directory`
- `Open Dist Directory`

### Project Properties Dialog

Минимальные вкладки:

1. `General`
   - имя проекта;
   - версия;
   - тип проекта;
2. `Build`
   - `Debug / Release`;
   - `C standard`;
   - `C++ standard`;
   - generator preference;
   - extra flags;
3. `Toolchain`
   - auto/manual mode;
   - detected kits;
   - selected kit;
   - local override paths;
   - `Rescan`;
4. `Export`
   - default export dir;
   - archive on export by default;
   - launcher naming / handoff notes (если это уже реально поддержано backend-ом).

### Toolbar UX

Quick-access toolbar должен получить:

- иконки для:
  - new project;
  - open;
  - save;
  - undo;
  - redo;
  - build;
  - run;
  - export;
  - module/workbench switching;
- tooltip;
- status-tip, показываемый в status-bar при hover;
- единый icon/style baseline без случайного смешения тем.

## Work Items

### Stage 1. Normalize Project Build Model

Нужно привести `BuildConfig` к продуктовой модели.

Сделать:

- убрать ambiguity между `buildTool` и реальным builder/generator;
- добавить:
  - `system`;
  - `generator`;
  - `profile`;
  - `cStandard`;
  - `cxxStandard`;
  - `extraCFlags`;
  - `extraCxxFlags`;
  - `toolchainMode`;
- зафиксировать, что build root для `v1` остаётся `build/`, а configurable output dir
  в UI не появляется;
- решить migration path для старых `.dqproj`.

Acceptance:

- старые `.dqproj` открываются без поломки;
- новые поля сериализуются/десериализуются;
- нет product-facing обещания configurable build root в `v1`;
- есть unit-тесты на roundtrip и migration defaults.

### Stage 2. Add Local Toolchain Override Model

Нужно ввести отдельную local-user persistence model.

Сделать:

- определить формат `.dqproj.user`;
- реализовать load/save рядом с проектом;
- добавить formal fields:
  - `selection_mode`;
  - `selected_kit_id`;
  - `manual_override`;
  - `last_resolved_fingerprint`;
- добавить ignore-recommendation для VCS;
- обеспечить fallback, если `.dqproj.user` отсутствует;
- зафиксировать precedence:
  - manual override;
  - selected kit;
  - auto.

Acceptance:

- проект открывается без `.dqproj.user`;
- local override сохраняется отдельно от `.dqproj`;
- semantics выбора kit-а и manual override не двусмысленна;
- перенос проекта на другую машину не требует редактировать committed `.dqproj`.

### Stage 3. Replace Simple Detector With Product-Facing Toolchain Scanner

Нужно развить текущий `CompilerDetector` до scanner/resolver уровня.

Сделать:

- сканировать:
  - `cmake`;
  - `ninja`;
  - `make`;
  - `gcc` / `g++`;
  - `clang` / `clang++`;
- хранить:
  - stable `kit_id`;
  - executable paths;
  - version;
  - availability;
  - completeness kit-а;
- добавить `Rescan`;
- добавить простой distro detection для install hints.

Acceptance:

- IDE показывает detected kits, а не только разрозненные бинарники;
- kit list имеет стабильные идентификаторы для local persistence;
- видно статус:
  - `ready`;
  - `partial`;
  - `missing`;
- scanner не блокирует UI дольше разумного времени.

### Stage 4. Add Metadata-Only Project Save And Configure Fingerprint

Нужно убрать системные блокеры перед user-facing `Project Properties`.

Сделать:

- выделить metadata-only project save path;
- выделить отдельное сохранение `.dqproj.user`;
- добавить resolved-config fingerprint для configure state;
- определить, какие изменения форсят reconfigure.

Acceptance:

- сохранение project metadata не записывает unrelated graphs/UI/modules;
- local-user toolchain state сохраняется отдельно;
- смена generator/toolchain/profile детерминированно инвалидирует configure state.

### Stage 5. Make BuildManager Respect Selected Toolchain

Это главный системный этап и он должен приземлиться **до** полноценного editing UI.

Сделать:

- `BuildPipeline` и `BuildManager` должны принимать resolved build config;
- `cmake configure` запускать с явными параметрами:
  - `-G ...`
  - `-D CMAKE_BUILD_TYPE=...`
  - `-D CMAKE_C_COMPILER=...`
  - `-D CMAKE_CXX_COMPILER=...`
- использовать локально выбранный `cmake` executable;
- использовать выбранный builder через `cmake --build`, а не через скрытый auto-assumption;
- reconfigure при смене generator/toolchain/profile/fingerprint.

Acceptance:

- смена generator/toolchain реально влияет на build;
- выбранная конфигурация отражается в build output;
- export использует ту же конфигурацию, что и обычный build.

Status on 2026-03-14:

- backend slice приземлён:
  - `BuildPipeline` и `BuildManager` принимают resolved build config;
  - `ToolchainResolver` строит stable kit model и resolved toolchain fingerprint;
  - `.dqproj.user` хранит selection mode / selected kit / manual override / last fingerprint;
  - обычный build сохраняет configure fingerprint и форсит reconfigure при смене toolchain-параметров;
- user-facing gaps ещё открыты:
  - нет `Project -> Properties...` surface;
  - нет product-facing `Rescan Toolchains`;
  - missing-toolchain cases пока выводятся как structured build error, но ещё не как guided diagnostics dialog.

### Stage 6. Add Project Properties UI

Нужно дать пользователю явную project-facing поверхность **после** того, как backend
build semantics уже стала правдой.

Сделать:

- новый `Project` menu;
- `Project Properties...` action;
- диалог с вкладками `General / Build / Toolchain / Export`;
- wiring к metadata-only save path, а не к полному `saveProject()`;
- восстановление project-level и local-user state после reopen.

Acceptance:

- свойства проекта можно менять без ручного редактирования `.dqproj`;
- изменения сохраняются и восстанавливаются после reopen;
- диалог не сохраняет unrelated graph/UI/module changes;
- ясно видно, какие поля project-level, а какие local-machine only.

Status on 2026-03-14:

- user-facing slice приземлён:
  - в `MainWindow` появился `Project` menu;
  - есть `Project Properties...`;
  - есть `Rescan Toolchains`;
  - есть быстрые actions `Open Build Directory` и `Open Dist Directory`;
  - dialog сохраняет project/local state через metadata-only save path;
  - dialog показывает detected kits и resolved toolchain summary;
- ограничения текущего UI-first slice:
  - нет browse/install assistant для toolchain setup;
  - missing-toolchain cases ещё не доведены до guided diagnostics dialog.

### Stage 7. Add Guided Missing-Dependency Diagnostics

Нужно превратить missing-toolchain cases в понятный UX.

Сделать:

- если отсутствует `cmake`, компилятор или builder:
  - показать structured diagnostic;
  - дать install hints;
  - предложить `Rescan`;
- если desktop-проекту не хватает `SDL2/SDL2_ttf`:
  - показывать это отдельно от generic configure failure;
  - не заставлять пользователя читать сырое `CMake` сообщение как единственный источник истины.

Acceptance:

- missing toolchain case объясняется без чтения raw log;
- missing desktop deps объясняются явно;
- diagnostics можно перепроверить после `Rescan`.

Status on 2026-03-14:

- guided diagnostics slice приземлён:
  - pre-build toolchain readiness failure теперь показывает structured guidance;
  - desktop `SDL2 / SDL2_ttf` configure failure теперь показывает dependency guidance;
  - guidance выводится не только в raw log, но и как отдельный user-facing message с next steps;
- границы текущего решения:
  - это ещё не install assistant;
  - guidance пока не покрывает каждый возможный `CMake` failure pattern;
  - основной покрытый scope: missing `cmake/compiler/builder` и missing desktop `SDL2/SDL2_ttf`.

### Stage 8. Upgrade Toolbar To Icon + Hover Description UX

Нужно закрыть product gap верхнего toolbar.

Сделать:

- завести набор IDE action icons;
- добавить icon binding в `ActionManager`;
- заполнить `toolTip` и `statusTip`;
- убедиться, что status-bar показывает hover description;
- выровнять toolbar composition с реально важными actions.

Acceptance:

- toolbar читается визуально быстрее;
- hover даёт понятное описание действия;
- toolbar не превращается в случайную смесь текстовых иконок и plain-text кнопок.

Status on 2026-03-14:

- toolbar slice приземлён:
  - `ActionManager` получил icon binding для основных project/build/view actions;
  - quick-access toolbar переведён в `icon-only` presentation для быстрого визуального чтения;
  - у actions заполнены `toolTip` и `statusTip`, так что hover description показывается не только в button hover, но и в status bar;
  - toolbar composition выровнен вокруг реально частых действий: project, save, undo/redo, build/run/export, view switching;
- regression coverage добавлено через `test_ActionManager` и `test_MainWindowEditorActions`;
- stage считается закрытым для `v1`;
- отдельно вне scope `v1` остаётся только возможный будущий visual-art polish поверх уже работающего UX contract.

### Stage 9. Finish Top-Level Russian Localization Pass

Нужно закрыть самые заметные пользовательские языковые дыры.

Сделать:

- прогнать `update_translations`;
- перевести новые top-level строки:
  - меню;
  - toolbar actions;
  - `Project Properties`;
  - `Toolchain`;
  - guided diagnostics;
  - export/build top-level feedback;
- убедиться, что русская локаль не выглядит как смесь RU/EN в первом контакте.

Acceptance:

- top-level workbench, build flow и project-properties surfaces покрыты `ru`;
- в основных сценариях не остаётся явных непереведённых action/menu/dialog строк.

Status on 2026-03-14:

- выполнен targeted top-level localization pass:
  - обновлён `deltaq_ru.ts` после `update_translations`;
  - переведены новые строки для:
    - `ActionManager`;
    - `Project Properties`;
    - toolchain/build diagnostics;
    - build/export status feedback;
  - собран актуальный `deltaq_ru.qm` через target `translations`;
- текущая граница решения:
  - это не full-repo translation completion;
  - сознательно закрыт именно first-contact layer: меню, toolbar, `Project / Build / Toolchain` flow и ключевой build/export feedback;
- для `v1` stage считается закрытым, потому что в основном пользовательском workbench/build flow больше нет явных EN-дыр на новых surfaces.

## Recommended Execution Order

Правильный порядок такой:

1. `Stage 1` — нормализовать model/persistence.
2. `Stage 2` — local override layer и formal kit selection model.
3. `Stage 3` — scanner/resolver.
4. `Stage 4` — metadata-only save + configure fingerprint.
5. `Stage 5` — реальная build integration.
6. `Stage 6` — project properties UI.
7. `Stage 7` — guided diagnostics.
8. `Stage 8` — toolbar icons/hover UX.
9. `Stage 9` — localization pass.

Причина такого порядка:

- UI editing surface не должна появляться раньше backend truth;
- configurable output dir сознательно не берётся в `v1`, чтобы не расползаться
  в cross-cutting audit всего `build/`-контура;
- localization до стабилизации surface даст лишнюю churn-работу;
- toolbar polish полезен, но не должен обгонять project/build truth.

## Acceptance Gates

Этап можно считать закрытым, когда выполняется всё ниже:

- пользователь может открыть `Project Properties...` и изменить build-конфигурацию проекта;
- `.dqproj` хранит portable project build intent;
- `.dqproj.user` или эквивалент хранит local toolchain overrides;
- kit selection model и precedence `manual -> selected kit -> auto` зафиксированы явно;
- IDE показывает найденные toolchain-и и умеет их пересканировать;
- `Project Properties` не имеет скрытых side effects на unrelated graph/UI/module data;
- `BuildManager` и `Export Linux Bundle` используют выбранный generator/toolchain;
- missing toolchain/dependency cases объясняются через guided diagnostic;
- quick toolbar использует иконки и status-tip hover descriptions;
- русская локализация закрывает top-level workbench и project/build configuration flow.

## Notes For Implementation

- Не стоит начинать с "красивого" диалога, пока не определены model и persistence.
- Не стоит обещать пользователю "установку компиляторов", пока IDE не умеет хотя бы
  честно различать `ready / partial / missing`.
- Нельзя класть абсолютные machine-local tool paths в committed `.dqproj`.
- Не стоит обещать editable output dir, пока не выполнен специальный audit всего
  `build/`-зависимого контура.
- Любой `Project Properties` UI должен иметь regression tests на:
  - save/reopen;
  - build uses selected config;
  - metadata-only save semantics;
  - stale configure invalidation при смене generator/toolchain.
