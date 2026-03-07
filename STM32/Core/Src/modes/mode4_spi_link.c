/**
 * ============================================================
 * Mode 4: SPI Communication Link (STUB — Not Yet Implemented)
 * ============================================================
 *
 * Planned functionality:
 *   - SPI master on STM32, communicating with Flipper Zero
 *   - Request/response protocol over SPI
 *   - Speed comparison vs UART
 *   - Demonstrates clock-based synchronous communication
 *
 * Wiring (additional to UART):
 *   STM32               Flipper Zero
 *   PA5 (SPI1_SCK)  --> Pin 5 (B3 / SCK)
 *   PA6 (SPI1_MISO) <-- Pin 4 (A4 / MISO)
 *   PA7 (SPI1_MOSI) --> Pin 3 (A6 / MOSI)
 *   PB6 (CS)        --> Pin 6 (B2 / CS)
 *
 * Key concepts:
 *   - SPI is synchronous (has a clock line, unlike UART)
 *   - Master controls the clock — slave must respond in sync
 *   - Full duplex — data flows both directions simultaneously
 *   - Much faster than UART (MHz vs kbps)
 */

#include "main.h"

extern void uart_print(const char *msg);
extern volatile uint8_t mode_reset_requested;

void mode4_run(void) {
    uart_print("\r\n");
    uart_print("========================================\r\n");
    uart_print("  Mode 4: SPI Communication Link\r\n");
    uart_print("  ** NOT YET IMPLEMENTED **\r\n");
    uart_print("========================================\r\n");
    uart_print("This mode will be built in Phase 2.\r\n");
    uart_print("Returning to menu...\r\n");

    mode_reset_requested = 1;
}
