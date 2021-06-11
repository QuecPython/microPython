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
#include <stdint.h>
#include "stdlib.h"
#include "mpconfigport.h"
#include "obj.h"
#include "compile.h"
#include "runtime.h"
#include "repl.h"
#include "mperrno.h"
#include "modutils.h"

extern unsigned long crc32(unsigned long, const unsigned char *, unsigned int);

typedef struct _crc32_obj_t {
	mp_obj_base_t base;
}crc32_obj_t;

const mp_obj_type_t mp_crc32_type;

STATIC mp_obj_t crc32_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
	crc32_obj_t *self = m_new_obj_with_finaliser(crc32_obj_t);
	
	self->base.type = &mp_crc32_type;
	
	return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t crc32_update(mp_obj_t self, mp_obj_t crc32_init, mp_obj_t data)
{
	unsigned long crc32_in = 0, crc32_out = 0;
    mp_buffer_info_t bufinfo;
    // char str[10] = {0};
    
    crc32_in = (unsigned long)mp_obj_get_int_truncated(crc32_init);
    mp_get_buffer_raise(data, &bufinfo, MP_BUFFER_READ);
    crc32_out = crc32(crc32_in, bufinfo.buf, bufinfo.len);
    return mp_obj_new_int_from_uint((int)crc32_out);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_3(crc32_update_obj, crc32_update);

STATIC const mp_rom_map_elem_t utils_crc32_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&crc32_update_obj) },
};

STATIC MP_DEFINE_CONST_DICT(utils_crc32_locals_dict, utils_crc32_locals_dict_table);

const mp_obj_type_t mp_crc32_type = {
    { &mp_type_type },
    .name = MP_QSTR_crc32,
    .make_new = crc32_make_new,
    .locals_dict = (mp_obj_t)&utils_crc32_locals_dict,
};

