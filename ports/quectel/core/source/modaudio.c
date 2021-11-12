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
#include "mpconfigport.h"

#include "compile.h"
#include "runtime.h"
#include "repl.h"
#include "mperrno.h"



#include <stdio.h>
#include "modaudio.h"


STATIC const mp_rom_map_elem_t mp_module_audio_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_audio) },
#if defined(CONFIG_TTS)
    { MP_ROM_QSTR(MP_QSTR_TTS), MP_ROM_PTR(&audio_tts_type) },
#endif
    { MP_ROM_QSTR(MP_QSTR_Audio), MP_ROM_PTR(&audio_audio_type) },
    { MP_ROM_QSTR(MP_QSTR_Record), MP_ROM_PTR(&audio_record_type) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_audio_globals, mp_module_audio_globals_table);


const mp_obj_module_t mp_module_audio = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_audio_globals,
};




