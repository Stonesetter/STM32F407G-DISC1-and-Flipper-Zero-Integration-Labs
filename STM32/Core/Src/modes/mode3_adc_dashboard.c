/**
 * ============================================================
 * Mode 3: ADC Sensor Dashboard (STUB — Not Yet Implemented)
 * ============================================================
 *
 * Planned functionality:
 *   - Read onboard temperature sensor via ADC
 *   - Read external analog input (potentiometer on PA1)
 *   - Periodic sampling with configurable rate
 *   - Stream formatted readings over UART
 *   - Commands: start, stop, rate <ms>, single
 *
 * Wiring:
 *   - Same 3-wire UART as Mode 1
 *   - (Optional) Potentiometer: wiper to PA1, ends to 3V3 and GND
 *
 * Key peripherals:
 *   - ADC1 Channel 1 (PA1) for external analog
 *   - ADC1 Internal Temperature Sensor
 *   - TIM2 or SysTick for periodic sampling
 */

#include "main.h"

extern void uart_print(const char *msg);
extern volatile uint8_t mode_reset_requested;

void mode3_run(void) {
    uart_print("\r\n");
    uart_print("========================================\r\n");
    uart_print("  Mode 3: ADC Sensor Dashboard\r\n");
    uart_print("  ** NOT YET IMPLEMENTED **\r\n");
    uart_print("========================================\r\n");
    uart_print("This mode will be built in Phase 2.\r\n");
    uart_print("Returning to menu...\r\n");

    mode_reset_requested = 1;
}
