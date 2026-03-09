# mini_checksum_sdk

`mini_checksum_sdk` — это маленькая fixture-library для второго imported-pack showcase в DeltaQ.

Она специально сделана:

- детерминированной;
- без внешних зависимостей;
- с `context`-типом, чтобы raw import показывал не только простые функции;
- достаточно маленькой, чтобы её можно было целиком аудировать в репозитории.

API:

- `mini_checksum_init(int seed)`
- `mini_checksum_push_text(mini_checksum_context *ctx, const char *text)`
- `mini_checksum_hex(mini_checksum_context *ctx)`
- `mini_checksum_match(mini_checksum_context *ctx, const char *expected_hex)`
- `mini_checksum_shutdown(mini_checksum_context *ctx)`
