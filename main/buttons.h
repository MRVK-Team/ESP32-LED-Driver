/* The three operator buttons.

   Wiring: GPIO ---- button ---- GND. The internal pull-up is enabled, so a
   pin reads HIGH when idle and LOW while the button is held.

   Buttons produce events only. They never change the robot state locally -
   the main controller decides that, after its own safety checks.

   Debounced in software (BUTTON_DEBOUNCE_MS) and reported exactly once per
   physical press, on the press edge; holding a button emits nothing further.
*/
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BUTTON_BEER = 0,        /* button 1 -> EVENT,BEER_LOADED */
    BUTTON_MISSION,         /* button 2 -> EVENT,MISSION_START */
    BUTTON_AUX,             /* button 3 -> EVENT,AUX_BUTTON */
    BUTTON_COUNT,
} button_id_t;

/* Name of the UART event this button sends, e.g. "BEER_LOADED". */
const char *button_event_name(button_id_t id);

void button_manager_init(void);

/* Non-blocking. If a button has just been pressed, stores its id in *out and
   returns true. Call repeatedly until it returns false, so that two presses
   in the same loop iteration are both reported. */
bool button_manager_poll(button_id_t *out);

#ifdef __cplusplus
}
#endif
