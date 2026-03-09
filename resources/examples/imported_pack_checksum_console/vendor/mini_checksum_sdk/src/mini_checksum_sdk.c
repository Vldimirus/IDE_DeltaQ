#include "mini_checksum_sdk.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct mini_checksum_context {
    uint32_t state;
    char hex[9];
};

static void mini_checksum_refresh_hex(mini_checksum_context *ctx)
{
    if (!ctx)
        return;
    snprintf(ctx->hex, sizeof(ctx->hex), "%08X", ctx->state);
}

mini_checksum_context *mini_checksum_init(int seed)
{
    mini_checksum_context *ctx = (mini_checksum_context *)malloc(sizeof(*ctx));
    if (!ctx)
        return NULL;

    ctx->state = 2166136261u ^ (uint32_t)seed;
    mini_checksum_refresh_hex(ctx);
    return ctx;
}

int mini_checksum_push_text(mini_checksum_context *ctx, const char *text)
{
    if (!ctx || !text)
        return -1;

    for (const unsigned char *cursor = (const unsigned char *)text; *cursor; ++cursor) {
        ctx->state ^= (uint32_t)(*cursor);
        ctx->state *= 16777619u;
    }

    mini_checksum_refresh_hex(ctx);
    return (int)(ctx->state & 0x7FFFFFFFu);
}

const char *mini_checksum_hex(mini_checksum_context *ctx)
{
    if (!ctx)
        return "";
    return ctx->hex;
}

int mini_checksum_match(mini_checksum_context *ctx, const char *expected_hex)
{
    if (!ctx || !expected_hex)
        return -1;
    return strcmp(ctx->hex, expected_hex) == 0 ? 1 : 0;
}

void mini_checksum_shutdown(mini_checksum_context *ctx)
{
    free(ctx);
}
