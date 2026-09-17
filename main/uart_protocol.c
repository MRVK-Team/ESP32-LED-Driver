#include "uart_protocol.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "crc16.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "led_animator.h"
#include "robot_state.h"
#include "state_manager.h"

#if ROBOT_LINK_USE_USB
#include "driver/usb_serial_jtag.h"
#else
#include "driver/uart.h"
#endif

static const char *TAG = "link";

#define RX_RING_SIZE        512
#define MAX_ARGS            6

static char s_rx[ROBOT_UART_MAX_FRAME + 1];
static size_t s_rx_len;
static bool s_overflow;             /* dropping the rest of an oversized line */
static int64_t s_last_valid_us;

/* Framing of the frame being handled, so the reply can match it. */
static bool s_reply_framed;

/* --- transport ---------------------------------------------------------- */
/* Two ways to reach the controller, chosen by ROBOT_LINK_USE_USB. Both are
   non-blocking: a read takes whatever has already arrived and returns. */

#if ROBOT_LINK_USE_USB

/* The native USB port, driven directly rather than through the console.
   This works whatever CONFIG_ESP_CONSOLE_* is set to, and keeps the log
   output (console) and the protocol on separate streams. */
static void link_init(void)
{
    usb_serial_jtag_driver_config_t cfg = {
        .rx_buffer_size = 256,
        .tx_buffer_size = 256,
    };
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&cfg));

    ESP_LOGI(TAG, "protocol on the native USB port");
}

static int link_read(uint8_t *buf, size_t len)
{
    int n = usb_serial_jtag_read_bytes(buf, len, 0);
    return (n > 0) ? n : 0;
}

static void link_write(const char *data, size_t len)
{
    usb_serial_jtag_write_bytes(data, len, 0);
}

#else

static void link_init(void)
{
    const uart_config_t cfg = {
        .baud_rate = ROBOT_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(ROBOT_UART_PORT, RX_RING_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(ROBOT_UART_PORT, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(ROBOT_UART_PORT, ROBOT_UART_TX_PIN, ROBOT_UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "protocol on UART%d %d 8N1, tx=%d rx=%d",
             ROBOT_UART_PORT, ROBOT_UART_BAUD, ROBOT_UART_TX_PIN, ROBOT_UART_RX_PIN);
}

static int link_read(uint8_t *buf, size_t len)
{
    return uart_read_bytes(ROBOT_UART_PORT, buf, len, 0);
}

static void link_write(const char *data, size_t len)
{
    uart_write_bytes(ROBOT_UART_PORT, data, len);
}

#endif

/* --- sending ----------------------------------------------------------- */

static void send_raw(const char *payload, bool framed)
{
    char out[ROBOT_UART_MAX_FRAME + 16];
    int n;

    if (framed) {
        uint16_t crc = crc16_ccitt_false(payload, strlen(payload));
        n = snprintf(out, sizeof(out), "$%s*%04X\n", payload, crc);
    } else {
        n = snprintf(out, sizeof(out), "%s\n", payload);
    }

    if (n <= 0) {
        return;
    }
    if (n > (int)sizeof(out)) {
        n = (int)sizeof(out);       /* truncated, still a well formed line */
    }

    link_write(out, (size_t)n);
    ESP_LOGD(TAG, "tx: %s", payload);
}

void uart_protocol_send(const char *payload)
{
    if (payload == NULL) {
        return;
    }
    send_raw(payload, ROBOT_UART_TX_FRAMED != 0);
}

/* Reply to the frame currently being handled. */
static void reply(const char *payload)
{
    send_raw(payload, s_reply_framed);
}

static void reply_fmt(const char *fmt, const char *arg)
{
    char buf[ROBOT_UART_MAX_FRAME];
    snprintf(buf, sizeof(buf), fmt, arg);
    reply(buf);
}

static void nack(const char *reason)
{
    reply_fmt("NACK,%s", reason);
}

/* --- command handling --------------------------------------------------- */

/* Split payload in place on commas. Returns the number of fields. */
static int split_args(char *payload, char *argv[], int max)
{
    int argc = 0;
    char *p = payload;

    while (argc < max) {
        argv[argc++] = p;
        char *comma = strchr(p, ',');
        if (comma == NULL) {
            break;
        }
        *comma = '\0';
        p = comma + 1;
    }

    return argc;
}

static void handle_command(char *payload)
{
    char *argv[MAX_ARGS];
    int argc = split_args(payload, argv, MAX_ARGS);

    if (argc == 0 || argv[0][0] == '\0') {
        nack("BAD_FORMAT");
        return;
    }

    const char *cmd = argv[0];

    if (strcmp(cmd, "PING") == 0) {
        reply("PONG");
        return;
    }

    if (strcmp(cmd, "GET_STATE") == 0) {
        reply_fmt("STATE,%s", robot_state_name(state_manager_get()));
        return;
    }

    if (strcmp(cmd, "SET_STATE") == 0) {
        if (argc < 2) {
            nack("BAD_FORMAT");
            return;
        }
        robot_state_t state;
        if (!robot_state_parse(argv[1], &state)) {
            nack("BAD_STATE");
            return;
        }
        state_manager_set(state);
        reply("ACK,SET_STATE");
        return;
    }

    if (strcmp(cmd, "SET_BRIGHTNESS") == 0) {
        if (argc < 2) {
            nack("BAD_FORMAT");
            return;
        }
        char *end = NULL;
        long value = strtol(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || value < 0 || value > 255) {
            nack("BAD_ARGUMENT");
            return;
        }
        led_animator_set_brightness((uint8_t)value);    /* RAM only */
        reply("ACK,SET_BRIGHTNESS");
        return;
    }

    nack("UNKNOWN_COMMAND");
}

/* --- framing ------------------------------------------------------------ */

static bool parse_hex16(const char *s, uint16_t *out)
{
    uint16_t value = 0;

    for (int i = 0; i < 4; i++) {
        char c = s[i];
        uint16_t digit;

        if (c >= '0' && c <= '9') {
            digit = (uint16_t)(c - '0');
        } else if (c >= 'A' && c <= 'F') {
            digit = (uint16_t)(c - 'A' + 10);
        } else if (c >= 'a' && c <= 'f') {
            digit = (uint16_t)(c - 'a' + 10);
        } else {
            return false;
        }
        value = (uint16_t)((value << 4) | digit);
    }

    if (s[4] != '\0') {
        return false;       /* trailing rubbish after the CRC */
    }

    *out = value;
    return true;
}

/* One complete line, without the newline. */
static void handle_line(char *line)
{
    if (line[0] == '\0') {
        return;             /* blank line, ignore */
    }

    if (line[0] == '$') {
        char *star = strchr(line, '*');
        if (star == NULL) {
            s_reply_framed = true;
            nack("BAD_FORMAT");
            return;
        }

        *star = '\0';
        char *payload = line + 1;
        const char *crc_text = star + 1;

        uint16_t received;
        if (!parse_hex16(crc_text, &received)) {
            s_reply_framed = true;
            nack("BAD_FORMAT");
            return;
        }

        /* CRC covers the payload only: not '$', '*', the CRC or the newline. */
        if (received != crc16_ccitt_false(payload, strlen(payload))) {
            s_reply_framed = true;
            nack("BAD_CRC");
            return;         /* state stays exactly as it was */
        }

        s_reply_framed = true;
        s_last_valid_us = esp_timer_get_time();
        handle_command(payload);
        return;
    }

#if ROBOT_UART_REQUIRE_CRC
    /* Protocol v1: unframed input is not accepted. */
    s_reply_framed = false;
    nack("BAD_FORMAT");
#else
    /* Protocol v0: plain lines are fine, handy from a serial terminal. */
    s_reply_framed = false;
    s_last_valid_us = esp_timer_get_time();
    handle_command(line);
#endif
}

static void feed_byte(char c)
{
    /* Terminals disagree about Enter: LF, CR or CRLF. Accept any of them as
       end of line - the empty line CRLF leaves behind is ignored anyway. */
    if (c == '\n' || c == '\r') {
        if (s_overflow) {
            s_overflow = false;
            s_rx_len = 0;
            s_reply_framed = false;
            nack("FRAME_TOO_LONG");
            return;
        }
        s_rx[s_rx_len] = '\0';
        handle_line(s_rx);
        s_rx_len = 0;
        return;
    }

    if (s_overflow) {
        return;             /* swallow the rest of the oversized line */
    }

    if (s_rx_len >= ROBOT_UART_MAX_FRAME) {
        s_overflow = true;  /* reported once the line ends, then recovered */
        return;
    }

    s_rx[s_rx_len++] = c;
}

/* --- public ------------------------------------------------------------- */

void uart_protocol_init(void)
{
    s_rx_len = 0;
    s_overflow = false;
    s_reply_framed = (ROBOT_UART_TX_FRAMED != 0);
    s_last_valid_us = esp_timer_get_time();

    link_init();

    ESP_LOGI(TAG, "crc %s", ROBOT_UART_REQUIRE_CRC ? "required" : "optional");
}

void uart_protocol_update(void)
{
    uint8_t buf[64];

    /* Takes what has arrived and returns immediately. */
    int n = link_read(buf, sizeof(buf));
    for (int i = 0; i < n; i++) {
        feed_byte((char)buf[i]);
    }
}

uint32_t uart_protocol_since_last_valid_ms(void)
{
    return (uint32_t)((esp_timer_get_time() - s_last_valid_us) / 1000);
}
