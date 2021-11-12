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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "audio_queue.h"
#include "helios_audio.h"
#include "helios_debug.h"
#include "objstr.h"

#define MAX_TTS_TEXT_LEN (500)

enum {
    TTS_MOD_UCS2_BE=1,
    TTS_MOD_GBK,
    TTS_MOD_UCS2_LE,
    TTS_MOD_UTF8,
};

const mp_obj_type_t audio_tts_type;

typedef struct _audio_tts_obj_t {
    mp_obj_base_t base;
	int inited;
	int tts_speed;
} audio_tts_obj_t;

static c_callback_t *g_tts_callback;
static audio_tts_obj_t *tts_obj = NULL;

#define HELIOS_TTS_LOG(fmt, ...) custom_log(audio_TTS, fmt, ##__VA_ARGS__)

/*=============================================================================*/
/* FUNCTION: helios_set_tts_callback                                               */
/*=============================================================================*/
/*!@brief 		: user callback function for tts playing.
 * @param[in] 	: callback - user's callback
 * @param[out] 	: 
 * @return		:
 */
/*=============================================================================*/
STATIC mp_obj_t helios_set_tts_callback(mp_obj_t self_in, mp_obj_t callback)
{
    static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
	g_tts_callback = &cb;
	mp_sched_schedule_callback_register(g_tts_callback, callback);
	
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(helios_set_tts_callback_obj, helios_set_tts_callback);


static void user_tts_callback(Helios_TTSENvent event)
{
	//mp_obj_t tuple[2] = {mp_obj_new_int(event), mp_obj_new_str(str, strlen(str))};
	
	if (g_tts_callback)
	{
		//uart_printf("[TTS] callback start.\r\n");
		mp_sched_schedule_ex(g_tts_callback, mp_obj_new_int(event));
		//uart_printf("[TTS] callback end.\r\n");
	}
}

/*=============================================================================*/
/* FUNCTION: app_helios_tts_callback                                                   */
/*=============================================================================*/
/*!@brief 		: callback function for tts playing.
 * @param[in] 	: event - TTS state; str - Prompt information
 * @param[out] 	: 
 * @return		:
 */
/*=============================================================================*/

void app_helios_tts_callback(Helios_TTSENvent event)
{
	HELIOS_TTS_LOG("app_helios_tts_callback:event = %d\n", event);
	user_tts_callback(event);
	
	switch (event)
	{
		case HELIOS_TTS_EVT_PLAY_START:
			Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
			audio.audio_state = AUDIO_PLAYING;
			Helios_Mutex_Unlock(audio.queue_mutex);
			break;
		case HELIOS_TTS_EVT_PLAY_FINISH:
			Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
			audio.audio_state = AUDIO_IDLE;
			Helios_Mutex_Unlock(audio.queue_mutex);
			uint8_t audio_msg = AUDIO_FINISH_EVENT;
			if (Helios_MsgQ_Put(audio.queue_msg, (const void *)&audio_msg, sizeof(uint8_t), HELIOS_NO_WAIT) == -1)
			{
				HELIOS_TTS_LOG("send tts msg failed.\r\n");
				return;
			}
			HELIOS_TTS_LOG("send tts finished signal successed.\r\n");
			break;
		case HELIOS_TTS_EVT_DEINIT:
		case HELIOS_TTS_EVT_PLAY_STOP:
		case HELIOS_TTS_EVT_PLAY_FAILED:
			Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
			audio.audio_state = AUDIO_IDLE;
			Helios_Mutex_Unlock(audio.queue_mutex);
			break;
		default:
			break;
	}
}

STATIC void audio_tts_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    //audio_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "tts ");
}

STATIC mp_obj_t audio_tts_make_new(const mp_obj_type_t *type,
    size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    int device = mp_obj_get_int(args[0]);
	
	if (device < 0 || device > 2) {
		mp_raise_ValueError("invalid device index, the value of device should be in[0,2]");
	}
	
	Helios_Audio_SetAudioChannle(device);

    if(NULL == tts_obj)
    {
        tts_obj = m_new_obj_with_finaliser(audio_tts_obj_t);
    }
    
    audio_tts_obj_t *self = tts_obj;
    self->base.type = &audio_tts_type;
	self->inited = 0;
	self->tts_speed = 4;//5 changed by freddy@20211008

	if (audio.inited == 0)
	{
		uint8_t i = 0;
		for (i=0; i<QUEUE_NUMS; i++)
		{
			audio_queue_init(&audio.audio_queue[i]);
		}
		audio.cur_priority = 0;
		audio.cur_breakin  = 0;
		audio.audio_state = AUDIO_IDLE;
		audio.inited = 1;
		audio.total_nums = 0;
		audio.queue_mutex  = 0;

		audio.queue_mutex = Helios_Mutex_Create();
		audio.queue_msg = Helios_MsgQ_Create(10, sizeof(uint8_t));
		if (!audio.queue_msg)
		{
			Helios_Mutex_Delete(audio.queue_mutex);
			mp_raise_ValueError("create audio object failed, can not create msgQ.");
		}
		
		Helios_ThreadAttr attr = {0};
		attr.name = "audio_queue_play";
		attr.stack_size = 4096;
		attr.priority = 95;
		attr.entry = helios_audio_queue_play_task;
		
		Helios_Thread_t id = Helios_Thread_Create(&attr);
		if (id == 0)
		{
			Helios_MsgQ_Delete(audio.queue_msg);
			Helios_Mutex_Delete(audio.queue_mutex);
			mp_raise_ValueError("create audio object failed, can not create queue play task.");
		}
	}
	
	int ret = Helios_TTS_Init(app_helios_tts_callback);
	if (ret == 0)
	{
		self->inited = 1;
	}
	else
	{
		mp_raise_ValueError("TTS Init failed.");
	}
	Helios_TTS_SetSpeed(self->tts_speed);

    return MP_OBJ_FROM_PTR(self);
}

/*=============================================================================*/
/* FUNCTION: audio_tts_play                                              */
/*=============================================================================*/
/*!@brief 		: tts play.
 * @param[in] 	: priority - Playback priority,0~4
 * @param[in] 	: breakin - 0 means it can't be interrupted,1 can be interrupted
 * @param[in] 	: mode - 
 * @param[in] 	: data - TTS data
 * @param[out] 	: 
 * @return		: -2 - queue if full; -1 - play failed; 0 - play successful; 1 - add to queue successful
 * @modify      : 
 				----------  ---------   ---------------------------------------------------
				2020/10/17  Jayceon.Fu  solve the problem that Add three interruptible tasks of the same priority in a row,
				                        the last one is not played.
				2020/10/17  Jayceon.Fu	Solve the problem of not playing when TTS and Audio interrupt each other .
				2020/09/27  Jayceon.Fu	Added play queue.
 */
/*=============================================================================*/
STATIC mp_obj_t audio_tts_play(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) 
{
	enum { ARG_priority, ARG_breakin, ARG_mode, ARG_data };
    static const mp_arg_t allowed_args[] = 
    {
    	{ MP_QSTR_priority, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    	{ MP_QSTR_breakin,  MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_mode,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_data,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };

	//audio_tts_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);

    int new_mode = vals[ARG_mode].u_int & 0x0000000F;
    int wtts_mod_mask = vals[ARG_mode].u_int & 0x000000F0;

	if ((new_mode < 1) || (new_mode > 3))
	{
		HELIOS_TTS_LOG("new_mode is %d\n",new_mode);
		mp_raise_ValueError("invalid mode");
		return mp_obj_new_int(-1);
	}

    if(new_mode == 2)
    {
        new_mode = TTS_MOD_UTF8;//UTF8 mode
    }

    new_mode |= (uint8_t)wtts_mod_mask;

	mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(vals[ARG_data].u_obj, &bufinfo, MP_BUFFER_READ);

	int new_priority = vals[ARG_priority].u_int;
	int new_breakin  = vals[ARG_breakin].u_int;
	if (new_priority < 0 || new_priority > 4) {
		mp_raise_ValueError("invalid priority value, must be in [0,4]");
	}
	if (new_breakin != 0 && new_breakin != 1) {
		mp_raise_ValueError("invalid breakin value, must be in [0,1]");
	}
	
	if(bufinfo.len > MAX_TTS_TEXT_LEN) {
		mp_raise_msg_varg(&mp_type_ValueError, "The string length cannot be greater than %d", MAX_TTS_TEXT_LEN);
	}
    
	Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
	if (audio.audio_state == AUDIO_IDLE)
	{
		audio.cur_priority = new_priority;
		audio.cur_breakin  = new_breakin;
		audio.cur_type     = AUDIO_TTS;
		audio.audio_state = AUDIO_PLAYING;
		Helios_Mutex_Unlock(audio.queue_mutex);
	}
	else if (audio.audio_state == AUDIO_PLAYING)
	{
		if (new_priority >= audio.cur_priority)
	    {
			if (audio.cur_breakin == 1)
			{
				audio.audio_state = AUDIO_PLAYING;

				if (!audio_queue_is_full(&audio.audio_queue[new_priority]))
				{
					//Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
					uint8_t rear = audio.audio_queue[new_priority].rear;
					uint8_t front = audio.audio_queue[new_priority].front;
					
					if (audio.audio_queue[new_priority].audio_data[rear].breakin == 0)
					{
						rear = (rear + 1) % QUEUE_SIZE;						
					}
					else if (audio.audio_queue[new_priority].audio_data[rear].breakin == 1)
					{
						if (front == 0)
						{
							front = QUEUE_SIZE - 1;
						}
						else
						{
							front = (front - 1) % QUEUE_SIZE;
						}
					}
					
					audio.audio_queue[new_priority].audio_data[rear].audio_type = AUDIO_TTS;
					audio.audio_queue[new_priority].audio_data[rear].priority = new_priority;
					audio.audio_queue[new_priority].audio_data[rear].breakin  = new_breakin;
					audio.audio_queue[new_priority].audio_data[rear].mode     = new_mode;
					memset(audio.audio_queue[new_priority].audio_data[rear].data, 0, 500);
					strncpy(audio.audio_queue[new_priority].audio_data[rear].data, bufinfo.buf, bufinfo.len);
					audio.audio_queue[new_priority].rear = rear;
					audio.audio_queue[new_priority].front = front;
					audio.total_nums++;
					Helios_Mutex_Unlock(audio.queue_mutex);
				}
				else
				{
					Helios_Mutex_Unlock(audio.queue_mutex);
					return mp_obj_new_int(-2);
				}

				uint8_t audio_msg = AUDIO_STOP_EVENT;
				if (Helios_MsgQ_Put(audio.queue_msg, (const void *)&audio_msg, sizeof(uint8_t), HELIOS_NO_WAIT) == -1)
				{
					HELIOS_TTS_LOG("send audio msg failed[%u].\r\n", audio_msg);
					return mp_obj_new_int(-1);
				}
				HELIOS_TTS_LOG("send audio stop signal successed[%u].\r\n", audio_msg);
				return mp_obj_new_int(0);//forrest.liu@20210427 reopen for TTS and Audio_file playing break function correctly
				
			}
			else if (audio.cur_breakin == 0)
			{
				if (!audio_queue_is_full(&audio.audio_queue[new_priority]))
				{
					//Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
					uint8_t rear = audio.audio_queue[new_priority].rear;
					if (audio.audio_queue[new_priority].audio_data[rear].breakin == 0)
					{
						rear = (rear + 1) % QUEUE_SIZE;
					}
					
					audio.audio_queue[new_priority].audio_data[rear].audio_type = AUDIO_TTS;
					audio.audio_queue[new_priority].audio_data[rear].priority = new_priority;
					audio.audio_queue[new_priority].audio_data[rear].breakin  = new_breakin;
					audio.audio_queue[new_priority].audio_data[rear].mode     = new_mode;
					memset(audio.audio_queue[new_priority].audio_data[rear].data, 0, 500);
					strncpy(audio.audio_queue[new_priority].audio_data[rear].data, bufinfo.buf, bufinfo.len);
					audio.audio_queue[new_priority].rear = rear;
					audio.total_nums++;
					Helios_Mutex_Unlock(audio.queue_mutex);
				}
				else
				{
					Helios_Mutex_Unlock(audio.queue_mutex);
					return mp_obj_new_int(-2);
				}
				return mp_obj_new_int(1);
			}
	    }
	    else
	    {
			if (!audio_queue_is_full(&audio.audio_queue[new_priority]))
			{
				//Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
				uint8_t rear = audio.audio_queue[new_priority].rear;
				uint8_t front = audio.audio_queue[new_priority].front;
				
				if (audio.audio_queue[new_priority].audio_data[rear].breakin == 0)
				{
					rear = (rear + 1) % QUEUE_SIZE;
				}
				else if (audio.audio_queue[new_priority].audio_data[rear].breakin == 1)
				{
					if (front == 0)
					{
						front = QUEUE_SIZE - 1;
					}
					else
					{
						front = (front - 1) % QUEUE_SIZE;
					}
				}
			
				audio.audio_queue[new_priority].audio_data[rear].audio_type = AUDIO_TTS;
				audio.audio_queue[new_priority].audio_data[rear].priority = new_priority;
				audio.audio_queue[new_priority].audio_data[rear].breakin  = new_breakin;
				audio.audio_queue[new_priority].audio_data[rear].mode     = new_mode;
				memset(audio.audio_queue[new_priority].audio_data[rear].data, 0, 500);
				strncpy(audio.audio_queue[new_priority].audio_data[rear].data, bufinfo.buf, bufinfo.len);
				audio.audio_queue[new_priority].rear = rear;
				audio.audio_queue[new_priority].front = front;
				audio.total_nums++;
				Helios_Mutex_Unlock(audio.queue_mutex);
			}
			else
			{
				Helios_Mutex_Unlock(audio.queue_mutex);
				return mp_obj_new_int(-2);
			}
			return mp_obj_new_int(1);
	    }
	}
	
	int ret = Helios_TTS_Start(new_mode, bufinfo.buf, bufinfo.len);
	if (ret == -1)
	{
		Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
		audio.audio_state = AUDIO_IDLE;
		Helios_Mutex_Unlock(audio.queue_mutex);
	}
	return mp_obj_new_int(ret);
}
MP_DEFINE_CONST_FUN_OBJ_KW(audio_tts_play_obj, 1, audio_tts_play);

STATIC mp_obj_t audio_tts_init(mp_obj_t self_in) {
    audio_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int ret = Helios_TTS_Init(NULL);
	if (ret == -1)
	{
		return mp_obj_new_int(-1);
	}
	self->inited = 1;

    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(audio_tts_init_obj, audio_tts_init);

STATIC mp_obj_t audio_tts_deinit(mp_obj_t self_in) {
    audio_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);

	int ret = Helios_TTS_Deinit();
	if (ret == -1)
	{
		return mp_obj_new_int(-1);
	}
	self->inited = 0;

    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(audio_tts_deinit_obj, audio_tts_deinit);

STATIC mp_obj_t aduio_tts_stop(mp_obj_t self_in)
{
	//audio_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);
	Helios_TTS_Stop();
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(aduio_tts_stop_obj, aduio_tts_stop);

STATIC mp_obj_t aduio_tts_stop_all(mp_obj_t self_in)
{
	uint8_t i = 0;
	Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
	for (i=0; i<QUEUE_NUMS; i++)
	{
		audio_queue_init(&audio.audio_queue[i]);
	}
	audio.cur_priority = 0;
	audio.cur_breakin  = 0;
	audio.total_nums = 0;
	//audio.audio_state = AUDIO_IDLE;
	Helios_Mutex_Unlock(audio.queue_mutex);

	Helios_TTS_Stop();

    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(aduio_tts_stop_all_obj, aduio_tts_stop_all);
/*=============================================================================*/
/* FUNCTION: aduio_tts_set_volume                                              */
/*=============================================================================*/
/*!@brief 		: set tts volume.
 * @param[in] 	: vol, [0~9]
 * @param[out] 	: 
 * @return		:
 *        -  0--success
 *        - -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t aduio_tts_set_volume(mp_obj_t self_in, mp_obj_t volume)
{
	int index = mp_obj_get_int(volume);
	if ((index > 9) || (index < 0))
	{
		mp_raise_ValueError("invalid value,TTS volume should be array between [0,9]");
		return mp_obj_new_int(-1);
	}
	
	//int vol_map[10] = {-32768, -25487, -18206, -10925, -3644, 3637, 10918, 18199, 25480, 32761};
	int ret = Helios_TTS_SetVolume(index);			//first para only works in unisoc
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(aduio_tts_set_volume_obj, aduio_tts_set_volume);


/*=============================================================================*/
/* FUNCTION: aduio_tts_get_volume                                              */
/*=============================================================================*/
/*!@brief 		: get tts volume.
 * @param[in] 	: vol, [0~9]
 * @param[out] 	: 
 * @return		:
 *        -  if get volume successful,return the volume.
 *        - -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t aduio_tts_get_volume(mp_obj_t self_in)
{
	int vol = Helios_TTS_GetVolume();
    return mp_obj_new_int(vol);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(aduio_tts_get_volume_obj, aduio_tts_get_volume);


/*=============================================================================*/
/* FUNCTION: aduio_tts_set_speed                                               */
/*=============================================================================*/
/*!@brief 		: set tts play speed.
 * @param[in] 	: speed, [0~9]
 * @param[out] 	: 
 * @return		:
 *        -  0--success
 *        - -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t aduio_tts_set_speed(mp_obj_t self_in, mp_obj_t speed)
{
	audio_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int index = mp_obj_get_int(speed);
	if ((index > 9) || (index < 0))
	{
		return mp_obj_new_int(-1);
	}
	self->tts_speed = index;

	int ret = Helios_TTS_SetSpeed(self->tts_speed);
	if(ret)
	{
		HELIOS_TTS_LOG("Helios_TTS_SetSpeed failed\n");
	}
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(aduio_tts_set_speed_obj, aduio_tts_set_speed);


/*=============================================================================*/
/* FUNCTION: aduio_tts_get_speed                                               */
/*=============================================================================*/
/*!@brief 		: get tts play speed.
 * @param[in] 	: speed, [0~9]
 * @param[out] 	: 
 * @return		:
 *        -  if get speed successful,return the speed.
 *        - -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t aduio_tts_get_speed(mp_obj_t self_in)
{
	audio_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);

    return mp_obj_new_int(Helios_TTS_GetSpeed());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(aduio_tts_get_speed_obj, aduio_tts_get_speed);


STATIC mp_obj_t aduio_tts_get_state(mp_obj_t self_in)
{
	int state = Helios_TTS_GetStatus();
    return mp_obj_new_int(state);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(aduio_tts_get_state_obj, aduio_tts_get_state);

STATIC mp_obj_t aduio_tts___del__(mp_obj_t self_in)
{
#ifdef PLAT_ASR
    audio_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);

    uint8_t i = 0;
	Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
	for (i=0; i<QUEUE_NUMS; i++)
	{
		audio_queue_init(&audio.audio_queue[i]);
	}
	audio.cur_priority = 0;
	audio.cur_breakin  = 0;
	audio.total_nums = 0;
	//audio.audio_state = AUDIO_IDLE;
	Helios_Mutex_Unlock(audio.queue_mutex);

	Helios_TTS_Stop();
	
	int ret = Helios_TTS_Deinit();
	if (ret == -1)
	{
		return mp_obj_new_int(-1);
	}
	self->inited = 0;
#endif
    tts_obj = NULL;
    g_tts_callback = NULL;
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(aduio_tts___del___obj, aduio_tts___del__);

STATIC const mp_rom_map_elem_t audio_tts_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&aduio_tts___del___obj) },
	{ MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&audio_tts_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&audio_tts_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&audio_tts_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&aduio_tts_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_stopAll), MP_ROM_PTR(&aduio_tts_stop_all_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&audio_tts_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_setVolume), MP_ROM_PTR(&aduio_tts_set_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_getVolume), MP_ROM_PTR(&aduio_tts_get_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_setSpeed), MP_ROM_PTR(&aduio_tts_set_speed_obj) },
    { MP_ROM_QSTR(MP_QSTR_getSpeed), MP_ROM_PTR(&aduio_tts_get_speed_obj) },
    { MP_ROM_QSTR(MP_QSTR_getState), MP_ROM_PTR(&aduio_tts_get_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_setCallback), MP_ROM_PTR(&helios_set_tts_callback_obj) },
    { MP_ROM_QSTR(MP_QSTR_text_utf16be), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_text_utf8), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_text_utf16le), MP_ROM_INT(3) },
#if defined(CONFIG_VIOCE_CALL)
    { MP_ROM_QSTR(MP_QSTR_wtts_enable), MP_ROM_INT(0x80) },
    { MP_ROM_QSTR(MP_QSTR_wtts_ul_enable), MP_ROM_INT(0x40) },
    { MP_ROM_QSTR(MP_QSTR_wtts_dl_enable), MP_ROM_INT(0x20) },
#endif
};

STATIC MP_DEFINE_CONST_DICT(audio_tts_locals_dict, audio_tts_locals_dict_table);

const mp_obj_type_t audio_tts_type = {
    { &mp_type_type },
    .name = MP_QSTR_TTS,
    .print = audio_tts_print,
    .make_new = audio_tts_make_new,
    .locals_dict = (mp_obj_dict_t *)&audio_tts_locals_dict,
};