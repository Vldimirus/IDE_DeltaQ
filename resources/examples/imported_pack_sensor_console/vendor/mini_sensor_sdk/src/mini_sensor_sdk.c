#include "mini_sensor_sdk.h"

#include <stdlib.h>
#include <string.h>

// Внутреннее представление контекста датчика.
struct mini_sensor_context {
    int base_value;
    int scale_factor;
    char device_name[64];
    char last_error[96];
};

// Вычисляет стабильное базовое значение по имени устройства без случайности и внешнего состояния.
static int mini_sensor_compute_base(const char *device_name)
{
    unsigned int sum = 0;
    const unsigned char *cursor = (const unsigned char *)(device_name ? device_name : "");
    while (*cursor) {
        sum += *cursor;
        ++cursor;
    }

    return 10 + (int)(sum % 41U);
}

// Обновляет строку последней ошибки внутри контекста.
static void mini_sensor_set_error(mini_sensor_context *ctx, const char *message)
{
    if (!ctx)
        return;

    if (!message)
        message = "";

    strncpy(ctx->last_error, message, sizeof(ctx->last_error) - 1);
    ctx->last_error[sizeof(ctx->last_error) - 1] = '\0';
}

mini_sensor_context *mini_sensor_init(const char *device_name)
{
    if (!device_name || !*device_name)
        return NULL;

    mini_sensor_context *ctx = (mini_sensor_context *)calloc(1, sizeof(mini_sensor_context));
    if (!ctx)
        return NULL;

    ctx->base_value = mini_sensor_compute_base(device_name);
    ctx->scale_factor = 1;
    strncpy(ctx->device_name, device_name, sizeof(ctx->device_name) - 1);
    ctx->device_name[sizeof(ctx->device_name) - 1] = '\0';
    mini_sensor_set_error(ctx, "");
    return ctx;
}

int mini_sensor_read(mini_sensor_context *ctx)
{
    if (!ctx)
        return -1;

    mini_sensor_set_error(ctx, "");
    return ctx->base_value * ctx->scale_factor;
}

int mini_sensor_scale(mini_sensor_context *ctx, int factor)
{
    if (!ctx)
        return -1;

    if (factor <= 0) {
        mini_sensor_set_error(ctx, "scale factor must be positive");
        return -1;
    }

    ctx->scale_factor = factor;
    mini_sensor_set_error(ctx, "");
    return mini_sensor_read(ctx);
}

const char *mini_sensor_last_error(mini_sensor_context *ctx)
{
    if (!ctx)
        return "sensor context is null";
    return ctx->last_error;
}

void mini_sensor_shutdown(mini_sensor_context *ctx)
{
    if (!ctx)
        return;
    free(ctx);
}
