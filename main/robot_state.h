/* The robot states this controller can display.

   The main robot controller is the authoritative source of the state; it is
   changed over UART with SET_STATE. Buttons only report events. */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ROBOT_STATE_LOAD_BEER = 0,
    ROBOT_STATE_AUTONOMOUS_DRIVING,
    ROBOT_STATE_FAULT,
    ROBOT_STATE_MISSION_COMPLETE,
    ROBOT_STATE_SCAN_QR,
    /* Added for the delivery mission. Appended, so the numbering of the
       five states above never moves. */
    ROBOT_STATE_UNLOAD_BEER,
    ROBOT_STATE_QR_CONFIRM,
    ROBOT_STATE_ACCEPTED,
    ROBOT_STATE_REJECTED,
    ROBOT_STATE_RETURNING,
    ROBOT_STATE_WAITING,
    ROBOT_STATE_NO_SIGNAL,
    ROBOT_STATE_COUNT,
} robot_state_t;

/* Wire name of a state, e.g. "LOAD_BEER". Never NULL. */
const char *robot_state_name(robot_state_t state);

/* Parse a wire name. Returns false for anything unknown - unknown state
   names are rejected, never silently accepted. */
bool robot_state_parse(const char *name, robot_state_t *out);

#ifdef __cplusplus
}
#endif
