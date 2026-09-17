#include "robot_state.h"

#include <string.h>

static const char *const s_names[ROBOT_STATE_COUNT] = {
    [ROBOT_STATE_LOAD_BEER]          = "LOAD_BEER",
    [ROBOT_STATE_AUTONOMOUS_DRIVING] = "AUTONOMOUS_DRIVING",
    [ROBOT_STATE_FAULT]              = "FAULT",
    [ROBOT_STATE_MISSION_COMPLETE]   = "MISSION_COMPLETE",
    [ROBOT_STATE_SCAN_QR]            = "SCAN_QR",
};

const char *robot_state_name(robot_state_t state)
{
    if (state >= ROBOT_STATE_COUNT) {
        return "UNKNOWN";
    }
    return s_names[state];
}

bool robot_state_parse(const char *name, robot_state_t *out)
{
    if (name == NULL || out == NULL) {
        return false;
    }
    for (int i = 0; i < ROBOT_STATE_COUNT; i++) {
        if (strcmp(name, s_names[i]) == 0) {
            *out = (robot_state_t)i;
            return true;
        }
    }
    return false;
}
