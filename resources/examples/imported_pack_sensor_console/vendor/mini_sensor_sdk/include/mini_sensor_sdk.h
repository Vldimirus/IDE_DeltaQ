#ifndef MINI_SENSOR_SDK_H
#define MINI_SENSOR_SDK_H

#ifdef __cplusplus
extern "C" {
#endif

// Контекст фиктивного датчика. Для showcase-проекта это аналог opaque handle из внешнего SDK.
typedef struct mini_sensor_context mini_sensor_context;

// Создаёт контекст датчика по имени устройства.
mini_sensor_context *mini_sensor_init(const char *device_name);

// Возвращает детерминированное "измерение" для текущего устройства и масштаба.
int mini_sensor_read(mini_sensor_context *ctx);

// Меняет коэффициент масштаба. Возвращает новое измерение или -1 при ошибке.
int mini_sensor_scale(mini_sensor_context *ctx, int factor);

// Возвращает текст последней ошибки или пустую строку.
const char *mini_sensor_last_error(mini_sensor_context *ctx);

// Освобождает контекст датчика.
void mini_sensor_shutdown(mini_sensor_context *ctx);

#ifdef __cplusplus
}
#endif

#endif // MINI_SENSOR_SDK_H
