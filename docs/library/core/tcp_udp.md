# Core TCP / UDP Modules

## Роль категории

`tcp_udp` в checked-in `core` на текущем срезе не пытается покрыть весь transport stack.
Его первая цель уже:

- открыть локальный datagram endpoint;
- отправить и получить небольшой текстовый payload;
- остаться частью compact probe/integration graph.

Это datagram-first useful baseline, а не полный networking framework.

## Essential

### `core.tcp_udp.udp_bind`

- Роль: `essential`
- Зачем нужен: shortest path к локальному UDP endpoint для probe flow
- Когда брать: если graph должен открыть loopback transport socket без отдельного runtime glue
- Ограничение: bind-ится только на loopback и не покрывает multicast/broadcast/server patterns

### `core.tcp_udp.udp_send`

- Роль: `essential`
- Зачем нужен: shortest path к отправке небольшого text payload через UDP
- Когда брать: если нужен datagram exchange в probe/integration сценарии
- Ограничение: только IPv4 text payload и без richer transport semantics

### `core.tcp_udp.udp_receive`

- Роль: `essential`
- Зачем нужен: shortest path к приёму одного datagram с timeout boundary
- Когда брать: если graph ждёт небольшой ответ/echo без async event-loop слоя
- Ограничение: читает один datagram в фиксированный буфер и не отдаёт metadata source endpoint-а

## Convenience

### `core.tcp_udp.udp_local_port`

- Роль: `convenience`
- Зачем нужен: сокращает bind-on-0 probe flow
- Когда брать: если после ephemeral bind нужно узнать фактический локальный port
- Ограничение: helper только для уже открытого socket fd

### `core.tcp_udp.udp_close`

- Роль: `convenience`
- Зачем нужен: делает cleanup transport scenario явным
- Когда брать: если graph не хочет оставлять socket lifecycle неявным
- Ограничение: thin cleanup helper без richer shutdown/state model

## Практическое правило

Если сценарий укладывается в:

- `udp_bind -> udp_send -> udp_receive -> udp_close`

то `core.tcp_udp` используется по назначению.

Если нужны TCP sessions, servers, multicast, TLS, protocol decoders или long-lived
transport state machines, это уже следующий layer, а не compact Stage 4 baseline.
