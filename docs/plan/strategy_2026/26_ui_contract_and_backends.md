# DeltaQ UI Contract And Backends

## Purpose

Зафиксировать стратегию, в которой UI в DeltaQ развивается как **модульная и backend-независимая система**, а текущая SDL2-реализация рассматривается не как окончательная суть UI, а как **временный и полезный backend v1**.

## Why It Matters

Если UI останется "SDL2-специфичной подсистемой", то:

- он будет хуже согласован с общей идеей DeltaQ;
- многоплатформенность начнёт протекать в пользовательские графы;
- пользовательские UI-модули будут зависеть от деталей конкретной библиотеки;
- переход к другим backend-ам станет болезненным.

Если же UI будет определён как модульный контракт, а SDL2 станет только backend-слоем, то:

- продуктовая идея "всё собирается из модулей" сохранится и для UI;
- пользовательский граф UI останется единым для разных платформ;
- переносимость будет решаться на уровне backend-пакетов, а не на уровне графов пользователя;
- дальнейшее развитие Win32/Linux-native backend-ов станет архитектурно возможным без слома пользовательской модели.

## Current State

Сейчас в проекте уже есть:

- UI Designer;
- generated UI sources;
- runtime-модули desktop/SDL;
- SDL2/SDL2_ttf как рабочий способ создать окно, обработать события и отрисовать UI.

Это уже даёт работоспособный путь, но пока смешивает:

- пользовательскую UI-модель;
- runtime UI-логику;
- backend SDL2.

## Target State

После реализации этой стратегии UI в DeltaQ должен выглядеть так:

1. Пользователь работает с **едиными UI-модулями DeltaQ**:
   - `ui.window`
   - `ui.button`
   - `ui.label`
   - `ui.text_field`
   - `ui.column`
   - `ui.row`
   - `ui.on_click`
   - и т.д.
2. Эти модули описывают **контракт UI**, а не SDL2/Win32/X11.
3. Внутри DeltaQ существует **единая runtime-модель UI**:
   - дерево виджетов;
   - layout;
   - state;
   - focus;
   - hit-testing;
   - dispatch событий;
   - theme/render primitives.
4. Конкретная платформа подключается как **backend**:
   - `backend_sdl2` для v1;
   - позже при необходимости:
     - `backend_win32`
     - `backend_linux_native`
     - другие backend-ы.
5. Пользовательский граф и `.dqui` не должны разветвляться на отдельные Windows/Linux версии только потому, что backend разный.

## Decisions

### 1. UI as contract, not as SDL2 API

Пользовательские UI-модули не должны быть:

- `sdl_button`
- `win_button`
- `linux_button`

Они должны быть абстрактными и переносимыми:

- `ui.button`
- `ui.label`
- `ui.container`
- `ui.text_field`

SDL2 и другие низкоуровневые технологии не должны протекать в пользовательский уровень.

### 2. Keep SDL2 as the first backend

На ближайшем горизонте не нужно выбрасывать SDL2.

SDL2 уже решает важные low-level задачи:

- создание окна;
- цикл событий;
- ввод;
- базовый рендер;
- переносимость между ОС.

Поэтому правильный путь:

- **не убирать SDL2 сейчас**;
- **перестать считать SDL2 сутью UI**;
- **перевести SDL2 в роль backend v1**.

### 3. No platform-specific graphs for users

Нельзя строить пользовательскую модель так, чтобы один и тот же UI собирался через:

- `win_button`
- `linux_button`

Это приведёт к дублированию графов и разрушит идею модульной переносимости.

Платформенные различия должны жить в backend-слое, а не в пользовательских графах.

### 4. Three-layer UI architecture

UI-подсистема должна быть разделена на три слоя.

#### Layer A. UI Contract Layer

Это пользовательский и продуктовый уровень:

- набор UI-модулей;
- их входы/выходы;
- события;
- свойства;
- layout semantics.

Этот слой должен быть backend-независимым.

#### Layer B. UI Runtime Model

Это внутренняя модель исполнения UI:

- дерево виджетов;
- state store;
- focus/hover/pressed;
- layout calculation;
- event routing;
- render tree;
- theme primitives.

Этот слой не должен быть привязан к SDL2 API напрямую.

#### Layer C. UI Backend Layer

Это конкретная реализация под платформу:

- окно;
- surface/renderer;
- fonts;
- текст;
- input events;
- frame presentation.

Для v1 таким backend-ом остаётся SDL2.

### 5. Widget modules vs render primitives

Нужно разделить:

- **primitive rendering modules / runtime primitives**
  - draw rect
  - draw border
  - draw text
  - measure text
  - clip region
- **widget modules**
  - button
  - label
  - text field
  - checkbox
  - slider
- **layout modules**
  - row
  - column
  - stack
  - anchor
- **event modules**
  - on_click
  - on_change
  - on_focus

Виджет не должен каждый раз заново решать текст, hit-test и базовую геометрию.

### 6. Backend packages may differ by OS

Многоплатформенность можно реализовывать так:

- под Windows подключается один backend package;
- под Linux другой;
- но contract layer остаётся тем же.

То есть различаться могут backend-модули и runtime-адаптеры, но не пользовательская модель UI.

## Implementation Directions

### Workstream 1. Define UI contract layer

Нужно описать базовый набор UI-контрактов:

- что такое window;
- что такое widget;
- какие обязательные свойства есть у виджетов;
- как выражаются layout relationships;
- как выражаются UI events;
- как выражается widget state.

Для v1 не нужно делать полный GUI framework.
Нужно определить минимально достаточный и строгий contract layer.

### Workstream 2. Extract runtime model from SDL2 specifics

Нужно выделить из существующей SDL2-реализации то, что относится не к SDL2, а к самой модели UI:

- дерево UI state;
- layout;
- render commands;
- event dispatch;
- lifecycle of frame.

После этого SDL2-код должен остаться главным образом backend-адаптером.

### Workstream 3. Rebuild UI modules around the contract

Нужно, чтобы UI-модули в DeltaQ описывали именно UI contract, а не детали SDL2.

Это касается:

- `UIModuleFactory`;
- generated UI modules;
- `.dqui`;
- runtime-модулей для desktop/UI.

### Workstream 4. Backend abstraction boundary

Нужно ввести внутренний интерфейс backend-а:

- create window
- poll events
- begin frame
- draw primitives
- present frame
- load font
- measure text
- destroy resources

SDL2 должен стать одной конкретной реализацией этого интерфейса.

### Workstream 5. Platform backend strategy

После стабилизации contract/runtime boundary можно решать:

- нужен ли `backend_win32`;
- нужен ли `backend_linux_native`;
- или SDL2 остаётся основным backend-ом надолго.

Этот шаг не должен быть первым.

## Phase Plan

### Stage 1. UI contract definition

Определить минимальный backend-agnostic UI contract для DeltaQ.

Нужно зафиксировать:

- обязательные типы UI-модулей;
- свойства виджетов;
- модель layout;
- модель событий;
- модель состояния.

Критерий готовности:

- можно описать UI без упоминания SDL2/Win32/Linux-specific виджетов.

Текущий статус:

- базовый UI contract metadata введён в `Module` и `UILayout`;
- `UIModuleFactory` больше не выдаёт SDL2-реализацию как пользовательский UI-модуль;
- пользовательские UI-модули описываются как contract-модули с widget type, properties, events и state outputs;
- `SDL2CodeGenerator` явно помечается как backend `SDL2`, а не как определение пользовательского контракта.

### Stage 2. SDL2 as backend v1

Оставить SDL2 рабочим backend-ом, но зафиксировать его как backend-реализацию, а не как продуктовую модель UI.

Нужно реализовать:

- backend boundary;
- mapping SDL2 events -> DeltaQ UI events;
- mapping DeltaQ render commands -> SDL2 drawing.

Критерий готовности:

- пользовательский UI contract больше не зависит концептуально от SDL2.

Текущий статус:

- generated `main.c` для `.dqui` больше не управляет окном, событиями и кадром через россыпь прямых `SDL_*` вызовов;
- в generated UI header/source введена явная boundary-модель:
  - `DQ_UIBackendContext`
  - `DQ_UIBackendRenderer`
  - `DQ_UIBackendEvent`
  - `dq_ui_backend_*` функции;
- SDL2-специфика окна, event pump и frame lifecycle вынесена в backend helper functions внутри generated `ui.c`;
- inline desktop runtime codegen в `GraphCompiler` тоже переведён на `dq_ui_backend_*` boundary;
- `core.desktop.*` модули помечены metadata как `ui_backend_runtime` слоя `backend` для `sdl2`.

### Stage 3. Runtime model consolidation

Выделить общий UI runtime:

- layout engine;
- widget tree;
- state;
- focus/input/event dispatch.

Критерий готовности:

- backend меняется отдельно от пользовательской модели UI.

Текущий статус:

- в generated UI введён нормализованный `DQ_UIRuntimeEvent`, который стал внутренней моделью UI-события поверх SDL2 backend;
- `dq_ui_backend_translate_event(...)` теперь выполняет перевод `SDL_Event -> DQ_UIRuntimeEvent`;
- `ui_handle_event(...)` больше не зависит от `SDL_Event` и работает только с runtime-событием;
- устаревший backend helper `dq_ui_backend_event_is_quit(...)` больше не нужен, потому что завершение теперь выражается через `DQ_UIRuntimeEvent_Quit`;
- в `UIState` введён `DQ_UIRuntimeContext`, который хранит `hovered/focused/active` widget и позицию указателя как backend-независимое внутреннее состояние UI;
- hit-test, sync render-state и базовый event routing вынесены в отдельные runtime helper functions:
  - `dq_ui_runtime_hit_test`
  - `dq_ui_runtime_sync_state`
  - `dq_ui_runtime_dispatch_click`
  - `dq_ui_runtime_*` для text/wheel/slider обработки;
- generated runtime получил явный layout pass `ui_apply_layout(...)`, который вычисляет абсолютные `rect` из `base_rect` и применяет:
  - `children`-иерархию
  - `layout = HBox/VBox/Grid/Flow`
  - `anchors` для контейнеров без auto-layout;
- layout и anchors теперь выражаются через явные runtime spec-структуры:
  - `DQ_UIRuntimeLayoutSpec`
  - `DQ_UIRuntimeAnchorSpec`;
- SDL2 backend теперь отдаёт размер окна через `dq_ui_backend_window_size(...)`, а runtime layout пересчитывается перед рендером;
- inline desktop runtime codegen в `GraphCompiler` использует тот же `DQ_UIRuntimeEvent`, что и `.dqui` pipeline;
- критерий Stage 3 достигнут: runtime-модель UI теперь отделена от SDL2 и включает события, состояние и размещение;
- следующий крупный незакрытый вопрос уже относится к `Stage 4`: выравнивание contract vocabulary между `.dqui`, UI-модулями и generated runtime.

### Stage 4. Module and `.dqui` alignment

Согласовать:

- UI-модули;
- `.dqui`;
- generated UI code;
- runtime modules.

Критерий готовности:

- нет раздвоения между "UI Designer моделью" и "модульной UI моделью DeltaQ".

Текущий статус:

- введён единый header `include/deltaq/UIContract.h`, который стал общим словарём UI-контрактов;
- в этом словаре теперь зафиксированы:
  - канонический `contractType`
  - legacy widget type для `.dqui` и дизайнера
  - display name и palette category
  - default size
  - default event
  - designer properties
  - module ports для тех контрактов, которые уже доступны как UI-модули;
- `UIWidget` теперь хранит contract metadata и умеет восстанавливать её даже для старых `.dqui`, где metadata ещё не была записана;
- `WidgetPalette`, `DesignScene`, `UIDesignerWidget` и `UIModuleFactory` переведены на один и тот же каталог вместо локальных списков и `if/else`-таблиц;
- `GraphCompiler` переведён на более общий разбор `deltaq.ui.events` и одноимённых state outputs, а не на отдельные branch-списки по типам виджетов;
- этим закрыт первый практический срез Stage 4:
  - палитра,
  - `.dqui`,
  - UI contract-модули
  уже используют единый vocabulary;
- затем выравнивание доведено и до оставшихся слоёв:
  - `SDL2CodeGenerator` теперь опирается на `contractType`, а не на legacy `w->type` при выборе runtime/render логики;
  - `PropertyEditor` строит type-specific свойства из общего UI-каталога, поэтому дизайнер больше не показывает отдельный локальный словарь полей;
  - `ObjectTreeWidget` и `ModuleManagerWidget` показывают display name и contract type в пользовательском UI;
  - generated UI runtime корректно продолжает работать даже если legacy `type` изменён, но metadata contract сохранена;
- критерий Stage 4 достигнут:
  - нет расхождения между `.dqui`,
  - UI contract-модулями,
  - generated UI/runtime,
  - и пользовательски видимой contract model в IDE.

### Stage 5. Optional native backends

Только после стабилизации предыдущих слоёв решать:

- Win32 backend;
- Linux-native backend;
- другие платформы.

Критерий готовности:

- второй backend подключается без переделки пользовательских UI-графов.

## Acceptance Criteria

- UI в DeltaQ можно описать как набор backend-независимых модулей.
- SDL2 остаётся рабочим, но больше не является единственной продуктовой моделью UI.
- Пользовательский UI graph не содержит Windows/Linux-specific модулей.
- Существующий UI workflow остаётся рабочим на SDL2 backend.
- Архитектурно появляется место для других backend-ов без слома `.dqui` и UI-модулей.

## Deferred / Not In This Phase

- Полноценная замена SDL2 на собственный низкоуровневый оконный стек.
- Реализация native backend-ов сразу для всех платформ.
- Production-grade text stack уровня больших GUI toolkit-ов.
- Сложная тема accessibility, IME, advanced text editing и богатых системных интеграций до стабилизации базовой модели.
