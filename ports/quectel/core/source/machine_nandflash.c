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
 * @file    machine_nandflash.c
 * @author  xxx
 * @version V1.0.0
 * @date    2019/04/16
 * @brief   xxx
 ******************************************************************************
 */
#include <stdio.h>
#include <string.h>

#include "runtime.h"
#include "gc.h"
#include "mphal.h"
#include "mperrno.h"


#include "modmachine.h"

#include "helios_flash.h"


const mp_obj_type_t machine_nandflash_type;


typedef struct machine_nandflash_obj_struct {
    mp_obj_base_t base;
    unsigned char is_init;
}machine_nandflash_obj_t;

static machine_nandflash_obj_t *self_nandflash = NULL;


STATIC mp_obj_t machine_nandflash_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
	if(self_nandflash && self_nandflash->is_init) 
		mp_raise_ValueError("nand flash has been initialized");

	if(!self_nandflash) {
    	self_nandflash = m_new_obj(machine_nandflash_obj_t);
	    self_nandflash->base.type = &machine_nandflash_type;
	}
	
    self_nandflash->is_init = 0;

	if(Helios_NandFlash_Init() != 0) {
		mp_raise_ValueError("nand flash initialization failed");
	}
	self_nandflash->is_init = 1;
	return MP_OBJ_FROM_PTR(self_nandflash);
}



STATIC const mp_arg_t machine_nandflash_open_allowed_args[] = {
    { MP_QSTR_filename,    	MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_mode, 		MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
};
STATIC mp_obj_t machine_nandflash_open(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_filename, ARG_mode};
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_nandflash_open_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(machine_nandflash_open_allowed_args), machine_nandflash_open_allowed_args, args);
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char *filename_ptr = (char *)mp_obj_str_get_str(args[ARG_filename].u_obj);
	char *fmode = (char *)mp_obj_str_get_str(args[ARG_mode].u_obj);
	printf("machine name = %s, mode = %s\n",filename_ptr, fmode);
	int ret = Helios_NandFlash_fopen(filename_ptr, fmode);
	printf("machine ret = %d\n",ret);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_nandflash_open_obj, 1, machine_nandflash_open);


STATIC mp_obj_t machine_nandflash_close(mp_obj_t self_in, mp_obj_t stream)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	unsigned int stream_id = mp_obj_get_int(stream);

	int ret = Helios_NandFlash_fclose(stream_id);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_close_obj, machine_nandflash_close);


STATIC const mp_arg_t machine_nandflash_read_allowed_args[] = {
    { MP_QSTR_databuf,     	MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_datasize, 	MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 8} },
    { MP_QSTR_stream, 		MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 8} },
};	
STATIC mp_obj_t machine_nandflash_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_databuf, ARG_datasize, ARG_stream};
	
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_nandflash_read_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(machine_nandflash_read_allowed_args), machine_nandflash_read_allowed_args, args);
		
	machine_nandflash_obj_t *self = (machine_nandflash_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);

	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_databuf].u_obj, &bufinfo, MP_BUFFER_READ);

	
	int length = (size_t)args[ARG_datasize].u_int > bufinfo.len ? bufinfo.len : (size_t)args[ARG_datasize].u_int;

	
	int strem_id = args[ARG_stream].u_int;

	int ret = Helios_NandFlash_fread((void*) bufinfo.buf, 1, length, strem_id);

	return mp_obj_new_int(ret);
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_nandflash_read_obj, 1, machine_nandflash_read);

STATIC const mp_arg_t machine_nandflash_write_allowed_args[] = {
    { MP_QSTR_databuf,     	MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_datasize, 	MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 8} },
    { MP_QSTR_stream, 		MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 8} },
};	
STATIC mp_obj_t machine_nandflash_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_databuf, ARG_datasize, ARG_stream};
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_nandflash_write_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(machine_nandflash_write_allowed_args), machine_nandflash_write_allowed_args, args);
		
	machine_nandflash_obj_t *self = (machine_nandflash_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");


    // get the buffer to write the data from
	mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_databuf].u_obj, &bufinfo, MP_BUFFER_READ);

	
	int length = (size_t)args[ARG_datasize].u_int > bufinfo.len ? bufinfo.len : (size_t)args[ARG_datasize].u_int;

	int strem_id = args[ARG_stream].u_int;

	int ret = Helios_NandFlash_fwrite((void*) bufinfo.buf, 1, length, strem_id);

	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_nandflash_write_obj, 1, machine_nandflash_write);


STATIC const mp_arg_t machine_nandflash_seek_allowed_args[] = {
    { MP_QSTR_stream, 		MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_offset, 		MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_wherefrom, 	MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
};	
STATIC mp_obj_t machine_nandflash_seek(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_stream, ARG_offset, ARG_wherefrom};
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_nandflash_seek_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(machine_nandflash_seek_allowed_args), machine_nandflash_seek_allowed_args, args);
		
	machine_nandflash_obj_t *self = (machine_nandflash_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");
	
	int offset = (size_t)args[ARG_offset].u_int;
	int strem_id = args[ARG_stream].u_int;
	int wherefrom = args[ARG_wherefrom].u_int;

	int ret = Helios_NandFlash_fseek(strem_id, offset, wherefrom);

	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_nandflash_seek_obj, 1, machine_nandflash_seek);

STATIC mp_obj_t machine_nandflash_remove(mp_obj_t self_in, mp_obj_t filename)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(filename);

	int ret = Helios_NandFlash_fremove(name);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_remove_obj, machine_nandflash_remove);


STATIC mp_obj_t machine_nandflash_rename(mp_obj_t self_in, mp_obj_t oldname, mp_obj_t newname)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* old_name = (char *)mp_obj_str_get_str(oldname);
	
	char* new_name = (char *)mp_obj_str_get_str(newname);

	int ret = Helios_NandFlash_frename(old_name, new_name);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_nandflash_rename_obj, machine_nandflash_rename);

STATIC mp_obj_t machine_nandflash_filesize(mp_obj_t self_in, mp_obj_t filename)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(filename);

    return mp_obj_new_int(Helios_NandFlash_fsize(name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_filesize_obj, machine_nandflash_filesize);

STATIC mp_obj_t machine_nandflash_mkdir(mp_obj_t self_in, mp_obj_t dirname)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(dirname);

    return mp_obj_new_int(Helios_NandFlash_fmkdir(name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_mkdir_obj, machine_nandflash_mkdir);

STATIC mp_obj_t machine_nandflash_rmdir(mp_obj_t self_in, mp_obj_t dirname)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(dirname);

    return mp_obj_new_int(Helios_NandFlash_frmdir(name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_rmdir_obj, machine_nandflash_rmdir);

STATIC mp_obj_t machine_nandflash_SetCurrentDir(mp_obj_t self_in, mp_obj_t dirname)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(dirname);

    return mp_obj_new_int(Helios_NandFlash_SetCurrentDir(name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_chdir_obj, machine_nandflash_SetCurrentDir);


STATIC mp_obj_t machine_nandflash_GetCurrentDir(mp_obj_t self_in, mp_obj_t dirname, mp_obj_t maxlen)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(dirname);
	int MaxLen = mp_obj_get_int(maxlen);
	char *ret = Helios_NandFlash_GetCurrentDir(name,MaxLen);

    return mp_obj_new_str(ret,strlen(ret));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_nandflash_getcwd_obj, machine_nandflash_GetCurrentDir);


STATIC mp_obj_t machine_nandflash_freesize(mp_obj_t self_in)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

    return mp_obj_new_int_from_uint(Helios_NandFlash_FreeSize());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_nandflash_freesize_obj, machine_nandflash_freesize);



STATIC mp_obj_t machine_nandflash_findfirst(mp_obj_t self_in, mp_obj_t dirname)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(dirname);
	NANDFILE_INFO fileinfo = {0};
	
	if(Helios_NandFlash_FindFirst(name, &fileinfo) != 0) {
		return mp_obj_new_int(-1);
	}

	mp_obj_t file_info[12] = {
		mp_obj_new_str(fileinfo.file_name, strlen(fileinfo.file_name)),
		mp_obj_new_int(fileinfo.time),
		mp_obj_new_int(fileinfo.date),
		mp_obj_new_int(fileinfo.size),
		mp_obj_new_int(fileinfo.owner_id),
		mp_obj_new_int(fileinfo.group_id),
		mp_obj_new_int(fileinfo.permissions),
		mp_obj_new_int(fileinfo.data_id),
		mp_obj_new_int(fileinfo.plr_id),
		mp_obj_new_int(fileinfo.plr_time),
		mp_obj_new_int(fileinfo.plr_date),
		mp_obj_new_int(fileinfo.plr_size),
	};
	
    return mp_obj_new_list(12, file_info);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_findfirst_obj, machine_nandflash_findfirst);

STATIC mp_obj_t machine_nandflash_findnext(mp_obj_t self_in)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	NANDFILE_INFO fileinfo = {0};
	
	if(Helios_NandFlash_FindNext(&fileinfo) != 0) {
		return mp_obj_new_int(-1);
	}

	mp_obj_t file_info[12] = {
		mp_obj_new_str(fileinfo.file_name, strlen(fileinfo.file_name)),
		mp_obj_new_int(fileinfo.time),
		mp_obj_new_int(fileinfo.date),
		mp_obj_new_int(fileinfo.size),
		mp_obj_new_int(fileinfo.owner_id),
		mp_obj_new_int(fileinfo.group_id),
		mp_obj_new_int(fileinfo.permissions),
		mp_obj_new_int(fileinfo.data_id),
		mp_obj_new_int(fileinfo.plr_id),
		mp_obj_new_int(fileinfo.plr_time),
		mp_obj_new_int(fileinfo.plr_date),
		mp_obj_new_int(fileinfo.plr_size),
	};
	
    return mp_obj_new_list(12, file_info);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_nandflash_findnext_obj, machine_nandflash_findnext);

STATIC mp_obj_t machine_nandflash_fromat(mp_obj_t self_in)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");
	
    return mp_obj_new_int(Helios_NandFlash_Fromat());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_nandflash_fromat_obj, machine_nandflash_fromat);


STATIC const mp_rom_map_elem_t nandflash_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&machine_nandflash_open_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&machine_nandflash_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&machine_nandflash_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&machine_nandflash_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&machine_nandflash_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&machine_nandflash_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&machine_nandflash_rename_obj) },
    { MP_ROM_QSTR(MP_QSTR_filesize), MP_ROM_PTR(&machine_nandflash_filesize_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&machine_nandflash_mkdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&machine_nandflash_rmdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&machine_nandflash_getcwd_obj) },
    { MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&machine_nandflash_chdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_freesize), MP_ROM_PTR(&machine_nandflash_freesize_obj) },
    { MP_ROM_QSTR(MP_QSTR_findfirst), MP_ROM_PTR(&machine_nandflash_findfirst_obj) },
    { MP_ROM_QSTR(MP_QSTR_findnext), MP_ROM_PTR(&machine_nandflash_findnext_obj) },
    { MP_ROM_QSTR(MP_QSTR_fromat), MP_ROM_PTR(&machine_nandflash_fromat_obj) },

};

STATIC MP_DEFINE_CONST_DICT(nandflash_locals_dict, nandflash_locals_dict_table);

const mp_obj_type_t machine_nandflash_type = {
    { &mp_type_type },
    .name = MP_QSTR_NandFlash,
    .make_new = machine_nandflash_make_new,
    .locals_dict = (mp_obj_t)&nandflash_locals_dict,
};


