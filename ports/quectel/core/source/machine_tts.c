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

#include "ql_tts.h"

extern void ql_set_audio_path_earphone(void);
extern void ql_set_audio_path_receiver(void);
extern void ql_set_audio_path_speaker(void);

const mp_obj_type_t machine_tts_type;

typedef struct _machine_tts_obj_t {
    mp_obj_base_t base;
	int  inited;
} machine_tts_obj_t;

STATIC void machine_tts_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "tts ");
}

STATIC mp_obj_t machine_tts_make_new(const mp_obj_type_t *type,
    size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    int device = mp_obj_get_int(args[0]);
	
	if (device < 0 || device > 2) {
		mp_raise_ValueError("invalid device index");
	}
	
	switch(device)
	{
		case 0:ql_set_audio_path_receiver();break;
		case 1:ql_set_audio_path_earphone();break;
		case 2:ql_set_audio_path_speaker();break;
	}

    machine_tts_obj_t *self = m_new_obj(machine_tts_obj_t);
    self->base.type = &machine_tts_type;
	self->inited = 0;
 
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_tts_play(size_t n_args,
    const mp_obj_t *args, mp_map_t *kw_args) {
	enum { ARG_mode, ARG_data };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_data, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args,
        MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
		
	 switch (vals[ARG_mode].u_int) {
        case 0:
            break;
        case 1:
        case 2:
        case 3:
            break;;
        default:
            mp_raise_ValueError("invalid mode");
            break;
    }
	int mode = vals[ARG_mode].u_int;
	char *data;
	if (mp_obj_is_str(vals[ARG_data].u_obj)) {
		data = mp_obj_str_get_str(vals[ARG_data].u_obj);
	} else {
		mp_raise_ValueError("invalid data");
	}

	machine_tts_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	if (self->inited == 0) {
		mp_raise_ValueError("tts not inited");
	}
	ql_tts_play(mode, data);
	
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_tts_play_obj, 1, machine_tts_play);

STATIC mp_obj_t machine_tts_init(mp_obj_t self_in) {
    machine_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);

	ql_tts_init(NULL);
	self->inited = 1;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_tts_init_obj, machine_tts_init);

STATIC mp_obj_t machine_tts_deinit(mp_obj_t self_in) {
    machine_tts_obj_t *self = MP_OBJ_TO_PTR(self_in);

	ql_tts_deinit();
	self->inited = 0;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_tts_deinit_obj, machine_tts_deinit);

STATIC const mp_rom_map_elem_t machine_tts_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_tts_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_tts_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&machine_tts_play_obj) },
};

STATIC MP_DEFINE_CONST_DICT(machine_tts_locals_dict,
    machine_tts_locals_dict_table);

const mp_obj_type_t machine_tts_type = {
    { &mp_type_type },
    .name = MP_QSTR_TTS,
    .print = machine_tts_print,
    .make_new = machine_tts_make_new,
    .locals_dict = (mp_obj_dict_t *)&machine_tts_locals_dict,
};