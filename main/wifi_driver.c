// wifi_driver.c
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "wifi_driver.h"
#include "app_context.h"

#define WIFI_CONNECTED_BIT   BIT0

#define WIFI_SSID            CONFIG_APP_WIFI_SSID
#define WIFI_PASS            CONFIG_APP_WIFI_PASS

static const char *TAG = "wifi_driver";

/* ======== Funciones auxiliares ======== */

static void log_sta_mac(void)
{
    uint8_t mac[6];
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
        ESP_LOGI(TAG, "MAC STA ESP32-S3: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        ESP_LOGW(TAG, "No se pudo obtener la MAC de la interfaz STA.");
    }
}

/* ======== Manejador de eventos WiFi/IP ======== */

static void wifi_event_handler(void* arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void* event_data)
{
    app_context_t *ctx = (app_context_t *)arg;

    ESP_LOGI(TAG, "wifi_event_handler: base=%s, id=%ld",
             event_base, (long)event_id);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START -> conectando...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi desconectado, reintentando...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "IP obtenida: " IPSTR, IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(ctx->wifi_event_group, WIFI_CONNECTED_BIT);
        log_sta_mac();
    }
}

/* ======== Inicialización Wi-Fi STA ======== */

esp_err_t wifi_init_sta(app_context_t *ctx)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ctx->wifi_event_group = xEventGroupCreate();
    if (ctx->wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Error creando EventGroup de Wi-Fi");
        return ESP_FAIL;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        ctx,
        &instance_any_id
    ));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        ctx,
        &instance_got_ip
    ));

    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Conectando a Wi-Fi '%s'...", WIFI_SSID);

    // 👇 Aquí nos quedamos bloqueados hasta que IP_EVENT_STA_GOT_IP ponga el bit
    xEventGroupWaitBits(
        ctx->wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY
    );
    ESP_LOGI(TAG, "Wi-Fi conectado (ya hay IP).");

    return ESP_OK;
}
