/**
 * ============================================================
 * Mode 2: GPIO Remote Control (STUB — Not Yet Implemented)
 * ============================================================
 *
 * Planned functionality:
 *   - Binary packet protocol (header, cmd, payload, checksum)
 *   - Individual GPIO pin control from Flipper/PC
 *   - STM32 reports pin states back
 *   - Bidirectional communication
 *
 * Wiring: Same 3-wire UART setup as Mode 1
 *
 * Protocol (planned):
 *   Request:  [0xAA] [CMD] [PIN] [VALUE] [CHECKSUM]
 *   Response: [0xBB] [CMD] [PIN] [VALUE] [CHECKSUM]
 *
 *   CMD bytes:
 *     0x01 = SET pin
 *     0x02 = CLEAR pin
 *     0x03 = TOGGLE pin
 *     0x04 = READ pin
 *     0x05 = READ ALL pins
 */

#include "main.h"

extern void uart_print(const char *msg);
extern volatile uint8_t mode_reset_requested;

void mode2_run(void) {
    uart_print("\r\n");
    uart_print("========================================\r\n");
    uart_print("  Mode 2: GPIO Remote Control\r\n");
    uart_print("  ** NOT YET IMPLEMENTED **\r\n");
    uart_print("========================================\r\n");
    uart_print("This mode will be built in the next phase.\r\n");
    uart_print("Returning to menu...\r\n");

    mode_reset_requested = 1;
}
