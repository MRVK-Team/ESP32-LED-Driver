#include "state_manager.h"

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "state";

static robot_state_t s_state = ROBOT_STATE_LOAD_BEER;
static int64_t s_entered_us;

static int64_t now_us(void)
{
    return esp_timer_get_time();
}

void state_manager_init(void)
{
    s_state = ROBOT_STATE_LOAD_BEER;
    s_entered_us = now_us();
    ESP_LOGI(TAG, "init, state %s", robot_state_name(s_state));
}

void state_manager_set(robot_state_t state)
{
    if (state >= ROBOT_STATE_COUNT) {
        return;
    }
    if (state == s_state) {
        /* Repeated SET_STATE for the current state must not restart the
           animation or otherwise disturb anything. */
        return;
    }

    ESP_LOGI(TAG, "%s -> %s", robot_state_name(s_state), robot_state_name(state));
    s_state = state;
    s_entered_us = now_us();
}

robot_state_t state_manager_get(void)
{
    return s_state;
}

uint32_t state_manager_time_in_state_ms(void)
{
    return (uint32_t)((now_us() - s_entered_us) / 1000);
}
