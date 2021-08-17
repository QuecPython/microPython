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
#include "stream.h"
#include "mperrno.h"
#include "rtmp.h"

typedef struct
{
	mp_obj_base_t base;
	RTMP rtmp;
}rtmp_obj_t;


STATIC mp_obj_t qpy_rtmp_enable_write(mp_obj_t self_in)
{
	rtmp_obj_t *self = MP_OBJ_TO_PTR(self_in);
	RTMP_EnableWrite(&self->rtmp);
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_rtmp_enable_write_obj, qpy_rtmp_enable_write);


STATIC mp_obj_t qpy_rtmp_connect(mp_obj_t self_in)
{
	rtmp_obj_t *self = MP_OBJ_TO_PTR(self_in);
	MP_THREAD_GIL_EXIT();
	int ret = RTMP_Connect(&self->rtmp, NULL);
	MP_THREAD_GIL_ENTER();
	return ((ret == 1) ? mp_obj_new_int(0) : mp_obj_new_int(-1));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_rtmp_connect_obj, qpy_rtmp_connect);


STATIC mp_obj_t qpy_rtmp_connect_stream(mp_obj_t self_in)
{
	rtmp_obj_t *self = MP_OBJ_TO_PTR(self_in);
	MP_THREAD_GIL_EXIT();
	int ret = RTMP_ConnectStream(&self->rtmp, 0);
	MP_THREAD_GIL_ENTER();
	return ((ret == 1) ? mp_obj_new_int(0) : mp_obj_new_int(-1));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_rtmp_connect_stream_obj, qpy_rtmp_connect_stream);

STATIC mp_obj_t qpy_rtmp_close(mp_obj_t self_in)
{
	rtmp_obj_t *self = MP_OBJ_TO_PTR(self_in);
	RTMP_Close(&self->rtmp);
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_rtmp_close_obj, qpy_rtmp_close);


STATIC const mp_rom_map_elem_t rtmp_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_enableWrite), 	MP_ROM_PTR(&qpy_rtmp_enable_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), 		MP_ROM_PTR(&qpy_rtmp_connect_obj) },
	{ MP_ROM_QSTR(MP_QSTR_connectStream), 	MP_ROM_PTR(&qpy_rtmp_connect_stream_obj) },
	{ MP_ROM_QSTR(MP_QSTR_close), 			MP_ROM_PTR(&qpy_rtmp_close_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read), 			MP_ROM_PTR(&mp_stream_read_obj) },
	{ MP_ROM_QSTR(MP_QSTR_write), 			MP_ROM_PTR(&mp_stream_write_obj) },
};
STATIC MP_DEFINE_CONST_DICT(rtmp_locals_dict, rtmp_locals_dict_table);


STATIC mp_uint_t qpy_rtmp_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode)
{
    rtmp_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int read_size = 0;

    if (size == 0)
	{
        return 0;
    }
	read_size = RTMP_Read(&self->rtmp, (char *)buf_in, size);
	if (read_size < 0)
	{
		*errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
	}
	
    return read_size;
}

STATIC mp_uint_t qpy_rtmp_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode)
{
    rtmp_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int write_size = RTMP_Write(&self->rtmp, (const char *)buf_in, size);
    if (write_size < 0)
	{
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    return write_size;
}


STATIC const mp_stream_p_t rtmp_stream_p = {
    .read = qpy_rtmp_read,
    .write = qpy_rtmp_write,
    .is_text = false,
};


STATIC mp_obj_t rtmp_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);

const mp_obj_type_t rtmp_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTMP,
    .make_new = rtmp_make_new,
    .protocol = &rtmp_stream_p,
    .locals_dict = (mp_obj_t)&rtmp_locals_dict,
};

STATIC mp_obj_t rtmp_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) 
{
    //mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
	mp_buffer_info_t urlinfo = {0};
	mp_get_buffer_raise(args[0], &urlinfo, MP_BUFFER_READ);
	
    rtmp_obj_t *self = m_new_obj(rtmp_obj_t);
    self->base.type = &rtmp_type;
	RTMP_Init(&self->rtmp);
	int ret = RTMP_SetupURL(&self->rtmp, (char *)urlinfo.buf);
	if (ret != 1)
	{
		mp_raise_ValueError("RTMP setup URL failed!");
	}

    return MP_OBJ_FROM_PTR(self);
}



STATIC const mp_rom_map_elem_t mp_module_rtmp_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_librtmp) },
    { MP_ROM_QSTR(MP_QSTR_RTMP), MP_ROM_PTR(&rtmp_type) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_rtmp_globals, mp_module_rtmp_globals_table);

const mp_obj_module_t mp_module_rtmp = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_rtmp_globals,
};

