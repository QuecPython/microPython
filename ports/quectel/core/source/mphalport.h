// empty file
#ifndef __MP_HAL_PORT_H__
#define __MP_HAL_PORT_H__

// temporary comment by Chavis for compilation on 1/7/2021

#include "ringbuf.h"
#include "helios_uart.h"
#include "interrupt_char.h"

#define QPY_REPL_UART   HELIOS_UART3

extern ringbuf_t stdin_ringbuf;

int mp_hal_stdio_init(void);
uint32_t mp_hal_ticks_ms(void);
	
#endif //__MP_HAL_PORT_H__