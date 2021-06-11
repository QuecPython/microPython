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

/**
 ******************************************************************************
 * @file    mphalport.c
 * @author  xxx
 * @version V1.0.0
 * @date    2019/04/16
 * @brief   xxx
 ******************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include "stream.h"
#include "obj.h"
#include "mphal.h"
#include "builtin.h"
#include "parse.h"
#include "mphalport.h"
#include "mpstate.h"
#include "helios.h"
#include "helios_os.h"
#include "helios_rtc.h"

STATIC uint8_t stdin_ringbuf_array[256];
ringbuf_t stdin_ringbuf = {stdin_ringbuf_array, sizeof(stdin_ringbuf_array), 0, 0};

static Helios_Sem_t mp_hal_stdin_sem = 0;

static void mp_hal_stdin_cb(uint64_t ind_type, Helios_UARTNum port, uint64_t size)
{
    UNUSED(ind_type);
    UNUSED(size);
    
    if(port == QPY_REPL_UART)
    {
        Helios_Semaphore_Release(mp_hal_stdin_sem);
    }
}

int mp_hal_stdio_init(void)
{
    Helios_UARTConfig UARTConfig = {
        HELIOS_UART_BAUD_115200,
        HELIOS_UART_DATABIT_8,
        HELIOS_UART_STOP_1,
        HELIOS_UART_PARITY_NONE,
        HELIOS_UART_FC_NONE
    };
    
    Helios_UARTInitStruct UARTInitStruct = {&UARTConfig, mp_hal_stdin_cb};
    int ret;
    mp_hal_stdin_sem = Helios_Semaphore_Create(1, 0);
    if(!mp_hal_stdin_sem) return -1;
    
    ret = (int)Helios_UART_Init(QPY_REPL_UART, &UARTInitStruct);
    if(ret)
    {
        Helios_Semaphore_Delete(mp_hal_stdin_sem);
        return -1;
    }

    return 0;
}

int mp_hal_stdin_rx_chr(void)
{
    while(1)
    {
    	unsigned char c = 0;
        int ret = (int)Helios_UART_Read(QPY_REPL_UART, (void *)&c, sizeof(unsigned char));
        if(ret > 0) return c;

        Helios_Semaphore_Acquire(mp_hal_stdin_sem, 10);

        MICROPY_EVENT_POLL_HOOK
    }
}

void mp_hal_stdout_tx_strn(const char *str, size_t len)
{
	if (!str|| !len) return;
    
    Helios_UART_Write(QPY_REPL_UART, (void *)str, len);
}

uint32_t mp_hal_ticks_ms(void) {
    return (uint32_t)(Helios_RTC_GetTicks()/32.768);
}

uint32_t mp_hal_ticks_us(void) {
    return (uint32_t)(Helios_RTC_GetTicks()*1000/32.768);
}

void mp_hal_delay_ms(uint32_t ms) {
	MP_THREAD_GIL_EXIT();
	Helios_msleep(ms);
	MP_THREAD_GIL_ENTER();
}

void mp_hal_delay_us(uint32_t us) {
	uint32_t ms = us / 1000;
    ms = ms ? ms : 1;
	mp_hal_delay_ms(ms);
}

mp_uint_t mp_hal_ticks_cpu(void) {
    return (mp_uint_t)Helios_RTC_GetTicks();
}


uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;
    if ((poll_flags & MP_STREAM_POLL_RD) && stdin_ringbuf.iget != stdin_ringbuf.iput) {
        ret |= MP_STREAM_POLL_RD;
    }
    return ret;
}

