/* Low level driver for an 8x8 WS2812B matrix on an ESP32-C3.

   Everything here works in (x, y) coordinates with y = 0 at the TOP row and
   x = 0 at the LEFT column, no matter how the panel is wired internally.
*/
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATRIX_GPIO         1       /* data-in pin of the matrix */
#define MATRIX_WIDTH        8
#define MATRIX_HEIGHT       8
#define MATRIX_LEDS         (MATRIX_WIDTH * MATRIX_HEIGHT)

/* --- panel wiring -----------------------------------------------------
   These four knobs describe how the LED chain is laid out physically. Get
   them right once and every pattern comes out the way you drew it.

   Run matrix_test_corners() and look at the panel the way you want to read
   it (data-in cable wherever it ends up):
     - red pixel      -> must be TOP-LEFT
     - green pixel    -> must be TOP-RIGHT
     - blue pixel     -> must be BOTTOM-LEFT
     - the white pixel is the first LED in the chain, index 0

   If red is not top-left, flip MATRIX_FLIP_X / MATRIX_FLIP_Y until it is.
   Then show PAT_DIAGONAL: it must be one clean corner-to-corner line. If it
   comes out broken into a zig-zag, toggle MATRIX_SERPENTINE.

   Symmetric pictures (arrows, checkerboards, full colours) look correct even
   when this is wrong, so always test with something asymmetric. */

/* Most 8x8 WS2812B panels are wired zig-zag: the first row runs one way, the
   next row back the other way. Set to 0 if every row starts on the same side
   (progressive wiring). */
#define MATRIX_SERPENTINE   1

/* Mirror left<->right / top<->bottom, for panels whose first LED is not at
   the top-left corner as you are looking at it. */
#define MATRIX_FLIP_X       0
#define MATRIX_FLIP_Y       0

/* Set to 1 if the chain runs in columns instead of rows (first LED at the
   top-left, then straight down). */
#define MATRIX_TRANSPOSE    0

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

/* Call once from app_main() before anything else. */
void matrix_init(void);

/* Global brightness, 0..255, applied to every colour on its way to the LEDs.
   Defaults to 64. 64 LEDs at full white draw ~3.8 A, so keep this low unless
   the panel has its own 5 V supply. */
void matrix_set_brightness(uint8_t brightness);
uint8_t matrix_get_brightness(void);

/* Write into the back buffer. Out-of-range coordinates are ignored. */
void matrix_set_xy(uint32_t x, uint32_t y, rgb_t color);
void matrix_fill(rgb_t color);
void matrix_clear(void);

/* Push the back buffer to the panel. Nothing is visible until you call this. */
void matrix_show(void);

/* Clear + show, i.e. turn the whole panel off right now. */
void matrix_blank(void);

/* Orientation test: red at (0,0) top-left, green at (7,0) top-right, blue at
   (0,7) bottom-left, and white on chain index 0 (the first physical LED).
   Use it to set the MATRIX_* wiring macros above. */
void matrix_test_corners(void);

#ifdef __cplusplus
}
#endif
