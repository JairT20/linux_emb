// app_context.h
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "mqtt_client.h"

#include "ultrasonic_driver.h"

// Comandos para el LED (antes estaban en app_main.c)
typedef enum {
    LED_CMD_OFF = 0,
    LED_CMD_ON,
    LED_CMD_TOGGLE
} led_cmd_t;

// Contexto global de la app, que comparten main, Wi-Fi y MQTT
typedef struct {
    EventGroupHandle_t       wifi_event_group;
    QueueHandle_t            distance_queue;   // float
    QueueHandle_t            led_cmd_queue;    // led_cmd_t
    esp_mqtt_client_handle_t mqtt_client;
    ultrasonic_sensor_t      ultrasonic;
} app_context_t;
