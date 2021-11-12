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
#include <string.h>
#include "runtime.h"
#include "helios_power.h"
#include "helios_debug.h"

#define MOD_POWERKEY_LOG(msg, ...)      custom_log(powerkey, msg, ##__VA_ARGS__)


typedef struct _misc_powerkey_obj_t {
    mp_obj_base_t base;
} misc_powerkey_obj_t;

STATIC misc_powerkey_obj_t *powerkey_obj = NULL;


STATIC c_callback_t *g_user_callback = NULL;

STATIC void powerkey_event_callback(uint8_t status)
{
    if(g_user_callback)
	{
		MOD_POWERKEY_LOG("powerkey event callback, status = %d\r\n", status);
        mp_sched_schedule_ex(g_user_callback, mp_obj_new_int(status));
    } 
}

STATIC mp_obj_t powerkey_event_register(mp_obj_t self_in, mp_obj_t usr_callback)
{
	int ret = 1;
	Helios_PowerInitStruct info = {powerkey_event_callback};
	
	static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
	g_user_callback = &cb;
	mp_sched_schedule_callback_register(g_user_callback, usr_callback);

	MOD_POWERKEY_LOG("powerkey_event_register\r\n");
	ret = Helios_Power_Init(&info);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(misc_powerkey_event_register_obj, powerkey_event_register);

STATIC mp_obj_t misc_powerkey_deinit(mp_obj_t self_in)
{
	int ret = -1;
	g_user_callback = NULL;
	powerkey_obj = NULL;
	ret = Helios_Power_DeInit();
	
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(misc_powerkey__del__obj, misc_powerkey_deinit);



STATIC const mp_rom_map_elem_t misc_powerkey_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&misc_powerkey__del__obj) },
    { MP_ROM_QSTR(MP_QSTR_powerKeyEventRegister), MP_ROM_PTR(&misc_powerkey_event_register_obj) },
};
STATIC MP_DEFINE_CONST_DICT(misc_powerkey_locals_dict, misc_powerkey_locals_dict_table);


STATIC mp_obj_t misc_powerkey_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
const mp_obj_type_t misc_powerkey_type = {
    { &mp_type_type },
    .name = MP_QSTR_PowerKey,
    .make_new = misc_powerkey_make_new,
    .locals_dict = (mp_obj_dict_t *)&misc_powerkey_locals_dict,
};


STATIC mp_obj_t misc_powerkey_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
	misc_powerkey_obj_t *self = NULL;
	if(powerkey_obj == NULL)
	{
		powerkey_obj = m_new_obj_with_finaliser(misc_powerkey_obj_t);
	}
	
    self = powerkey_obj;
    self->base.type = &misc_powerkey_type;
    return MP_OBJ_FROM_PTR(self);
}
