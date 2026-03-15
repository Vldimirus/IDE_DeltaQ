# Serial Probe Console

Этот example показывает последний useful Stage 4 baseline для `serial + timers`.

Что здесь происходит:

1. graph создаёт локальный loopback PTY path;
2. открывает и настраивает serial fd в raw 8N1 mode;
3. отправляет `serial-probe` в loopback endpoint;
4. читает ответ обратно, измеряет elapsed time и проверяет timeout budget;
5. печатает результат и явно закрывает serial fd.

Ожидаемый вывод:

```text
received=serial-probe
elapsed_ms=<n>
timed_out=0
```

`elapsed_ms` зависит от среды и на очень быстром PTY loopback может быть `0`,
но `timed_out` должен оставаться `0`.
