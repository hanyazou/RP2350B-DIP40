#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#define DS3231_ADDR 0x68

static uint8_t bin_to_bcd(int v) {
    return (uint8_t)(((v / 10) << 4) | (v % 10));
}

static int bcd_to_bin(uint8_t v) {
    return ((v >> 4) * 10) + (v & 0x0f);
}

static bool ds3231_set_datetime(int year, int month, int day, int hour, int minute, int second) {
    uint8_t buf[8];

    // DS3231 register map starts at 0x00.
    buf[0] = 0x00;
    buf[1] = bin_to_bcd(second);
    buf[2] = bin_to_bcd(minute);
    buf[3] = bin_to_bcd(hour);   // 24-hour mode
    buf[4] = 1;                  // Day of week: dummy value (1..7)
    buf[5] = bin_to_bcd(day);
    buf[6] = bin_to_bcd(month);
    buf[7] = bin_to_bcd(year % 100);

    int ret = i2c_write_blocking(i2c_default, DS3231_ADDR, buf, sizeof(buf), false);
    return ret == (int)sizeof(buf);
}

static bool ds3231_read_datetime(int *year, int *month, int *day,
                                 int *hour, int *minute, int *second) {
    uint8_t reg = 0x00;
    uint8_t buf[7];

    if (i2c_write_blocking(i2c_default, DS3231_ADDR, &reg, 1, true) != 1) {
        return false;
    }
    if (i2c_read_blocking(i2c_default, DS3231_ADDR, buf, sizeof(buf), false) != (int)sizeof(buf)) {
        return false;
    }

    *second = bcd_to_bin(buf[0] & 0x7f);
    *minute = bcd_to_bin(buf[1] & 0x7f);
    *hour   = bcd_to_bin(buf[2] & 0x3f);   // 24-hour mode
    *day    = bcd_to_bin(buf[4] & 0x3f);
    *month  = bcd_to_bin(buf[5] & 0x1f);
    *year   = 2000 + bcd_to_bin(buf[6]);

    return true;
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    printf("DS3231 test start\r\n");

    // TODO: Re-enable stdio input once fgets() works reliably on UART.
#if 0
    do {
        printf("Input date/time as YYYY/M/D HH:MM\r\n");
        printf("Press Enter only to use default: 2026/3/22 00:00\r\n");
        printf("> ");

        int year, month, day, hour, minute, second;
        char line[64] = {'\0'};
        if (!fgets(line, sizeof(line), stdin)) break;

        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') break;
        int n = sscanf(line, "%d/%d/%d %d:%d", &year, &month, &day, &hour, &minute);
        if (n != 5) {
            printf("Invalid input\r\n");
            break;
        }
        if (!ds3231_set_datetime(year, month, day, hour, minute, second)) {
            printf("Failed to set DS3231 time\r\n");
            break;
        }
        printf("Time set to %04d/%d/%d %02d:%02d:%02d\r\n",
               year, month, day, hour, minute, second);
    } while (0);
#endif

    while (1) {
        int ry, rmon, rday, rh, rmin, rs;
        if (ds3231_read_datetime(&ry, &rmon, &rday, &rh, &rmin, &rs)) {
            printf("%04d/%02d/%02d %02d:%02d:%02d\r\n",
                   ry, rmon, rday, rh, rmin, rs);
        } else {
            printf("Failed to read DS3231 time\r\n");
        }
        sleep_ms(1000);
    }
}
