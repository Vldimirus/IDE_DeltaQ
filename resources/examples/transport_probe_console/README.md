# Transport Probe Console

Этот example показывает datagram-first slice для `tcp_udp + timers`.

Что здесь происходит:

1. graph открывает UDP socket на loopback с ephemeral port;
2. узнаёт фактический локальный порт после bind;
3. отправляет `probe-packet` на этот же loopback endpoint;
4. получает datagram обратно, измеряет elapsed time и проверяет timeout budget;
5. печатает результат и закрывает socket.

Ожидаемый вывод:

```text
received=probe-packet
elapsed_ms=<n>
timed_out=0
```

`elapsed_ms` зависит от среды и на очень быстрых loopback-запусках может быть `0`,
но `timed_out` должен оставаться `0`.
