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

#ifndef _VFS_EFS_H_
#define _VFS_EFS_H_

#if defined(PLAT_Qualcomm)
#include "py/lexer.h"
#include "py/obj.h"

extern const mp_obj_type_t mp_type_vfs_efs;
extern const mp_obj_type_t mp_type_vfs_efs_fileio;
extern const mp_obj_type_t mp_type_vfs_efs_textio;

#define EFS_DIR_PATH_ROOT "/datatx/usr"

typedef struct _mp_obj_vfs_efs_t {
    mp_obj_base_t base;
	vstr_t cur_dir;
    vstr_t root;
    size_t root_len;
    bool readonly;
} mp_obj_vfs_efs_t;

mp_obj_t mp_vfs_efs_file_open(mp_obj_t self_in, const mp_obj_type_t *type, mp_obj_t file_in, mp_obj_t mode_in);

#endif // PLAT_Qualcomm

#endif // _VFS_EFS_H_
