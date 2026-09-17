/* Hardware and timing configuration for the robot state controller.

   Everything board-specific lives here, except the LED matrix pins and
   wiring, which stay in matrix.h next to the driver that uses them.

   These values must be checked against the real PCB. */
#pragma once

/* --- buttons (button -> GND, internal pull-up, pressed reads LOW) ------- */
#define BTN_BEER_GPIO           2       /* button 1: beer loaded */
#define BTN_MISSION_GPIO        3       /* button 2: mission start */
#define BTN_AUX_GPIO            4       /* button 3: spare */
#define BUTTON_DEBOUNCE_MS      40

/* --- link to the main robot controller --------------------------------- */

/* Where the protocol talks.
     1 = the USB cable, i.e. the console port. Commands can be typed into
         idf.py monitor and replies appear there. Best for bench testing;
         log lines share the same stream.
     0 = UART1 on the pins below, a clean stream for the real robot
         controller. */
#define ROBOT_LINK_USE_USB      1

/* Used when ROBOT_LINK_USE_USB is 0. 115200 8N1. */
#define ROBOT_UART_PORT         1       /* UART1; UART0 is the console */
#define ROBOT_UART_RX_PIN       20
#define ROBOT_UART_TX_PIN       21
#define ROBOT_UART_BAUD         115200
#define ROBOT_UART_MAX_FRAME    96      /* longest accepted line */

/* Protocol version. 0 = accept plain lines ("PING") as well as CRC framed
   ones; 1 = require "$PAYLOAD*CRC16" and reject anything else.
   Replies always use the same framing as the command they answer. */
#define ROBOT_UART_REQUIRE_CRC  0

/* Framing for messages the ESP32 sends on its own (BOOT,READY and events),
   where there is no incoming frame to mirror. */
#define ROBOT_UART_TX_FRAMED    0

/* --- display ----------------------------------------------------------- */
#define LED_BRIGHTNESS_DEFAULT  80

#define LOAD_BEER_ON_MS         900     /* full beer icon held */
#define LOAD_BEER_OFF_MS        200     /* gap between the parts */
#define LOAD_BEER_FILL_MS       220     /* one step of the glass filling */
#define LOAD_BEER_ARROW_MS      450     /* "put it in here" arrow */
#define LOAD_BEER_TEXT_MS       320     /* one letter of L-O-A-D */
#define AUTONOMOUS_FRAME_MS     180     /* moving arrows */
#define FAULT_BLINK_MS          500     /* red X on, then off */
#define MISSION_FRAME_MS        500     /* check / smile / beer */
#define SCAN_QR_HOLD_MS         1600    /* QR badge held */
#define SCAN_QR_TEXT_MS         320     /* one letter of S-C-A-N */

/* --- main loop --------------------------------------------------------- */
#define MAIN_LOOP_MS            5
