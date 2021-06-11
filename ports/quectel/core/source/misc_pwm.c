/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
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

#include "runtime.h"

#include "helios_pwm.h"


// Forward dec'l
const mp_obj_type_t misc_pwm_type;

typedef enum
{
	PWM0 = 0,
	PWM1 = 1,
	PWM2 = 2,
	PWM3 = 3,
}PWMn;


typedef struct _misc_pwm_obj_t {
    mp_obj_base_t base;
    unsigned int pin;
    unsigned short period;
    unsigned short duty;
} misc_pwm_obj_t;

STATIC void misc_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    misc_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "PWM(pin:%u high time:%u duty:%u)", self->pin, self->period, self->duty);
}

STATIC void misc_pwm_init_helper(misc_pwm_obj_t *self,
    size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_period, ARG_duty };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_period, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_duty, MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args,
        MP_ARRAY_SIZE(allowed_args), allowed_args, args);
		

    // get high time
    if (args[ARG_period].u_int > 0 && args[ARG_period].u_int < 65535) {
		self->period = args[ARG_period].u_int;
	} else {
		mp_raise_ValueError("invalid period value, must be in {0~65535}");
	}

    // get cycle time
    if (args[ARG_duty].u_int > 0 && args[ARG_duty].u_int < 65535) {
		self->duty = args[ARG_duty].u_int;
	} else {
		mp_raise_ValueError("invalid duty value, must be in {0~65535}");
	}
	
	Helios_PWM_Init((Helios_PwnNum) self->pin);
}

STATIC mp_obj_t misc_pwm_make_new(const mp_obj_type_t *type,
    size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    unsigned int pin_id = mp_obj_get_int(args[0]);

    // create PWM object from the given pin
    misc_pwm_obj_t *self = m_new_obj(misc_pwm_obj_t);
    self->base.type = &misc_pwm_type;
    self->pin = pin_id;
    self->period = 0;
    self->duty = 0;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    misc_pwm_init_helper(self, n_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t misc_pwm_init(size_t n_args,
    const mp_obj_t *args, mp_map_t *kw_args) {
    misc_pwm_init_helper(args[0], n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(misc_pwm_init_obj, 1, misc_pwm_init);

STATIC mp_obj_t misc_pwm_enable(mp_obj_t self_in) {
    misc_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);

	int ret = Helios_PWM_Start((Helios_PwnNum) self->pin, (uint32_t) self->period, (uint32_t) self->duty);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(misc_pwm_enable_obj, misc_pwm_enable);

STATIC mp_obj_t misc_pwm_disable(mp_obj_t self_in) {
    misc_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);

	int ret = Helios_PWM_Stop((Helios_PwnNum) self->pin);
	if (ret != 0)
	{
		ret = -1;
	}

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(misc_pwm_disable_obj, misc_pwm_disable);

STATIC const mp_rom_map_elem_t misc_pwm_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&misc_pwm_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&misc_pwm_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&misc_pwm_disable_obj) },
    { MP_ROM_QSTR(MP_QSTR_PWM0), MP_ROM_INT(PWM0) },
    { MP_ROM_QSTR(MP_QSTR_PWM1), MP_ROM_INT(PWM1) },
    { MP_ROM_QSTR(MP_QSTR_PWM2), MP_ROM_INT(PWM2) },
    { MP_ROM_QSTR(MP_QSTR_PWM3), MP_ROM_INT(PWM3) },
};

STATIC MP_DEFINE_CONST_DICT(misc_pwm_locals_dict,
    misc_pwm_locals_dict_table);

const mp_obj_type_t misc_pwm_type = {
    { &mp_type_type },
    .name = MP_QSTR_PWM,
    .print = misc_pwm_print,
    .make_new = misc_pwm_make_new,
    .locals_dict = (mp_obj_dict_t *)&misc_pwm_locals_dict,
};
