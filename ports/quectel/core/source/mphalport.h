/*
 * Copyright (c) Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  
 *     http://www.apache.org/licenses/LICENSE-2.0
 *  
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// empty file
#ifndef __MP_HAL_PORT_H__
#define __MP_HAL_PORT_H__

// temporary comment by Chavis for compilation on 1/7/2021

#include "ringbuf.h"
#include "helios_uart.h"
#include "interrupt_char.h"

#if defined(PLAT_RDA)
#define QPY_REPL_UART   HELIOS_UART0
#define		HAL_TICK1S	16.384
/*Due to low power consumption of BC25 series, the baud rate needs to be set to 57600, please do not modify.*/
#define HELIOS_UART_BAUD   HELIOS_UART_BAUD_57600 
#elif defined(PLAT_Qualcomm)
#define		HAL_TICK1S	32.768
#define QPY_REPL_UART   HELIOS_UART0
#define HELIOS_UART_BAUD   HELIOS_UART_BAUD_115200
#else
#define		HAL_TICK1S	32.768
#define QPY_REPL_UART   HELIOS_UART3
#define HELIOS_UART_BAUD   HELIOS_UART_BAUD_115200
#endif

extern ringbuf_t stdin_ringbuf;

int mp_hal_stdio_init(void);
uint32_t mp_hal_ticks_ms(void);
#if !defined(PLAT_RDA) && !defined(PLAT_Qualcomm)
void quecpython_send_msg_to_sleep_func(void);
mp_uint_t quecpython_sleep_deal_fun(mp_uint_t ms);
#endif
void mp_hal_port_open(uint8_t state);

#endif //__MP_HAL_PORT_H__
