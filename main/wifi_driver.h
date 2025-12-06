// wifi_driver.h
#pragma once

#include "esp_err.h"
#include "app_context.h"

// Inicializa Wi-Fi STA y bloquea hasta obtener IP
esp_err_t wifi_init_sta(app_context_t *ctx);
