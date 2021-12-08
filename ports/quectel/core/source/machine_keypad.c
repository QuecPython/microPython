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
 * @file    mod_keypad.h
 * @author  burols.wang
 * @version V1.0.0
 * @date    2021/10/29
 * @brief   IOT data interaction function module
 ******************************************************************************
 */

#if defined(PLAT_ASR) || defined(PLAT_Unisoc)
#include <stdio.h>
#include <stdint.h>
#include <string.h>


#include "compile.h"
#include "runtime.h"
#include "repl.h"
#include "mperrno.h"
#include "helios_keypad.h"
#include "helios_debug.h"
#ifdef MICROPY_PY_MACHINE


static c_callback_t *helios_keypad_callback = NULL;

typedef struct _machine_keypad_obj_t {
    mp_obj_base_t base;
} machine_keypad_obj_t;

static machine_keypad_obj_t *keypad_obj = NULL;

uint32_t helios_keypad_callback_to_python(Helios_KeyPad_Event event)
{
	mp_obj_t decode_cb[5] = {
		mp_obj_new_int((mp_int_t)event.event_id),
	 	mp_obj_new_int((mp_int_t)event.param_01),
        mp_obj_new_int((mp_int_t)event.param_02),
        mp_obj_new_int((mp_int_t)event.param_03),
        mp_obj_new_int((mp_int_t)event.key_v),
     };
	
	if( helios_keypad_callback == NULL) {
		return -1;
	}
	
    mp_sched_schedule_ex(helios_keypad_callback, MP_OBJ_FROM_PTR(mp_obj_new_list(5, decode_cb)));
	return 0;
}

STATIC mp_obj_t helios_keypad_set_callback(mp_obj_t self_in,mp_obj_t callback)
{
	
	static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
	helios_keypad_callback = &cb;
	mp_sched_schedule_callback_register(helios_keypad_callback, callback);
	
	Helios_KeyPad_SetCb((Helios_KeyPad_CallBack_t )helios_keypad_callback_to_python);
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(keypad_uart_set_callback_obj, helios_keypad_set_callback);

STATIC mp_obj_t keypad_set_mulikeyen(mp_obj_t self_in,mp_obj_t value)
{
	int en_de = mp_obj_get_int(value);
	if (en_de !=0 && en_de !=1 )
	{
		mp_raise_ValueError("invalid value must be 0 or 1");
	}
	Helios_KeyPad_MuliKeyEn(en_de);
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(keypad_set_mulikeyen_obj, keypad_set_mulikeyen);

STATIC mp_obj_t keypad_set_light(mp_obj_t self_in,mp_obj_t value)
{
	int on_off = mp_obj_get_int(value);
	if (on_off !=0 && on_off !=1 )
	{
		mp_raise_ValueError("invalid value must be 0 or 1");
	}
	int ret = Helios_KeyPad_Light_Ctl(on_off);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(keypad_set_light_obj, keypad_set_light);

STATIC mp_obj_t keypad_set_lightlevel(mp_obj_t self_in,mp_obj_t value)
{
	int level = mp_obj_get_int(value);
	if (!(Helios_KeyPad_Light_Level_0 <= level &&  level < Helios_KeyPad_Light_Level_MAX ) )
	{
		mp_raise_ValueError("invalid value shuld be in [Helios_KeyPad_Light_Level_0,...,Helios_KeyPad_Light_Level_MAX]");
	}
	int ret = Helios_KeyPad_Light_Level(level);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(keypad_set_lightlevel_obj, keypad_set_lightlevel);

STATIC mp_obj_t keypad_init(mp_obj_t self_in)
{
	int ret = Helios_KeyPad_Init();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(keypad_init_obj, keypad_init);

STATIC mp_obj_t keypad_deinit(mp_obj_t self_in)
{
	helios_keypad_callback = NULL;
	keypad_obj = NULL;
	
	int ret = Helios_KeyPad_Deinit();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(keypad_deinit_obj, keypad_deinit);


STATIC MP_DEFINE_CONST_FUN_OBJ_1(keypad__del__obj, keypad_deinit);


STATIC const mp_rom_map_elem_t keypad_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_keypad) },
    { MP_ROM_QSTR(MP_QSTR___del__), 	MP_ROM_PTR(&keypad__del__obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&keypad_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&keypad_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setMuliKeyen), MP_ROM_PTR(&keypad_set_mulikeyen_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setLight), MP_ROM_PTR(&keypad_set_light_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setLightLeve), MP_ROM_PTR(&keypad_set_lightlevel_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_callback), MP_ROM_PTR(&keypad_uart_set_callback_obj) },
};

STATIC MP_DEFINE_CONST_DICT(keypad_module_globals, keypad_module_globals_table);
STATIC mp_obj_t machine_keypad_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);

const mp_obj_type_t machine_keypad_type = {
    { &mp_type_type },
    .name = MP_QSTR_KeyPad,
    .make_new = machine_keypad_make_new,
    .locals_dict = (mp_obj_dict_t *)&keypad_module_globals,
};

STATIC mp_obj_t machine_keypad_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) 
{
    mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, true);
	if(keypad_obj == NULL)
	{
		keypad_obj = m_new_obj_with_finaliser(machine_keypad_obj_t);
	}
    machine_keypad_obj_t *self = keypad_obj;
    self->base.type = &machine_keypad_type;
    return MP_OBJ_FROM_PTR(self);
}

#endif
#endif /* MICROPY_PY_KEYPAD && PLAT_ASR || PLAT_Unisoc*/