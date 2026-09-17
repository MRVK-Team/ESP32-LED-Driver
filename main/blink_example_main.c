/* Robot state controller for an ESP32-C3 mini.

   The ESP32 does not control robot motion. It holds the robot state, shows
   it on the 8x8 WS2812B matrix, and reports button presses to the main robot
   controller over UART. The controller is the authoritative source of state:
   it changes only through SET_STATE.

   Modules:
     matrix.c        pixels on the panel
     patterns.c      the 8x8 pictures
     led_animator.c  state -> animation
     buttons.c       three debounced buttons
     uart_protocol.c line protocol to the controller
     state_manager.c the current state
     config.h        pins and timings

   This code is in the Public Domain (or CC0 licensed, at your option.)
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "buttons.h"
#include "config.h"
#include "led_animator.h"
#include "matrix.h"
#include "state_manager.h"
#include "uart_protocol.h"

static const char *TAG = "main";

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void app_main(void)
{
    matrix_init();
    button_manager_init();
    uart_protocol_init();
    state_manager_init();
    led_animator_init();

    state_manager_set(ROBOT_STATE_LOAD_BEER);
    uart_protocol_send("BOOT,READY");
    ESP_LOGI(TAG, "ready");

    while (1) {
        uint32_t now = now_ms();

        /* Read whatever the controller has sent. Never blocks. */
        uart_protocol_update();

        /* Report every press. Buttons do not change the state themselves -
           the controller decides, and answers with SET_STATE. */
        button_id_t pressed;
        while (button_manager_poll(&pressed)) {
            char msg[48];
            snprintf(msg, sizeof(msg), "EVENT,%s", button_event_name(pressed));
            uart_protocol_send(msg);
        }

        /* Draw at most one frame, then straight back round the loop. */
        led_animator_set_state(state_manager_get());
        led_animator_update(now);

        vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_MS));
    }
}
