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

#if defined(PLAT_Qualcomm)
#include <stdio.h>

#include "py/mphal.h"
#include "py/mpthread.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "extmod/vfs_efs.h"

#include "helios_fs.h"


typedef struct _mp_obj_vfs_efs_file_t {
    mp_obj_base_t base;
    HeliosFILE * fd;
} mp_obj_vfs_efs_file_t;

#ifdef MICROPY_CPYTHON_COMPAT
STATIC void check_fd_is_open(const mp_obj_vfs_efs_file_t *o) {
    if (o->fd < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("I/O operation on closed file"));
    }
}
#else
#define check_fd_is_open(o)
#endif

STATIC void vfs_efs_file_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    mp_obj_vfs_efs_file_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<io.%s %d>", mp_obj_get_type_str(self_in), self->fd);
}

mp_obj_t mp_vfs_efs_file_open(mp_obj_t self_in, const mp_obj_type_t *type, mp_obj_t file_in, mp_obj_t mode_in) {
    mp_obj_vfs_efs_t *self = MP_OBJ_TO_PTR(self_in);
	mp_obj_vfs_efs_file_t *o = m_new_obj(mp_obj_vfs_efs_file_t);
    const char *mode = mp_obj_str_get_str(mode_in);
    const char *fname = mp_obj_str_get_str(file_in);
    HeliosFILE * fd;
	char f_path[64] = "";

	if(NULL != self)
	{
		if(self->readonly)
		{
			sprintf(f_path, "%s%s", "/bak", fname);
		}
		else
		{
			sprintf(f_path, "%s%s", "/usr", fname);
		}
	}
	else
	{
		sprintf(f_path, "%s%s", "/usr", fname);
	}

	o->base.type = type;
	fd = Helios_fopen(f_path, mode);
	
	if(NULL == fd)
	{
		mp_raise_OSError(-1);
		return mp_const_none;
	}
    o->fd = fd;
	
    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t vfs_efs_file_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_file, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_QSTR(MP_QSTR_r)} },
    };

    mp_arg_val_t arg_vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, arg_vals);
    return mp_vfs_efs_file_open(NULL, type, arg_vals[0].u_obj, arg_vals[1].u_obj);
}

STATIC mp_obj_t vfs_efs_file_fileno(mp_obj_t self_in) {
    mp_obj_vfs_efs_file_t *self = MP_OBJ_TO_PTR(self_in);
    check_fd_is_open(self);
    return MP_OBJ_NEW_SMALL_INT(self->fd);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(vfs_efs_file_fileno_obj, vfs_efs_file_fileno);

STATIC mp_obj_t vfs_efs_file___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    return mp_stream_close(args[0]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(vfs_efs_file___exit___obj, 4, 4, vfs_efs_file___exit__);

STATIC mp_uint_t vfs_efs_file_read(mp_obj_t o_in, void *buf, mp_uint_t size, int *errcode) {
    mp_obj_vfs_efs_file_t *o = MP_OBJ_TO_PTR(o_in);
    check_fd_is_open(o);
    int r;
	
	r = Helios_fread(buf, size, 1, o->fd);
	if(r < 0)
	{
		*errcode = -r;
		return MP_STREAM_ERROR;
	}

    return (mp_uint_t)r;
}

STATIC mp_uint_t vfs_efs_file_write(mp_obj_t o_in, const void *buf, mp_uint_t size, int *errcode) {
    mp_obj_vfs_efs_file_t *o = MP_OBJ_TO_PTR(o_in);
    check_fd_is_open(o);
    #if MICROPY_PY_OS_DUPTERM
    if (o->fd <= STDERR_FILENO) {
        mp_hal_stdout_tx_strn(buf, size);
        return size;
    }
    #endif
    int r;
	
	r = Helios_fwrite((void *)buf, size, 1, o->fd);
	if(r < 0)
	{
		*errcode = -r;
		return MP_STREAM_ERROR;
	}

    return (mp_uint_t)r;
}

STATIC mp_uint_t vfs_efs_file_ioctl(mp_obj_t o_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    mp_obj_vfs_efs_file_t *o = MP_OBJ_TO_PTR(o_in);
    check_fd_is_open(o);
    switch (request) {
        case MP_STREAM_FLUSH: {
            /*int ret;
            MP_HAL_RETRY_SYSCALL(ret, fsync(o->fd), {
                if (err == EINVAL
                    && (o->fd == STDIN_FILENO || o->fd == STDOUT_FILENO || o->fd == STDERR_FILENO)) {
                    // fsync(stdin/stdout/stderr) may fail with EINVAL, but don't propagate that
                    // error out.  Because data is not buffered by us, and stdin/out/err.flush()
                    // should just be a no-op.
                    return 0;
                }
                *errcode = err;
                return MP_STREAM_ERROR;
            });
            return 0;*/
        }
        case MP_STREAM_SEEK: {
            struct mp_stream_seek_t *s = (struct mp_stream_seek_t *)arg;
			int res = Helios_fseek(o->fd, s->offset, s->whence);
            if (res < 0) {
                *errcode = -res;
                return MP_STREAM_ERROR;
            }

            s->offset = res;
            return 0;
        }
        case MP_STREAM_CLOSE:
			Helios_fclose(o->fd);
            #ifdef MICROPY_CPYTHON_COMPAT
            o->fd = NULL;
            #endif
            return 0;
        case MP_STREAM_GET_FILENO:
            return 0;
        default:
            *errcode = 0;
            return MP_STREAM_ERROR;
    }
}

STATIC const mp_rom_map_elem_t vfs_efs_rawfile_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_fileno), MP_ROM_PTR(&vfs_efs_file_fileno_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&vfs_efs_file___exit___obj) },
};

STATIC MP_DEFINE_CONST_DICT(vfs_efs_rawfile_locals_dict, vfs_efs_rawfile_locals_dict_table);

#if MICROPY_PY_IO_FILEIO
STATIC const mp_stream_p_t vfs_efs_fileio_stream_p = {
    .read = vfs_efs_file_read,
    .write = vfs_efs_file_write,
    .ioctl = vfs_efs_file_ioctl,
};

const mp_obj_type_t mp_type_vfs_efs_fileio = {
    { &mp_type_type },
    .name = MP_QSTR_FileIO,
    .print = vfs_efs_file_print,
    .make_new = vfs_efs_file_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &vfs_efs_fileio_stream_p,
    .locals_dict = (mp_obj_dict_t *)&vfs_efs_rawfile_locals_dict,
};
#endif

STATIC const mp_stream_p_t vfs_efs_textio_stream_p = {
    .read = vfs_efs_file_read,
    .write = vfs_efs_file_write,
    .ioctl = vfs_efs_file_ioctl,
    .is_text = true,
};

const mp_obj_type_t mp_type_vfs_efs_textio = {
    { &mp_type_type },
    .name = MP_QSTR_TextIOWrapper,
    .print = vfs_efs_file_print,
    .make_new = vfs_efs_file_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &vfs_efs_textio_stream_p,
    .locals_dict = (mp_obj_dict_t *)&vfs_efs_rawfile_locals_dict,
};

/*const mp_obj_vfs_efs_file_t mp_sys_stdin_obj = {{&mp_type_vfs_efs_textio}, STDIN_FILENO};
const mp_obj_vfs_efs_file_t mp_sys_stdout_obj = {{&mp_type_vfs_efs_textio}, STDOUT_FILENO};
const mp_obj_vfs_efs_file_t mp_sys_stderr_obj = {{&mp_type_vfs_efs_textio}, STDERR_FILENO};
*/

#endif // PLAT_Qualcomm
