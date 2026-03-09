# Desktop UI Flow

Этот проект-пример показывает desktop/UI сценарий DeltaQ как самостоятельный reference flow.

Что здесь есть:

- корневой граф `graphs/main.dqgraph` с полным SDL2 lifecycle;
- UI layout `ui/window1.dqui`;
- кастомный `src/ui/window1_events.c`, который содержит живые обработчики и `ui_update()`;
- generated `src/main.c`, `src/ui/window1.h` и `src/ui/window1.c`, которые создаются на этапе pre-build.

Сценарий выполнения:

1. граф инициализирует SDL2 и UI runtime;
2. окно показывает счётчик, поле ввода, две кнопки и текстовый лог;
3. `ui_update()` увеличивает счётчик раз в секунду;
4. кнопка `Copy to Log` пишет текущее значение счётчика в лог;
5. кнопка `Text to Log` переносит текст из `TextField` в `TextArea`.

Что важно:

- пример показывает связку `graph + .dqui + generated UI runtime`;
- runtime-логика живёт в `src/ui/window1_events.c` и не должна теряться на pre-build;
- для headless/regression запуска можно задать `DQ_DESKTOP_UI_FLOW_AUTOCLOSE_MS`, чтобы пример сам послал `SDL_QUIT`.
