#include "ultrasonic_driver.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "ULTRASONIC";

#define TIMEOUT_US 30000  // 30 ms = ~5 m distancia

esp_err_t ultrasonic_init(ultrasonic_sensor_t *sensor)
{
    if (!sensor) return ESP_ERR_INVALID_ARG;

    // Configurar TRIG como salida
    gpio_config_t trig_conf = {
        .pin_bit_mask = 1ULL << sensor->trig_pin,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&trig_conf);

    // Configurar ECHO como entrada
    gpio_config_t echo_conf = {
        .pin_bit_mask = 1ULL << sensor->echo_pin,
        .mode = GPIO_MODE_INPUT,
    };
    gpio_config(&echo_conf);

    gpio_set_level(sensor->trig_pin, 0);
    return ESP_OK;
}

esp_err_t ultrasonic_read_cm(ultrasonic_sensor_t *sensor, float *distance_cm)
{
    if (!sensor || !distance_cm) return ESP_ERR_INVALID_ARG;

    // 1) Pulso TRIG 10us
    gpio_set_level(sensor->trig_pin, 0);
    esp_rom_delay_us(2);
    gpio_set_level(sensor->trig_pin, 1);
    esp_rom_delay_us(10);
    gpio_set_level(sensor->trig_pin, 0);

    // 2) Esperar inicio del pulso ECHO → HIGH
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(sensor->echo_pin) == 0) {
        if ((esp_timer_get_time() - start) > TIMEOUT_US)
            return ESP_ERR_TIMEOUT;
    }

    // 3) Medir duración del pulso HIGH
    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(sensor->echo_pin) == 1) {
        if ((esp_timer_get_time() - echo_start) > TIMEOUT_US)
            return ESP_ERR_TIMEOUT;
    }
    int64_t echo_end = esp_timer_get_time();

    int64_t duration_us = echo_end - echo_start;

    // Fórmula distancia en cm
    *distance_cm = (duration_us * 0.0343f) / 2.0f;

    return ESP_OK;
}
