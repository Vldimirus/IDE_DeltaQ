# Core Conversion Modules

## Роль категории

`conversion` в `core` нужен не сам по себе, а как связующий слой между
категориями:

- `input/output`;
- `text`;
- `math`;
- простыми typed API в графе.

Это не богатая система преобразований, а только тот минимум, который делает
основной workflow DeltaQ связным.

## Essential

### `core.conversion.int_to_string`

- Роль: `essential`
- Зачем нужен: основной путь от `int` к text output и string-based API
- Когда брать: если число нужно вывести, залогировать или передать в UI text
- Почему essential: это ключевая composable-альтернатива узким typed-output shortcut-ам

### `core.conversion.string_to_int`

- Роль: `essential`
- Зачем нужен: базовый путь от user input к числовой логике
- Когда брать: после `read_line` и в простых input-driven графах
- Ограничение: использует `atoi`, то есть не даёт отдельного канала диагностики ошибок

## Legacy

### `core.conversion.float_to_int`

- Роль: `legacy`
- Что делает: отбрасывает дробную часть и возвращает `int`
- Когда брать: только для совместимости со старыми графами
- Почему выведен из baseline: partial float conversion в `core` не образует достаточно сильный numeric baseline

### `core.conversion.int_to_float`

- Роль: `legacy`
- Что делает: преобразует `int` в `float`
- Когда брать: только для совместимости со старыми графами
- Почему выведен из baseline: partial float conversion в `core` не образует достаточно сильный numeric baseline

## Практическое правило

Если нужен новый conversion-модуль, сначала нужно проверить:

- он связывает действительно разные части baseline;
- или он просто экономит один частный шаг ради удобства.

В `core` должны оставаться те преобразования, которые помогают композиции,
а не раздувают библиотеку множеством редких typed helper-ов.
