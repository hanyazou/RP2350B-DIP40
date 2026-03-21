#include "pico/stdlib.h"

#define LED_DELAY_MS 500
const int led_pins[] = { 24, 25 };
const int num_led_pins = sizeof(led_pins)/sizeof(*led_pins);

int main() {
    timer_hw->dbgpause = 0;   // Do not pause the RP2040 timer while halted in the debugger

    // Initialize the GPIO for the LED
    for (int i = 0; i < num_led_pins; i++) {
        gpio_init(led_pins[i]);
        gpio_set_dir(led_pins[i], GPIO_OUT);
    }

    int i = 0;
    int last_led_pin = led_pins[0];
    while (true) {
        gpio_put(last_led_pin, true);  // Turn the LED on or off
        gpio_put(led_pins[i], false);  // Turn the LED on or on
        last_led_pin = led_pins[i];
        if (num_led_pins <= ++i) {
            i = 0;
        }
        sleep_ms(LED_DELAY_MS);
    }
}
