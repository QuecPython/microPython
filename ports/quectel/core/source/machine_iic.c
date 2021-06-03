/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014, 2015 Damien P. George
 * Copyright (c) 2019, NXP
 * Copyright (c) 2020, QUECTEL
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
#include <string.h>

#include "runtime.h"
#include "gc.h"
#include "mphal.h"
#include "mperrno.h"
#include "machine_i2c.h"

#include "helios_iic.h"

const mp_obj_type_t machine_hard_i2c_type;

typedef struct _machine_hard_i2c_obj_t {
    mp_obj_base_t base;
	uint32_t bus_id;
	uint32_t fastmode;
} machine_hard_i2c_obj_t;

STATIC void machine_hard_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hard_i2c_obj_t *self = self_in;
    mp_printf(print, "I2C%d", self->bus_id);
}

mp_obj_t machine_hard_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_fastmode };
    static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_fastmode, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 1} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	uint32_t bus_id = args[ARG_id].u_int;
	uint32_t fastmode = args[ARG_fastmode].u_int;
    

    if (bus_id > HELIOS_I2C3) {
       mp_raise_ValueError("bus id must be (0~3)");
    }
   
    machine_hard_i2c_obj_t *self = m_new_obj(machine_hard_i2c_obj_t);

    self->base.type = &machine_hard_i2c_type;
	self->bus_id = bus_id;
	self->fastmode = fastmode;
	if(0 != Helios_I2C_Init((Helios_I2CEnum) self->bus_id, (Helios_I2CMode) self->fastmode)) {
		return mp_const_false;
	}
    return MP_OBJ_FROM_PTR(self);
}
STATIC const mp_arg_t machine_i2c_mem_allowed_args[] = {
    { MP_QSTR_slaveaddr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_regaddr, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_regaddr_len, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_databuf,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_datasize, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 8} },
};
STATIC mp_obj_t machine_i2c_write_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_slaveaddr, ARG_regaddr, ARG_regaddr_len, ARG_databuf, ARG_datasize };
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_mem_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(machine_i2c_mem_allowed_args), machine_i2c_mem_allowed_args, args);
		
	machine_hard_i2c_obj_t *self = (machine_hard_i2c_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);

	mp_buffer_info_t regaddr_bufinfo;
    mp_get_buffer_raise(args[ARG_regaddr].u_obj, &regaddr_bufinfo, MP_BUFFER_READ);
	int regaddr_length = args[ARG_regaddr_len].u_int;

    // get the buffer to write the data from
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_databuf].u_obj, &bufinfo, MP_BUFFER_READ);
	int length = (size_t)args[ARG_datasize].u_int > bufinfo.len ? bufinfo.len : (size_t)args[ARG_datasize].u_int;
	//uart_printf("i2c wirte: busid:%x slaveddr:%x, regaddr:%x, data:%s datalen:%d\n", self->bus_id, args[ARG_slaveaddr].u_int, args[ARG_regaddr].u_int, 
	//							bufinfo.buf, args[ARG_datasize].u_int);
    // do the transfer
	int ret = HELIOS_I2C_Write((Helios_I2CEnum) self->bus_id, (uint8_t) args[ARG_slaveaddr].u_int, (uint8_t*) regaddr_bufinfo.buf,(size_t) regaddr_length,(void*) bufinfo.buf, (size_t) length);
    if (ret < 0) {
        ret = -1;
        return mp_obj_new_int(ret);
    }

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_write_obj, 1, machine_i2c_write_mem);

STATIC const mp_arg_t machine_i2c_mem_allowed_read_args[] = {
    { MP_QSTR_slaveaddr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_regaddr, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_regaddr_len, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_databuf,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_datasize, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 8} },
    { MP_QSTR_dalay, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 8} },
};
STATIC mp_obj_t machine_i2c_read_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_slaveaddr, ARG_regaddr, ARG_regaddr_len, ARG_databuf, ARG_datasize, ARG_dalay };
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_mem_allowed_read_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(machine_i2c_mem_allowed_read_args), machine_i2c_mem_allowed_read_args, args);
		
	machine_hard_i2c_obj_t *self = (machine_hard_i2c_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);

	mp_buffer_info_t regaddr_bufinfo;
    mp_get_buffer_raise(args[ARG_regaddr].u_obj, &regaddr_bufinfo, MP_BUFFER_READ);
	int regaddr_length = args[ARG_regaddr_len].u_int;

    // get the buffer to read the data into
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_databuf].u_obj, &bufinfo, MP_BUFFER_WRITE);
	
	int length = (size_t)args[ARG_datasize].u_int > bufinfo.len ? bufinfo.len : (size_t)args[ARG_datasize].u_int;
	
	//uart_printf("i2c read: busid:%x slaveddr:%x, regaddr:%x, datalen:%d\n", self->bus_id, args[ARG_slaveaddr].u_int, args[ARG_regaddr].u_int, 
	//							args[ARG_datasize].u_int);
    // do the transfer	
	int ret = HELIOS_I2C_Read((Helios_I2CEnum) self->bus_id, (uint8_t) args[ARG_slaveaddr].u_int,(uint8_t*) regaddr_bufinfo.buf, (size_t) regaddr_length, (void*) bufinfo.buf, (size_t) length,(uint32_t) args[ARG_dalay].u_int);
	if (ret < 0) {
        ret = -1;
        return mp_obj_new_int(ret);
    }

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_read_obj, 1, machine_i2c_read_mem);

STATIC const mp_rom_map_elem_t machine_i2c_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&machine_i2c_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&machine_i2c_write_obj) },
#if 1	
	// class constants
    { MP_ROM_QSTR(MP_QSTR_I2C0),       MP_ROM_INT(HELIOS_I2C0) },
    { MP_ROM_QSTR(MP_QSTR_I2C1),       MP_ROM_INT(HELIOS_I2C1) },
    { MP_ROM_QSTR(MP_QSTR_I2C2),       MP_ROM_INT(HELIOS_I2C2) },
    { MP_ROM_QSTR(MP_QSTR_I2C3),       MP_ROM_INT(HELIOS_I2C3) },
    { MP_ROM_QSTR(MP_QSTR_STANDARD_MODE),       MP_ROM_INT(HELIOS_STANDARD_MODE) },
    { MP_ROM_QSTR(MP_QSTR_FAST_MODE),       MP_ROM_INT(HELIOS_FAST_MODE) },
#endif
};

MP_DEFINE_CONST_DICT(mp_machine_hard_i2c_locals_dict, machine_i2c_locals_dict_table);

const mp_obj_type_t machine_hard_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = machine_hard_i2c_print,
    .make_new = machine_hard_i2c_make_new,
    .locals_dict = (mp_obj_dict_t *)&mp_machine_hard_i2c_locals_dict,
};
