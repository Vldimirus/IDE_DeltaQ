# Process Timer Console

Этот example показывает второй practical Stage 4 baseline для `process + timers`.

Что здесь происходит:

1. graph запускает `../tools/mock_worker.sh` через `/bin/sh` и захватывает stdout;
2. сразу после этого измеряет elapsed time и проверяет, вышел ли шаг за budget;
3. затем повторно запускает тот же helper для проверки `exit_code`;
4. снова измеряет elapsed time и печатает оба результата.

Ожидаемый вывод:

```text
stdout=worker-ready
stdout_elapsed_ms=<n>
stdout_timed_out=0
exit_code=0
exit_elapsed_ms=<n>
exit_timed_out=0
```

Значения `<n>` зависят от среды, но должны быть положительными и заметно меньше
`500` ms budget.
