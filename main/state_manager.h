/* The one place where the robot state is stored and changed.

   Every transition goes through state_manager_set(), so there is a single
   log line and a single hook for anything that must react to a change. */
#pragma once

#include <stdint.h>
#include "robot_state.h"

#ifdef __cplusplus
extern "C" {
#endif

void state_manager_init(void);

/* Set the current state. Setting the state it is already in is harmless and
   does not restart the animation. */
void state_manager_set(robot_state_t state);

robot_state_t state_manager_get(void);

/* Milliseconds since the current state was entered. */
uint32_t state_manager_time_in_state_ms(void);

#ifdef __cplusplus
}
#endif
