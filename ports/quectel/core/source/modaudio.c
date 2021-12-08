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
#include <stdint.h>
#include <string.h>
#include "mpconfigport.h"

#include "compile.h"
#include "runtime.h"
#include "repl.h"
#include "mperrno.h"
#include "modaudio.h"
#include "helios_debug.h"
#include "helios_audio.h"

#define HELIOS_MODAUDIO_LOG(msg, ...)      custom_log("mod_audio", msg, ##__VA_ARGS__)


static AudioChannelSupport_t audio_channel_tab = {
#if defined(BOARD_EC600NCN_LA)
	{1, 0, 0}
#elif defined(BOARD_EC600NCN_LC)
	{1, 0, 0}
#elif defined(BOARD_EC600NCN_LD)
	{1, 0, 0}
#elif defined(BOARD_EC600SCN_LA)
	{1, 0, 0}
#elif defined(BOARD_EC600SCN_LB)
	{1, 0, 0}
#elif defined(BOARD_EC800NCN_LA)
	{1, 0, 0}
#elif defined(BOARD_EC200UCN_AA)
	{1, 1, 1}
#elif defined(BOARD_EC200UCN_LB)
	{1, 1, 1}
#elif defined(BOARD_EC200UEU_AB)
	{1, 1, 1}
#elif defined(BOARD_EC600UCN_LB) || defined(BOARD_EC600UCN_LB_VOLTE)
	{1, 1, 1}
#elif defined(BOARD_EC600UCN_LC)
	{1, 1, 1}
#elif defined(BOARD_EC600UEU_AB) || defined(BOARD_EC600UEU_AB_VOLTE)
	{1, 1, 1}
#elif defined(BOARD_BG95M3)
	{0, 0, 0}
#elif defined(BOARD_BC25PA)
	{0, 0, 0}
#else
	#error "Please add the corresponding platform configuration items in audio_channel_tab."
#endif
};


/*
	return value:
	0 - The current platform does not support this channel
	1 - The current platform support this channel
   -1 - error
*/
int audio_channel_check(int channel)
{
	if ((channel < 0) || (channel > 2))
	{
		return 0;
	}
	
	return (int)audio_channel_tab.channel_support[channel];
}

#if defined (PLAT_ASR)

#define CHECK_PROFILE_ID(id) (((HELIOS_VC_HANDSET <= id &&  HELIOS_VC_MAXCOUNT > id) || (HELIOS_AC_HANDSET <= id &&  HELIOS_AC_MAXCOUNT > id)))
#define CHECK_LEVLE(x) ()

STATIC mp_obj_t helios_set_DSPGain(mp_obj_t profile_id, mp_obj_t dsp_gain_table)
{
	int id = mp_obj_get_int(profile_id);
	int i = 0;
	int16_t dsp_gain[12] = {0};
	mp_obj_t *items = NULL;
	mp_buffer_info_t dsp_table_data;
	if (!CHECK_PROFILE_ID(id))
	{
		mp_raise_ValueError("The ID parameter is wrong. The value is [0,4] or [64,69]");
		return mp_obj_new_int(-1);
	}

	mp_obj_get_array_fixed_n(dsp_gain_table, 12, &items);

	for (i = 0; i < 12; i++) {
		dsp_gain[i] = (int16_t)mp_obj_get_int(items[i]);
		HELIOS_MODAUDIO_LOG("table[%d]: %d\n",i, dsp_gain[i]);
	}
	
	int ret = Helios_Audio_SetDSPGainTable((uint32_t)id, dsp_gain);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(audio_DSPGain_obj, helios_set_DSPGain);

static int codec_gain_num(unsigned int profile_id, signed int level, uint32_t* data)
{
	if(CHECK_PROFILE_ID((int)profile_id));
	else{
		return -1;	
	}
	if(level < 0 || level > HELIOS_AUDIOHAL_SPK_VOL_11){	
		return -1;
	}
	if(data[0] > 38){	
		return -1;
	}	
	if(data[1] > 4){	
		return -1;
	}
	if(data[2] > 4){	
		return -1;
	}
	if(data[3] > 1){	
		return -1;
	}
	if(data[4] > 2){	
		return -1;
	}
	return 0;
}


STATIC mp_obj_t helios_set_CodecGain(mp_obj_t profile_id, mp_obj_t level, mp_obj_t codec_gain_table)
{
	int id = mp_obj_get_int(profile_id);
	int32_t codec_level = mp_obj_get_int(level);
	int i = 0;
	uint32_t codec_gain[5] = {0};
	mp_obj_t *items = NULL;
	int8_t *data = NULL;

	mp_obj_get_array_fixed_n(codec_gain_table, 5, &items);

	for (i = 0; i < 5; i++) {
		codec_gain[i] = (uint32_t)mp_obj_get_int(items[i]);
		HELIOS_MODAUDIO_LOG("table[%d]: %d\n",i, codec_gain[i]);
	}
	
	if( -1 == codec_gain_num(id, codec_level, codec_gain)) {
		mp_raise_ValueError("Parameter error, please check whether the parameter meets the value range");
	}
	
	int ret = Helios_Audio_SetCodecGainTable((uint32_t)id, codec_level, codec_gain);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(audio_CodecGain_obj, helios_set_CodecGain);

STATIC mp_obj_t helios_get_current_ID()
{
	int ret = -1;
	uint32_t id = Helios_get_current_profileID();
	HELIOS_MODAUDIO_LOG("profile ID = %d\n", id);
	if(0xffff !=  id) {
		ret = (int)id;
	}
	
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(audio_get_current_ID_obj, helios_get_current_ID);

#endif



STATIC const mp_rom_map_elem_t mp_module_audio_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_audio) },
#if defined(CONFIG_TTS)
    { MP_ROM_QSTR(MP_QSTR_TTS), MP_ROM_PTR(&audio_tts_type) },
#endif
    { MP_ROM_QSTR(MP_QSTR_Audio), MP_ROM_PTR(&audio_audio_type) },
    { MP_ROM_QSTR(MP_QSTR_Record), MP_ROM_PTR(&audio_record_type) },
#if defined (PLAT_ASR)
    { MP_ROM_QSTR(MP_QSTR_set_DSPGain), MP_ROM_PTR(&audio_DSPGain_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_CodecGain), MP_ROM_PTR(&audio_CodecGain_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_CurrentID), MP_ROM_PTR(&audio_get_current_ID_obj) },
#endif
};
STATIC MP_DEFINE_CONST_DICT(mp_module_audio_globals, mp_module_audio_globals_table);


const mp_obj_module_t mp_module_audio = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_audio_globals,
};




