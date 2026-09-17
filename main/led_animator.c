#include "led_animator.h"

#include <stddef.h>

#include "config.h"
#include "esp_log.h"
#include "matrix.h"
#include "patterns.h"

static const char *TAG = "anim";

/* One entry per animation frame: what to draw and how long it stays. */
typedef struct {
    const pattern_t *pattern;   /* NULL = blank panel */
    uint32_t hold_ms;
} frame_t;

/* LOAD_BEER: an instruction, not just an icon. The glass fills up, an arrow
   points into it, then L-O-A-D is spelled out, and it repeats. The beer icon
   stays the dominant part because it is understood instantly. */
static const frame_t s_load_beer[] = {
    { &PAT_BEER_EMPTY,  LOAD_BEER_FILL_MS },
    { &PAT_BEER_FILL1,  LOAD_BEER_FILL_MS },
    { &PAT_BEER_FILL2,  LOAD_BEER_FILL_MS },
    { &PAT_BEER_FILL3,  LOAD_BEER_FILL_MS },
    { &PAT_BEER,        LOAD_BEER_ON_MS },
    { NULL,             LOAD_BEER_OFF_MS },
    { &PAT_LOAD_ARROW,  LOAD_BEER_ARROW_MS },
    { NULL,             LOAD_BEER_OFF_MS },
    { &PAT_TEXT_L,      LOAD_BEER_TEXT_MS },
    { &PAT_TEXT_O,      LOAD_BEER_TEXT_MS },
    { &PAT_TEXT_A,      LOAD_BEER_TEXT_MS },
    { &PAT_TEXT_D,      LOAD_BEER_TEXT_MS },
    { NULL,             LOAD_BEER_OFF_MS },
};

/* AUTONOMOUS_DRIVING: amber arrow travelling right. */
static const frame_t s_driving[] = {
    { &PAT_DRIVE_0, AUTONOMOUS_FRAME_MS },
    { &PAT_DRIVE_1, AUTONOMOUS_FRAME_MS },
    // { &PAT_DRIVE_2, AUTONOMOUS_FRAME_MS },
};

/* FAULT: red X flashing. Highest visual priority, deliberately plain. */
static const frame_t s_fault[] = {
    { &PAT_X, FAULT_BLINK_MS },
    { NULL,   FAULT_BLINK_MS },
};

/* MISSION_COMPLETE: check, smile, beer. */
static const frame_t s_mission_complete[] = {
    { &PAT_CHECK,  MISSION_FRAME_MS },
    { &PAT_SMILEY, MISSION_FRAME_MS },
    { &PAT_BEER,   MISSION_FRAME_MS },
};

/* SCAN_QR: show the QR badge, then spell SCAN. */
static const frame_t s_scan_qr[] = {
    { &PAT_QR,     SCAN_QR_HOLD_MS },
    { NULL,        LOAD_BEER_OFF_MS },
    { &PAT_TEXT_S, SCAN_QR_TEXT_MS },
    { &PAT_TEXT_C, SCAN_QR_TEXT_MS },
    { &PAT_TEXT_A, SCAN_QR_TEXT_MS },
    { &PAT_TEXT_N, SCAN_QR_TEXT_MS },
    { NULL,        LOAD_BEER_OFF_MS },
};

typedef struct {
    const frame_t *frames;
    uint8_t count;
} animation_t;

static const animation_t s_animations[ROBOT_STATE_COUNT] = {
    [ROBOT_STATE_LOAD_BEER]          = { s_load_beer,        sizeof(s_load_beer) / sizeof(s_load_beer[0]) },
    [ROBOT_STATE_AUTONOMOUS_DRIVING] = { s_driving,          sizeof(s_driving) / sizeof(s_driving[0]) },
    [ROBOT_STATE_FAULT]              = { s_fault,            sizeof(s_fault) / sizeof(s_fault[0]) },
    [ROBOT_STATE_MISSION_COMPLETE]   = { s_mission_complete, sizeof(s_mission_complete) / sizeof(s_mission_complete[0]) },
    [ROBOT_STATE_SCAN_QR]            = { s_scan_qr,         sizeof(s_scan_qr) / sizeof(s_scan_qr[0]) },
};

static robot_state_t s_state = ROBOT_STATE_LOAD_BEER;
static uint8_t s_frame;
static uint32_t s_frame_started_ms;
static bool s_dirty = true;         /* frame still needs drawing */

static void draw_frame(void)
{
    const animation_t *anim = &s_animations[s_state];

    if (anim->frames == NULL || anim->count == 0) {
        matrix_blank();
        return;
    }

    const pattern_t *p = anim->frames[s_frame].pattern;
    if (p == NULL) {
        matrix_blank();
    } else {
        visualise(p);
    }
}

void led_animator_init(void)
{
    matrix_set_brightness(LED_BRIGHTNESS_DEFAULT);
    s_state = ROBOT_STATE_LOAD_BEER;
    s_frame = 0;
    s_frame_started_ms = 0;
    s_dirty = true;
}

void led_animator_set_state(robot_state_t state)
{
    if (state >= ROBOT_STATE_COUNT || state == s_state) {
        return;
    }
    ESP_LOGI(TAG, "showing %s", robot_state_name(state));
    s_state = state;
    s_frame = 0;
    s_dirty = true;         /* redraw on the next update, from frame 0 */
}

void led_animator_update(uint32_t now_ms)
{
    const animation_t *anim = &s_animations[s_state];
    uint8_t count = (anim->frames != NULL) ? anim->count : 0;

    if (s_dirty) {
        s_frame_started_ms = now_ms;
        s_dirty = false;
        draw_frame();
        return;
    }

    if (count <= 1) {
        return;             /* a still picture, nothing to advance */
    }

    uint32_t hold = anim->frames[s_frame].hold_ms;
    if (now_ms - s_frame_started_ms < hold) {
        return;             /* current frame still has time left */
    }

    s_frame = (uint8_t)((s_frame + 1) % count);
    s_frame_started_ms = now_ms;
    draw_frame();
}

void led_animator_set_brightness(uint8_t brightness)
{
    matrix_set_brightness(brightness);
    s_dirty = true;         /* repaint so the change is visible at once */
}

uint8_t led_animator_get_brightness(void)
{
    return matrix_get_brightness();
}
