/**
 * ============================================================
 * Mode 2: GPIO Remote Control
 * ============================================================
 *
 * Control STM32 GPIO pins remotely via UART. Supports both
 * human-readable text commands (for PuTTY testing) and a binary
 * packet protocol (for programmatic control / Flipper app).
 *
 * The mode auto-detects which protocol is being used:
 *   - Bytes starting with 0xAA are treated as binary packets
 *   - All other input is treated as text commands
 *
 * Wiring: Same 3-wire UART setup as Mode 1
 *
 * TEXT COMMANDS:
 *   help                 - Show available commands
 *   set <pin> <on|off>   - Set pin high or low
 *   toggle <pin>         - Toggle pin state
 *   read <pin>           - Read pin state
 *   read all             - Read all pin states
 *   status               - Show all pin states (same as read all)
 *   reset                - Return to boot menu
 *
 *   Pin names: green, orange, red, blue, button, all
 *
 * BINARY PROTOCOL:
 *   Request:  [0xAA] [CMD] [PIN] [VALUE] [CHECKSUM]
 *   Response: [0xBB] [STATUS] [PIN] [VALUE] [CHECKSUM]
 *
 *   CMD:  0x01=SET, 0x02=CLEAR, 0x03=TOGGLE, 0x04=READ, 0x05=READ_ALL
 *   PIN:  0x00=green, 0x01=orange, 0x02=red, 0x03=blue, 0x04=button
 *   STATUS: 0x00=OK, 0x01=ERROR
 *   CHECKSUM: XOR of all preceding bytes
 */

#include "main.h"
#include <string.h>
#include <stdio.h>

/* Externs from main.c */
extern UART_HandleTypeDef huart2;
extern void uart_print(const char *msg);
extern void uart_printf(const char *fmt, ...);
extern volatile uint8_t mode_reset_requested;

/* ---- Pin mapping ---- */

/* Output pins (LEDs on port D) */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    const char   *name;
    uint8_t       is_output;  /* 1 = output, 0 = input */
} PinDef;

#define NUM_PINS 5
#define PIN_ALL  0xFF

static const PinDef pins[NUM_PINS] = {
    { GPIOD, GPIO_PIN_12, "green",  1 },  /* Pin 0 */
    { GPIOD, GPIO_PIN_13, "orange", 1 },  /* Pin 1 */
    { GPIOD, GPIO_PIN_14, "red",    1 },  /* Pin 2 */
    { GPIOD, GPIO_PIN_15, "blue",   1 },  /* Pin 3 */
    { GPIOA, GPIO_PIN_0,  "button", 0 },  /* Pin 4 — USER button, read-only */
};

/* ---- Binary protocol constants ---- */

#define PKT_HEADER_REQ  0xAA
#define PKT_HEADER_RESP 0xBB
#define PKT_SIZE        5

#define CMD_SET      0x01
#define CMD_CLEAR    0x02
#define CMD_TOGGLE   0x03
#define CMD_READ     0x04
#define CMD_READ_ALL 0x05

#define STATUS_OK    0x00
#define STATUS_ERROR 0x01

/* ---- Helper: compute XOR checksum ---- */
static uint8_t xor_checksum(const uint8_t *data, uint8_t len) {
    uint8_t cs = 0;
    for (uint8_t i = 0; i < len; i++) {
        cs ^= data[i];
    }
    return cs;
}

/* ---- Helper: send binary response ---- */
static void send_response(uint8_t status, uint8_t pin, uint8_t value) {
    uint8_t resp[PKT_SIZE];
    resp[0] = PKT_HEADER_RESP;
    resp[1] = status;
    resp[2] = pin;
    resp[3] = value;
    resp[4] = xor_checksum(resp, 4);
    HAL_UART_Transmit(&huart2, resp, PKT_SIZE, HAL_MAX_DELAY);
}

/* ---- Helper: read a pin's current state ---- */
static uint8_t read_pin(uint8_t pin_id) {
    if (pin_id >= NUM_PINS) return 0;
    return HAL_GPIO_ReadPin(pins[pin_id].port, pins[pin_id].pin) == GPIO_PIN_SET ? 1 : 0;
}

/* ---- Process a binary packet (4 bytes after header already received) ---- */
static void process_binary_packet(const uint8_t *pkt) {
    /* pkt[0]=header(0xAA), pkt[1]=cmd, pkt[2]=pin, pkt[3]=value, pkt[4]=checksum */
    uint8_t expected_cs = xor_checksum(pkt, 4);
    if (pkt[4] != expected_cs) {
        send_response(STATUS_ERROR, 0, 0);
        return;
    }

    uint8_t cmd = pkt[1];
    uint8_t pin_id = pkt[2];
    uint8_t value = pkt[3];

    /* Handle READ_ALL specially */
    if (cmd == CMD_READ_ALL) {
        /* Pack all 5 pin states into the value byte as bits 0-4 */
        uint8_t all_states = 0;
        for (int i = 0; i < NUM_PINS; i++) {
            if (read_pin(i)) {
                all_states |= (1 << i);
            }
        }
        send_response(STATUS_OK, PIN_ALL, all_states);
        return;
    }

    /* Validate pin ID */
    if (pin_id >= NUM_PINS) {
        send_response(STATUS_ERROR, pin_id, 0);
        return;
    }

    switch (cmd) {
        case CMD_SET:
            if (!pins[pin_id].is_output) {
                send_response(STATUS_ERROR, pin_id, 0);
                return;
            }
            HAL_GPIO_WritePin(pins[pin_id].port, pins[pin_id].pin, GPIO_PIN_SET);
            send_response(STATUS_OK, pin_id, 1);
            break;

        case CMD_CLEAR:
            if (!pins[pin_id].is_output) {
                send_response(STATUS_ERROR, pin_id, 0);
                return;
            }
            HAL_GPIO_WritePin(pins[pin_id].port, pins[pin_id].pin, GPIO_PIN_RESET);
            send_response(STATUS_OK, pin_id, 0);
            break;

        case CMD_TOGGLE:
            if (!pins[pin_id].is_output) {
                send_response(STATUS_ERROR, pin_id, 0);
                return;
            }
            HAL_GPIO_TogglePin(pins[pin_id].port, pins[pin_id].pin);
            send_response(STATUS_OK, pin_id, read_pin(pin_id));
            break;

        case CMD_READ:
            send_response(STATUS_OK, pin_id, read_pin(pin_id));
            break;

        default:
            send_response(STATUS_ERROR, pin_id, 0);
            break;
    }
}

/* ---- Resolve pin name to pin ID ---- */
static int resolve_pin(const char *name) {
    for (int i = 0; i < NUM_PINS; i++) {
        if (strcmp(name, pins[i].name) == 0) return i;
    }
    if (strcmp(name, "all") == 0) return PIN_ALL;
    return -1;
}

/* ---- Process a text command ---- */
static void process_text_command(char *cmd) {
    /* Trim leading spaces */
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return;

    /* Split into tokens (up to 3) */
    char *argv[3] = {0};
    int argc = 0;
    char *p = cmd;
    while (*p && argc < 3) {
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) { *p = '\0'; p++; }
        while (*p == ' ') p++;
    }

    /* ---- HELP ---- */
    if (strcmp(argv[0], "help") == 0) {
        uart_print("\r\n--- GPIO Remote Control Commands ---\r\n");
        uart_print("  set <pin> <on|off>  - Set pin high/low\r\n");
        uart_print("  toggle <pin>        - Toggle pin\r\n");
        uart_print("  read <pin>          - Read pin state\r\n");
        uart_print("  read all            - Read all pin states\r\n");
        uart_print("  status              - Same as 'read all'\r\n");
        uart_print("  reset               - Return to menu\r\n");
        uart_print("  Pins: green, orange, red, blue, button, all\r\n");
        uart_print("  (button is read-only)\r\n");
        return;
    }

    /* ---- RESET ---- */
    if (strcmp(argv[0], "reset") == 0) {
        mode_reset_requested = 1;
        return;
    }

    /* ---- STATUS / READ ALL ---- */
    if (strcmp(argv[0], "status") == 0 ||
        (strcmp(argv[0], "read") == 0 && argc >= 2 && strcmp(argv[1], "all") == 0)) {
        uart_print("\r\n  Pin States:\r\n");
        for (int i = 0; i < NUM_PINS; i++) {
            uart_printf("    %-8s : %s\r\n", pins[i].name,
                        read_pin(i) ? "HIGH (1)" : "LOW  (0)");
        }
        return;
    }

    /* ---- SET <pin> <on|off> ---- */
    if (strcmp(argv[0], "set") == 0) {
        if (argc < 3) {
            uart_print("  Usage: set <pin> <on|off>\r\n");
            return;
        }
        int pin_id = resolve_pin(argv[1]);
        if (pin_id < 0) {
            uart_printf("  Unknown pin: %s\r\n", argv[1]);
            return;
        }

        uint8_t val;
        if (strcmp(argv[2], "on") == 0 || strcmp(argv[2], "1") == 0) {
            val = 1;
        } else if (strcmp(argv[2], "off") == 0 || strcmp(argv[2], "0") == 0) {
            val = 0;
        } else {
            uart_print("  Use 'on' or 'off'\r\n");
            return;
        }

        if (pin_id == PIN_ALL) {
            /* Set all LEDs */
            for (int i = 0; i < NUM_PINS; i++) {
                if (pins[i].is_output) {
                    HAL_GPIO_WritePin(pins[i].port, pins[i].pin,
                                      val ? GPIO_PIN_SET : GPIO_PIN_RESET);
                }
            }
            uart_printf("  All LEDs: %s\r\n", val ? "ON" : "OFF");
        } else {
            if (!pins[pin_id].is_output) {
                uart_printf("  %s is read-only\r\n", pins[pin_id].name);
                return;
            }
            HAL_GPIO_WritePin(pins[pin_id].port, pins[pin_id].pin,
                              val ? GPIO_PIN_SET : GPIO_PIN_RESET);
            uart_printf("  %s: %s\r\n", pins[pin_id].name, val ? "ON" : "OFF");
        }
        return;
    }

    /* ---- TOGGLE <pin> ---- */
    if (strcmp(argv[0], "toggle") == 0) {
        if (argc < 2) {
            uart_print("  Usage: toggle <pin>\r\n");
            return;
        }
        int pin_id = resolve_pin(argv[1]);
        if (pin_id < 0) {
            uart_printf("  Unknown pin: %s\r\n", argv[1]);
            return;
        }
        if (pin_id == PIN_ALL) {
            for (int i = 0; i < NUM_PINS; i++) {
                if (pins[i].is_output) {
                    HAL_GPIO_TogglePin(pins[i].port, pins[i].pin);
                }
            }
            uart_print("  All LEDs toggled\r\n");
        } else {
            if (!pins[pin_id].is_output) {
                uart_printf("  %s is read-only\r\n", pins[pin_id].name);
                return;
            }
            HAL_GPIO_TogglePin(pins[pin_id].port, pins[pin_id].pin);
            uart_printf("  %s: %s\r\n", pins[pin_id].name,
                        read_pin(pin_id) ? "ON" : "OFF");
        }
        return;
    }

    /* ---- READ <pin> ---- */
    if (strcmp(argv[0], "read") == 0) {
        if (argc < 2) {
            uart_print("  Usage: read <pin>  (or 'read all')\r\n");
            return;
        }
        int pin_id = resolve_pin(argv[1]);
        if (pin_id < 0) {
            uart_printf("  Unknown pin: %s\r\n", argv[1]);
            return;
        }
        if (pin_id == PIN_ALL) {
            /* Already handled above, but just in case */
            uart_print("\r\n  Pin States:\r\n");
            for (int i = 0; i < NUM_PINS; i++) {
                uart_printf("    %-8s : %s\r\n", pins[i].name,
                            read_pin(i) ? "HIGH (1)" : "LOW  (0)");
            }
        } else {
            uart_printf("  %s: %s\r\n", pins[pin_id].name,
                        read_pin(pin_id) ? "HIGH (1)" : "LOW  (0)");
        }
        return;
    }

    uart_printf("  Unknown command: %s (type 'help')\r\n", argv[0]);
}

/* ---- Main mode loop ---- */

#define CMD_BUF_SIZE 64

void mode2_run(void) {
    char cmd_buf[CMD_BUF_SIZE];
    uint8_t cmd_pos = 0;
    uint8_t rx_byte;
    uint8_t bin_buf[PKT_SIZE];
    uint8_t bin_pos = 0;
    uint8_t in_binary = 0;  /* 1 = currently receiving a binary packet */

    uart_print("\r\n");
    uart_print("========================================\r\n");
    uart_print("  Mode 2: GPIO Remote Control\r\n");
    uart_print("========================================\r\n");
    uart_print("Type 'help' for commands.\r\n");
    uart_print("Also accepts binary packets (0xAA header).\r\n\r\n");
    uart_print("> ");

    while (!mode_reset_requested) {
        if (HAL_UART_Receive(&huart2, &rx_byte, 1, 10) != HAL_OK) {
            continue;
        }

        /* ---- Binary packet detection ---- */
        if (rx_byte == PKT_HEADER_REQ && !in_binary && cmd_pos == 0) {
            /* Start of binary packet */
            in_binary = 1;
            bin_buf[0] = rx_byte;
            bin_pos = 1;
            continue;
        }

        if (in_binary) {
            bin_buf[bin_pos++] = rx_byte;
            if (bin_pos >= PKT_SIZE) {
                process_binary_packet(bin_buf);
                in_binary = 0;
                bin_pos = 0;
            }
            continue;
        }

        /* ---- Text command handling ---- */
        if (rx_byte == '\r' || rx_byte == '\n') {
            uart_print("\r\n");
            cmd_buf[cmd_pos] = '\0';
            if (cmd_pos > 0) {
                process_text_command(cmd_buf);
            }
            cmd_pos = 0;
            if (!mode_reset_requested) {
                uart_print("> ");
            }
        } else if (rx_byte == 0x7F || rx_byte == '\b') {
            /* Backspace */
            if (cmd_pos > 0) {
                cmd_pos--;
                uart_print("\b \b");
            }
        } else if (cmd_pos < CMD_BUF_SIZE - 1) {
            cmd_buf[cmd_pos++] = (char)rx_byte;
            /* Echo */
            HAL_UART_Transmit(&huart2, &rx_byte, 1, HAL_MAX_DELAY);
        }
    }
}
