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

//static Helios_Sem_t mp_hal_stdin_sem = 0;
#if !defined(PLAT_RDA)
#define QUECPYTHON_CALLBACK_MSG_MAX_NUM 50
static Helios_MsgQ_t quecpython_callback_deal_queue = 0;
#endif

//forrest.liu@20210809 add for quecpython task repl using waiting msg
#define MP_HAL_STDIN_MSG_MAX_NUM 50
static Helios_MsgQ_t mp_hal_stdin_msg = 0;
void mp_hal_stdin_send_msg_to_rx_chr(void);

static void mp_hal_stdin_cb(uint64_t ind_type, Helios_UARTNum port, uint64_t size)
{
    UNUSED(ind_type);
    UNUSED(size);
    UNUSED(port);
	
    //Helios_Semaphore_Release(mp_hal_stdin_sem);
    mp_hal_stdin_send_msg_to_rx_chr();//forrest.liu@20210809 add for quecpython task repl using waiting msg
}

int mp_hal_stdio_init(void)
{
    Helios_UARTConfig UARTConfig = {
        HELIOS_UART_BAUD,
        HELIOS_UART_DATABIT_8,
        HELIOS_UART_STOP_1,
        HELIOS_UART_PARITY_NONE,
        HELIOS_UART_FC_NONE
    };
    
    Helios_UARTInitStruct UARTInitStruct = {&UARTConfig, mp_hal_stdin_cb};
    int ret;
    //mp_hal_stdin_sem = Helios_Semaphore_Create(1, 0);
    //if(!mp_hal_stdin_sem) return -1;
    
    //forrest.liu@20210809 add for quecpython task repl using waiting msg
    mp_hal_stdin_msg = Helios_MsgQ_Create(MP_HAL_STDIN_MSG_MAX_NUM, sizeof(mp_uint_t));
	if(!mp_hal_stdin_msg) return -1;
	
    ret = (int)Helios_UART_Init(QPY_REPL_UART, &UARTInitStruct);
    if(ret)
    {
        //Helios_Semaphore_Delete(mp_hal_stdin_sem);
        Helios_MsgQ_Delete(mp_hal_stdin_msg);
        return -1;
    }

    return 0;
}

int mp_hal_stdin_rx_chr(void)
{
    while(1)
    {
    	unsigned char c = 0;
		mp_uint_t msg;
		
        int ret = (int)Helios_UART_Read(QPY_REPL_UART, (void *)&c, sizeof(unsigned char));
        if(ret > 0) return c;

        //Helios_Semaphore_Acquire(mp_hal_stdin_sem, 10);
        //forrest.liu@20210809 add for quecpython task repl using waiting msg
        MP_THREAD_GIL_EXIT();
		Helios_MsgQ_Get(mp_hal_stdin_msg, (mp_uint_t*)&msg, sizeof(msg), HELIOS_WAIT_FOREVER);
        MP_THREAD_GIL_ENTER();
        MICROPY_EVENT_POLL_HOOK
    }
}

void mp_hal_stdout_tx_strn(const char *str, size_t len)
{
	if (!str|| !len) return;
    
    Helios_UART_Write(QPY_REPL_UART, (void *)str, len);
}

uint32_t mp_hal_ticks_ms(void) {
	return (uint32_t)(Helios_RTC_GetTicks()/HAL_TICK1S);
}

uint32_t mp_hal_ticks_us(void) {
    return (uint32_t)(Helios_RTC_GetTicks()*1000/HAL_TICK1S);
}

void mp_hal_delay_ms(uint32_t ms) {
#if !defined(PLAT_RDA)
	mp_uint_t dt = 0;
	mp_uint_t t0 = 0,t1 = 0;
	Helios_Thread_t taskid = 0;
	extern Helios_Thread_t ql_micropython_task_ref;
	taskid = Helios_Thread_GetID();
	//Added by Freddy @20210521 仅主线程的sleep需要换为wait queue
	if(ql_micropython_task_ref == taskid)
	{
		for(;;)
		{
			t0 = mp_hal_ticks_us();
			MP_THREAD_GIL_EXIT();
			mp_uint_t wait_time = quecpython_sleep_deal_fun(ms);
			MP_THREAD_GIL_ENTER();
			if(wait_time >= ms)
			{
				return;
			}
			MICROPY_EVENT_POLL_HOOK;
			t1 = mp_hal_ticks_us();
			dt = t1 - t0;
			if(dt / 1000 >= ms)
			{
				return;
			}
			ms -= dt / 1000;
		}
	}
	else
	{
		MP_THREAD_GIL_EXIT();
		Helios_msleep(ms);
		MP_THREAD_GIL_ENTER();
	}
#else
	MP_THREAD_GIL_EXIT();
	Helios_msleep(ms);
	MP_THREAD_GIL_ENTER();
#endif
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

#if !defined(PLAT_RDA)
void quecpython_callback_deal_queue_create(void)
{
	quecpython_callback_deal_queue = Helios_MsgQ_Create(QUECPYTHON_CALLBACK_MSG_MAX_NUM, sizeof(mp_uint_t));
}

//Added by Freddy @20210520 发送消息至sleep的queue
void quecpython_send_msg_to_sleep_func(void)
{
	mp_uint_t msg = 0;
	if(0 != quecpython_callback_deal_queue)
	{
		Helios_MsgQ_Put(quecpython_callback_deal_queue, (const void*)(&msg), sizeof(mp_uint_t), 0);
	}
}

//Added by Freddy @20210520 sleep接口循环调用此接口达到sleep的目的，
//当有callback要执行时，给此queue发消息，即可立即退�?
mp_uint_t quecpython_sleep_deal_fun(mp_uint_t ms)
{
	if(quecpython_callback_deal_queue)
	{
		mp_uint_t dt;
		mp_uint_t t0 = mp_hal_ticks_us();
		mp_uint_t msg;
		int ret = Helios_MsgQ_Get(quecpython_callback_deal_queue, (mp_uint_t*)&msg, sizeof(msg), ms);
		if(ret < 0)
		{
			return ms;
		}
		else
		{
			mp_uint_t t1 = mp_hal_ticks_us();
			dt = t1 - t0;
			return dt / 1000;
		}
	}
	else
	{
		Helios_msleep(ms);
		return ms;
	}
}
#endif

//forrest.liu@20210809 add for quecpython task repl using waiting msg
void mp_hal_stdin_send_msg_to_rx_chr(void)
{
	mp_uint_t msg = 0;
	if(mp_hal_stdin_msg)
	{
		Helios_MsgQ_Put(mp_hal_stdin_msg, (void*)(&msg), sizeof(mp_uint_t), HELIOS_NO_WAIT);
	}
}

