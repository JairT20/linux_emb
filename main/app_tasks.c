#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_err.h"

#include "app_context.h"
#include "mqtt_driver.h"
#include "ultrasonic_driver.h"
#include "led_driver.h"

static const char *TAG_ULTRA = "ULTRA";
static const char *TAG_MQTT = "MQTT_PUB";
static const char *TAG_LED   = "LED_TASK";

/* ============================================================================
 *  Tarea: Lectura periódica del sensor ultrasónico (productor)
 * ============================================================================ */
void ultrasonic_reader_task(void *pvParameters)
{
    app_context_t *ctx = (app_context_t *)pvParameters;
    float distance_cm = 0.0f;

    ESP_LOGI(TAG_ULTRA, "Tarea ultrasónica iniciada");

    while (1) {
        if (ultrasonic_read_cm(&ctx->ultrasonic, &distance_cm) == ESP_OK) {
            ESP_LOGI(TAG_ULTRA, "Distancia: %.2f cm", distance_cm);
            
            if (ctx->distance_queue) {
                BaseType_t ok = xQueueSend(
                    ctx->distance_queue,
                    &distance_cm,
                    pdMS_TO_TICKS(10)
                );
                if (ok != pdPASS) {
                    ESP_LOGW(TAG_ULTRA, "Cola llena, distancia perdida");
                }
            }
        } else {
            ESP_LOGW(TAG_ULTRA, "Error al medir distancia");
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // 500 ms
    }
}

/* ============================================================================
 *  Tarea: Publicador MQTT del promedio cada 10s (consumidor)
 * ============================================================================ */
void mqtt_avg_publisher_task(void *pvParameters)
{
    app_context_t *ctx = (app_context_t *)pvParameters;

    const TickType_t window_ticks = pdMS_TO_TICKS(10000);
    float distance = 0.0f;
    char payload[32];

    ESP_LOGI(TAG_MQTT, "Tarea MQTT promedio iniciada");

    while (1) {
        TickType_t start = xTaskGetTickCount();
        float sum = 0.0f;
        uint32_t count = 0;

        while ((xTaskGetTickCount() - start) < window_ticks) {
            if (ctx->distance_queue &&
                xQueueReceive(ctx->distance_queue, &distance, pdMS_TO_TICKS(1000)) == pdPASS)
            {
                sum += distance;
                count++;
            }
        }

        if (count > 0) {
            float avg = sum / (float)count;
            snprintf(payload, sizeof(payload), "%.2f", avg);

            ESP_LOGI(TAG_MQTT, "Promedio 10s: %.2f cm (n=%ld)", avg, count);

            if (ctx->mqtt_client) {
                esp_mqtt_client_publish(
                    ctx->mqtt_client,
                    TOPIC_DISTANCIA,
                    payload,
                    0,
                    0,
                    0
                );
            }
        }
    }
}

/* ============================================================================
 *  Tarea: Control del LED según comandos (consumidor)
 * ============================================================================ */
void led_task(void *pvParameters)
{
    app_context_t *ctx = (app_context_t *)pvParameters;
    led_cmd_t cmd;

    ESP_LOGI(TAG_LED, "Tarea LED iniciada");

    while (1) {
        if (ctx->led_cmd_queue &&
            xQueueReceive(ctx->led_cmd_queue, &cmd, portMAX_DELAY) == pdPASS)
        {
            switch (cmd) {
            case LED_CMD_ON:     led_turn_on();  break;
            case LED_CMD_OFF:    led_turn_off(); break;
            case LED_CMD_TOGGLE: led_toggle();   break;
            default: break;
            }
        }
    }
}
