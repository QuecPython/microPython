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
#include <stdlib.h>
#include <string.h>
#include "mpconfigport.h"
#include "obj.h"
#include "compile.h"
#include "runtime.h"
#include "helios_usb.h"
#include "helios_debug.h"

#define HELIOS_MODUSB_LOG(msg, ...)      custom_log("modusb", msg, ##__VA_ARGS__)

typedef struct _misc_usb_obj_t 
{
    mp_obj_base_t base;
}misc_usb_obj_t;


static c_callback_t *g_py_callback = NULL;
static misc_usb_obj_t *usb_obj = NULL;


static void ql_usb_detect_handler(Helios_USB_Status_e status)
{
	if (g_py_callback)
	{
		HELIOS_MODUSB_LOG("callback start, status = %u.\r\n", status);
		mp_sched_schedule_ex(g_py_callback, mp_obj_new_int(status));
		HELIOS_MODUSB_LOG("callback end.\r\n");
	}
}

STATIC mp_obj_t misc_usb_register_cb(mp_obj_t self_in, mp_obj_t usr_callback)
{
    static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
	g_py_callback = &cb;
	mp_sched_schedule_callback_register(g_py_callback, usr_callback);

    Helios_USB_DeInit();
	Helios_USBInitStruct info = {ql_usb_detect_handler};
	int ret = Helios_USB_Init(&info);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(misc_usb_register_cb_obj, misc_usb_register_cb);


STATIC mp_obj_t misc_usb_get_status(mp_obj_t self_in)
{
	int status = Helios_USB_GetStatus();
	return mp_obj_new_int(status);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(misc_usb_get_status_obj, misc_usb_get_status);

STATIC mp_obj_t misc_usb__Deint(mp_obj_t self_in)
{
	int ret = -1;
	g_py_callback = NULL;
	usb_obj = NULL;
	ret = Helios_USB_DeInit();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(misc_usb__del__obj, misc_usb__Deint);



STATIC const mp_rom_map_elem_t misc_usb_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___del__), 	MP_ROM_PTR(&misc_usb__del__obj) },
	{ MP_ROM_QSTR(MP_QSTR_getStatus), MP_ROM_PTR(&misc_usb_get_status_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setCallback), MP_ROM_PTR(&misc_usb_register_cb_obj) },
};

STATIC MP_DEFINE_CONST_DICT(misc_usb_locals_dict, misc_usb_locals_dict_table);

STATIC mp_obj_t misc_usb_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) ;

const mp_obj_type_t misc_usb_type = {
    { &mp_type_type },
    .name = MP_QSTR_USB,
    .make_new = misc_usb_make_new,
    .locals_dict = (mp_obj_dict_t *)&misc_usb_locals_dict,
};

STATIC mp_obj_t misc_usb_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) 
{
    mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, true);
	misc_usb_obj_t *self = NULL;

	if(usb_obj == NULL)
	{
		usb_obj = m_new_obj_with_finaliser(misc_usb_obj_t);
	}
	
    self = usb_obj;
    self->base.type = &misc_usb_type;
	
    return MP_OBJ_FROM_PTR(self);
}
