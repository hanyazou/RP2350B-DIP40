#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define USE_SOFT_SPI 0

#if !USE_SOFT_SPI
#include "hardware/spi.h"
#endif

#define PIN_MISO  40
#define PIN_CS    41
#define PIN_SCK   42
#define PIN_MOSI  43

#if !USE_SOFT_SPI
#define SD_SPI          spi1
#define SD_BAUD_SLOW    (400 * 1000)
#define SD_BAUD_FAST    (12 * 1000 * 1000)
#else
#define SD_BAUD_DELAY_US 5
#endif

static inline void cs_high(void) {
    gpio_put(PIN_CS, 1);
}

static inline void cs_low(void) {
    gpio_put(PIN_CS, 0);
}

#if USE_SOFT_SPI

static inline void sck_low(void) {
    gpio_put(PIN_SCK, 0);
}

static inline void sck_high(void) {
    gpio_put(PIN_SCK, 1);
}

static inline void mosi_write(int v) {
    gpio_put(PIN_MOSI, v ? 1 : 0);
}

static inline int miso_read(void) {
    return gpio_get(PIN_MISO);
}

/*
 * SPI mode 0:
 * - Clock idles low
 * - Data changes while clock is low
 * - Data is sampled on the rising edge
 */
static uint8_t spi_xfer_byte(uint8_t tx) {
    uint8_t rx = 0;

    for (int i = 0; i < 8; i++) {
        mosi_write((tx & 0x80u) != 0);
        sleep_us(SD_BAUD_DELAY_US);

        sck_high();
        sleep_us(SD_BAUD_DELAY_US);

        rx <<= 1;
        if (miso_read()) {
            rx |= 1;
        }

        sck_low();
        sleep_us(SD_BAUD_DELAY_US);

        tx <<= 1;
    }

    return rx;
}

static void sd_bus_init(void) {
    gpio_init(PIN_MOSI);
    gpio_set_dir(PIN_MOSI, GPIO_OUT);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);

    gpio_init(PIN_SCK);
    gpio_set_dir(PIN_SCK, GPIO_OUT);

    gpio_init(PIN_MISO);
    gpio_set_dir(PIN_MISO, GPIO_IN);
    gpio_disable_pulls(PIN_MISO);

    mosi_write(1);
    sck_low();
    cs_high();
}

static void sd_set_fast_clock(void) {
    /* No runtime clock change is needed for software SPI. */
}

#else

static uint8_t spi_xfer_byte(uint8_t v) {
    uint8_t rx = 0xff;
    spi_write_read_blocking(SD_SPI, &v, &rx, 1);
    return rx;
}

static void sd_bus_init(void) {
    spi_init(SD_SPI, SD_BAUD_SLOW);
    spi_set_format(SD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    cs_high();
}

static void sd_set_fast_clock(void) {
    spi_set_baudrate(SD_SPI, SD_BAUD_FAST);
}

#endif

static void sd_idle_clocks(void) {
    cs_high();
#if USE_SOFT_SPI
    mosi_write(1);
#endif
    for (int i = 0; i < 10; i++) {
        spi_xfer_byte(0xff);
    }
}

static uint8_t sd_wait_r1(void) {
    for (int i = 0; i < 16; i++) {
        uint8_t r = spi_xfer_byte(0xff);
        if ((r & 0x80u) == 0) {
            return r;
        }
    }
    return 0xff;
}

static uint8_t sd_wait_r1_debug(void) {
    for (int i = 0; i < 16; i++) {
        uint8_t r = spi_xfer_byte(0xff);
        printf("resp[%d] = 0x%02x\n", i, r);
        if ((r & 0x80u) == 0) {
            return r;
        }
    }
    return 0xff;
}

static uint8_t sd_cmd_r1(uint8_t cmd, uint32_t arg, uint8_t crc) {
    cs_high();
    spi_xfer_byte(0xff);
    cs_low();
    spi_xfer_byte(0xff);

    spi_xfer_byte(0x40 | cmd);
    spi_xfer_byte((uint8_t)(arg >> 24));
    spi_xfer_byte((uint8_t)(arg >> 16));
    spi_xfer_byte((uint8_t)(arg >> 8));
    spi_xfer_byte((uint8_t)(arg >> 0));
    spi_xfer_byte(crc);

#if 0
    return sd_wait_r1_debug();
#else
    return sd_wait_r1();
#endif
}

static uint8_t sd_cmd_r3r7(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t out[5]) {
    uint8_t r1 = sd_cmd_r1(cmd, arg, crc);
    out[0] = r1;
    for (int i = 1; i < 5; i++) {
        out[i] = spi_xfer_byte(0xff);
    }
    cs_high();
    spi_xfer_byte(0xff);
    return r1;
}

static uint8_t sd_cmd_acmd41(uint32_t arg) {
    uint8_t r1;

    r1 = sd_cmd_r1(55, 0, 0x01);
    cs_high();
    spi_xfer_byte(0xff);
    if (r1 > 1) {
        return r1;
    }

    r1 = sd_cmd_r1(41, arg, 0x01);
    cs_high();
    spi_xfer_byte(0xff);
    return r1;
}

static bool sd_read_block(uint32_t lba, uint8_t *buf512) {
    uint8_t r1 = sd_cmd_r1(17, lba, 0x01);
    if (r1 != 0x00) {
        cs_high();
        spi_xfer_byte(0xff);
        printf("CMD17 failed, R1=0x%02x\n", r1);
        return false;
    }

    int timeout = 300000;
    uint8_t token;
    do {
        token = spi_xfer_byte(0xff);
    } while (token == 0xff && --timeout > 0);

    if (token != 0xfe) {
        cs_high();
        spi_xfer_byte(0xff);
        printf("Read token timeout or bad token: 0x%02x\n", token);
        return false;
    }

    for (int i = 0; i < 512; i++) {
        buf512[i] = spi_xfer_byte(0xff);
    }

    spi_xfer_byte(0xff);
    spi_xfer_byte(0xff);

    cs_high();
    spi_xfer_byte(0xff);
    return true;
}

static void hexdump(const uint8_t *buf, size_t size) {
    for (int i = 0; i < size; i += 16) {
        printf("%04x :", i);
        for (int j = 0; j < 16; j++) {
            printf(" %02x", buf[i + j]);
        }
        printf("  ");
        for (int j = 0; j < 16; j++) {
            uint8_t c = buf[i + j];
            putchar((c >= 32 && c <= 126) ? c : '.');
        }
        printf("\n");
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

#if USE_SOFT_SPI
    printf("\nSD software SPI probe start\n");
#else
    printf("\nSD hardware SPI probe start\n");
#endif

    sd_bus_init();
    sleep_ms(10);
    sd_idle_clocks();

    uint8_t r1 = 0xff;
    bool ok = false;

    for (int i = 0; i < 20; i++) {
        r1 = sd_cmd_r1(0, 0, 0x95);
        cs_high();
        spi_xfer_byte(0xff);
        printf("CMD0 try %d: R1=0x%02x\n", i, r1);
        if (r1 == 0x01) {
            ok = true;
            break;
        }
        sleep_ms(20);
    }

    if (!ok) {
        printf("CMD0 failed\n");
        while (1) {
            sleep_ms(1000);
        }
    }

    printf("CMD0 ok, R1=0x%02x\n", r1);

    uint8_t r7[5];
    r1 = sd_cmd_r3r7(8, 0x000001aa, 0x87, r7);
    bool sdhc_like = false;

    if (r1 == 0x01) {
        printf("CMD8 ok, R7 = %02x %02x %02x %02x %02x\n",
               r7[0], r7[1], r7[2], r7[3], r7[4]);

        ok = false;
        for (int i = 0; i < 1000; i++) {
            r1 = sd_cmd_acmd41(0x40000000);
            if (r1 == 0x00) {
                ok = true;
                break;
            }
            sleep_ms(10);
        }
        if (!ok) {
            printf("ACMD41 failed for SDv2\n");
            while (1) {
                sleep_ms(1000);
            }
        }

        uint8_t r3[5];
        r1 = sd_cmd_r3r7(58, 0, 0x01, r3);
        if (r1 != 0x00) {
            printf("CMD58 failed\n");
            while (1) {
                sleep_ms(1000);
            }
        }

        uint32_t ocr = ((uint32_t)r3[1] << 24) |
                       ((uint32_t)r3[2] << 16) |
                       ((uint32_t)r3[3] << 8)  |
                       ((uint32_t)r3[4] << 0);

        printf("OCR = 0x%08lx\n", (unsigned long)ocr);
        sdhc_like = (ocr & 0x40000000u) != 0;
        printf("Card type: %s\n", sdhc_like ? "SDHC/SDXC" : "SDSC");
    } else if (r1 == 0x05) {
        printf("CMD8 illegal command; old SDSC/MMC is not handled by this test\n");
        while (1) {
            sleep_ms(1000);
        }
    } else {
        printf("CMD8 unexpected R1=0x%02x\n", r1);
        while (1) {
            sleep_ms(1000);
        }
    }

    sd_set_fast_clock();

    uint8_t sector[512];
    memset(sector, 0, sizeof(sector));

    if (sd_read_block(0, sector)) {
        printf("Sector 0 read OK\n");
        hexdump(sector, 512);

        if (sector[510] == 0x55 && sector[511] == 0xaa) {
            printf("MBR/boot sector signature 0x55AA found\n");
        } else {
            printf("No 0x55AA signature at sector0[510:511]\n");
        }
    } else {
        printf("Sector 0 read failed\n");
    }

    memset(sector, 0, sizeof(sector));
    if (sd_read_block(8192, sector)) {
        printf("Sector 8192 read OK\n");
        hexdump(sector, 512);
    } else {
        printf("Sector 8192 read failed\n");
    }

    while (1) {
        sleep_ms(1000);
    }
}
