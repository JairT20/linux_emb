#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int trig_pin;
    int echo_pin;
} ultrasonic_sensor_t;

// Inicializa el sensor
esp_err_t ultrasonic_init(ultrasonic_sensor_t *sensor);

// Obtiene la distancia en centímetros
esp_err_t ultrasonic_read_cm(ultrasonic_sensor_t *sensor, float *distance_cm);

#ifdef __cplusplus
}
#endif
