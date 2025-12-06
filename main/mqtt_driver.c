// mqtt_driver.c
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt_driver.h"

static const char *TAG = "mqtt_driver";

/* ======== código movido desde app_main.c ======== */

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    app_context_t *ctx = (app_context_t *)handler_args;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT conectado al broker");
        esp_mqtt_client_publish(ctx->mqtt_client, TOPIC_STATUS, "online", 0, 1, 1);
        esp_mqtt_client_subscribe(ctx->mqtt_client, TOPIC_LED_CONTROL, 1);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT desconectado");
        break;

    case MQTT_EVENT_DATA: {
        ESP_LOGI(TAG, "MQTT DATA topic=%.*s data=%.*s",
                 event->topic_len, event->topic,
                 event->data_len, event->data);

        const char *topic_led = TOPIC_LED_CONTROL;
        size_t topic_led_len = strlen(topic_led);

        if (event->topic_len == (int)topic_led_len &&
            strncmp(event->topic, topic_led, topic_led_len) == 0)
        {
            led_cmd_t cmd;
            if (event->data_len >= 1 && event->data[0] == '1') {
                cmd = LED_CMD_ON;
                ESP_LOGI(TAG, "Orden MQTT: ENCENDER LED");
            } else if (event->data_len >= 1 && event->data[0] == '0') {
                cmd = LED_CMD_OFF;
                ESP_LOGI(TAG, "Orden MQTT: APAGAR LED");
            } else {
                cmd = LED_CMD_TOGGLE;
                ESP_LOGI(TAG, "Orden MQTT: TOGGLE LED");
            }

            if (ctx->led_cmd_queue) {
                BaseType_t ok = xQueueSend(ctx->led_cmd_queue, &cmd, pdMS_TO_TICKS(10));
                if (ok != pdPASS) {
                    ESP_LOGW(TAG, "Cola de LED llena, comando perdido");
                }
            }
        }
        break;
    }

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Error en MQTT");
        break;

    default:
        break;
    }
}

esp_err_t mqtt_app_start(app_context_t *ctx)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_APP_MQTT_URI,
        .credentials.client_id = "esp32s3-ultra-client"
    };

    ctx->mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        ctx->mqtt_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        ctx
    ));
    ESP_ERROR_CHECK(esp_mqtt_client_start(ctx->mqtt_client));

    return ESP_OK;
}
