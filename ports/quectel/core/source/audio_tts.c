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
#include "utf8togbk.h"
#include "helios_audio.h"
#include "helios_debug.h"
#include "objstr.h"

const mp_obj_type_t audio_tts_type;

typedef struct _audio_tts_obj_t {
    mp_obj_base_t base;
	int  inited;
} audio_tts_obj_t;

static mp_obj_t g_tts_callback;

static int device = 0;
static int tts_speed = 5;

#define audio_printf(fmt, ...) custom_log(audio_audio, fmt, ##__VA_ARGS__)

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
	g_tts_callback = callback;
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(helios_set_tts_callback_obj, helios_set_tts_callback);


static void user_tts_callback(Helios_TTSENvent event)
{
	//mp_obj_t tuple[2] = {mp_obj_new_int(event), mp_obj_new_str(str, strlen(str))};
	
	if (g_tts_callback)
	{
		//uart_printf("[TTS] callback start.\r\n");
		mp_sched_schedule(g_tts_callback, mp_obj_new_int(event));
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
	//uart_printf("app_helios_tts_callback:event = %d,str = %s\n", event, str);
	user_tts_callback(event);
}

STATIC void audio_tts_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    //audio_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "tts ");
}

STATIC mp_obj_t audio_tts_make_new(const mp_obj_type_t *type,
    size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    device = mp_obj_get_int(args[0]);
	
	if (device < 0 || device > 2) {
		mp_raise_ValueError("invalid device index");
	}
	
	Helios_Audio_SetAudioChannle(device);

    audio_tts_obj_t *self = m_new_obj(audio_tts_obj_t);
    self->base.type = &audio_tts_type;
	self->inited = 0;

	/*
	** Jayceon.Fu-2020/09/27:audio queue initialization
	*/
#ifdef PLAT_ASR
	Helios_TTS_Init(app_helios_tts_callback);
	Helios_TTS_SetSpeed(tts_speed);
#endif
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

    int new_mode = vals[ARG_mode].u_int;

	if ((new_mode < 1) || (new_mode > 3))
	{
		audio_printf("new_mode is %d\n",new_mode);
		mp_raise_ValueError("invalid mode");
		return mp_obj_new_int(-1);
	}
    
	//char *new_data;
	const char *src_data;
	if (mp_obj_is_str(vals[ARG_data].u_obj)) 
	{
		src_data = mp_obj_str_get_str(vals[ARG_data].u_obj);
	} 
	else 
	{	
		src_data = NULL;
		mp_raise_ValueError("invalid data");
	}
	audio_printf("src_data is %s\n",src_data);

	int  len = strlen(src_data);
	audio_printf("len is %d\n",len);

	int new_priority = vals[ARG_priority].u_int;
	int new_breakin  = vals[ARG_breakin].u_int;
	audio_printf("new_priority is %d\n",new_priority);
	audio_printf("new_breakin is %d\n",new_breakin);

#ifdef PLAT_ASR
	int ret = Helios_TTS_Start(new_mode, src_data, len, new_priority, new_breakin, device);
	return mp_obj_new_int(ret);
#else
	return mp_obj_new_int(0);
#endif	
    
}
MP_DEFINE_CONST_FUN_OBJ_KW(audio_tts_play_obj, 1, audio_tts_play);

STATIC mp_obj_t audio_tts_init(mp_obj_t self_in) {
#ifdef PLAT_ASR
    audio_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int ret = Helios_TTS_Init(NULL);
	if (ret == -1)
	{
		return mp_obj_new_int(-1);
	}
	self->inited = 1;
#endif
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(audio_tts_init_obj, audio_tts_init);

STATIC mp_obj_t audio_tts_deinit(mp_obj_t self_in) {
#ifdef PLAT_ASR
    audio_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);

	int ret = Helios_TTS_Deinit();
	if (ret == -1)
	{
		return mp_obj_new_int(-1);
	}
	self->inited = 0;
#endif
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(audio_tts_deinit_obj, audio_tts_deinit);

STATIC mp_obj_t aduio_tts_stop(mp_obj_t self_in)
{
	//audio_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);
#ifdef PLAT_ASR
	Helios_TTS_Stop();
#endif
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(aduio_tts_stop_obj, aduio_tts_stop);


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
#ifdef PLAT_ASR
	int index = mp_obj_get_int(volume);
	if ((index > 9) || (index < 0))
	{
		mp_raise_ValueError("invalid value,TTS volume should be array between [0,9]");
		return mp_obj_new_int(-1);
	}
	
	//int vol_map[10] = {-32768, -25487, -18206, -10925, -3644, 3637, 10918, 18199, 25480, 32761};
	int ret = Helios_TTS_SetVolume(index);			//first para only works in unisoc
#else
	int ret = 0;
#endif
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
#ifdef PLAT_ASR
	int vol = Helios_TTS_GetVolume();
#else
	int vol = 0;
#endif
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
	int index = mp_obj_get_int(speed);
	if ((index > 9) || (index < 0))
	{
		return mp_obj_new_int(-1);
	}
	tts_speed = index;
#ifdef PLAT_ASR
	int ret = Helios_TTS_SetSpeed(tts_speed);
	if(ret)
	{
		audio_printf("Helios_TTS_SetSpeed failed\n");
	}
    return mp_obj_new_int(ret);
#else
	return mp_obj_new_int(-1);
#endif
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
#ifdef PLAT_ASR
    return mp_obj_new_int(Helios_TTS_GetSpeed());
#else
	return mp_obj_new_int(tts_speed);
#endif
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(aduio_tts_get_speed_obj, aduio_tts_get_speed);


STATIC mp_obj_t aduio_tts_get_state(mp_obj_t self_in)
{
#ifdef PLAT_ASR
	int state = Helios_TTS_GetStatus();
    return mp_obj_new_int(state);
#else
	return mp_obj_new_int(-1);
#endif
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(aduio_tts_get_state_obj, aduio_tts_get_state);

STATIC const mp_rom_map_elem_t audio_tts_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&audio_tts_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&audio_tts_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&audio_tts_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&aduio_tts_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&audio_tts_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_setVolume), MP_ROM_PTR(&aduio_tts_set_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_getVolume), MP_ROM_PTR(&aduio_tts_get_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_setSpeed), MP_ROM_PTR(&aduio_tts_set_speed_obj) },
    { MP_ROM_QSTR(MP_QSTR_getSpeed), MP_ROM_PTR(&aduio_tts_get_speed_obj) },
    { MP_ROM_QSTR(MP_QSTR_getState), MP_ROM_PTR(&aduio_tts_get_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_setCallback), MP_ROM_PTR(&helios_set_tts_callback_obj) },
};

STATIC MP_DEFINE_CONST_DICT(audio_tts_locals_dict, audio_tts_locals_dict_table);

const mp_obj_type_t audio_tts_type = {
    { &mp_type_type },
    .name = MP_QSTR_TTS,
    .print = audio_tts_print,
    .make_new = audio_tts_make_new,
    .locals_dict = (mp_obj_dict_t *)&audio_tts_locals_dict,
};