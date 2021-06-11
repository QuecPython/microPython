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
#include "obj.h"
#include "runtime.h"
#include "mphalport.h"
#include "helios_dev.h"


STATIC mp_obj_t queclib_dev_product_id()
{
	char product_id_str[64] = {0};
	int ret = Helios_Dev_GetPID((void *)product_id_str, sizeof(product_id_str));
	if(ret == 0)
	{
		return mp_obj_new_str(product_id_str, strlen(product_id_str));
	}
	return mp_obj_new_int(-1);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(queclib_dev_product_id_obj, queclib_dev_product_id);



STATIC mp_obj_t queclib_dev_serial_number()
{
	char serial_number_str[64] = {0};
	int ret = Helios_Dev_GetSN((void *)serial_number_str, sizeof(serial_number_str));
	if(ret == 0)
	{
		return mp_obj_new_str(serial_number_str, strlen(serial_number_str));
	}
	return mp_obj_new_int(-1);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(queclib_dev_serial_number_obj, queclib_dev_serial_number);


STATIC mp_obj_t queclib_dev_model()
{
	char model_str[64] = {0};
	int ret = Helios_Dev_GetModel((void *)model_str, sizeof(model_str));
	if(ret == 0)
	{
		return mp_obj_new_str(model_str, strlen(model_str));
	}
	return mp_obj_new_int(-1);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(queclib_dev_model_obj, queclib_dev_model);


STATIC mp_obj_t queclib_dev_fw_version()
{
	char fw_version_str[64] = {0};
	int ret = Helios_Dev_GetFwVersion((void *)fw_version_str, sizeof(fw_version_str));
	if(ret == 0)
	{
		return mp_obj_new_str(fw_version_str, strlen(fw_version_str));
	}
	return mp_obj_new_int(-1);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(queclib_dev_fw_version_obj, queclib_dev_fw_version);



STATIC mp_obj_t queclib_dev_imei()
{
	//the imei length is 15-17 bytes
	char imei_str[19] = {0};
	int ret = Helios_Dev_GetIMEI((void *)imei_str, sizeof(imei_str));
	if(ret == 0)
	{
		return mp_obj_new_str(imei_str, strlen(imei_str));
	}
	return mp_obj_new_int(-1);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(queclib_dev_imei_obj, queclib_dev_imei);


STATIC const mp_rom_map_elem_t mp_module_modem_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_modem) },
    { MP_ROM_QSTR(MP_QSTR_getDevSN), MP_ROM_PTR(&queclib_dev_serial_number_obj) },
    { MP_ROM_QSTR(MP_QSTR_getDevImei), MP_ROM_PTR(&queclib_dev_imei_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getDevModel), MP_ROM_PTR(&queclib_dev_model_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getDevFwVersion), MP_ROM_PTR(&queclib_dev_fw_version_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getDevProductId), MP_ROM_PTR(&queclib_dev_product_id_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_modem_globals, mp_module_modem_globals_table);


const mp_obj_module_t mp_module_modem = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_modem_globals,
};

