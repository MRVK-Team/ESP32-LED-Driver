#include "buttons.h"

#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "buttons";

typedef struct {
    int gpio;
    const char *event;
} button_cfg_t;

static const button_cfg_t s_cfg[BUTTON_COUNT] = {
    [BUTTON_BEER]    = { BTN_BEER_GPIO,    "BEER_LOADED" },
    [BUTTON_MISSION] = { BTN_MISSION_GPIO, "MISSION_START" },
    [BUTTON_AUX]     = { BTN_AUX_GPIO,     "AUX_BUTTON" },
};

typedef struct {
    int last_raw;           /* level seen on the previous poll */
    int stable;             /* debounced level; 1 = released, 0 = held */
    int64_t changed_us;     /* when last_raw last changed */
    bool press_pending;     /* debounced press not yet reported */
} button_state_t;

static button_state_t s_state[BUTTON_COUNT];

const char *button_event_name(button_id_t id)
{
    if (id >= BUTTON_COUNT) {
        return "UNKNOWN";
    }
    return s_cfg[id].event;
}

void button_manager_init(void)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        gpio_config_t cfg = {
            .pin_bit_mask = 1ULL << s_cfg[i].gpio,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&cfg));

        s_state[i].last_raw = gpio_get_level(s_cfg[i].gpio);
        s_state[i].stable = s_state[i].last_raw;
        s_state[i].changed_us = esp_timer_get_time();
        s_state[i].press_pending = false;

        ESP_LOGI(TAG, "GPIO %d -> EVENT,%s", s_cfg[i].gpio, s_cfg[i].event);
    }
}

/* Update the debounced level of every button, latching new presses. */
static void sample_all(int64_t now_us)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        int raw = gpio_get_level(s_cfg[i].gpio);

        if (raw != s_state[i].last_raw) {
            /* Still bouncing, restart the timer. */
            s_state[i].last_raw = raw;
            s_state[i].changed_us = now_us;
            continue;
        }

        if (raw == s_state[i].stable) {
            continue;
        }

        if (now_us - s_state[i].changed_us < (int64_t)BUTTON_DEBOUNCE_MS * 1000) {
            continue;   /* not held steady long enough yet */
        }

        s_state[i].stable = raw;

        /* Active low: one event on the press edge, nothing on release and
           nothing while it stays down. */
        if (raw == 0) {
            s_state[i].press_pending = true;
        }
    }
}

bool button_manager_poll(button_id_t *out)
{
    sample_all(esp_timer_get_time());

    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (s_state[i].press_pending) {
            s_state[i].press_pending = false;
            if (out != NULL) {
                *out = (button_id_t)i;
            }
            ESP_LOGI(TAG, "pressed: %s", s_cfg[i].event);
            return true;
        }
    }

    return false;
}
