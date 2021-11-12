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
#include "runtime.h"
#include "audio_queue.h"
#include "helios_audio.h"
#include "helios_debug.h"
#include "helios_gpio.h"

typedef struct _audio_obj_t {
    mp_obj_base_t base;
	int device;
	//int  inited;
} audio_obj_t;

#define HELIOS_AUDIO_LOG(fmt, ...) custom_log(audio_audio, fmt, ##__VA_ARGS__)

extern const mp_obj_type_t audio_audio_type;

static c_callback_t *g_audio_callback;
static c_callback_t *g_audio_speakerpa_callback;
static int g_gpio_pa = -1;

STATIC void delay_us(unsigned int us)
{
	int cnt = 26;
	volatile int i = 0;
	while(us--){
		i = cnt;
		while(i--);
	}
}

STATIC void set_clk_high(void)
{
	Helios_GPIO_SetLevel(g_gpio_pa, HELIOS_LVL_HIGH);
}

STATIC void set_clk_low(void)
{
	Helios_GPIO_SetLevel(g_gpio_pa, HELIOS_LVL_LOW);
}

STATIC void audio_open_power_amplifier(void)
{
	// 拉高 -> 低 -> 高
	set_clk_high();
	delay_us(3);
	set_clk_low();
	delay_us(3);
	set_clk_high();
}

static void user_audio_speakerpa_callback(unsigned int event)
{
	if(g_gpio_pa != -1)
	{
		if (event == 1)
		{
			//打开PA
			audio_open_power_amplifier();
			HELIOS_AUDIO_LOG("audio set high, event 1.\r\n");
	    } else {
	       //关闭PA
	 	    set_clk_low();
			HELIOS_AUDIO_LOG("audio set low, event 0.\r\n");
		}
	}
	if (g_audio_speakerpa_callback)
	{
		mp_sched_schedule_ex(g_audio_speakerpa_callback, mp_obj_new_int(event));
	}
}

#if defined (PLAT_ASR) || defined(PLAT_Unisoc)
STATIC mp_obj_t helios_set_pa(mp_obj_t self_in, mp_obj_t gpio)
{
	int ret = 0;
	int gpio_num = mp_obj_get_int(gpio);
	g_gpio_pa = gpio_num;

	ret = Helios_Audio_SetPa(gpio_num);
	if (ret != 0)
	{
		return mp_obj_new_int(0);
	}
	return mp_obj_new_int(1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(helios_set_pa_obj, helios_set_pa);


STATIC mp_obj_t helios_set_speakerpa_callback(mp_obj_t self_in, mp_obj_t callback)
{
    static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
	g_audio_speakerpa_callback = &cb;
	mp_sched_schedule_callback_register(g_audio_speakerpa_callback, callback);
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(helios_set_speakerpa_callback_obj, helios_set_speakerpa_callback);
#endif

/*=============================================================================*/
/* FUNCTION: helios_set_audio_callback                                             */
/*=============================================================================*/
/*!@brief 		: user callback function for audio playing.
 * @param[in] 	: callback - user's callback
 * @param[out] 	: 
 * @return		:
 */
/*=============================================================================*/
STATIC mp_obj_t helios_set_audio_callback(mp_obj_t self_in, mp_obj_t callback)
{
    static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
	g_audio_callback = &cb;
	mp_sched_schedule_callback_register(g_audio_callback, callback);

	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(helios_set_audio_callback_obj, helios_set_audio_callback);

/*=============================================================================*/
/* FUNCTION: audio_play_callback                                               */
/*=============================================================================*/
/*!@brief 		: callback function for audio file playing.
 * @param[in] 	: state - audio state
 * @param[out] 	: 
 * @return		:
 */
/*=============================================================================*/
int audio_play_callback(char *ptr, size_t lens, Helios_EnumAudPlayerState state)
{
	int audio_event = state;
	Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
	uint8_t stop_flag = audio.user_call_stop;
	uint8_t audio_state = audio.audio_state;
	Helios_Mutex_Unlock(audio.queue_mutex);
	HELIOS_AUDIO_LOG("[Audio] callback, event = %d, stop_flag = %d\r\n", state, stop_flag);
	
	if (state == HELIOS_AUD_PLAYER_FINISHED)
	{
		audio_event = 7;
	}
	else if ((state == HELIOS_AUD_PLAYER_CLOSE) && (stop_flag == 1))
	{
		audio_event = 7;
		state = HELIOS_AUD_PLAYER_FINISHED;
		Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
		audio.user_call_stop = 0;
		Helios_Mutex_Unlock(audio.queue_mutex);
	}
	
	if ((g_audio_callback != NULL) && (audio_state != AUDIO_IDLE))
	{
		HELIOS_AUDIO_LOG("[Audio] callback start.\r\n");
		mp_sched_schedule_ex(g_audio_callback, mp_obj_new_int(audio_event));
		HELIOS_AUDIO_LOG("[Audio] callback end.\r\n");
	}

	switch (state)
	{
		case HELIOS_AUD_PLAYER_START:
		case HELIOS_AUD_PLAYER_RESUME:
			Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
			audio.audio_state = AUDIO_PLAYING;
			Helios_Mutex_Unlock(audio.queue_mutex);
			break;
		case HELIOS_AUD_PLAYER_FINISHED:
			Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
			audio.audio_state = AUDIO_IDLE;
			Helios_Mutex_Unlock(audio.queue_mutex);
			uint8_t audio_msg = AUDIO_FINISH_EVENT;
			if (Helios_MsgQ_Put(audio.queue_msg, (const void *)&audio_msg, sizeof(uint8_t), HELIOS_NO_WAIT) == -1)
			{
				HELIOS_AUDIO_LOG("send audio msg failed[%u].\r\n", audio_msg);
				return -1;
			}
			HELIOS_AUDIO_LOG("send audio finished signal successed[%u].\r\n", audio_msg);
			break;
		case HELIOS_AUD_PLAYER_ERROR:
		case HELIOS_AUD_PLAYER_PAUSE:
		case HELIOS_AUD_PLAYER_CLOSE:
			Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
			audio.audio_state = AUDIO_IDLE;
			Helios_Mutex_Unlock(audio.queue_mutex);
			break;
		default:
			break;
	}
	//int ret = helios_audio_play_callback(p_data, len, state);
	return 0;
}

STATIC mp_obj_t audio_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) 
{
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
	int device = HELIOS_OUTPUT_RECEIVER;
    device = mp_obj_get_int(args[0]);
	
	if (device < 0 || device > 2) {
		mp_raise_ValueError("invalid device index, the value of device should be in[0,2]");
	}
	
    audio_obj_t *self = m_new_obj_with_finaliser(audio_obj_t);
    self->base.type = &audio_audio_type;
	self->device = device;
	//self->inited = 0;

	Helios_Audio_Init();
	#if defined(PLAT_ASR) || defined(PLAT_Unisoc)
	Helios_Audio_SetPaCallback((Helios_AudOutputType)device, user_audio_speakerpa_callback);
	#endif

	if(device == HELIOS_OUTPUT_RECEIVER||device == HELIOS_OUTPUT_SPEAKER||device == HELIOS_OUTPUT_HEADPHONE){
		Helios_Audio_SetAudioChannle(device);
	}
	else{
		device = HELIOS_OUTPUT_RECEIVER;			//default replay path
		Helios_Audio_SetAudioChannle(device);
	}

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
		audio.queue_msg = 0;
		audio.user_call_stop = 0;
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
	
    return MP_OBJ_FROM_PTR(self);
}



/*=============================================================================*/
/* FUNCTION: helios_play_file_start                                               */
/*=============================================================================*/
/*!@brief 		: play the audio file(mp3,amr)
 * @param[in] 	: filename
 * @param[out] 	: 
 * @return		:
 *        -  0--success
 *        - -1--error
 * @modify      : 
 				----------  ---------   ---------------------------------------------------
				2020/10/16  Jayceon.Fu	add queue, priority, breakin.
 */
/*=============================================================================*/
STATIC mp_obj_t helios_play_file_start(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	enum { ARG_priority, ARG_breakin, ARG_data };
    static const mp_arg_t allowed_args[] = 
    {
    	{ MP_QSTR_priority, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    	{ MP_QSTR_breakin,  MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_data,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
   
	int new_priority = vals[ARG_priority].u_int;
	int new_breakin  = vals[ARG_breakin].u_int;
	char *new_data = NULL;
	if (mp_obj_is_str(vals[ARG_data].u_obj)) 
	{
		new_data = (char *)mp_obj_str_get_str(vals[ARG_data].u_obj);
	} 
	else 
	{
		mp_raise_ValueError("invalid data");
	}

	Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
	if (audio.audio_state == AUDIO_IDLE)
	{
		audio.cur_priority = new_priority;
		audio.cur_breakin  = new_breakin;
		audio.cur_type     = AUDIO_FILE;
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
				
					audio.audio_queue[new_priority].audio_data[rear].audio_type = AUDIO_FILE;
					audio.audio_queue[new_priority].audio_data[rear].priority = new_priority;
					audio.audio_queue[new_priority].audio_data[rear].breakin  = new_breakin;
					audio.audio_queue[new_priority].audio_data[rear].mode     = 0;
					memset(audio.audio_queue[new_priority].audio_data[rear].data, 0, 500);
					strncpy(audio.audio_queue[new_priority].audio_data[rear].data, new_data, strlen(new_data));
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
					HELIOS_AUDIO_LOG("send audio msg failed.\r\n");
					return mp_obj_new_int(-1);
				}
				HELIOS_AUDIO_LOG("send audio stop signal successed.\r\n");
				return mp_obj_new_int(0);
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
				
					audio.audio_queue[new_priority].audio_data[rear].audio_type = AUDIO_FILE;
					audio.audio_queue[new_priority].audio_data[rear].priority = new_priority;
					audio.audio_queue[new_priority].audio_data[rear].breakin  = new_breakin;
					audio.audio_queue[new_priority].audio_data[rear].mode     = 0;
					memset(audio.audio_queue[new_priority].audio_data[rear].data, 0, 500);
					strncpy(audio.audio_queue[new_priority].audio_data[rear].data, new_data, strlen(new_data));
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

				audio.audio_queue[new_priority].audio_data[rear].audio_type = AUDIO_FILE;
				audio.audio_queue[new_priority].audio_data[rear].priority = new_priority;
				audio.audio_queue[new_priority].audio_data[rear].breakin  = new_breakin;
				audio.audio_queue[new_priority].audio_data[rear].mode     = 0;
				memset(audio.audio_queue[new_priority].audio_data[rear].data, 0, 500);
				strncpy(audio.audio_queue[new_priority].audio_data[rear].data, new_data, strlen(new_data));
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
	
	int ret = Helios_Audio_FilePlayStart(new_data, HELIOS_AUDIO_PLAY_TYPE_LOCAL, audio_play_callback);
	if (ret == -1)
	{
		Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
		audio.audio_state = AUDIO_IDLE;
		Helios_Mutex_Unlock(audio.queue_mutex);
	}
	
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(helios_play_file_start_obj, 1, helios_play_file_start);


/*=============================================================================*/
/* FUNCTION: helios_play_file_stop                                                */
/*=============================================================================*/
/*!@brief 		: stop playing the audio file(mp3,amr)
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		:
 *        -  0--success
 */
/*=============================================================================*/
STATIC mp_obj_t helios_play_file_stop(mp_obj_t self_in)
{
	Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
	audio.user_call_stop = 1;
	Helios_Mutex_Unlock(audio.queue_mutex);
	int ret = Helios_Audio_FilePlayStop();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(helios_play_file_stop_obj, helios_play_file_stop);


STATIC mp_obj_t helios_play_file_stop_all(mp_obj_t self_in)
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
	audio.user_call_stop = 1;
	Helios_Mutex_Unlock(audio.queue_mutex);

	int ret = Helios_Audio_FilePlayStop();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(helios_play_file_stop_all_obj, helios_play_file_stop_all);


/*=============================================================================*/
/* FUNCTION: helios_play_stream_start                                               */
/*=============================================================================*/
/*!@brief 		: play the audio file(mp3,amr)
 * @param[in] 	: filename
 * @param[out] 	: 
 * @return		:
 *        -  0--success
 *        - -1--error
 * @modify      : 
 				----------  ---------   ---------------------------------------------------
				2020/10/16  Jayceon.Fu	add queue, priority, breakin.
 */
/*=============================================================================*/
STATIC mp_obj_t helios_play_stream_start(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	enum { ARG_format, ARG_audiobuf };
    static const mp_arg_t allowed_args[] = 
    {
    	{ MP_QSTR_format, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_audiobuf,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
   
	int format = 1;
	format = vals[ARG_format].u_int;
	audio_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	mp_buffer_info_t buff_a;
	if(vals[ARG_audiobuf].u_obj != mp_const_none) {
		mp_get_buffer_raise(vals[ARG_audiobuf].u_obj, &buff_a, MP_BUFFER_READ);
		if(buff_a.len <= 0 || buff_a.buf == NULL) {
			mp_raise_ValueError("invalid audio_buff data");
		}
	}
	MP_THREAD_GIL_EXIT();
	int ret = Helios_Audio_StreamPlayStart(format, buff_a.buf, buff_a.len, HELIOS_AUDIO_PLAY_TYPE_LOCAL, self->device, audio_play_callback);
	MP_THREAD_GIL_ENTER();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(helios_play_stream_start_obj, 1, helios_play_stream_start);


/*=============================================================================*/
/* FUNCTION: helios_play_stream_stop                                                */
/*=============================================================================*/
/*!@brief 		: stop playing the audio file(mp3,amr)
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		:
 *        -  0--success
 */
/*=============================================================================*/
STATIC mp_obj_t helios_play_stream_stop(mp_obj_t self_in)
{
	int ret = Helios_Audio_StreamPlayStop();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(helios_play_stream_stop_obj, helios_play_stream_stop);



/*=============================================================================*/
/* FUNCTION: helios_get_audio_state                                               */
/*=============================================================================*/
/*!@brief 		: get the initialization state of audio.
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		:
 *        - -1 -- False
 *        - 0 -- True
 */
/*=============================================================================*/
STATIC mp_obj_t helios_get_audio_state(mp_obj_t self_in)
{
	//int ret = helios_audio_getInitState();
	int ret = 0;
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(helios_get_audio_state_obj, helios_get_audio_state);


/*=============================================================================*/
/* FUNCTION: helios_set_volume                                                    */
/*=============================================================================*/
/*!@brief 		: set volume.
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		: 
 */
/*=============================================================================*/
STATIC mp_obj_t helios_set_volume(mp_obj_t self_in, mp_obj_t volume)
{
	int vol = mp_obj_get_int(volume);
	if (vol < 0 || vol > 11)
	{
		mp_raise_ValueError("invalid value,TTS volume should be array between [0,11]");
		return mp_obj_new_int(-1);
	}
	
	Helios_AudPlayerType type = 1;
	int ret = Helios_Audio_SetVolume(type, vol);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(helios_set_volume_obj, helios_set_volume);


/*=============================================================================*/
/* FUNCTION: helios_get_volume                                                    */
/*=============================================================================*/
/*!@brief 		: get volume.
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		: current volume
 */
/*=============================================================================*/
STATIC mp_obj_t helios_get_volume(mp_obj_t self_in)
{
	Helios_AudPlayerType type = 1;
	int vol = Helios_Audio_GetVolume(type);
	return mp_obj_new_int(vol);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(helios_get_volume_obj, helios_get_volume);


#if defined (PLAT_Unisoc)
STATIC mp_obj_t helios_play_stream_jump(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	enum { ARG_second, ARG_drection };
    static const mp_arg_t allowed_args[] = 
    {
    	{ MP_QSTR_second, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    	{ MP_QSTR_drection, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
   
	int second = 0;
	second = vals[ARG_second].u_int;

	int drection = 0;
	drection = vals[ARG_drection].u_int;
	Helios_JumpInfoStruct info = {0,0};
	info = Helios_Audio_Stream_Jump(second,drection);	
	if(info.ofs == 0&&info.bytenow == 0)
		return mp_obj_new_int(-1);
	else
		return mp_obj_new_int(0);
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(helios_play_stream_jump_obj, 1, helios_play_stream_jump);

STATIC mp_obj_t helios_play_stream_pause(mp_obj_t self_in)
{
	int s = Helios_Audio_Stream_Pause();
	return mp_obj_new_int(s);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(helios_play_stream_pause_obj, helios_play_stream_pause);

STATIC mp_obj_t helios_play_stream_continue(mp_obj_t self_in)
{
	int s = Helios_Audio_Stream_Continue();
	return mp_obj_new_int(s);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(helios_play_stream_continue_obj, helios_play_stream_continue);

STATIC mp_obj_t helios_play_stream_seek(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	enum { ARG_second };
    static const mp_arg_t allowed_args[] = 
    {
    	{ MP_QSTR_second, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
   
	uint32_t second = 0;
	second = vals[ARG_second].u_int;

	long unsigned int bytepos = Helios_Audio_Stream_Seek(second);	

	return mp_obj_new_int(bytepos);
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(helios_play_stream_seek_obj, 1, helios_play_stream_seek);
#endif


STATIC mp_obj_t helios_audio___del__(mp_obj_t self_in)
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
	audio.user_call_stop = 1;
	Helios_Mutex_Unlock(audio.queue_mutex);
    g_audio_callback = NULL;
    g_audio_speakerpa_callback = NULL;

	int ret = Helios_Audio_FilePlayStop();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(helios_audio___del___obj, helios_audio___del__);


STATIC const mp_rom_map_elem_t audio_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&helios_audio___del___obj) },
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&helios_play_file_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&helios_play_file_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_stopAll), MP_ROM_PTR(&helios_play_file_stop_all_obj) },
    { MP_ROM_QSTR(MP_QSTR_getState), MP_ROM_PTR(&helios_get_audio_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_setVolume), MP_ROM_PTR(&helios_set_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_getVolume), MP_ROM_PTR(&helios_get_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_setCallback), MP_ROM_PTR(&helios_set_audio_callback_obj) },
	{ MP_ROM_QSTR(MP_QSTR_playStream), MP_ROM_PTR(&helios_play_stream_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stopPlayStream), MP_ROM_PTR(&helios_play_stream_stop_obj) },
#if defined (PLAT_ASR) || defined (PLAT_Unisoc)
	{ MP_ROM_QSTR(MP_QSTR_set_pa), MP_ROM_PTR(&helios_set_pa_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setSpeakerpaCallback), MP_ROM_PTR(&helios_set_speakerpa_callback_obj) },
#endif
#if defined (PLAT_Unisoc)
	{ MP_ROM_QSTR(MP_QSTR_StreamJump), MP_ROM_PTR(&helios_play_stream_jump_obj) },
	{ MP_ROM_QSTR(MP_QSTR_StreamPause), MP_ROM_PTR(&helios_play_stream_pause_obj) },
	{ MP_ROM_QSTR(MP_QSTR_StreamContinue), MP_ROM_PTR(&helios_play_stream_continue_obj) },
	{ MP_ROM_QSTR(MP_QSTR_StreamSeek), MP_ROM_PTR(&helios_play_stream_seek_obj) },

#endif
};
STATIC MP_DEFINE_CONST_DICT(audio_locals_dict, audio_locals_dict_table);

const mp_obj_type_t audio_audio_type = {
    { &mp_type_type },
    .name = MP_QSTR_Audio,
    .make_new = audio_make_new,
    .locals_dict = (mp_obj_dict_t *)&audio_locals_dict,
};





