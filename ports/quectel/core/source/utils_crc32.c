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

