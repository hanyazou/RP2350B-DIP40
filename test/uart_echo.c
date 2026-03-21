#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#define UART_ID        uart_default
#define BAUD_RATE      PICO_DEFAULT_UART_BAUD_RATE
#define UART_TX_PIN    PICO_DEFAULT_UART_TX_PIN
#define UART_RX_PIN    PICO_DEFAULT_UART_RX_PIN

int main(void) {
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

    // Give the UART a moment to settle
    sleep_ms(100);

    // Initial message
    uart_puts(UART_ID, "Hello\r\n");

    // Echo RX to TX
    while (true) {
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            uart_putc(UART_ID, c);
        }
    }
}
