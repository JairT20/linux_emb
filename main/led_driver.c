// main/led_driver.c

#include "led_driver.h"

// Variable estática (privada a este archivo) para guardar el pin configurado
static gpio_num_t s_led_pin;
static int s_led_state = 0; // 0 = OFF, 1 = ON

void led_init(gpio_num_t pin)
{
    s_led_pin = pin;
    gpio_reset_pin(s_led_pin);
    gpio_set_direction(s_led_pin, GPIO_MODE_OUTPUT);
    // Asegurarse de que el LED empieza apagado
    led_turn_off(); 
}

void led_turn_on(void)
{
    s_led_state = 1;
    gpio_set_level(s_led_pin, s_led_state);
}

void led_turn_off(void)
{
    s_led_state = 0;
    gpio_set_level(s_led_pin, s_led_state);
}

void led_toggle(void)
{
    s_led_state = !s_led_state;
    gpio_set_level(s_led_pin, s_led_state);
}