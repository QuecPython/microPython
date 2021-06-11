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

#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "extmod/vfs.h"
//#include "vfs_fat.h"
#include "extmod/vfs_lfs.h"
#include "mpversion.h"
#include "helios_dev.h"


static char sysname[20] = {0};
static char nodename[20] = {0};
static char machine[30] = {0};
static char qpy_version[20] = {0};

extern const mp_obj_type_t mp_fat_vfs_type;


STATIC const qstr os_uname_info_fields[] = {
    MP_QSTR_sysname, MP_QSTR_nodename,MP_QSTR_release,
    MP_QSTR_version, MP_QSTR_machine, MP_QSTR_qpyver,
};

STATIC  MP_DEFINE_STR_OBJ(os_uname_info_sysname_obj, sysname);
STATIC  MP_DEFINE_STR_OBJ(os_uname_info_nodename_obj, nodename);
STATIC  MP_DEFINE_STR_OBJ(os_uname_info_release_obj, MICROPY_VERSION_STRING);
STATIC  MP_DEFINE_STR_OBJ(os_uname_info_version_obj, MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE);
STATIC  MP_DEFINE_STR_OBJ(os_uname_info_machine_obj, machine);
STATIC  MP_DEFINE_STR_OBJ(os_uname_info_qpyver_obj, qpy_version);

STATIC MP_DEFINE_ATTRTUPLE(
    os_uname_info_obj,
    os_uname_info_fields,
    6,
    (mp_obj_t)&os_uname_info_sysname_obj,
    (mp_obj_t)&os_uname_info_nodename_obj,
    (mp_obj_t)&os_uname_info_release_obj,
    (mp_obj_t)&os_uname_info_version_obj,
    (mp_obj_t)&os_uname_info_machine_obj,
    (mp_obj_t)&os_uname_info_qpyver_obj
    );

STATIC mp_obj_t os_uname2(void) {
	Helios_Dev_GetProductName((void *)sysname, sizeof(sysname));
    Helios_Dev_GetModel((void *)nodename, sizeof(nodename));
	Helios_Dev_GetQpyVersion((void *)qpy_version, sizeof(qpy_version));
	snprintf(machine, sizeof(machine), "%s with QUECTEL", nodename);
	
	os_uname_info_sysname_obj.len = strlen(sysname);
	os_uname_info_nodename_obj.len = strlen(nodename);
	os_uname_info_machine_obj.len = strlen(machine);
	os_uname_info_qpyver_obj.len = strlen(qpy_version);
	return (mp_obj_t)&os_uname_info_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(os_uname2_obj, os_uname2);


STATIC mp_obj_t os_uname(void) {
	char sysname[40] = {0};
	char nodname[20] = {0};
	char release[20] = {0};
	char machine[30] = {0};
	char version[128] = {0};
	char qpy_ver[20] = {0};

    char mob_usb_product[64] = {0};
    char mob_model_id[64] = {0};
    char _qpy_ver[20] = {0};

    Helios_Dev_GetProductName((void *)mob_usb_product, sizeof(mob_usb_product));
    Helios_Dev_GetModel((void *)mob_model_id, sizeof(mob_model_id));
    Helios_Dev_GetQpyVersion((void *)_qpy_ver, sizeof(_qpy_ver));

	snprintf(sysname, sizeof(sysname), "sysname=%s", mob_usb_product);
	snprintf(nodname, sizeof(nodname), "nodename=%s", mob_model_id);
	snprintf(release, sizeof(release), "release=%s", MICROPY_VERSION_STRING);
	snprintf(version, sizeof(version), "version=%s on %s", MICROPY_GIT_TAG, MICROPY_BUILD_DATE);
	snprintf(machine, sizeof(machine), "machine=%s with QUECTEL", mob_model_id);
	snprintf(qpy_ver, sizeof(qpy_ver), "qpyver=%s", _qpy_ver);

	mp_obj_t tuple[6] = {
		mp_obj_new_str(sysname, strlen(sysname)), 
		mp_obj_new_str(nodname, strlen(nodname)), 
		mp_obj_new_str(release, strlen(release)),
		mp_obj_new_str(version, strlen(version)), 
		mp_obj_new_str(machine, strlen(machine)), 
		mp_obj_new_str(qpy_ver, strlen(qpy_ver)),
	};
			
    return mp_obj_new_tuple(6, tuple);
}


STATIC MP_DEFINE_CONST_FUN_OBJ_0(os_uname_obj, os_uname);

STATIC mp_obj_t os_urandom(mp_obj_t num) {
    mp_int_t n = mp_obj_get_int(num);
    vstr_t vstr;
    vstr_init_len(&vstr, n);
    uint32_t r = 0;
    for (int i = 0; i < n; i++) {
        if ((i & 3) == 0) {
            r = rand(); // returns 32-bit hardware random number
        }
        vstr.buf[i] = r;
        r >>= 8;
    }
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(os_urandom_obj, os_urandom);

#if MICROPY_PY_OS_DUPTERM
STATIC mp_obj_t os_dupterm_notify(mp_obj_t obj_in) {
    (void)obj_in;
    for (;;) {
        int c = mp_uos_dupterm_rx_chr();
        if (c < 0) {
            break;
        }
        ringbuf_put(&stdin_ringbuf, c);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(os_dupterm_notify_obj, os_dupterm_notify);
#endif

STATIC const mp_rom_map_elem_t os_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_uos) },
    { MP_ROM_QSTR(MP_QSTR_uname), MP_ROM_PTR(&os_uname_obj) },
	{ MP_ROM_QSTR(MP_QSTR_uname2), MP_ROM_PTR(&os_uname2_obj) },
    { MP_ROM_QSTR(MP_QSTR_urandom), MP_ROM_PTR(&os_urandom_obj) },
    #if MICROPY_PY_OS_DUPTERM
    { MP_ROM_QSTR(MP_QSTR_dupterm), MP_ROM_PTR(&mp_uos_dupterm_obj) },
    { MP_ROM_QSTR(MP_QSTR_dupterm_notify), MP_ROM_PTR(&os_dupterm_notify_obj) },
    #endif
    #if MICROPY_VFS
    { MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&mp_vfs_ilistdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_listdir), MP_ROM_PTR(&mp_vfs_listdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&mp_vfs_mkdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&mp_vfs_rmdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&mp_vfs_chdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&mp_vfs_getcwd_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&mp_vfs_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&mp_vfs_rename_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&mp_vfs_stat_obj) },
    { MP_ROM_QSTR(MP_QSTR_statvfs), MP_ROM_PTR(&mp_vfs_statvfs_obj) },
    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&mp_vfs_mount_obj) },
    { MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&mp_vfs_umount_obj) },
    #if MICROPY_VFS_FAT
    { MP_ROM_QSTR(MP_QSTR_VfsFat), MP_ROM_PTR(&mp_fat_vfs_type) },
    #endif
    #if MICROPY_VFS_LFS1
    { MP_ROM_QSTR(MP_QSTR_VfsLfs1), MP_ROM_PTR(&mp_type_vfs_lfs1) },
    #endif
    #if MICROPY_VFS_LFS2
    { MP_ROM_QSTR(MP_QSTR_VfsLfs2), MP_ROM_PTR(&mp_type_vfs_lfs2) },
    #endif
    #endif
};

STATIC MP_DEFINE_CONST_DICT(os_module_globals, os_module_globals_table);

const mp_obj_module_t uos_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&os_module_globals,
};

