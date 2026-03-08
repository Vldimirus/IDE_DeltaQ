# Core Desktop Modules

## Роль категории

`desktop` в `core` — это не пользовательский UI API, а backend-runtime слой
для desktop pipeline DeltaQ.

Эти модули нужны, чтобы:

- поднять SDL2 lifecycle;
- создать окно и renderer;
- инициализировать generated UI state;
- передать управление event loop;
- корректно освободить ресурсы.

То есть категория `desktop` относится к инфраструктуре исполнения, а не к
предметной логике приложения.

## Essential

### Lifecycle

#### `core.desktop.sdl_init`

- Роль: `essential`
- Когда брать: в начале desktop/UI runtime на backend `SDL2`

#### `core.desktop.ttf_init`

- Роль: `essential`
- Когда брать: если generated UI использует text/font rendering через `SDL_ttf`

#### `core.desktop.ttf_quit`

- Роль: `essential`
- Когда брать: при штатном завершении text backend

#### `core.desktop.sdl_quit`

- Роль: `essential`
- Когда брать: как финальный lifecycle-шаг desktop runtime

### Window And Renderer

#### `core.desktop.create_window`

- Роль: `essential`
- Когда брать: для создания основного desktop-окна

#### `core.desktop.create_renderer`

- Роль: `essential`
- Когда брать: после окна, если дальше будет generated UI/rendering

#### `core.desktop.destroy_renderer`

- Роль: `essential`
- Когда брать: при завершении после окончания render phase

#### `core.desktop.destroy_window`

- Роль: `essential`
- Когда брать: при завершении desktop-flow после освобождения renderer

### Generated UI Runtime

#### `core.desktop.ui_init`

- Роль: `essential`
- Когда брать: если нужно создать `ui_state` для generated desktop UI

#### `core.desktop.event_loop`

- Роль: `essential`
- Когда брать: когда управление передаётся главному runtime loop приложения

#### `core.desktop.ui_cleanup_font`

- Роль: `essential`
- Когда брать: если `ui_init` создал font/state ресурсы, которые нужно освободить

## Важное ограничение категории

Все `core.desktop.*` модули:

- завязаны на текущий backend `SDL2`;
- относятся к backend/runtime-слою;
- не должны восприниматься как универсальные переносимые UI-модули пользователя.

Пользовательский уровень DeltaQ UI должен жить выше этого слоя, в contract-модели.

## Практическое правило

Если модуль:

- поднимает окно;
- работает с renderer;
- управляет event loop;
- освобождает backend resources,

то он кандидат в `desktop` backend layer.

Если же модуль описывает пользовательский виджет или поведение интерфейса,
его место не в `core.desktop`, а в UI contract/model layer.
