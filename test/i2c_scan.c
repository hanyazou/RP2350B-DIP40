#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    printf("I2C scan start\n");
    printf("i2c=%d, SDA=%d, SCL=%d\n",
           PICO_DEFAULT_I2C,
           PICO_DEFAULT_I2C_SDA_PIN,
           PICO_DEFAULT_I2C_SCL_PIN);

    i2c_init(i2c_default, 100 * 1000);

    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);

    // Safe even if the module already has pull-ups.
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    sleep_ms(10);

    for (int addr = 0; addr < 128; addr++) {
        uint8_t dummy;
        int ret = i2c_read_blocking(i2c_default, addr, &dummy, 1, false);
        if (ret >= 0) {
            printf("Found device at 0x%02x\n", addr);
        }
    }

    while (1) {
        sleep_ms(1000);
    }
}
