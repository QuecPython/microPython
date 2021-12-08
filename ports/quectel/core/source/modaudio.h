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

#ifndef __MODAUDIO_H__
#define __MODAUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "obj.h"

#if defined(CONFIG_TTS)
extern const mp_obj_type_t audio_tts_type;
#endif
extern const mp_obj_type_t audio_audio_type;
extern const mp_obj_type_t audio_record_type;


typedef struct
{
	//char *model;
	/*
	** channel_support[0]-recever
	** channel_support[1]-headphone
	** channel_support[2]-speaker
	** 
	** channel_support[n] is 0, non-support 
	** channel_support[n] is 1, support 
	*/
	uint8_t channel_support[3]; 
}AudioChannelSupport_t;

#ifdef __cplusplus
}
#endif

#endif

