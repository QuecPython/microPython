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
#include "helios_audio.h"
#include "Helios_debug.h"

typedef struct _audio_obj_t {
    mp_obj_base_t base;
	//int  inited;
} audio_obj_t;

#define audio_printf(fmt, ...) custom_log(audio_audio, fmt, ##__VA_ARGS__)

extern const mp_obj_type_t audio_audio_type;

static mp_obj_t g_audio_callback;

int device = HELIOS_OUTPUT_RECEIVER;



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
	g_audio_callback = callback;
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
int audio_play_callback(Helios_EnumAudPlayerState state)
{
	if (state == 2)
	{
		state = 7;
	}
	int ret = 0;
	if (g_audio_callback)
	{
		audio_printf("[Audio] callback start.\r\n");
		ret = mp_sched_schedule(g_audio_callback, mp_obj_new_int(state));
		audio_printf("[Audio] callback end.\r\n");
	}
	else
		ret = -1;
	//int ret = helios_audio_play_callback(p_data, len, state);
	return ret;
}

STATIC mp_obj_t audio_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) 
{
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    device = mp_obj_get_int(args[0]);
	
	if (device < 0 || device > 2) {
		mp_raise_ValueError("invalid device index");
	}
	
	//Helios_Audio_SetAudioChannle(device);

    audio_obj_t *self = m_new_obj(audio_obj_t);
    self->base.type = &audio_audio_type;
	//self->inited = 0;

	/*Jayceon 2020/10/16 - audio queue initialization --begin--*/
	Helios_Audio_Init();
	/*Jayceon 2020/10/16 - audio queue initialization --end--*/
	if(device == HELIOS_OUTPUT_RECEIVER||device == HELIOS_OUTPUT_SPEAKER||device == HELIOS_OUTPUT_HEADPHONE){
		Helios_Audio_SetAudioChannle(device);
	}
	else{
		device = HELIOS_OUTPUT_RECEIVER;			//default replay path
		Helios_Audio_SetAudioChannle(device);
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
	int ret = Helios_Audio_FilePlayStart(new_data, new_priority, new_breakin, HELIOS_AUDIO_PLAY_TYPE_LOCAL, device,(void *)audio_play_callback);

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
	int ret = Helios_Audio_FilePlayStop();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(helios_play_file_stop_obj, helios_play_file_stop);




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

	mp_buffer_info_t buff_a;
	if(vals[ARG_audiobuf].u_obj != mp_const_none) {
		mp_get_buffer_raise(vals[ARG_audiobuf].u_obj, &buff_a, MP_BUFFER_READ);
		if(buff_a.len <= 0 || buff_a.buf == NULL) {
			mp_raise_ValueError("invalid audio_buff data");
		}
	}

	int ret = Helios_Audio_StreamPlayStart(format, buff_a.buf, buff_a.len, HELIOS_AUDIO_PLAY_TYPE_LOCAL, device, (void *)audio_play_callback);

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
	if (vol < 1 || vol > 11)
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



STATIC const mp_rom_map_elem_t audio_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&helios_play_file_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&helios_play_file_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_getState), MP_ROM_PTR(&helios_get_audio_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_setVolume), MP_ROM_PTR(&helios_set_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_getVolume), MP_ROM_PTR(&helios_get_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_setCallback), MP_ROM_PTR(&helios_set_audio_callback_obj) },
	{ MP_ROM_QSTR(MP_QSTR_playStream), MP_ROM_PTR(&helios_play_stream_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stopPlayStream), MP_ROM_PTR(&helios_play_stream_stop_obj) },
};
STATIC MP_DEFINE_CONST_DICT(audio_locals_dict, audio_locals_dict_table);

const mp_obj_type_t audio_audio_type = {
    { &mp_type_type },
    .name = MP_QSTR_Audio,
    .make_new = audio_make_new,
    .locals_dict = (mp_obj_dict_t *)&audio_locals_dict,
};