// app_tasks.h
#pragma once

#include "app_context.h"

// Prototipos de las tareas
void ultrasonic_reader_task(void *pvParameters);
void mqtt_avg_publisher_task(void *pvParameters);
void led_task(void *pvParameters);
