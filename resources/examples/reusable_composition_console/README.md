# Reusable Composition Console

Этот пример показывает практическую ценность составных модулей в DeltaQ.

Что здесь есть:

- составной модуль `echo_with_prefix`
- составной модуль `measure_text`
- повторное использование `echo_with_prefix` в двух местах корневого графа
- отдельные generated `echo_with_prefix.h/.c` и `measure_text.h/.c`

Сценарий выполнения:

1. программа читает строку из stdin;
2. подмодуль `echo_with_prefix` печатает `Hello, <ввод>`;
3. подмодуль `measure_text` вычисляет длину строки;
4. тот же `echo_with_prefix` повторно используется для печати `Length: <длина>`.

Ожидаемый вывод для ввода `DeltaQ`:

```text
Hello, DeltaQ
Length: 6
```
