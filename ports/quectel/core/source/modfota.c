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
 * @file    modfota.c
 * @author  Pawn
 * @version V1.0.0
 * @date    2020/08/18
 * @brief   fota
 ******************************************************************************
 */
 
#include <stdio.h>
#include <stdlib.h>
#include "obj.h"
#include <string.h>
#include "runtime.h"
#include "mphalport.h"
#include "helios_fota.h"
#include "helios_power.h"
#include "helios_debug.h"

#define MOD_FOTA_LOG(msg, ...)      custom_log(FOTA, msg, ##__VA_ARGS__)

typedef struct _fota_obj_t {
	mp_obj_base_t base;
	int ctx;
}fota_obj_t;

const mp_obj_type_t mp_fota_type;

STATIC mp_obj_t fota_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
	fota_obj_t *self = m_new_obj_with_finaliser(fota_obj_t);
	
	self->base.type = &mp_fota_type;
	self->ctx = Helios_Fota_Init();

	return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t fota_write(size_t n_args, const mp_obj_t *args)
{
	int ret;
	int file_size;
	
	fota_obj_t *self = MP_OBJ_TO_PTR(args[0]);

	if(n_args > 2)
	{
		file_size = mp_obj_get_int(args[2]);
	}
	else
	{
		MOD_FOTA_LOG("*** input param invalid \r\n***");
		return mp_obj_new_int(-1);
	}
	
	mp_buffer_info_t bufinfo;
	mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);

	MOD_FOTA_LOG(" buff len : %d   file_size : %d\r\n", bufinfo.len, file_size);
	
	ret = Helios_Fota_PackageWrite(self->ctx, bufinfo.buf, bufinfo.len, (size_t)file_size);

	if(ret)
	{
		MOD_FOTA_LOG("*** fota package write fail ***\r\n");
		return mp_obj_new_int(-1);
	}
	if(ret < 0)
	{
		MOD_FOTA_LOG("*** fota package file read fail ***\r\n");
		return mp_obj_new_int(-1);
	}
	
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(fota_write_obj, 1, 3, fota_write);



STATIC mp_obj_t fota_flush(const mp_obj_t arg0)
{
	int ret = 0; 
	fota_obj_t *self = MP_OBJ_TO_PTR(arg0);
	
	ret = Helios_Fota_PackageFlush(self->ctx);
	if(ret)
	{
		MOD_FOTA_LOG("*** fota package flush fail ***\r\n");
		return mp_obj_new_int(-1);
	}
	MOD_FOTA_LOG("fota package write done, verifing ...\r\n");
	
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(fota_flush_obj, fota_flush);

STATIC mp_obj_t fota_verify(const mp_obj_t arg0)
{
	int ret = 0; 
	fota_obj_t *self = MP_OBJ_TO_PTR(arg0);
	ret = Helios_Fota_PackageVerify(self->ctx);

	if(ret)
	{
		MOD_FOTA_LOG("*** fota package verify fail ***\r\n");
		return mp_obj_new_int(-1);
	}
	MOD_FOTA_LOG("fota package verify done, will restart to update ...\r\n");
	
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(fota_verify_obj, fota_verify);

STATIC mp_obj_t fota_callback = NULL;


static void mpFotaProgressCB(int sta, int progress)
{
    if(fota_callback != NULL){
		mp_obj_t fota_list[2] = { 											
				mp_obj_new_int(sta),												
				mp_obj_new_int(progress),											
			};	

	    mp_sched_schedule(fota_callback, mp_obj_new_list(2, fota_list));
    }
	
	if(sta == 1)
	{
		MOD_FOTA_LOG("fota test downloading (%d)%d ...\r\n", sta, progress);
	}
	else if(sta == 0)
	{
		MOD_FOTA_LOG("fota test downloading (%d)%d ...\r\n", sta, progress);
	}
	else if(sta == 2)
	{
		MOD_FOTA_LOG("fota test update flag setted, will restart to update ...\r\n");
		Helios_Power_Reset(1);
	}
	else if(sta == -1)
	{
		MOD_FOTA_LOG("fota test download failed (%d)%d\r\n", sta, progress);
		MOD_FOTA_LOG("========== fota test end ==========\r\n");
	}
}

STATIC mp_obj_t fota_firmware_download(size_t n_args, const mp_obj_t *args)
{	
	int ret;
	char *server_address1 = NULL;
	char *server_address2 = NULL;

	if (n_args == 3)
	{	
		server_address1 = (char *)mp_obj_str_get_str(args[1]);
		server_address2 = (char *)mp_obj_str_get_str(args[2]);
		
		ret  = Helios_Fota_firmware_download(server_address1, server_address2, mpFotaProgressCB);
	}
	else if(n_args == 4)
	{
		server_address1 = (char *)mp_obj_str_get_str(args[1]);
		server_address2 = (char *)mp_obj_str_get_str(args[2]);
		fota_callback = args[3];

		ret  = Helios_Fota_firmware_download(server_address1, server_address2, mpFotaProgressCB);
	}
	else
	{
		MOD_FOTA_LOG("*** input param invalid ***\r\n");
		return mp_obj_new_int(-1);
	}
	
	if(ret)
	{
		MOD_FOTA_LOG("*** fota firmware download fail ***\r\n");
		return mp_obj_new_int(-1);
	}

	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(fota_firmware_download_obj, 2, 5, fota_firmware_download);



STATIC const mp_rom_map_elem_t fota_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_fota) },
	{ MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&fota_write_obj) },
	{ MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&fota_flush_obj) },
	{ MP_ROM_QSTR(MP_QSTR_verify), MP_ROM_PTR(&fota_verify_obj) },
	{ MP_ROM_QSTR(MP_QSTR_httpDownload), MP_ROM_PTR(&fota_firmware_download_obj) },
};
STATIC MP_DEFINE_CONST_DICT(fota_locals_dict, fota_locals_dict_table);

const mp_obj_type_t mp_fota_type = {
    { &mp_type_type },
    .name = MP_QSTR_fota,
	.make_new = fota_make_new,
	.locals_dict = (mp_obj_dict_t *)&fota_locals_dict,
};
