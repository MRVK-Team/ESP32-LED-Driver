/* Turns the current robot state into pixels.

   Completely non-blocking: led_animator_update() draws at most one frame and
   returns immediately, so UART reception and button handling keep running.
   It never decides anything about the state - it only visualises it. */
#pragma once

#include <stdint.h>
#include "robot_state.h"

#ifdef __cplusplus
extern "C" {
#endif

void led_animator_init(void);

/* Select what to show. Passing the state it is already showing does nothing,
   so it is safe to call every loop iteration. */
void led_animator_set_state(robot_state_t state);

/* Advance the animation. now_ms is a free-running millisecond counter. */
void led_animator_update(uint32_t now_ms);

void led_animator_set_brightness(uint8_t brightness);
uint8_t led_animator_get_brightness(void);

#ifdef __cplusplus
}
#endif
