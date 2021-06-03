/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2021, QUECTEL  
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

