#include "patterns.h"

#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ---------------------------------------------------------------------- */
/* Palette. Tweak the RGB values here and every pattern follows.           */
/* These are full-scale values; matrix_set_brightness() dims them all.     */
/* ---------------------------------------------------------------------- */

static const rgb_t s_palette[CLR_COUNT] = {
    [CLR_OFF]           = {   0,   0,   0 },
    [CLR_RED]           = { 255,   0,   0 },
    [CLR_ORANGE]        = { 255,  72,   0 },
    [CLR_YELLOW]        = { 255, 170,   0 },
    [CLR_GREEN]         = {   0, 255,   0 },
    [CLR_CYAN]          = {   0, 200, 200 },
    [CLR_BLUE]          = {   0,   0, 255 },
    [CLR_PURPLE]        = { 180,   0, 255 },
    [CLR_WHITE]         = { 255, 255, 255 },
    [CLR_AMBER]         = { 255, 126,   0 },
    [CLR_TURQUOISE]     = {  64, 224, 204 },
    [CLR_GRAY]      = {  64, 77, 77 },
    [CLR_QRTOMI]     = {  255, 140, 0 },
    [CLR_QRTOMI2]     = {  0, 0, 255 },
    
};

rgb_t color_rgb(color_t c)
{
    if (c >= CLR_COUNT) {
        return s_palette[CLR_OFF];
    }
    return s_palette[c];
}

/* ---------------------------------------------------------------------- */
/* One-letter colour macros, so a pattern reads like a picture.            */
/* They are undefined again at the end of the tables, nothing leaks out.   */
/* ---------------------------------------------------------------------- */

#define K   CLR_OFF        /* blacK / off */
#define R   CLR_RED
#define O   CLR_ORANGE
#define Y   CLR_YELLOW
#define G   CLR_GREEN
#define C   CLR_CYAN
#define B   CLR_BLUE
#define P   CLR_PURPLE
#define W   CLR_WHITE
#define T   CLR_TURQUOISE
#define A   CLR_AMBER
#define Q   CLR_QRTOMI
#define Q2  CLR_QRTOMI2
#define GR   CLR_GRAY

/* ---------------------------------------------------------------------- */
/* Patterns. Top row first, left column first.                             */
/* ---------------------------------------------------------------------- */

const pattern_t PAT_BLANK = {
    .name = "blank",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_FULL_ORANGE = {
    .name = "full orange",
    .px = {
        { O,O,O,O,O,O,O,O },
        { O,O,O,O,O,O,O,O },
        { O,O,O,O,O,O,O,O },
        { O,O,O,O,O,O,O,O },
        { O,O,O,O,O,O,O,O },
        { O,O,O,O,O,O,O,O },
        { O,O,O,O,O,O,O,O },
        { O,O,O,O,O,O,O,O },
    },
};

const pattern_t PAT_BORDER = {
    .name = "border",
    .px = {
        { O,O,O,O,O,O,O,O },
        { O,K,K,K,K,K,K,O },
        { O,K,K,K,K,K,K,O },
        { O,K,K,K,K,K,K,O },
        { O,K,K,K,K,K,K,O },
        { O,K,K,K,K,K,K,O },
        { O,K,K,K,K,K,K,O },
        { O,O,O,O,O,O,O,O },
    },
};

const pattern_t PAT_CHECKER_A = {
    .name = "checker a",
    .px = {
        { O,K,O,K,O,K,O,K },
        { K,O,K,O,K,O,K,O },
        { O,K,O,K,O,K,O,K },
        { K,O,K,O,K,O,K,O },
        { O,K,O,K,O,K,O,K },
        { K,O,K,O,K,O,K,O },
        { O,K,O,K,O,K,O,K },
        { K,O,K,O,K,O,K,O },
    },
};

const pattern_t PAT_CHECKER_B = {
    .name = "checker b",
    .px = {
        { K,O,K,O,K,O,K,O },
        { O,K,O,K,O,K,O,K },
        { K,O,K,O,K,O,K,O },
        { O,K,O,K,O,K,O,K },
        { K,O,K,O,K,O,K,O },
        { O,K,O,K,O,K,O,K },
        { K,O,K,O,K,O,K,O },
        { O,K,O,K,O,K,O,K },
    },
};

const pattern_t PAT_CROSS = {
    .name = "cross",
    .px = {
        { K,K,K,R,R,K,K,K },
        { K,K,K,R,R,K,K,K },
        { K,K,K,R,R,K,K,K },
        { R,R,R,R,R,R,R,R },
        { R,R,R,R,R,R,R,R },
        { K,K,K,R,R,K,K,K },
        { K,K,K,R,R,K,K,K },
        { K,K,K,R,R,K,K,K },
    },
};

const pattern_t PAT_X = {
    .name = "x",
    .px = {
        { R,R,K,K,K,K,R,R },
        { R,R,R,K,K,R,R,R },
        { K,R,R,R,R,R,R,K },
        { K,K,R,R,R,R,K,K },
        { K,K,R,R,R,R,K,K },
        { K,R,R,R,R,R,R,K },
        { R,R,R,K,K,R,R,R },
        { R,R,K,K,K,K,R,R },
    },
};

const pattern_t PAT_HEART = {
    .name = "heart",
    .px = {
        { K,R,R,K,K,R,R,K },
        { R,R,R,R,R,R,R,R },
        { R,R,R,R,R,R,R,R },
        { R,R,R,R,R,R,R,R },
        { K,R,R,R,R,R,R,K },
        { K,K,R,R,R,R,K,K },
        { K,K,K,R,R,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_SMILEY = {
    .name = "smiley",
    .px = {
        { K,K,Y,Y,Y,Y,K,K },
        { K,Y,K,K,K,K,Y,K },
        { Y,K,Y,K,K,Y,K,Y },
        { Y,K,K,K,K,K,K,Y },
        { Y,K,Y,K,K,Y,K,Y },
        { Y,K,K,Y,Y,K,K,Y },
        { K,Y,K,K,K,K,Y,K },
        { K,K,Y,Y,Y,Y,K,K },
    },
};

const pattern_t PAT_SAD = {
    .name = "sad",
    .px = {
        { K,K,Y,Y,Y,Y,K,K },
        { K,Y,K,K,K,K,Y,K },
        { Y,K,Y,K,K,Y,K,Y },
        { Y,K,K,K,K,K,K,Y },
        { Y,K,K,Y,Y,K,K,Y },
        { Y,K,Y,K,K,Y,K,Y },
        { K,Y,K,K,K,K,Y,K },
        { K,K,Y,Y,Y,Y,K,K },
    },
};

const pattern_t PAT_ARROW_UP = {
    .name = "arrow up",
    .px = {
        { K,K,K,G,G,K,K,K },
        { K,K,G,G,G,G,K,K },
        { K,G,G,G,G,G,G,K },
        { G,G,G,G,G,G,G,G },
        { K,K,K,G,G,K,K,K },
        { K,K,K,G,G,K,K,K },
        { K,K,K,G,G,K,K,K },
        { K,K,K,G,G,K,K,K },
    },
};

const pattern_t PAT_FULL_TURQUOISE = {
    .name = "full turquoise",
    .px = {
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
    },
};

const pattern_t PAT_HALF_TURQUOISE = {
    .name = "half turquoise",
    .px = {
        { T,K,T,K,T,K,T,K },
        { K,T,K,T,K,T,K,T },
        { T,K,T,K,T,K,T,K },
        { K,T,K,T,K,T,K,T },
        { T,K,T,K,T,K,T,K },
        { K,T,K,T,K,T,K,T },
        { T,K,T,K,T,K,T,K },
        { K,T,K,T,K,T,K,T },
    },
};

/* Beer mug: glass outline in white, beer in yellow, handle on the right.
   The rim and the base span the same columns as the two side walls (x1..x5),
   otherwise the top and bottom of the glass look offset from the body. */
const pattern_t PAT_BEER = {
    .name = "beer",
    .px = {
        { K,K,W,W,W,K,K,K },
        { K,W,W,W,W,W,K,K },
        { K,W,W,W,W,W,W,K },
        { K,W,Y,Y,Y,W,K,W },
        { K,W,Y,Y,Y,W,K,W },
        { K,W,Y,Y,Y,W,K,W },
        { K,W,Y,Y,Y,W,W,K },
        { K,W,W,W,W,W,K,K },
    },
};



const pattern_t PAT_ARROW_DOWN = {
    .name = "arrow down",
    .px = {
        { K,K,K,B,B,K,K,K },
        { K,K,K,B,B,K,K,K },
        { K,K,K,B,B,K,K,K },
        { K,K,K,B,B,K,K,K },
        { B,B,B,B,B,B,B,B },
        { K,B,B,B,B,B,B,K },
        { K,K,B,B,B,B,K,K },
        { K,K,K,B,B,K,K,K },
    },
};

const pattern_t PAT_DIAGONAL = {
    .name = "diagonal",
    .px = {
        { C,K,K,K,K,K,K,K },
        { K,C,K,K,K,K,K,K },
        { K,K,C,K,K,K,K,K },
        { K,K,K,C,K,K,K,K },
        { K,K,K,K,C,K,K,K },
        { K,K,K,K,K,C,K,K },
        { K,K,K,K,K,K,C,K },
        { K,K,K,K,K,K,K,C },
    },
};

const pattern_t PAT_RAINBOW_ROWS = {
    .name = "rainbow rows",
    .px = {
        { R,R,R,R,R,R,R,R },
        { O,O,O,O,O,O,O,O },
        { Y,Y,Y,Y,Y,Y,Y,Y },
        { G,G,G,G,G,G,G,G },
        { C,C,C,C,C,C,C,C },
        { B,B,B,B,B,B,B,B },
        { P,P,P,P,P,P,P,P },
        { W,W,W,W,W,W,W,W },
    },
};

/* The eight frames of the orange top-to-bottom sweep. */

const pattern_t PAT_ROW0 = {
    .name = "row 0",
    .px = {
        { O,O,O,O,O,O,O,O },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_ROW1 = {
    .name = "row 1",
    .px = {
        { K,K,K,K,K,K,K,K },
        { O,O,O,O,O,O,O,O },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_ROW2 = {
    .name = "row 2",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { O,O,O,O,O,O,O,O },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_ROW3 = {
    .name = "row 3",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { O,O,O,O,O,O,O,O },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_ROW4 = {
    .name = "row 4",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { O,O,O,O,O,O,O,O },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_ROW5 = {
    .name = "row 5",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { O,O,O,O,O,O,O,O },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_ROW6 = {
    .name = "row 6",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { O,O,O,O,O,O,O,O },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_ROW7 = {
    .name = "row 7",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { O,O,O,O,O,O,O,O },
    },
};


/* --- robot state icons ------------------------------------------------- */

/* Green tick for MISSION_COMPLETE. */
const pattern_t PAT_CHECK = {
    .name = "check",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,G,K },
        { K,K,K,K,K,G,G,K },
        { G,K,K,K,G,G,K,K },
        { G,G,K,G,G,K,K,K },
        { K,G,G,G,K,K,K,K },
        { K,K,G,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

/* Amber arrow travelling to the right, three frames, for
   AUTONOMOUS_DRIVING. */
const pattern_t PAT_DRIVE_0 = {
    .name = "drive 0",
    .px = {
        // { K,K,K,A,A,K,K,K },
        // { K,K,K,K,A,A,K,K },
        // { A,A,A,A,A,A,A,A },
        // { K,K,K,K,A,A,K,K },
        // { K,K,K,A,A,K,K,K },
        // { K,K,K,K,K,K,K,K },
        // { K,K,K,K,K,K,K,K },
        // { K,K,K,K,K,K,K,K },
        { T,K,T,K,T,K,T,K },
        { K,T,K,T,K,T,K,T },
        { T,K,T,K,T,K,T,K },
        { K,T,K,T,K,T,K,T },
        { T,K,T,K,T,K,T,K },
        { K,T,K,T,K,T,K,T },
        { T,K,T,K,T,K,T,K },
        { K,T,K,T,K,T,K,T },
    },
};

const pattern_t PAT_DRIVE_1 = {
    .name = "drive 1",
    .px = {
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
        { T,T,T,T,T,T,T,T },
    },
};

const pattern_t PAT_DRIVE_2 = {
    .name = "drive 2",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

/* --- LOAD_BEER sequence ------------------------------------------------ */

/* The same glass as PAT_BEER, filling up. PAT_BEER itself is the last,
   full frame - keep the outline identical in all of them or the glass
   appears to jump. */
const pattern_t PAT_BEER_EMPTY = {
    .name = "beer empty",
    .px = {
        { K,K,W,W,W,K,K,K },
        { K,W,W,W,W,W,K,K },
        { K,W,W,W,W,W,W,K },
        { K,W,K,K,K,W,K,W },
        { K,W,K,K,K,W,K,W },
        { K,W,K,K,K,W,K,W },
        { K,W,K,K,K,W,W,K },
        { K,W,W,W,W,W,K,K },
    },
};

const pattern_t PAT_BEER_FILL1 = {
    .name = "beer fill 1",
    .px = {
        { K,K,W,W,W,K,K,K },
        { K,W,W,W,W,W,K,K },
        { K,W,W,W,W,W,W,K },
        { K,W,K,K,K,W,K,W },
        { K,W,K,K,K,W,K,W },
        { K,W,K,K,K,W,K,W },
        { K,W,Y,Y,Y,W,W,K },
        { K,W,W,W,W,W,K,K },
    },
};

const pattern_t PAT_BEER_FILL2 = {
    .name = "beer fill 2",
    .px = {
        { K,K,W,W,W,K,K,K },
        { K,W,W,W,W,W,K,K },
        { K,W,W,W,W,W,W,K },
        { K,W,K,K,K,W,K,W },
        { K,W,K,K,K,W,K,W },
        { K,W,Y,Y,Y,W,K,W },
        { K,W,Y,Y,Y,W,W,K },
        { K,W,W,W,W,W,K,K },
    },
};

const pattern_t PAT_BEER_FILL3 = {
    .name = "beer fill 3",
    .px = {
        { K,K,W,W,W,K,K,K },
        { K,W,W,W,W,W,K,K },
        { K,W,W,W,W,W,W,K },
        { K,W,K,K,K,W,K,W },
        { K,W,Y,Y,Y,W,K,W },
        { K,W,Y,Y,Y,W,K,W },
        { K,W,Y,Y,Y,W,W,K },
        { K,W,W,W,W,W,K,K },
    },
};

/* Amber arrow pointing down into the glass: "put it in here". */
const pattern_t PAT_LOAD_ARROW = {
    .name = "load arrow",
    .px = {
        { K,K,K,A,A,K,K,K },
        { K,K,K,A,A,K,K,K },
        { A,A,K,A,A,K,A,A },
        { K,A,A,A,A,A,A,K },
        { K,K,A,A,A,A,K,K },
        { K,K,K,A,A,K,K,K },
        { K,W,W,W,W,W,W,K },
        { K,W,W,W,W,W,W,K },
    },
};

/* L-O-A-D, shown one letter at a time. */
const pattern_t PAT_TEXT_L = {
    .name = "letter L",
    .px = {
        { K,A,K,K,K,K,K,K },
        { K,A,K,K,K,K,K,K },
        { K,A,K,K,K,K,K,K },
        { K,A,K,K,K,K,K,K },
        { K,A,K,K,K,K,K,K },
        { K,A,K,K,K,K,K,K },
        { K,A,A,A,A,A,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_TEXT_O = {
    .name = "letter O",
    .px = {
        { K,K,A,A,A,K,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,K,A,A,A,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_TEXT_A = {
    .name = "letter A",
    .px = {
        { K,K,A,A,A,K,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,A,A,A,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_TEXT_D = {
    .name = "letter D",
    .px = {
        { K,A,A,A,A,K,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,A,A,A,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

/* --- SCAN_QR ----------------------------------------------------------- */

/* A QR-like badge: three finder squares plus data modules. A real QR code
   needs at least 21x21 modules, so this is a recognisable symbol telling the
   operator to scan, not a scannable code. */
const pattern_t PAT_QR = {
    .name = "qr",
    .px = {
        { W,W,W,K,K,W,W,W },
        { W,K,W,K,K,W,K,W },
        { W,W,W,K,K,W,W,W },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { W,W,W,K,K,W,W,K },
        { W,K,W,K,K,W,W,K },
        { W,W,W,K,K,K,K,K },
    },
};

const pattern_t PAT_QR_TO = {
    .name = "qr",
    .px = {
        { Q2,Q2,Q2,Q,Q,Q2,Q2,Q2 },
        { Q2,Q,Q2,Q,Q,Q2,Q,Q2 },
        { Q2,Q2,Q2,Q,Q,Q2,Q2,Q2 },
        { Q,Q,Q,Q,Q,Q,Q,Q },
        { Q,Q,Q,Q,Q,Q,Q,Q },
        { Q2,Q2,Q2,Q,Q,Q2,Q2,Q },
        { Q2,Q,Q2,Q,Q,Q2,Q2,Q },
        { Q2,Q2,Q2,Q,Q,Q,Q,Q },
    },
};
/* S-C-A-N. The A is PAT_TEXT_A, shared with the LOAD sequence. */
const pattern_t PAT_TEXT_S = {
    .name = "letter S",
    .px = {
        { K,K,A,A,A,K,K,K },
        { K,A,K,K,K,K,K,K },
        { K,A,K,K,K,K,K,K },
        { K,K,A,A,A,K,K,K },
        { K,K,K,K,K,A,K,K },
        { K,K,K,K,K,A,K,K },
        { K,K,A,A,A,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_TEXT_C = {
    .name = "letter C",
    .px = {
        { K,K,A,A,A,K,K,K },
        { K,A,K,K,K,K,K,K },
        { K,A,K,K,K,K,K,K },
        { K,A,K,K,K,K,K,K },
        { K,A,K,K,K,K,K,K },
        { K,A,K,K,K,K,K,K },
        { K,K,A,A,A,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_TEXT_N = {
    .name = "letter N",
    .px = {
        { K,A,K,K,K,A,K,K },
        { K,A,A,K,K,A,K,K },
        { K,A,A,K,K,A,K,K },
        { K,A,K,A,K,A,K,K },
        { K,A,K,K,A,A,K,K },
        { K,A,K,K,A,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

/* --- UNLOAD_BEER ------------------------------------------------------- */

/* The mirror image of PAT_LOAD_ARROW: a big arrow rising OUT of an open
   crate, i.e. "take it off here". Deliberately the same visual grammar as
   the load arrow with the direction reversed, and green instead of amber,
   so at three metres the direction alone already tells the two apart. */
const pattern_t PAT_UNLOAD_ARROW = {
    .name = "unload arrow",
    .px = {
        { K,K,K,G,G,K,K,K },
        { K,K,G,G,G,G,K,K },
        { K,G,G,G,G,G,G,K },
        { G,G,G,G,G,G,G,G },
        { K,K,K,G,G,K,K,K },
        { K,K,K,G,G,K,K,K },
        { W,W,K,K,K,K,W,W },
        { W,W,W,W,W,W,W,W },
    },
};

/* The only letter UNLOAD needs that no other word here already has: N is
   shared with S-C-A-N, and L, O, A and D with LOAD. */
const pattern_t PAT_TEXT_U = {
    .name = "letter U",
    .px = {
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,A,K,K,K,A,K,K },
        { K,K,A,A,A,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

/* --- QR_CONFIRM -------------------------------------------------------- */

/* A large amber question mark over two QR finder squares: "QR code seen,
   your decision". No letters and no animation, so it cannot be mistaken for
   the SCAN_QR badge-plus-S-C-A-N cycle. */
const pattern_t PAT_QR_CONFIRM = {
    .name = "qr confirm",
    .px = {
        { K,K,A,A,A,A,K,K },
        { K,A,A,K,K,A,A,K },
        { K,K,K,K,K,A,A,K },
        { K,K,K,K,A,A,K,K },
        { K,K,K,A,A,K,K,K },
        { K,K,K,A,A,K,K,K },
        { W,W,K,K,K,K,W,W },
        { W,W,K,A,A,K,W,W },
    },
};

/* --- ACCEPTED / REJECTED ----------------------------------------------- */

/* A fat green tick. PAT_CHECK is one pixel thin and is kept for
   MISSION_COMPLETE; this one has to read across the arena. */
const pattern_t PAT_CHECK_BIG = {
    .name = "check big",
    .px = {
        { K,K,K,K,K,K,G,G },
        { K,K,K,K,K,G,G,G },
        { K,K,K,K,G,G,G,K },
        { G,K,K,G,G,G,K,K },
        { G,G,G,G,G,K,K,K },
        { G,G,G,G,K,K,K,K },
        { K,G,G,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

/* A "no entry" sign: ring with a bar through it. Rejection is a decision,
   not a breakdown, so it must NOT be the red X that FAULT uses. */
const pattern_t PAT_NO_ENTRY = {
    .name = "no entry",
    .px = {
        { K,K,R,R,R,R,K,K },
        { K,R,R,K,K,K,R,K },
        { R,K,R,R,K,K,K,R },
        { R,K,K,R,R,K,K,R },
        { R,K,K,K,R,R,K,R },
        { R,K,K,K,K,R,R,R },
        { K,R,K,K,K,K,R,K },
        { K,K,R,R,R,R,K,K },
    },
};

/* --- RETURNING --------------------------------------------------------- */

/* House with a lit doorway. Together with the arrow frames below it reads
   as "heading home", which the plain turquoise AUTONOMOUS_DRIVING does
   not. */
const pattern_t PAT_HOME = {
    .name = "home",
    .px = {
        { K,K,K,W,W,K,K,K },
        { K,K,W,W,W,W,K,K },
        { K,W,W,W,W,W,W,K },
        { W,W,W,W,W,W,W,W },
        { K,W,W,W,W,W,W,K },
        { K,W,W,C,C,W,W,K },
        { K,W,W,C,C,W,W,K },
        { K,W,W,C,C,W,W,K },
    },
};

/* The same arrow two pixels apart, so it visibly travels towards the house
   before the house itself appears. */
const pattern_t PAT_RETURN_ARROW_0 = {
    .name = "return arrow 0",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,C,K,K,K,K },
        { K,K,K,C,C,K,K,K },
        { C,C,C,C,C,C,K,K },
        { C,C,C,C,C,C,K,K },
        { K,K,K,C,C,K,K,K },
        { K,K,K,C,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_RETURN_ARROW_1 = {
    .name = "return arrow 1",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,C,K,K },
        { K,K,K,K,K,C,C,K },
        { K,K,C,C,C,C,C,C },
        { K,K,C,C,C,C,C,C },
        { K,K,K,K,K,C,C,K },
        { K,K,K,K,K,C,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

/* --- WAITING ----------------------------------------------------------- */

/* Three fat dots appearing one by one, slowly. Neutral white, no red and no
   amber: nothing is wrong, the robot is simply not ready yet. */
const pattern_t PAT_WAIT_1 = {
    .name = "wait 1",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { W,W,K,K,K,K,K,K },
        { W,W,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_WAIT_2 = {
    .name = "wait 2",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { W,W,K,W,W,K,K,K },
        { W,W,K,W,W,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

const pattern_t PAT_WAIT_3 = {
    .name = "wait 3",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { W,W,K,W,W,K,W,W },
        { W,W,K,W,W,K,W,W },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
    },
};

/* --- NO_SIGNAL --------------------------------------------------------- */

/* Signal bars with a red bar struck through them. White bars keep it clearly
   apart from the all-red FAULT cross, and it stays still while FAULT
   blinks. */
const pattern_t PAT_NO_SIGNAL_1 = {
    .name = "no signal",
    .px = {
        { R,R,K,K,K,K,K,K },
        { K,R,R,K,K,K,K,K },
        { K,K,R,R,K,K,W,W },
        { K,K,K,R,R,K,W,W },
        { K,K,K,W,R,R,W,W },
        { K,K,K,W,W,R,R,W },
        { W,W,K,W,W,K,R,R },
        { W,W,K,W,W,K,W,R },
    },
};

const pattern_t PAT_NO_SIGNAL_2 = {
    .name = "no signal",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { W,W,K,K,K,K,K,K },
        { W,W,K,K,K,K,K,K },
    },
};

const pattern_t PAT_NO_SIGNAL_3 = {
    .name = "no signal",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,W,W,K,K,K },
        { K,K,K,W,W,K,K,K },
        { W,W,K,W,W,K,K,K },
        { W,W,K,W,W,K,K,K },
    },
};

const pattern_t PAT_NO_SIGNAL_4 = {
    .name = "no signal",
    .px = {
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,K,K },
        { K,K,K,K,K,K,W,W },
        { K,K,K,K,K,K,W,W },
        { K,K,K,W,W,K,W,W },
        { K,K,K,W,W,K,W,W },
        { W,W,K,W,W,K,W,W },
        { W,W,K,W,W,K,W,W },
    },
};


// const pattern_t PAT_NO_SIGNAL = {
//     .name = "no signal",
//     .px = {
//         { K,K,W,W,W,W,K,R },
//         { K,W,K,K,K,K,R,K },
//         { W,K,K,K,K,R,K,W },
//         { K,K,W,W,R,W,K,K },
//         { K,W,K,R,K,K,W,K },
//         { K,K,R,K,K,K,K,K },
//         { K,R,K,W,W,K,K,K },
//         { R,K,K,K,K,K,K,K },
//     },
// };

#undef K
#undef R
#undef O
#undef Y
#undef G
#undef C
#undef B
#undef P
#undef W
#undef T
#undef A

/* ---------------------------------------------------------------------- */
/* Sequences: a list of frames plus how long each one is shown.            */
/* ---------------------------------------------------------------------- */

static const pattern_t *const s_row_sweep_frames[] = {
    &PAT_ROW0, &PAT_ROW1, &PAT_ROW2, &PAT_ROW3,
    &PAT_ROW4, &PAT_ROW5, &PAT_ROW6, &PAT_ROW7,
};
const sequence_t SEQ_ROW_SWEEP = {
    .name = "row sweep",
    .frames = s_row_sweep_frames,
    .frame_count = sizeof(s_row_sweep_frames) / sizeof(s_row_sweep_frames[0]),
    .frame_ms = 120,
};

static const pattern_t *const s_checker_frames[] = {
    &PAT_CHECKER_A, &PAT_CHECKER_B,
};
const sequence_t SEQ_CHECKER = {
    .name = "checker",
    .frames = s_checker_frames,
    .frame_count = sizeof(s_checker_frames) / sizeof(s_checker_frames[0]),
    .frame_ms = 400,
};

static const pattern_t *const s_face_frames[] = {
    &PAT_SMILEY, &PAT_SAD,
};
const sequence_t SEQ_FACES = {
    .name = "faces",
    .frames = s_face_frames,
    .frame_count = sizeof(s_face_frames) / sizeof(s_face_frames[0]),
    .frame_ms = 700,
};



static const pattern_t *const s_arrow_frames[] = {
    &PAT_ARROW_UP, &PAT_BLANK, &PAT_ARROW_DOWN, &PAT_BLANK,
};
const sequence_t SEQ_ARROWS = {
    .name = "arrows",
    .frames = s_arrow_frames,
    .frame_count = sizeof(s_arrow_frames) / sizeof(s_arrow_frames[0]),
    .frame_ms = 350,
};

static const pattern_t *const s_shape_frames[] = {
    &PAT_BORDER, &PAT_CROSS, &PAT_X, &PAT_DIAGONAL, &PAT_HEART, &PAT_SMILEY,
};
const sequence_t SEQ_SHAPES = {
    .name = "shapes",
    .frames = s_shape_frames,
    .frame_count = sizeof(s_shape_frames) / sizeof(s_shape_frames[0]),
    .frame_ms = 600,
};


/* ---------------------------------------------------------------------- */
/* Drawing                                                                 */
/* ---------------------------------------------------------------------- */

void visualise(const pattern_t *p)
{
    if (p == NULL) {
        return;
    }
    for (uint32_t y = 0; y < MATRIX_HEIGHT; y++) {
        for (uint32_t x = 0; x < MATRIX_WIDTH; x++) {
            matrix_set_xy(x, y, color_rgb(p->px[y][x]));
        }
    }
    matrix_show();
}

void visualise_mono(const pattern_t *p, color_t c)
{
    if (p == NULL) {
        return;
    }
    rgb_t on = color_rgb(c);
    rgb_t off = color_rgb(CLR_OFF);
    for (uint32_t y = 0; y < MATRIX_HEIGHT; y++) {
        for (uint32_t x = 0; x < MATRIX_WIDTH; x++) {
            matrix_set_xy(x, y, p->px[y][x] == CLR_OFF ? off : on);
        }
    }
    matrix_show();
}

void visualise_for(const pattern_t *p, uint32_t ms)
{
    visualise(p);
    vTaskDelay(pdMS_TO_TICKS(ms));
    matrix_blank();
}

void blink(const pattern_t *p, uint32_t on_ms, uint32_t off_ms, uint32_t times)
{
    for (uint32_t i = 0; times == 0 || i < times; i++) {
        visualise(p);
        vTaskDelay(pdMS_TO_TICKS(on_ms));
        matrix_blank();
        vTaskDelay(pdMS_TO_TICKS(off_ms));
    }
}

void play(const sequence_t *s, uint32_t loops)
{
    if (s == NULL || s->frame_count == 0) {
        return;
    }
    for (uint32_t i = 0; loops == 0 || i < loops; i++) {
        for (uint8_t f = 0; f < s->frame_count; f++) {
            visualise(s->frames[f]);
            vTaskDelay(pdMS_TO_TICKS(s->frame_ms));
        }
    }
}

/* ---------------------------------------------------------------------- */
/* Procedural effects, for when writing out every frame would be tedious.  */
/* ---------------------------------------------------------------------- */

static void draw_row(uint32_t y, color_t c, uint32_t hold_ms)
{
    matrix_clear();
    for (uint32_t x = 0; x < MATRIX_WIDTH; x++) {
        matrix_set_xy(x, y, color_rgb(c));
    }
    matrix_show();
    vTaskDelay(pdMS_TO_TICKS(hold_ms));
}

void sweep_rows_down(color_t c, uint32_t row_ms)
{
    for (uint32_t y = 0; y < MATRIX_HEIGHT; y++) {
        draw_row(y, c, row_ms);
    }
    matrix_blank();
}

void sweep_rows_up(color_t c, uint32_t row_ms)
{
    for (uint32_t y = MATRIX_HEIGHT; y-- > 0; ) {
        draw_row(y, c, row_ms);
    }
    matrix_blank();
}

void sweep_cols_right(color_t c, uint32_t col_ms)
{
    for (uint32_t x = 0; x < MATRIX_WIDTH; x++) {
        matrix_clear();
        for (uint32_t y = 0; y < MATRIX_HEIGHT; y++) {
            matrix_set_xy(x, y, color_rgb(c));
        }
        matrix_show();
        vTaskDelay(pdMS_TO_TICKS(col_ms));
    }
    matrix_blank();
}

void fade_in_out(const pattern_t *p, uint32_t step_ms)
{
    uint8_t saved = matrix_get_brightness();

    for (int b = 0; b <= saved; b += 4) {
        matrix_set_brightness((uint8_t)b);
        visualise(p);
        vTaskDelay(pdMS_TO_TICKS(step_ms));
    }
    for (int b = saved; b >= 0; b -= 4) {
        matrix_set_brightness((uint8_t)b);
        visualise(p);
        vTaskDelay(pdMS_TO_TICKS(step_ms));
    }

    matrix_set_brightness(saved);
    matrix_blank();
}
