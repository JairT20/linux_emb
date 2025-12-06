// app_main.c
#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "app_context.h"
#include "wifi_driver.h"
#include "mqtt_driver.h"
#include "app_tasks.h"
#include "ultrasonic_driver.h"
#include "led_driver.h"

#define LED_CONTROL_PIN  GPIO_NUM_9

static const char *TAG = "MAIN_APP";

void app_main(void)
{
    // 🔊 Subir nivel de log para ver bien todo
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_driver", ESP_LOG_VERBOSE);
    esp_log_level_set("wifi_driver", ESP_LOG_VERBOSE);

    ESP_LOGI(TAG, "Iniciando aplicación ESP32-S3 Ultrasónico + MQTT");

    /* ---------- Inicializar NVS ---------- */
    ESP_ERROR_CHECK(nvs_flash_init());

    /* ---------- Crear contexto global ---------- */
    app_context_t *ctx = (app_context_t *)calloc(1, sizeof(app_context_t));
    if (!ctx) {
        ESP_LOGE(TAG, "Error al reservar memoria para app_context_t");
        return;
    }

    /* ---------- Inicializar el sensor ultrasónico ---------- */
    ctx->ultrasonic.trig_pin = 7;
    ctx->ultrasonic.echo_pin = 5;
    ESP_ERROR_CHECK(ultrasonic_init(&ctx->ultrasonic));

    /* ---------- Inicializar LED ---------- */
    led_init(LED_CONTROL_PIN);

    /* ---------- Crear colas ---------- */
    ctx->distance_queue = xQueueCreate(20, sizeof(float));
    ctx->led_cmd_queue  = xQueueCreate(10, sizeof(led_cmd_t));

    if (!ctx->distance_queue || !ctx->led_cmd_queue) {
        ESP_LOGE(TAG, "Error al crear colas");
        return;
    }

    /* ---------- Inicializar Wi-Fi (bloquea hasta tener IP) ---------- */
    ESP_ERROR_CHECK(wifi_init_sta(ctx));

    /* ---------- Inicializar MQTT (YA hay IP aquí) ---------- */
    ESP_ERROR_CHECK(mqtt_app_start(ctx));

    /* ---------- Crear tareas ---------- */
    xTaskCreate(
        ultrasonic_reader_task,
        "ultra_reader",
        4096,
        ctx,
        6,
        NULL
    );

    xTaskCreate(
        mqtt_avg_publisher_task,
        "mqtt_avg_publisher",
        4096,
        ctx,
        5,
        NULL
    );

    xTaskCreate(
        led_task,
        "led_task",
        2048,
        ctx,
        4,
        NULL
    );

    ESP_LOGI(TAG, "Inicialización completa. Tareas ejecutándose...");
    vTaskDelete(NULL); // app_main termina, el sistema sigue con las tareas
}
