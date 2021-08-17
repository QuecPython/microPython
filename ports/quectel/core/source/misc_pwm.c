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

#include "runtime.h"

#include "helios_pwm.h"

#include "helios_debug.h"

#define	PWM_LOG(msg, ...)      custom_log("pwm", msg, ##__VA_ARGS__)



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
	unsigned int cycle_range;
    unsigned short high_time;
    unsigned short cycle_time;
} misc_pwm_obj_t;

typedef enum HELIOS_PWM_CYCLE_RANGE_ENUM
{
    HELIOS_PWM_CYCLE_ABOVE_1US,
    HELIOS_PWM_CYCLE_ABOVE_1MS,
    HELIOS_PWM_CYCLE_ABOVE_10US,
    HELIOS_PWM_CYCLE_ABOVE_BELOW_US,
} HELIOS_PWM_CYCLE_RANGE_E;


STATIC void misc_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    misc_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "PWM(pin:%u high time:%u cycle_time:%u)", self->pin, self->high_time, self->cycle_time);
}

STATIC void misc_pwm_init_helper(misc_pwm_obj_t *self,
    size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_high_time, ARG_cycle_time };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_high_time, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_cycle_time, MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args,
        MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	PWM_LOG("self->cycle_range = %d\n",self->cycle_range);
   if(self->cycle_range != HELIOS_PWM_CYCLE_ABOVE_1US && self->cycle_range != HELIOS_PWM_CYCLE_ABOVE_1MS && self->cycle_range != HELIOS_PWM_CYCLE_ABOVE_10US && self->cycle_range != HELIOS_PWM_CYCLE_ABOVE_BELOW_US ) {

   		mp_raise_ValueError("invalid cycle_range value, must be in {0:us or 1:ms or 2:10us}");
   }

    // get high time
    if (args[ARG_high_time].u_int > 0 && args[ARG_high_time].u_int < 65535) {
		self->high_time = args[ARG_high_time].u_int;
	} else {
		mp_raise_ValueError("invalid high_time value, must be in {0~65535}");
	}

    // get cycle time
    if (args[ARG_cycle_time].u_int > 0 && args[ARG_cycle_time].u_int < 65535) {
		self->cycle_time = args[ARG_cycle_time].u_int;
	} else {
		mp_raise_ValueError("invalid cycle_time value, must be in {0~65535}");
	}

	if(self->high_time > self->cycle_time) {
		mp_raise_ValueError("invalid high_time value, must less more cycle_time or be equal to cycle_time");
	}

	if(Helios_PWM_Init(self->pin) != 0) {
		mp_raise_ValueError("fail");
	}

}

STATIC mp_obj_t misc_pwm_make_new(const mp_obj_type_t *type,
    size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    unsigned int pin_id = mp_obj_get_int(args[0]);
	unsigned int cycle_range_id = mp_obj_get_int(args[1]);

	PWM_LOG("n_args = %d\n",n_args);

    // create PWM object from the given pin
    misc_pwm_obj_t *self = m_new_obj(misc_pwm_obj_t);
    self->base.type = &misc_pwm_type;
    self->pin = pin_id;
    self->high_time = 0;
    self->cycle_time = 0;
	self->cycle_range = cycle_range_id;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
	misc_pwm_init_helper(self, n_args - 2, args + 2, &kw_args);
    

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

	uint32_t frequency = 0;
	float duty = 0;
	if(self->cycle_range == HELIOS_PWM_CYCLE_ABOVE_1US || self->cycle_range == HELIOS_PWM_CYCLE_ABOVE_10US) {
		frequency = 1000000 /self->cycle_time;
	} else if (self->cycle_range == HELIOS_PWM_CYCLE_ABOVE_1MS) {
		frequency = 1000 /self->cycle_time;
	} else if(self->cycle_range == HELIOS_PWM_CYCLE_ABOVE_BELOW_US) {
		frequency = 1000000000 /self->cycle_time;
	}
	duty = (float)self->high_time / (float)self->cycle_time;

	
	int ret = Helios_PWM_Start(self->pin, frequency, duty);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(misc_pwm_enable_obj, misc_pwm_enable);

STATIC mp_obj_t misc_pwm_disable(mp_obj_t self_in) {
    misc_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);

	int ret = Helios_PWM_Stop(self->pin);
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
#ifdef PLAT_ASR
    { MP_ROM_QSTR(MP_QSTR_PWM1), MP_ROM_INT(PWM1) },
    { MP_ROM_QSTR(MP_QSTR_PWM2), MP_ROM_INT(PWM2) },
    { MP_ROM_QSTR(MP_QSTR_PWM3), MP_ROM_INT(PWM3) },
#endif   
    { MP_ROM_QSTR(MP_QSTR_ABOVE_1US), MP_ROM_INT(HELIOS_PWM_CYCLE_ABOVE_1US) },
    { MP_ROM_QSTR(MP_QSTR_ABOVE_MS), MP_ROM_INT(HELIOS_PWM_CYCLE_ABOVE_1MS) },
    { MP_ROM_QSTR(MP_QSTR_ABOVE_10US), MP_ROM_INT(HELIOS_PWM_CYCLE_ABOVE_10US) },
    { MP_ROM_QSTR(MP_QSTR_ABOVE_BELOW_US), MP_ROM_INT(HELIOS_PWM_CYCLE_ABOVE_BELOW_US) },
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
