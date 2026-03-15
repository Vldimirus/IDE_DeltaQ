# Core Timer Modules

## Роль категории

`timers` в checked-in `core` не возвращает DeltaQ к blocking sleep helpers.
Его задача в Stage 4 уже:

- зафиксировать monotonic timestamp;
- посчитать elapsed duration вокруг sync step;
- один раз проверить runtime budget.

Это useful baseline для коротких checks и tool-runner flow, а не scheduler framework.

## Essential

### `core.timers.now_ms`

- Роль: `essential`
- Зачем нужен: даёт monotonic timestamp для budget/elapsed сценариев
- Когда брать: если graph должен засечь момент перед внешним шагом или короткой runtime операцией
- Ограничение: возвращает `int`, поэтому не предназначен для долгоживущего timer state

### `core.timers.timeout_once`

- Роль: `essential`
- Зачем нужен: позволяет один раз проверить, вышел ли sync step за заданный timeout
- Когда брать: если после process/file/network шага нужен budget check без блокирующей задержки
- Ограничение: не является recurring timer-ом, event-loop API или scheduler-ом

## Convenience

### `core.timers.elapsed_ms`

- Роль: `convenience`
- Зачем нужен: сокращает graph вокруг elapsed reporting/logging
- Когда брать: если результат duration нужен как обычное число для stdout или дальнейшего решения
- Ограничение: helper для short-lived measurement path, а не для сложного timer state

## Практическое правило

Если сценарий укладывается в:

- `now_ms -> sync step -> elapsed_ms / timeout_once`

то `core.timers` используется правильно.

Если хочется interval scheduler, recurring tick, wall-clock formatting или calendar time,
это уже другой слой, а не compact Stage 4 baseline.
