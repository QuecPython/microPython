/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2021, QUECTEL  
 * Copyright (c) Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
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


#include "nlr.h"
#include "objlist.h"
#include "objstr.h"
#include "runtime.h"
#include "mperrno.h"

#include "helios_datacall.h"
#include "helios_celllocator.h"
#include "helios_debug.h"


#define QPY_MODLBS_LOG(msg, ...)      custom_log(celllocator, msg, ##__VA_ARGS__)

/*=============================================================================*/
/* FUNCTION: queclib_cell_locator_perform                                      */
/*=============================================================================*/
/*!@brief 		: perform cell locator query 
 * @param[in] 	: server_ip
 * @param[in] 	: port
 * @param[in] 	: token
 * @param[in] 	: timeout
 * @param[in] 	: profile_id
 * @param[out] 	: 
 * @return		:
 *        - -1--error
 *        - if successful, return : (latitude, longtitude, accuracy)
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_lbs_get_cell_coordinates(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	Helios_LBSInfoStruct position = {0};
	Helios_LBSConfigStruct config_info = {0};
	Helios_DataCallInfoStruct datacall_info = {0};
	int port = mp_obj_get_int(args[1]);
	int timeout = mp_obj_get_int(args[3]);
	int profile_idx = mp_obj_get_int(args[4]);

	mp_buffer_info_t serverinfo = {0};
	mp_buffer_info_t tokeninfo = {0};
	mp_get_buffer_raise(args[0], &serverinfo, MP_BUFFER_READ);
 	mp_get_buffer_raise(args[2], &tokeninfo, MP_BUFFER_READ);
	
	if (serverinfo.len >= 255)
	{
		return mp_obj_new_int(-2);
	}
	if (tokeninfo.len != 16)
	{
		return mp_obj_new_int(-3);
	}
	if ((timeout < 1) || (timeout > 300))
	{
		return mp_obj_new_int(-4);
	}
	if ((profile_idx < (int)HELIOS_PROFILE_IDX_MIN) || (profile_idx > (int)HELIOS_PROFILE_IDX_MAX))
	{
		mp_raise_ValueError("profileIdx invalid.");
	}
	
	ret = Helios_DataCall_GetInfo(profile_idx, 0, &datacall_info);
	if (ret == 0)
	{
		if ((datacall_info.v4.state == 0) && ((datacall_info.v6.state == 0)))
		{
			return mp_obj_new_int(-5);
		}
	}
	else
	{
		return mp_obj_new_int(-5);
	}

	config_info.server_addr = (char *)serverinfo.buf;
	config_info.token = (char *)tokeninfo.buf;
	config_info.port  = port;
	config_info.timeout = timeout;
	config_info.profile_idx = profile_idx;

	ret = Helios_LBS_SetConfiguration(0, &config_info);
	if (ret != 0)
	{
		QPY_MODLBS_LOG("para config failed!\r\n");
		return mp_obj_new_int(-1);
	}

	MP_THREAD_GIL_EXIT();
	ret = Helios_LBS_GetPosition(&position);
	MP_THREAD_GIL_ENTER();
	if (ret != 0)
	{
		return mp_obj_new_int(-6);
	}

	mp_obj_t result_tuple[3] = 
	{
		mp_obj_new_float(position.longitude),
		mp_obj_new_float(position.latitude),
		mp_obj_new_int(position.accuracy),
	};

	return mp_obj_new_tuple(3, result_tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_lbs_get_cell_coordinates_obj, 4, 5, qpy_lbs_get_cell_coordinates);



STATIC const mp_rom_map_elem_t mp_module_celllocator_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_cellLocator) },
	{ MP_ROM_QSTR(MP_QSTR_getLocation), MP_ROM_PTR(&qpy_lbs_get_cell_coordinates_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_celllocator_globals, mp_module_celllocator_globals_table);


const mp_obj_module_t mp_module_celllocator = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_celllocator_globals,
};




