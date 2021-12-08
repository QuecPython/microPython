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
#include "mpconfigport.h"
#include "obj.h"
#include "compile.h"
#include "runtime.h"
#include "helios_usbnet.h"
#include "helios_debug.h"

#define HELIOS_MODUSB_LOG(msg, ...)      custom_log("modusbnet", msg, ##__VA_ARGS__)

STATIC mp_obj_t misc_usbnet_set_type(mp_obj_t type_in)
{
    int type = mp_obj_get_int(type_in);
    if(type != HELIOS_USBNET_TYPE_ECM && type != HELIOS_USBNET_TYPE_RNDIS)
    {
        mp_raise_ValueError("USBNET type must be Type_ECM or Type_RNDIS.");
    }
    int ret = Helios_USBNET_SetType(type);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(misc_usbnet_set_type_obj, misc_usbnet_set_type);


STATIC mp_obj_t misc_usbnet_get_type(void)
{
	Helios_USBNET_Type_e type = 0;
	int ret = Helios_USBNET_GetType(&type);
	if (ret == 0)
	{
		ret = type;
	}
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(misc_usbnet_get_type_obj, misc_usbnet_get_type);


STATIC mp_obj_t misc_usbnet_get_status(void)
{
	uint8_t status = 0;
	int ret = Helios_USBNET_GetStatus(&status);
	if (ret == 0)
	{
		ret = status;
	}
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(misc_usbnet_get_status_obj, misc_usbnet_get_status);


STATIC mp_obj_t misc_usbnet_open(void)
{
	int ret = Helios_USBNET_Open();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(misc_usbnet_open_obj, misc_usbnet_open);


STATIC mp_obj_t misc_usbnet_close(void)
{
	int ret = Helios_USBNET_Close();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(misc_usbnet_close_obj, misc_usbnet_close);



STATIC const mp_rom_map_elem_t misc_usbnet_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), 	MP_ROM_QSTR(MP_QSTR_USBNET) },
	{ MP_ROM_QSTR(MP_QSTR_set_worktype), 	MP_ROM_PTR(&misc_usbnet_set_type_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_worktype), 	MP_ROM_PTR(&misc_usbnet_get_type_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_status), 		MP_ROM_PTR(&misc_usbnet_get_status_obj) },
	{ MP_ROM_QSTR(MP_QSTR_open), 			MP_ROM_PTR(&misc_usbnet_open_obj) },
	{ MP_ROM_QSTR(MP_QSTR_close), 			MP_ROM_PTR(&misc_usbnet_close_obj) },
	{ MP_ROM_QSTR(MP_QSTR_Type_ECM), 		MP_ROM_INT(HELIOS_USBNET_TYPE_ECM) },
	{ MP_ROM_QSTR(MP_QSTR_Type_RNDIS), 		MP_ROM_INT(HELIOS_USBNET_TYPE_RNDIS) },
};

STATIC MP_DEFINE_CONST_DICT(misc_usbnet_locals_dict, misc_usbnet_locals_dict_table);


const mp_obj_module_t misc_usbnet_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&misc_usbnet_locals_dict,
};

