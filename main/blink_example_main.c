/* 8x8 WS2812B matrix demo for an ESP32-C3 mini, data-in on GPIO1.

   The matrix driver lives in matrix.c, the pictures and animations in
   patterns.c. Here you only pick what to show.

   This code is in the Public Domain (or CC0 licensed, at your option.)
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "matrix.h"
#include "patterns.h"

void app_main(void)
{
    matrix_init();
    matrix_set_brightness(48);   /* 0..255, keep it low on USB power */

    while (1) {
        /* The original effect: an orange line running top to bottom. */
        // ESP_LOGI(TAG, "%s", SEQ_ROW_SWEEP.name);
        // play(&SEQ_ROW_SWEEP, 3);

        // /* Same thing without frame tables, any colour you like. */
        // sweep_rows_down(CLR_ORANGE, 100);
        // sweep_cols_right(CLR_CYAN, 100);

        // /* A single picture, held for a while. */
        // visualise_for(&PAT_HEART, 1000);

        // /* A blinking picture. */
        // blink(&PAT_CROSS, 200, 200, 4);

        // /* Any pattern re-coloured on the fly. */
        // visualise_mono(&PAT_X, CLR_PURPLE);
        // vTaskDelay(pdMS_TO_TICKS(800));

        // /* Animations. */
        // play(&SEQ_CHECKER, 3);
        // play(&SEQ_FACES, 2);
        // play(&SEQ_ARROWS, 2);
        // play(&SEQ_SHAPES, 1);
        visualise_for(&PAT_BEER, 5000);
        blink(&PAT_HALF_TURQUOISE,  500, 200, 20);

        // blink(&PAT_BEER, 200, 200, 4);
        // matrix_blank();
        // vTaskDelay(pdMS_TO_TICKS(500));
    }
}
