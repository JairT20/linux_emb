// mqtt_driver.h
#pragma once

#include "esp_err.h"
#include "app_context.h"

// Tópicos (los usas en tasks y en MQTT)
#define TOPIC_STATUS         "lab/esp32s3/status"
#define TOPIC_DISTANCIA      "lab/esp32s3/ultrasonico/promedio"
#define TOPIC_LED_CONTROL    "esp32s3/led/control"

// Inicializa cliente MQTT y arranca la conexión
esp_err_t mqtt_app_start(app_context_t *ctx);
