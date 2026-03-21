#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

static void test_pin(uint pin) {
    // Put the pin under SIO control and configure it as input.
    gpio_set_function(pin, GPIO_FUNC_SIO);
    gpio_set_dir(pin, GPIO_IN);
    gpio_set_input_enabled(pin, true);

    printf("GPIO%u\n", pin);

    gpio_disable_pulls(pin);
    sleep_ms(10);
    printf("  no pull   : gpio_get=%d gpio_get_pad=%d\n",
           gpio_get(pin), gpio_get_pad(pin));

    gpio_pull_up(pin);
    sleep_ms(10);
    printf("  pull-up   : gpio_get=%d gpio_get_pad=%d\n",
           gpio_get(pin), gpio_get_pad(pin));

    gpio_pull_down(pin);
    sleep_ms(10);
    printf("  pull-down : gpio_get=%d gpio_get_pad=%d\n",
           gpio_get(pin), gpio_get_pad(pin));
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    printf("NUM_BANK0_GPIOS=%d\n", (int)NUM_BANK0_GPIOS);

#ifdef PICO_RP2350A
    printf("PICO_RP2350A=%d\n", (int)PICO_RP2350A);
#else
    printf("PICO_RP2350A is not defined\n");
#endif

    for (int i = 0; i < (int)NUM_BANK0_GPIOS; i++) {
        test_pin(i);
    }

    while (1) sleep_ms(1000);
}
