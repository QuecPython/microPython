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

#include "stat.h"


#include "runtime.h"
#include "gc.h"
#include "mphal.h"
#include "mperrno.h"


#include "modmachine.h"

#include "helios_flash.h"
#include "fs_vfs.h"
#include "helios_debug.h"

#define NANDFLASH_LOG(msg, ...)      custom_log("nand", msg, ##__VA_ARGS__)


const mp_obj_type_t machine_nandflash_type;


typedef struct machine_nandflash_obj_struct {
    mp_obj_base_t base;
    unsigned char is_init;
}machine_nandflash_obj_t;

static machine_nandflash_obj_t *self_nandflash = NULL;

extern int yaffsVfsMount(const char *base_path, 
                 size_t cache_count, size_t sfile_reserved_lb,
                 bool read_only);

STATIC mp_obj_t machine_nandflash_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{

	if(!self_nandflash) {
    	self_nandflash = m_new_obj(machine_nandflash_obj_t);
	    self_nandflash->base.type = &machine_nandflash_type;
	}
	

#ifdef CONFIG_NANDFLASH_YAFFS2
	if(self_nandflash->is_init == 0) {
		if(yaffsVfsMount("/", 10000, 0,0) != 0){
			mp_raise_ValueError("nand flash initialization failed");
		}
	}
#else
	mp_raise_ValueError("Fat file system is not supported");
#endif
	self_nandflash->is_init = 1;
	return MP_OBJ_FROM_PTR(self_nandflash);
}



#define FILE_OPEN_FLAGS_R		(YAFFS_RDONLY)
#define FILE_OPEN_FLAGS_W		(YAFFS_WRONLY | YAFFS_CREAT | YAFFS_TRUNC)
#define FILE_OPEN_FLAGS_A		(YAFFS_WRONLY | YAFFS_CREAT | YAFFS_APPEND)
#define FILE_OPEN_FLAGS_RPluse	(YAFFS_RDWR)
#define FILE_OPEN_FLAGS_WPluse	(YAFFS_RDWR | YAFFS_CREAT | YAFFS_TRUNC)
#define FILE_OPEN_FLAGS_APluse	(YAFFS_RDWR | YAFFS_CREAT | YAFFS_APPEND)

static int Helios_file_open_mode2flags(const char * mode)
{
	int mode_len = 0;

	if(mode == NULL) return -1;

	mode_len = strlen(mode);
	
	if(mode_len == 1)
	{
		if(mode[0] == 'r') return FILE_OPEN_FLAGS_R;
		if(mode[0] == 'w') return FILE_OPEN_FLAGS_W;
		if(mode[0] == 'a') return FILE_OPEN_FLAGS_A;
		
		return -1;
	}
	else if(mode_len == 2)
	{
		char *r_chr = NULL, *w_chr = NULL, *a_chr = NULL;
		char *b_chr = NULL, *t_chr = NULL;
		char *plus_chr = NULL;
		int rwa_cnt = 0, btplus_cnt = 0;

		r_chr = strchr(mode, 'r');
		w_chr = strchr(mode, 'w');
		a_chr = strchr(mode, 'a');
		if(r_chr) rwa_cnt++;
		if(w_chr) rwa_cnt++;
		if(a_chr) rwa_cnt++;
		if(rwa_cnt != 1) return -1;

		b_chr = strchr(mode, 'b');
		t_chr = strchr(mode, 't');
		plus_chr = strchr(mode, '+');
		if(b_chr) btplus_cnt++;
		if(t_chr) btplus_cnt++;
		if(plus_chr) btplus_cnt++;
		if(btplus_cnt != 1) return -1;

		if(plus_chr)
		{
			if(r_chr) return FILE_OPEN_FLAGS_RPluse;
			if(w_chr) return FILE_OPEN_FLAGS_WPluse;
			if(a_chr) return FILE_OPEN_FLAGS_APluse;
		}
		else
		{
			if(r_chr) return FILE_OPEN_FLAGS_R;
			if(w_chr) return FILE_OPEN_FLAGS_W;
			if(a_chr) return FILE_OPEN_FLAGS_A;
		}
	}
	else if(mode_len == 3)
	{
		char *r_chr = NULL, *w_chr = NULL, *a_chr = NULL;
		char *b_chr = NULL, *t_chr = NULL;
		char *plus_chr = NULL;
		int rwa_cnt = 0, bt_cnt = 0, plus_cnt = 0;

		r_chr = strchr(mode, 'r');
		w_chr = strchr(mode, 'w');
		a_chr = strchr(mode, 'a');
		if(r_chr) rwa_cnt++;
		if(w_chr) rwa_cnt++;
		if(a_chr) rwa_cnt++;
		if(rwa_cnt != 1) return -1;

		b_chr = strchr(mode, 'b');
		t_chr = strchr(mode, 't');
		if(b_chr) bt_cnt++;
		if(t_chr) bt_cnt++;
		if(bt_cnt != 1) return -1;

		plus_chr = strchr(mode, '+');
		if(plus_chr) plus_cnt++;
		if(plus_cnt != 1) return -1;

		if(r_chr) return FILE_OPEN_FLAGS_RPluse;
		if(w_chr) return FILE_OPEN_FLAGS_WPluse;
		if(a_chr) return FILE_OPEN_FLAGS_APluse;
	}

	return -1;
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
	
	unsigned long oflag = Helios_file_open_mode2flags(fmode);
	
	int ret = vfs_open(filename_ptr, oflag);
	
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_nandflash_open_obj, 1, machine_nandflash_open);


STATIC mp_obj_t machine_nandflash_close(mp_obj_t self_in, mp_obj_t stream)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	unsigned int stream_id = mp_obj_get_int(stream);
	
    int res;
	res = vfs_close(stream_id);

    return mp_obj_new_int(res);
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

    int res;
    res = vfs_read(strem_id,(void*) bufinfo.buf, length);
    if(res < 0)
    {
        mp_raise_ValueError("vfs_read faile\n");
    }


	return mp_obj_new_int(res);
	
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
	
    int ret = vfs_write(strem_id, (void*) bufinfo.buf, length);

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
	
	int ret = vfs_lseek(strem_id, offset, wherefrom);

	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_nandflash_seek_obj, 1, machine_nandflash_seek);

STATIC mp_obj_t machine_nandflash_remove(mp_obj_t self_in, mp_obj_t filename)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(filename);

	int ret = vfs_unlink(name);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_remove_obj, machine_nandflash_remove);


STATIC mp_obj_t machine_nandflash_rename(mp_obj_t self_in, mp_obj_t oldname, mp_obj_t newname)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* old_name = (char *)mp_obj_str_get_str(oldname);
	
	char* new_name = (char *)mp_obj_str_get_str(newname);

	int ret = vfs_rename(old_name, new_name);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_nandflash_rename_obj, machine_nandflash_rename);

STATIC mp_obj_t machine_nandflash_filesize(mp_obj_t self_in, mp_obj_t filename)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
 	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(filename);

	struct stat st = {0};
	if(-1 == vfs_stat(name, &st)) return mp_obj_new_int(-1);

    return mp_obj_new_int(st.st_size);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_filesize_obj, machine_nandflash_filesize);

STATIC mp_obj_t machine_nandflash_mkdir(mp_obj_t self_in, mp_obj_t dirname)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(dirname);

    return mp_obj_new_int(vfs_mkdir(name,0));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_mkdir_obj, machine_nandflash_mkdir);

STATIC mp_obj_t machine_nandflash_rmdir(mp_obj_t self_in, mp_obj_t dirname)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(dirname);

    return mp_obj_new_int(vfs_rmdir(name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_rmdir_obj, machine_nandflash_rmdir);

STATIC mp_obj_t machine_nandflash_SetCurrentDir(mp_obj_t self_in, mp_obj_t dirname)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(dirname);

	
	struct stat st = {0};
	//if(-1 == vfs_stat(name, &st) || st.st_mode != S_IFDIR) return mp_obj_new_int(-1);
	if(-1 == vfs_stat(name, &st)) return mp_obj_new_int(-1);

	NANDFLASH_LOG("st mode = %d\n",st.st_mode);

    return mp_obj_new_int(vfs_chdir(name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_chdir_obj, machine_nandflash_SetCurrentDir);


STATIC mp_obj_t machine_nandflash_GetCurrentDir(mp_obj_t self_in)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");
	char dir_str[50] = {0};

	char *ret = vfs_getcwd(dir_str,50);

    return mp_obj_new_str(ret,strlen(ret));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_nandflash_getcwd_obj, machine_nandflash_GetCurrentDir);


STATIC mp_obj_t machine_nandflash_freesize(mp_obj_t self_in)
{
	unsigned int  free_size;
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");
	
	int err = 0;
	struct statvfs vfs_state = {0};
	err = vfs_statvfs("/", &vfs_state);
	if(err < 0)
	{
		return mp_obj_new_int(-1);
	}
	free_size = (vfs_state.f_bsize) * (vfs_state.f_bavail);

    return mp_obj_new_int_from_uint(free_size);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_nandflash_freesize_obj, machine_nandflash_freesize);


STATIC mp_obj_t machine_nandflash_format(mp_obj_t self_in)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");
    return mp_obj_new_int(vfs_format("/"));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_nandflash_format_obj, machine_nandflash_format);

#define HELIOS_LISTDIR_LEN 10*1024

char list_dir[HELIOS_LISTDIR_LEN] = {0};

static int Helios_vfs_ListDir(const char *dir_ptr, char* fileinfo_ptr) {
	struct dirent *de;
	char* file_dir_list = fileinfo_ptr;
	

	int fd = vfs_open(dir_ptr ,YAFFS_RDONLY, 0);
	if(fd < 0)
	{
		NANDFLASH_LOG("open of dir failed\n");
		return -1;
	}
	else
	{

		while((de = vfs_readdir_fd(fd)) != NULL)
		{
			if(de->d_name) {
				if(strlen(file_dir_list) + strlen(de->d_name) > HELIOS_LISTDIR_LEN-1) {
					mp_raise_ValueError("Too many catalog files to display");
					break;
				}
				sprintf(file_dir_list,"%s %s",file_dir_list,de->d_name);
				NANDFLASH_LOG("%s type %d \n",de->d_name,de->d_type);
			}
		}

		vfs_close(fd);
		NANDFLASH_LOG("file_dir_list = %s \n",file_dir_list);
	}

	return 0;
}


STATIC mp_obj_t machine_nandflash_listdir(size_t n_args, const mp_obj_t *args)
{

    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");
	
	int ret = 0;
	char dir_str[256] = {0};

	memset(list_dir, 0, HELIOS_LISTDIR_LEN);
	/*if(list_dir == NULL) {
		list_dir = calloc(1,HELIOS_LISTDIR_LEN);
		if(list_dir == NULL) return mp_obj_new_int(-1);
	}
	memset(list_dir, 0, HELIOS_LISTDIR_LEN);*/
	
	if(n_args == 1) {
		char *data = vfs_getcwd(dir_str,256);
		NANDFLASH_LOG("listdir = %s\n",data);
		ret = Helios_vfs_ListDir(data,list_dir);
	} else if(n_args == 2) {
		char* dir_name = (char *)mp_obj_str_get_str(args[1]);
		ret = Helios_vfs_ListDir(dir_name, list_dir);
	} else {
		mp_raise_ValueError("wrong number of parameters");
	}

	if(ret != 0) return mp_obj_new_int(-1);
	
    return mp_obj_new_str(list_dir, strlen(list_dir));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_nandflash_listdir_obj, 1, 2, machine_nandflash_listdir);

STATIC mp_obj_t machine_nandflash_exist(mp_obj_t self_in, mp_obj_t filename)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(filename);

	struct stat st = {0};
	if(-1 == vfs_stat(name, &st)) return mp_obj_new_int(0);

    return mp_obj_new_int(1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_exist_obj, machine_nandflash_exist);

STATIC mp_obj_t machine_nandflash_sync(mp_obj_t self_in, mp_obj_t filename)
{
	
    machine_nandflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->is_init == 0) mp_raise_ValueError("nand flash is not initialized");

	char* name = (char *)mp_obj_str_get_str(filename);

	struct stat st = {0};
	if(-1 == vfs_stat(name, &st)) return mp_obj_new_int(-1);

	if(-1 == vfs_sync(name)) return mp_obj_new_int(-1);

    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_nandflash_sync_obj, machine_nandflash_sync);


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
    { MP_ROM_QSTR(MP_QSTR_format), MP_ROM_PTR(&machine_nandflash_format_obj) },
    { MP_ROM_QSTR(MP_QSTR_listdir), MP_ROM_PTR(&machine_nandflash_listdir_obj) },
	{ MP_ROM_QSTR(MP_QSTR_exist), MP_ROM_PTR(&machine_nandflash_exist_obj) },
	{ MP_ROM_QSTR(MP_QSTR_sync), MP_ROM_PTR(&machine_nandflash_sync_obj) },
	
};

STATIC MP_DEFINE_CONST_DICT(nandflash_locals_dict, nandflash_locals_dict_table);

const mp_obj_type_t machine_nandflash_type = {
    { &mp_type_type },
    .name = MP_QSTR_NandFlash,
    .make_new = machine_nandflash_make_new,
    .locals_dict = (mp_obj_t)&nandflash_locals_dict,
};


