/* Pattern library for the 8x8 matrix.

   A pattern is literally an 8x8 picture: eight rows of eight one-letter colour
   macros, top row first. To show one, call

       visualise(&PAT_HEART);              // draw it and leave it lit
       visualise_for(&PAT_HEART, 1000);    // ... for 1 second, then blank
       blink(&PAT_HEART, 200, 200, 5);     // blink it 5 times

   A sequence is a list of patterns played as an animation:

       play(&SEQ_ROW_SWEEP, 3);            // 3 loops (0 = forever)

   To add your own pattern, copy one of the tables in patterns.c.
*/
#pragma once

#include <stdint.h>
#include "matrix.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Palette indices. These are what the one-letter macros in patterns.c expand
   to; the actual RGB values live in the palette table there. */
typedef enum {
    CLR_OFF = 0,
    CLR_RED,
    CLR_ORANGE,
    CLR_YELLOW,
    CLR_GREEN,
    CLR_CYAN,
    CLR_BLUE,
    CLR_PURPLE,
    CLR_WHITE,
    CLR_TURQUOISE,
    CLR_AMBER,
    CLR_GRAY,
    CLR_QRTOMI,
    CLR_QRTOMI2,
    CLR_COUNT,

} color_t;

typedef struct {
    const char *name;
    uint8_t px[MATRIX_HEIGHT][MATRIX_WIDTH];   /* px[y][x], y = 0 is the top row */
} pattern_t;

typedef struct {
    const char *name;
    const pattern_t *const *frames;
    uint8_t frame_count;
    uint16_t frame_ms;      /* how long each frame stays on screen */
} sequence_t;

/* --- drawing ---------------------------------------------------------- */

rgb_t color_rgb(color_t c);

/* Draw the pattern and push it to the panel. It stays lit until something
   else is drawn. */
void visualise(const pattern_t *p);

/* Same, then blank the panel after ms milliseconds. */
void visualise_for(const pattern_t *p, uint32_t ms);

/* Blink a pattern. times = 0 blinks forever. */
void blink(const pattern_t *p, uint32_t on_ms, uint32_t off_ms, uint32_t times);

/* Draw the pattern in a single colour, ignoring the colours in the table but
   keeping its shape (every non-off cell becomes `c`). */
void visualise_mono(const pattern_t *p, color_t c);

/* Play a sequence. loops = 0 plays forever and never returns. */
void play(const sequence_t *s, uint32_t loops);

/* Procedural sweeps, for when a full set of frames would be tedious. */
void sweep_rows_down(color_t c, uint32_t row_ms);
void sweep_rows_up(color_t c, uint32_t row_ms);
void sweep_cols_right(color_t c, uint32_t col_ms);
void fade_in_out(const pattern_t *p, uint32_t step_ms);

/* --- patterns --------------------------------------------------------- */

extern const pattern_t PAT_BLANK;
extern const pattern_t PAT_FULL_ORANGE;
extern const pattern_t PAT_FULL_TURQUOISE;
extern const pattern_t PAT_HALF_TURQUOISE;
extern const pattern_t PAT_BEER;
extern const pattern_t PAT_BEER_EMPTY;
extern const pattern_t PAT_BEER_FILL1;
extern const pattern_t PAT_BEER_FILL2;
extern const pattern_t PAT_BEER_FILL3;
extern const pattern_t PAT_LOAD_ARROW;
extern const pattern_t PAT_TEXT_L;
extern const pattern_t PAT_TEXT_O;
extern const pattern_t PAT_TEXT_A;
extern const pattern_t PAT_TEXT_D;
extern const pattern_t PAT_QR;
extern const pattern_t PAT_TEXT_S;
extern const pattern_t PAT_TEXT_C;
extern const pattern_t PAT_TEXT_N;

/* Mission states added on top of the original five (see led_animator.c). */
extern const pattern_t PAT_UNLOAD_ARROW;
extern const pattern_t PAT_TEXT_U;
extern const pattern_t PAT_QR_CONFIRM;
extern const pattern_t PAT_CHECK_BIG;
extern const pattern_t PAT_NO_ENTRY;
extern const pattern_t PAT_HOME;
extern const pattern_t PAT_RETURN_ARROW_0;
extern const pattern_t PAT_RETURN_ARROW_1;
extern const pattern_t PAT_WAIT_1;
extern const pattern_t PAT_WAIT_2;
extern const pattern_t PAT_WAIT_3;
extern const pattern_t PAT_NO_SIGNAL_1;
extern const pattern_t PAT_NO_SIGNAL_2;
extern const pattern_t PAT_NO_SIGNAL_3;
extern const pattern_t PAT_NO_SIGNAL_4;

/* Robot state icons (see led_animator.c). */
extern const pattern_t PAT_CHECK;
extern const pattern_t PAT_DRIVE_0;
extern const pattern_t PAT_DRIVE_1;
extern const pattern_t PAT_DRIVE_2;
extern const pattern_t PAT_BORDER;
extern const pattern_t PAT_CHECKER_A;
extern const pattern_t PAT_CHECKER_B;
extern const pattern_t PAT_CROSS;
extern const pattern_t PAT_X;
extern const pattern_t PAT_HEART;
extern const pattern_t PAT_SMILEY;
extern const pattern_t PAT_SAD;
extern const pattern_t PAT_ARROW_UP;
extern const pattern_t PAT_ARROW_DOWN;
extern const pattern_t PAT_DIAGONAL;
extern const pattern_t PAT_RAINBOW_ROWS;
extern const pattern_t PAT_ROW0;
extern const pattern_t PAT_ROW1;
extern const pattern_t PAT_ROW2;
extern const pattern_t PAT_ROW3;
extern const pattern_t PAT_ROW4;
extern const pattern_t PAT_ROW5;
extern const pattern_t PAT_ROW6;
extern const pattern_t PAT_ROW7;

/* --- sequences -------------------------------------------------------- */

extern const sequence_t SEQ_ROW_SWEEP;     /* orange line, top -> bottom */
extern const sequence_t SEQ_CHECKER;       /* checkerboard flip-flop */
extern const sequence_t SEQ_FACES;         /* smiley <-> sad */
extern const sequence_t SEQ_ARROWS;        /* up / down arrows */
extern const sequence_t SEQ_SHAPES;        /* small tour of the shapes */

#ifdef __cplusplus
}
#endif
