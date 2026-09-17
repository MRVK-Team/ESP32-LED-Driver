#include "matrix.h"

#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "led_strip.h"

static const char *TAG = "matrix";

#if MATRIX_MIRROR
#define MATRIX_OUTPUTS  2
#else
#define MATRIX_OUTPUTS  1
#endif

/* One handle per data pin. Every write goes to all of them, so the panels
   always show the same frame. */
static led_strip_handle_t s_strip[MATRIX_OUTPUTS];
static const int s_gpio[MATRIX_OUTPUTS] = {
    MATRIX_GPIO,
#if MATRIX_MIRROR
    MATRIX_GPIO_MIRROR,
#endif
};

static rgb_t s_buf[MATRIX_HEIGHT][MATRIX_WIDTH];
static uint8_t s_brightness = 64;

static void strip_set(uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
    for (int s = 0; s < MATRIX_OUTPUTS; s++) {
        if (s_strip[s] != NULL) {
            ESP_ERROR_CHECK(led_strip_set_pixel(s_strip[s], index, r, g, b));
        }
    }
}

static void strip_refresh(void)
{
    for (int s = 0; s < MATRIX_OUTPUTS; s++) {
        if (s_strip[s] != NULL) {
            ESP_ERROR_CHECK(led_strip_refresh(s_strip[s]));
        }
    }
}

static void strip_clear(void)
{
    for (int s = 0; s < MATRIX_OUTPUTS; s++) {
        if (s_strip[s] != NULL) {
            ESP_ERROR_CHECK(led_strip_clear(s_strip[s]));
        }
    }
}

/* Map (x, y), y = 0 at the top, to the index in the LED chain.
   See the MATRIX_* wiring macros in matrix.h. */
static uint32_t xy_to_index(uint32_t x, uint32_t y)
{
#if MATRIX_FLIP_X
    x = MATRIX_WIDTH - 1 - x;
#endif
#if MATRIX_FLIP_Y
    y = MATRIX_HEIGHT - 1 - y;
#endif
#if MATRIX_TRANSPOSE
    uint32_t t = x;
    x = y;
    y = t;
#endif
#if MATRIX_SERPENTINE
    /* Reverse the odd rows, or the even ones if the chain starts by running
       right->left. */
    if (((y & 1) ^ (MATRIX_FIRST_ROW_REVERSED & 1)) != 0) {
        x = MATRIX_WIDTH - 1 - x;
    }
#endif
    return y * MATRIX_WIDTH + x;
}

void matrix_init(void)
{
    for (int s = 0; s < MATRIX_OUTPUTS; s++) {
        ESP_LOGI(TAG, "%dx%d WS2812B matrix on GPIO %d", MATRIX_WIDTH, MATRIX_HEIGHT, s_gpio[s]);

        led_strip_config_t strip_config = {
            .strip_gpio_num = s_gpio[s],
            .max_leds = MATRIX_LEDS,
            .led_model = LED_MODEL_WS2812,
            .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
            .flags.invert_out = false,
        };
        led_strip_rmt_config_t rmt_config = {
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .resolution_hz = 10 * 1000 * 1000, // 10MHz
            /* 48 symbols = exactly one RMT memory block on the ESP32-C3.
               Asking for 64 rounds up to two blocks, and then the second
               output has none left to allocate. */
            .mem_block_symbols = 48,
            .flags.with_dma = false,
        };
        esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip[s]);
        if (err != ESP_OK) {
            /* Do not panic: a failed mirror output must not take down the
               first panel. */
            ESP_LOGE(TAG, "GPIO %d failed: %s", s_gpio[s], esp_err_to_name(err));
            s_strip[s] = NULL;
        }
    }

    matrix_clear();
    matrix_show();
}

void matrix_set_brightness(uint8_t brightness)
{
    s_brightness = brightness;
}

uint8_t matrix_get_brightness(void)
{
    return s_brightness;
}

void matrix_set_xy(uint32_t x, uint32_t y, rgb_t color)
{
    if (x >= MATRIX_WIDTH || y >= MATRIX_HEIGHT) {
        return;
    }
    s_buf[y][x] = color;
}

void matrix_fill(rgb_t color)
{
    for (uint32_t y = 0; y < MATRIX_HEIGHT; y++) {
        for (uint32_t x = 0; x < MATRIX_WIDTH; x++) {
            s_buf[y][x] = color;
        }
    }
}

void matrix_clear(void)
{
    rgb_t off = { 0, 0, 0 };
    matrix_fill(off);
}

void matrix_show(void)
{
    for (uint32_t y = 0; y < MATRIX_HEIGHT; y++) {
        for (uint32_t x = 0; x < MATRIX_WIDTH; x++) {
            rgb_t c = s_buf[y][x];
            /* Scale by the global brightness; +1 keeps 255 mapping to full. */
            uint8_t r = (uint16_t)c.r * (s_brightness + 1) >> 8;
            uint8_t g = (uint16_t)c.g * (s_brightness + 1) >> 8;
            uint8_t b = (uint16_t)c.b * (s_brightness + 1) >> 8;
            strip_set(xy_to_index(x, y), r, g, b);
        }
    }
    strip_refresh();
}

void matrix_blank(void)
{
    matrix_clear();
    matrix_show();
}

void matrix_set_index(uint32_t i, rgb_t c)
{
    if (i >= MATRIX_LEDS) {
        return;
    }
    uint8_t r = (uint16_t)c.r * (s_brightness + 1) >> 8;
    uint8_t g = (uint16_t)c.g * (s_brightness + 1) >> 8;
    uint8_t b = (uint16_t)c.b * (s_brightness + 1) >> 8;
    strip_set(i, r, g, b);
}

void matrix_refresh_raw(void)
{
    strip_refresh();
}

void matrix_walk_chain(uint32_t step_ms)
{
    const rgb_t white = { 255, 255, 255 };

    for (uint32_t i = 0; i < MATRIX_LEDS; i++) {
        strip_clear();
        matrix_set_index(i, white);
        matrix_refresh_raw();
        ESP_LOGI(TAG, "chain index %u", (unsigned)i);
        vTaskDelay(pdMS_TO_TICKS(step_ms));
    }
    strip_clear();
    matrix_refresh_raw();
}

void matrix_test_chain(void)
{
    const rgb_t red   = { 255,   0,   0 };
    const rgb_t green = {   0, 255,   0 };
    const rgb_t blue  = {   0,   0, 255 };

    strip_clear();
    for (uint32_t i = 0; i < 8; i++) {
        matrix_set_index(i, red);
    }
    for (uint32_t i = 8; i < 16; i++) {
        matrix_set_index(i, green);
    }
    for (uint32_t i = 56; i < MATRIX_LEDS; i++) {
        matrix_set_index(i, blue);
    }
    matrix_refresh_raw();
}

void matrix_test_corners(void)
{
    const rgb_t red   = { 255,   0,   0 };
    const rgb_t green = {   0, 255,   0 };
    const rgb_t blue  = {   0,   0, 255 };
    const rgb_t white = { 255, 255, 255 };

    matrix_clear();
    matrix_set_xy(0, 0, red);                                   /* top-left */
    matrix_set_xy(MATRIX_WIDTH - 1, 0, green);                  /* top-right */
    matrix_set_xy(0, MATRIX_HEIGHT - 1, blue);                  /* bottom-left */

    /* Mark the first LED of the chain, unless it is already one of the three
       corners above. */
    for (uint32_t y = 0; y < MATRIX_HEIGHT; y++) {
        for (uint32_t x = 0; x < MATRIX_WIDTH; x++) {
            if (xy_to_index(x, y) == 0) {
                if (s_buf[y][x].r == 0 && s_buf[y][x].g == 0 && s_buf[y][x].b == 0) {
                    matrix_set_xy(x, y, white);
                }
                ESP_LOGI(TAG, "chain index 0 is at x=%u y=%u", (unsigned)x, (unsigned)y);
            }
        }
    }

    matrix_show();
}
