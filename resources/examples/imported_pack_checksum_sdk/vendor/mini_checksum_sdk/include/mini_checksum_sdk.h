#ifndef MINI_CHECKSUM_SDK_H
#define MINI_CHECKSUM_SDK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mini_checksum_context mini_checksum_context;

mini_checksum_context *mini_checksum_init(int seed);
int mini_checksum_push_text(mini_checksum_context *ctx, const char *text);
const char *mini_checksum_hex(mini_checksum_context *ctx);
int mini_checksum_match(mini_checksum_context *ctx, const char *expected_hex);
void mini_checksum_shutdown(mini_checksum_context *ctx);

#ifdef __cplusplus
}
#endif

#endif
