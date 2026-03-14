# Linux Project Export v1

## Purpose

Зафиксировать **детальный исполнимый план** следующего Linux-first этапа:

- довести DeltaQ от сценария `build -> run в среде разработки` до сценария
  `build -> export -> handoff-ready artifact`;
- сделать так, чтобы пользователь получал не только собранный бинарник в `build/`,
  но и готовую Linux-сборку своего приложения;
- не смешивать эту задачу с packaging самой IDE.

## Why It Matters

Сейчас DeltaQ уже хорошо закрывает внутренний engineering workflow:

- `graph/ui -> generated code -> CMake -> build -> run`;
- Linux release bundle и AppImage для **самой IDE** уже доведены почти до release-grade состояния;
- примеры и smoke-path подтверждают, что проект пользователя можно открыть, собрать и запустить.

Но главный product gap остаётся открытым:

- пользовательское приложение пока заканчивается на `build/<target>`;
- desktop-проекты по-прежнему зависят от системных `SDL2/SDL2_ttf`;
- у пользователя нет явной команды и контракта вида
  `получить готовую Linux-сборку программы`.

Пока этот gap не закрыт, DeltaQ остаётся сильной Linux IDE, но ещё не доказывает
полный delivery path конечного ПО.

## Current State

На сегодня уже есть:

- end-to-end verification для `console` и `desktop` сценариев на уровне
  `pre-build -> CMake -> build -> run`;
- стабильный `BuildPipeline`, который честно завершает compile phase;
- `CMakeGenerator`, который умеет собирать console/desktop проекты и учитывать
  build requirements imported pack-ов;
- Linux packaging flow для самой IDE (`tar.gz`, `AppImage`, runtime smoke).

На сегодня ещё нет:

- отдельного export pipeline для пользовательского проекта;
- стабильного artifact contract для готовой сборки приложения;
- надёжного API, который резолвит **runtime executable**, а не служебный build-system marker;
- copy/bundle stage для runtime payload конечной программы;
- формализованного desktop-contract для font/data assets;
- explicit policy, обязан ли export пересобирать проект или может паковать stale build;
- честного smoke-path вида
  `exported project bundle -> launch on compatible Linux -> expected runtime`.

## Target State

Нужен следующий пользовательский результат:

1. пользователь открывает или собирает проект в DeltaQ;
2. выбирает `Export Linux Bundle`;
3. DeltaQ создаёт bundle в `dist/<ProjectName>/`;
4. bundle можно передать на другую совместимую Linux-машину без DeltaQ checkout;
5. bundle запускается как готовое приложение, а не как development tree;
6. user-facing export по умолчанию сначала подтверждает свежую сборку проекта,
   а уже потом собирает bundle;
7. при желании из того же bundle собирается `.tar.gz`.

Для `v1` достаточно, чтобы это было честно закрыто для:

- `console` проектов;
- `desktop` проектов на текущем SDL2 runtime;
- checked-in templates/examples и обычных пользовательских проектов,
  проходящих через текущий `CMakeGenerator`.

## Scope

Этот этап **не** про:

- Windows/macOS export;
- AppImage export для пользовательских проектов;
- мобильные платформы;
- universal binary promise на все Linux-дистрибутивы;
- code signing / notarization.

Этот этап про:

- Linux-only project export;
- handoff-ready directory bundle;
- optional `.tar.gz` из того же bundle;
- честную фиксацию, что именно считается "независимой сборкой" в `v1`.

## Decisions

### 1. Формат `v1` — directory bundle, а не AppImage

Первый export должен быть максимально прозрачен и отлаживаем.

Поэтому `v1` формат:

- bundle directory в `dist/<ProjectName>/`;
- optional `.tar.gz` из него.

`AppImage` для пользовательских проектов можно рассматривать только после того,
как directory bundle станет стабильным и тестируемым.

### 2. Export backend важнее UI-кнопки

Сначала нужно получить рабочий backend/service/script с automated smoke.

Только после этого стоит добавлять:

- action в меню;
- progress/output внутри IDE;
- пользовательский handoff flow.

### 3. Export должен переиспользовать текущий build pipeline

Нельзя строить отдельную "магическую" систему упаковки мимо уже существующего
`PreBuildProcessor -> CMakeGenerator -> BuildManager`.

Экспорт должен опираться на уже собранный target и только добавлять stage:

- resolve built executable;
- collect runtime payload;
- assemble bundle;
- verify exported artifact.

### 4. Export не должен использовать `BuildManager::expectedBuildArtifact(...)` как бинарный resolver

Текущий `BuildManager::expectedBuildArtifact(...)` нужен для другой задачи:

- он определяет наличие generated build-system file (`Makefile`, `build.ninja`);
- он используется как сигнал "build tree configured";
- он **не** возвращает runtime executable проекта.

Для export нужен отдельный контракт:

- resolver реального executable по `projectName/targetName`;
- поддержка поиска внутри `build/` и его подкаталогов;
- в дальнейшем желательно опора на более явный target metadata channel, а не только
  на эвристику по имени файла.

На первом цикле допустимо опереться на существующие heuristic-идеи вроде:

- `MainWindow::resolveProjectExecutable()`;
- тестовый helper `findBuiltExecutable(...)`;

но уже как на основу **нового** export-specific resolver-а, а не как на reuse
`expectedBuildArtifact(...)`.

### 5. `Независимая сборка` в `v1` означает совместимый Linux bundle, а не universal ABI promise

Для `v1` надо честно обещать следующее:

- приложение не требует установленной DeltaQ IDE;
- приложение не требует исходного project tree рядом;
- desktop-bundle не требует developer packages вроде `libsdl2-dev`.

Для `v1` **не** обещается:

- запуск на любом дистрибутиве без оговорок;
- совместимость через большие скачки по `glibc`/toolchain.

### 6. Dependency model должен быть явным

`v1` должен хорошо работать для:

- console проектов;
- desktop-проектов на текущем SDL2 runtime;
- self-contained примеров, где зависимости либо собираются вместе с проектом,
  либо копируются в bundle как runtime payload.

Отдельным open piece остаются imported pack-и, которым для runtime нужны
внешние `.so` или другие payload-файлы без явной export metadata.

Это нужно зафиксировать сразу, чтобы не обещать больше, чем реально закрыто.

### 7. Desktop export в `v1` включает не только `.so`, но и обязательный font/data baseline

Текущий SDL2 UI runtime ещё опирается на hardcoded probe системных шрифтов.

Из этого следует жёсткое правило:

- desktop export нельзя считать закрытым одной упаковкой `SDL2`/`SDL2_ttf`;
- font/data asset contract должен входить в основной desktop export scope;
- generated runtime должен уметь сначала искать bundled font/resource path, а уже
  потом при необходимости использовать system fallback как dev-mode fallback.

Иначе exported desktop bundle останется частично зависимым от состояния хоста.

### 8. User-facing export по умолчанию build-coupled

Для обычного пользовательского пути export не должен silently паковать последний
успешный build неизвестной свежести.

По умолчанию export делает:

1. pre-build;
2. configure/build;
3. проверку успеха;
4. только потом assemble/export.

Для `v1` можно допустить отдельный internal/testing mode вроде `--skip-build`, но:

- он не должен быть default behavior;
- он не должен определять semantics UI action.

## Proposed Bundle Contract

Рекомендуемый базовый layout:

```text
dist/<ProjectName>/
  <ProjectName>            # launcher script or launcher entry point
  bin/
    <ProjectName>.bin      # реальный собранный executable
  lib/                     # bundle-local shared libraries
  assets/
    fonts/                 # bundled desktop font baseline
    ...                    # runtime assets, если есть
  EXPORT_INFO.txt          # что входит в bundle и как он собран
```

Требования к контракту:

- launcher не зависит от текущего `build/` каталога;
- launcher поднимает bundle-local runtime через `LD_LIBRARY_PATH` или эквивалент;
- все пути внутри bundle относительные;
- desktop runtime умеет находить bundled font/resource path без системного font probe
  как обязательной предпосылки;
- при запуске приложение не читает файлы из source checkout, если они не были
  явно экспортированы в `assets/`.

## Work Items

### Stage 1. Define Export Artifact Contract

Нужно зафиксировать, что именно считается готовой Linux-сборкой проекта.

Обязательные решения:

- стабильный output path;
- bundle layout;
- launcher contract;
- contract разрешения **реального executable**, а не build-system marker;
- freshness policy:
  - export всегда rebuild-ит;
  - или export допускает reuse только после явной stale-check policy;
- список обязательных payload-категорий:
  - executable;
  - shared libraries;
  - mandatory desktop font/data baseline;
  - runtime assets;
  - export manifest;
- что именно **не** попадает в export:
  - `.dqgraph`;
  - `.dqui`;
  - development build tree;
  - IDE-specific metadata, не нужная runtime.

Acceptance:

- у export есть один прозрачный layout;
- зафиксировано, как export находит runtime executable;
- зафиксировано, когда export rebuild-ит проект и когда отказывается от stale artifact;
- bundle можно проверить без чтения исходников DeltaQ.

### Stage 2. Add Backend Export Service And Executable Resolver

Нужно ввести отдельный backend слой, например:

- `LinuxProjectExporter`;
- или `ProjectExportService`.

Задачи слоя:

- принимать `projectDir`, `projectName`, `projectType`;
- запускать или требовать явно подтверждённый fresh build;
- находить **runtime executable**, а не build-system marker;
- собирать output directory `dist/<ProjectName>/`;
- копировать executable в bundle;
- генерировать launcher и `EXPORT_INFO.txt`;
- возвращать явные ошибки export stage.

Предпочтительная интеграция:

- reuse существующего build pipeline для стадии сборки;
- отдельный `ProjectExecutableResolver`/`ExportTargetResolver` поверх текущих
  heuristic-путей поиска executable;
- отдельный сервис, а не перегрузка обязанностей `BuildManager`.

Отдельное правило semantics:

- user-facing export path по умолчанию делает `pre-build -> build -> export`;
- exporter не пакует старый бинарник после проваленной или пропущенной пересборки;
- если нужен internal fast-path без rebuild, он оформляется отдельным флагом и
  отдельной диагностикой.

Acceptance:

- console template экспортируется в runnable bundle;
- export не зависит от `BuildManager::expectedBuildArtifact(...)` как от resolver-а;
- bundle запускается из временной директории вне project tree.

### Stage 3. Add Desktop Runtime Dependency And Font/Data Bundling

Нужно закрыть главный Linux gap для desktop-проектов:

- собрать список shared library dependencies у built executable;
- определить policy, какие библиотеки копируются в bundle, а какие считаются
  системными и остаются вне него;
- положить bundle-local runtime в `lib/`;
- убрать зависимость desktop acceptance path от hardcoded системного поиска шрифтов;
- добавить bundled font/data baseline и понятный runtime lookup order;
- настроить launcher на корректный `LD_LIBRARY_PATH`.

Практический `v1` фокус:

- `SDL2`;
- `SDL2_ttf`;
- базовый bundled font, на который desktop runtime может опереться без host probe;
- их прямые runtime dependencies, если они не входят в базовый allowlist
  системных библиотек.

Рекомендуемое техническое направление:

- generator/runtime сначала пытается открыть bundled font по относительному пути
  внутри export bundle;
- system font probe остаётся только как fallback для dev-path, но не как основа
  export acceptance.

Acceptance:

- `desktop_ui_flow` экспортируется в bundle;
- exported desktop bundle не зависит от host font layout как от обязательного условия;
- exported bundle проходит headless smoke вне исходного project tree;
- runtime больше не зависит от developer packages на машине проверки.

### Stage 4. Define Runtime Assets And Pack Payload Rules

Нужно честно определить, как export забирает не только `.so`, но и другие
runtime payload-ы.

Минимум для `v1`:

- поддержка project-local assets через предсказуемые директории и/или явный список;
- фиксация правил для imported pack runtime payload.

Важно:

- desktop font/data baseline не должен оставаться "опциональным asset story";
- он уже должен быть закрыт в Stage 3 как часть базового desktop export contract;
- Stage 4 закрывает более широкий asset/payload story поверх этого baseline.

Рекомендуемое правило `v1`:

- self-contained/vendored cases поддерживаются;
- imported pack-и с внешними shared libs или data files требуют явной export metadata;
- если такой metadata нет, export должен падать с понятной диагностикой, а не
  собирать silently broken artifact.

Acceptance:

- export не производит artifact, который "как будто собран", но не может
  стартовать из-за неучтённого runtime payload;
- ограничения documented и user-visible.

### Stage 5. Add Automated Verification

Нужны отдельные проверки именно exported artifact-а, а не просто build tree.

Обязательные проверки:

- console export bundle runs end-to-end;
- desktop export bundle runs end-to-end в headless режиме;
- exported bundle можно переместить во временную директорию и запустить оттуда;
- runtime не читает `build/` и не требует репозиторий DeltaQ рядом.
- есть clean-environment gate, который проверяет не только "работает на текущей машине",
  но и claim о self-contained runtime;
- есть loader-level proof, что non-allowlisted runtime libs берутся из bundle, а не
  молча резолвятся из host system.

Рекомендуемые deliverables:

- `tests/...` integration tests для export backend;
- shell smoke script для ручной и CI-проверки exported bundle.
- отдельный clean container/VM smoke;
- при необходимости дополнительная проверка через `ldd`, `readelf` или
  `LD_DEBUG=libs` для desktop bundle.

Acceptance:

- `build -> export -> run exported artifact` становится regression-covered path;
- desktop self-contained claim подтверждён не только на developer machine;
- проблемы export локализуются отдельно от compile/build проблем.

### Stage 6. Integrate Into IDE UX

Когда backend и smoke уже стабильны, нужно добавить пользовательский вход:

- action `Build -> Export Linux Bundle` с чёткой semantics:
  export включает rebuild, а не пакует неизвестно какой старый build;
- вывод статуса export в существующий output flow;
- понятную диагностику при missing runtime payload;
- default output path в `dist/`.

Acceptance:

- пользователь проходит путь `Build -> Export Linux Bundle` без внутреннего знания
  о layout и scripts;
- export воспринимается как штатная часть product workflow.

### Stage 7. Documentation And Handoff

После стабилизации реализации нужно обновить:

- `README`;
- onboarding;
- release/docs для Linux-first narrative;
- manual verification path для exported user projects.

Acceptance:

- DeltaQ можно честно показывать как IDE, которая не только собирает проект,
  но и выпускает готовую Linux-сборку пользовательского ПО.

## Acceptance Gates

- console project даёт runnable exported bundle вне project tree;
- desktop project даёт runnable exported bundle с нужным runtime payload;
- desktop project не зависит от host font layout как от скрытого runtime requirement;
- exported artifact не зависит от DeltaQ checkout;
- exported artifact резолвит реальный executable, а не build-system marker;
- user-facing export не пакует stale build silently;
- self-contained claim подтверждён clean-environment gate-ом;
- `.tar.gz` можно получить из того же validated bundle;
- scope и ограничения `v1` описаны честно;
- ошибки export видны как отдельный этап, а не смешиваются с build pipeline.

## Recommended Order

1. contract and scope lock;
2. console export backend;
3. desktop runtime bundling;
4. export smoke/regression;
5. IDE action and diagnostics;
6. docs and handoff.

## Deferred For Later

- user-project AppImage export;
- Windows/macOS export;
- code signing;
- universal cross-distro compatibility claim;
- advanced runtime payload model для всех типов external pack-ов.

## Progress Notes

- `2026-03-14`: создан новый execution track после переоценки Linux-first priorities.
- `2026-03-14`: зафиксировано, что packaging самой IDE и export пользовательского
  приложения являются разными delivery задачами.
- `2026-03-14`: `Linux-first release polish` остаётся открытым по visual proof и
  final narrative, но главный implementation focus смещён на `build -> export ->
  runnable Linux bundle`.
- `2026-03-14`: execution spec ужесточён после review:
  - убрана ошибочная опора на `BuildManager::expectedBuildArtifact(...)` как на
    resolver runtime binary;
  - desktop font/data baseline поднят в основной desktop export contract;
  - добавлен clean-environment verification gate;
  - зафиксировано, что user-facing export по умолчанию build-coupled.
- `2026-03-14`: реализован первый кодовый срез `Stage 2`:
  - добавлен `ProjectExecutableResolver`, который ищет реальный runtime executable,
    а не build-system marker;
  - добавлен `LinuxProjectExporter` для `console`-bundle в `dist/`;
  - `MainWindow` переведён на новый executable resolver для run/debug path;
  - autotest подтверждает `build -> export -> run exported bundle` вне project tree
    для console-проекта;
  - desktop export пока остаётся честно незакрытым и явно отклоняется exporter-ом.
- `2026-03-14`: реализован следующий execution slice:
  - `LinuxProjectExporter` расширен до локально runnable `desktop`-bundle с runtime
    `.so` payload и bundled font baseline;
  - `SDL2CodeGenerator` переведён на bundle-aware font lookup через
    `DELTAQ_FONT_PATH` / `DELTAQ_ASSET_ROOT`;
  - autotest подтверждает desktop bundle после удаления исходного project tree;
  - в `MainWindow` добавлен user-facing путь `Build -> Export Linux Bundle` с
    обязательным `pre-build -> build -> export`;
  - clean-environment / isolated verification всё ещё остаётся открытым gate и не
    считается закрытым только по локальному smoke.
- `2026-03-14`: реализован handoff/verification slice:
  - exporter умеет собирать `.tar.gz` из того же validated bundle;
  - появились `verify_project_export_bundle.sh` и
    `verify_project_export_archive.sh`;
  - regression-тест теперь покрывает extracted archive run после удаления исходного
    bundle directory;
  - launcher переписан без зависимости от внешнего `dirname`, поэтому handoff
    bundle не требует host `PATH` только ради собственного старта;
  - для desktop export добавлен loader-level proof через `ldd` + проверку того,
    что non-allowlisted libs резолвятся из `bundle/lib`;
  - clean-environment / container-level gate всё ещё остаётся отдельным незакрытым
    шагом.
- `2026-03-14`: user-facing path усилен ещё на один шаг:
  - startup automation поддерживает `open -> build -> export`;
  - CI/release workflow теперь гоняет `smoke_linux_example_export.sh`;
  - user-visible checklist и ограничения `v1` вынесены в отдельную release-документацию;
  - документированный scope теперь честно отличает Linux project export от packaging самой IDE.
- `2026-03-14`: clean-environment gate закрыт:
  - добавлен `verify_project_export_clean_env.sh` на базе `bubblewrap`;
  - extracted console и desktop bundle теперь проходят sandboxed run вне project tree;
  - `smoke_linux_example_export.sh` дополнен clean-env verification step;
  - CI/release workflow устанавливают `bubblewrap` и гоняют export smoke не только для `minimal_console_flow`, но и для `desktop_ui_flow`;
  - `test_LinuxProjectExporter` подтверждает clean-env path end-to-end.
