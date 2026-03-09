# Minimal Console Flow

Этот проект-пример предназначен как самый короткий путь первого знакомства с DeltaQ.

Что здесь есть:

- один корневой граф `graphs/main.dqgraph`;
- только стандартные core-модули:
  - `core.io.string_constant`
  - `core.io.println`
  - `core.io.read_line`
- generated `src/main.c`, который целиком пересобирается из графа на этапе pre-build.

Сценарий выполнения:

1. граф печатает приветствие `Hello from DeltaQ!`;
2. программа ждёт одну строку из stdin;
3. введённая строка сразу печатается обратно.

Ожидаемый вывод для ввода `DeltaQ`:

```text
Hello from DeltaQ!
DeltaQ
```

Что важно:

- это минимальный воспроизводимый путь `module -> graph -> generated code -> build -> run`;
- пример достаточно маленький, чтобы целиком открыть граф и тут же проверить generated `src/main.c`;
- после него логично переходить к `reusable_composition_console` и `imported_pack_sensor_console`.
