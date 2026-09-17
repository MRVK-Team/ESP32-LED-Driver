/* Line based protocol to the main robot controller.

   Incoming, either plain or CRC framed (see ROBOT_UART_REQUIRE_CRC):

       PING                        $PING*XXXX
       GET_STATE                   $GET_STATE*XXXX
       SET_STATE,FAULT             $SET_STATE,FAULT*XXXX
       SET_BRIGHTNESS,80           $SET_BRIGHTNESS,80*XXXX

   Outgoing:

       PONG
       STATE,<state>
       ACK,<command>
       NACK,<reason>               BAD_CRC, BAD_FORMAT, UNKNOWN_COMMAND,
                                   BAD_STATE, BAD_ARGUMENT, FRAME_TOO_LONG
       EVENT,<name>
       BOOT,READY

   A reply uses the same framing as the command it answers. Everything here
   is non-blocking: uart_protocol_update() reads whatever has arrived and
   returns, so animations never wait on it and it never waits on them. */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void uart_protocol_init(void);

/* Read and process whatever bytes have arrived. Call every loop iteration. */
void uart_protocol_update(void);

/* Send a message the controller did not ask for, framed according to
   ROBOT_UART_TX_FRAMED. payload is e.g. "EVENT,BEER_LOADED". */
void uart_protocol_send(const char *payload);

/* Milliseconds since the last valid frame was received, for a future
   communication watchdog. */
uint32_t uart_protocol_since_last_valid_ms(void);

#ifdef __cplusplus
}
#endif
