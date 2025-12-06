// main/led_driver.h

#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include "driver/gpio.h"

/**
 * @brief Inicializa un pin GPIO para controlar un LED.
 * 
 * @param pin El número de pin GPIO a utilizar para el LED.
 */
void led_init(gpio_num_t pin);

/**
 * @brief Enciende el LED.
 */
void led_turn_on(void);

/**
 * @brief Apaga el LED.
 */
void led_turn_off(void);

/**
 * @brief Cambia el estado del LED (si está encendido lo apaga, y viceversa).
 */
void led_toggle(void);

#endif // LED_DRIVER_H