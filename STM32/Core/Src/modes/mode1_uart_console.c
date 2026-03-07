/**
 * ============================================================
 * Mode 1: UART Command Console
 * ============================================================
 *
 * A simple command-line interface over UART. Type commands from
 * a PC terminal (through the Flipper's USB-UART Bridge) and the
 * STM32 parses and responds.
 *
 * Commands:
 *   help              - Show all available commands
 *   status            - Show uptime, LED states
 *   led <color> <op>  - Control LEDs (green/orange/red/blue, on/off/toggle)
 *   led all <op>      - Control all LEDs at once
 *   echo <text>       - Echo back the text you type
 *   reset             - Return to mode selection menu
 *
 * Learning goals:
 *   - String parsing in C (tokenization with strtok)
 *   - Command dispatch pattern (if/else chain on command names)
 *   - GPIO output control (HAL_GPIO_WritePin / TogglePin)
 *   - Interrupt-driven UART receive with line buffering
 */

#include "main.h"
#include <string.h>
#include <stdio.h>

/* ---- External references (defined in main.c) ---- */
extern UART_HandleTypeDef huart2;
extern void uart_print(const char *msg);
extern void uart_printf(const char *fmt, ...);
extern volatile uint8_t mode_reset_requested;

/* ---- Line buffer for receiving commands ---- */
#define CMD_BUF_SIZE 128
static char cmd_buf[CMD_BUF_SIZE];
static uint8_t cmd_pos = 0;

/* Single-byte receive buffer */
static uint8_t rx_byte;

/* LED state tracking */
static uint8_t led_state[4] = {0, 0, 0, 0};  /* green, orange, red, blue */

/* LED pin mapping: PD12=green, PD13=orange, PD14=red, PD15=blue */
static const uint16_t led_pins[4] = {
    GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15
};
static const char *led_names[4] = {"green", "orange", "red", "blue"};

/* ---- LED helpers ---- */
static int find_led(const char *name) {
    for (int i = 0; i < 4; i++) {
        if (strcmp(name, led_names[i]) == 0) return i;
    }
    return -1;
}

static void set_led(int idx, uint8_t state) {
    HAL_GPIO_WritePin(GPIOD, led_pins[idx], state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    led_state[idx] = state;
}

static void toggle_led(int idx) {
    HAL_GPIO_TogglePin(GPIOD, led_pins[idx]);
    led_state[idx] = !led_state[idx];
}

/* ---- Command handlers ---- */

static void cmd_help(void) {
    uart_print("\r\n--- UART Command Console ---\r\n");
    uart_print("  help              Show this help message\r\n");
    uart_print("  status            Show uptime and LED states\r\n");
    uart_print("  led <color> <op>  Control an LED\r\n");
    uart_print("      colors: green, orange, red, blue, all\r\n");
    uart_print("      ops:    on, off, toggle\r\n");
    uart_print("  echo <text>       Echo text back to terminal\r\n");
    uart_print("  reset             Return to mode selection\r\n");
}

static void cmd_status(void) {
    uint32_t tick = HAL_GetTick();
    uint32_t sec = tick / 1000;
    uint32_t min = sec / 60;
    sec %= 60;

    uart_printf("\r\nUptime: %lum %lus\r\n", min, sec);
    uart_print("LEDs: ");
    for (int i = 0; i < 4; i++) {
        uart_printf(" %s=%s", led_names[i], led_state[i] ? "ON" : "off");
    }
    uart_print("\r\n");
}

static void cmd_led(char *args) {
    /* args should be something like "green on" or "all toggle" */
    char *color = strtok(args, " ");
    char *op = strtok(NULL, " ");

    if (!color || !op) {
        uart_print("\r\nUsage: led <color> <on/off/toggle>\r\n");
        uart_print("Colors: green, orange, red, blue, all\r\n");
        return;
    }

    /* Determine which LED(s) to affect */
    int start = 0, end = 4;
    if (strcmp(color, "all") != 0) {
        int idx = find_led(color);
        if (idx < 0) {
            uart_printf("\r\nUnknown color: %s\r\n", color);
            return;
        }
        start = idx;
        end = idx + 1;
    }

    /* Apply operation */
    if (strcmp(op, "on") == 0) {
        for (int i = start; i < end; i++) set_led(i, 1);
        uart_print("\r\nOK\r\n");
    } else if (strcmp(op, "off") == 0) {
        for (int i = start; i < end; i++) set_led(i, 0);
        uart_print("\r\nOK\r\n");
    } else if (strcmp(op, "toggle") == 0) {
        for (int i = start; i < end; i++) toggle_led(i);
        uart_print("\r\nOK\r\n");
    } else {
        uart_printf("\r\nUnknown operation: %s (use on/off/toggle)\r\n", op);
    }
}

static void cmd_echo(const char *args) {
    if (args && strlen(args) > 0) {
        uart_printf("\r\n%s\r\n", args);
    } else {
        uart_print("\r\nUsage: echo <text>\r\n");
    }
}

/* ---- Process a complete command line ---- */
static void process_command(char *line) {
    /* Skip leading spaces */
    while (*line == ' ') line++;

    /* Empty line — just print a new prompt */
    if (strlen(line) == 0) return;

    /* Extract the command word */
    char *cmd = strtok(line, " ");
    char *args = strtok(NULL, "");  /* rest of line */

    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "status") == 0) {
        cmd_status();
    } else if (strcmp(cmd, "led") == 0) {
        cmd_led(args);
    } else if (strcmp(cmd, "echo") == 0) {
        cmd_echo(args);
    } else if (strcmp(cmd, "reset") == 0) {
        uart_print("\r\nReturning to mode selection...\r\n");
        mode_reset_requested = 1;
        return;
    } else {
        uart_printf("\r\nUnknown command: %s (type 'help' for commands)\r\n", cmd);
    }
}

/* ---- Main entry point for Mode 1 ---- */
void mode1_run(void) {
    /* Reset state */
    cmd_pos = 0;
    memset(led_state, 0, sizeof(led_state));

    /* Turn off all LEDs */
    HAL_GPIO_WritePin(GPIOD,
        GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
        GPIO_PIN_RESET);

    /* Print welcome */
    uart_print("\r\n");
    uart_print("========================================\r\n");
    uart_print("  Mode 1: UART Command Console\r\n");
    uart_print("========================================\r\n");
    uart_print("Type 'help' for available commands.\r\n");
    uart_print("\r\n> ");

    while (!mode_reset_requested) {
        /* Poll for a byte with short timeout */
        if (HAL_UART_Receive(&huart2, &rx_byte, 1, 10) == HAL_OK) {

            if (rx_byte == '\r' || rx_byte == '\n') {
                /* End of line — process the command */
                cmd_buf[cmd_pos] = '\0';
                process_command(cmd_buf);
                cmd_pos = 0;

                if (!mode_reset_requested) {
                    uart_print("> ");
                }
            } else if (rx_byte == 0x7F || rx_byte == '\b') {
                /* Backspace — erase last character */
                if (cmd_pos > 0) {
                    cmd_pos--;
                    uart_print("\b \b");
                }
            } else if (cmd_pos < CMD_BUF_SIZE - 1) {
                /* Normal character — add to buffer and echo */
                cmd_buf[cmd_pos++] = (char)rx_byte;
                HAL_UART_Transmit(&huart2, &rx_byte, 1, HAL_MAX_DELAY);
            }
        }
    }
}
