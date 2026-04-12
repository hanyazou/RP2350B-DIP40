#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#define LED_DELAY_MS 500
const int led_pin = 24;

#define UART_ID        uart_default
#define BAUD_RATE      PICO_DEFAULT_UART_BAUD_RATE
#define UART_TX_PIN    PICO_DEFAULT_UART_TX_PIN
#define UART_RX_PIN    PICO_DEFAULT_UART_RX_PIN

struct pin_table_s {
    int pin;
    char *desc;
} pin_table[] = {
    {39, "DIP 1: RE3"},
    { 8, "DIP 2: RA0"},
    { 9, "DIP 3: RA1"},
    {10, "DIP 4: RA2"},
    {11, "DIP 5: RA3"},
    {12, "DIP 6: RA4"},
    {13, "DIP 7: RA5"},
    {36, "DIP 8: RE0"},
    {37, "DIP 9: RE1"},
    {38, "DIP10: RE2"},
    {35, "DIP11: VDD(5V)"},
    {-1, "DIP12: VSS(GND)"},
    {15, "DIP13: RA7"},
    {14, "DIP14: RA6"},
    {16, "DIP15: RC0"},
    {17, "DIP16: RC1"},
    {18, "DIP17: RC2"},
    {19, "DIP18: RC3"},
    {24, "DIP19: RD0"},
    {25, "DIP20: RD1"},
    {26, "DIP21: RD2"},
    {27, "DIP22: RD3"},
    {20, "DIP23: RC4"},
    {21, "DIP24: RC5"},
    {22, "DIP25: RC6"},
    {23, "DIP26: RC7"},
    {28, "DIP27: RD4"},
    {29, "DIP28: RD5"},
    {30, "DIP29: RD6"},
    {31, "DIP30: RD7"},
    {-1, "DIP31: VSS(GND)"},
    {35, "DIP32: VDD(5V)"},
    { 0, "DIP33: RB0"},
    { 1, "DIP34: RB1"},
    { 2, "DIP35: RB2"},
    { 3, "DIP36: RB3"},
    { 4, "DIP37: RB4"},
    { 5, "DIP38: RB5"},
    { 6, "DIP39: RB6"},
    { 7, "DIP40: RB7"},
};
const int num_pins = sizeof(pin_table)/sizeof(*pin_table);

int main(void) {
    timer_hw->dbgpause = 0;   // Do not pause the RP2040 timer while halted in the debugger

    stdio_init_all();

    // UART init
    uart_init(UART_ID, BAUD_RATE);

    // Set GPIO function to UART
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // 115200 8N1
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // No hardware flow control
    uart_set_hw_flow(UART_ID, false, false);

    // Enable FIFO
    uart_set_fifo_enabled(UART_ID, true);

    // Give the UART and the USB a moment to settle
    sleep_ms(2000);

    // Initial message
    printf("Flip on / off GPIO\r\n");

    bool on = true;  // Turn the LED on or off
    int flip_pin = 25;
    int pos = 0;
    int last_pos = -1;

    // Initialize the GPIO for the LED
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    while (true) {
        if (last_pos != pos) {
            printf("%s\r\n", pin_table[pos].desc);
            flip_pin = pin_table[pos].pin;
            gpio_init(flip_pin);
            gpio_set_dir(flip_pin, GPIO_OUT);
            last_pos = pos;
        }

        gpio_put(led_pin, on);
        gpio_put(flip_pin, on);
        on = !on;

        const int divider = 25;
        for (int i = 0; i < divider; i++) {
            int delta = 0;
            if (uart_is_readable(UART_ID)) {
                char c = uart_getc(UART_ID);
                if (c == '\n' || c == '\r') {
                    delta = 1;
                } else if (c == 0x1b) {
                    c = uart_getc(UART_ID);
                    if (c == '[') {
                        c = uart_getc(UART_ID);
                        if (c == 'A') {
                            delta = -1;
                        } else
                        if (c == 'B') {
                            delta = 1;
                        }
                    }
                } else if (c == 0xff) {
                    /* ignore */
                } else {
                    printf("unknown input %02x\r\n", c);
                }
            }
            if (delta != 0) {
                pos += delta;
                if (pos < 0) pos += num_pins;
                if (num_pins <= pos) pos -= num_pins;
                break;
            }
            sleep_ms(LED_DELAY_MS / divider);
        }
    }
}
