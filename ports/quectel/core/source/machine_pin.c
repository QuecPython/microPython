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
#include <string.h>

#include "runtime.h"
#include "gc.h"
#include "mphal.h"

#include "helios_gpio.h"
#include "helios_debug.h"

#define HELIOS_PIN_LOG(msg, ...)      custom_log("machine pin", msg, ##__VA_ARGS__)
//#define HELIOS_PIN_LOG(msg, ...)      QL_LOG_PUSH("machine pin", msg, ##__VA_ARGS__)



typedef struct _machine_pin_obj_t {
    mp_obj_base_t base;
    uint  pin;
	uint  dir;
	uint  pull;
	uint  value;
} machine_pin_obj_t;

const mp_obj_type_t machine_pin_type;


enum {
	GPIO0 = 0,
	GPIO1,
	GPIO2,
	GPIO3,
	GPIO4,
	GPIO5,
	GPIO6,
	GPIO7,
	GPIO8,
	GPIO9,
	GPIO10,
	GPIO11,
	GPIO12,
	GPIO13,
	GPIO14,
	GPIO15,
	GPIO16,
	GPIO17,
	GPIO18,
	GPIO19,
	GPIO20,
	GPIO21,
	GPIO22,
	GPIO23,
	GPIO24,
	GPIO25,
	GPIO26,
	GPIO27,
	GPIO28,
	GPIO29,
	GPIO30,
	GPIO31,
	GPIOMAX,
};

const mp_obj_type_t machine_pin_type;

struct pin_map_t{
	int export_pin;
	int internal_pin;
};

static struct pin_map_t pin_map[] = {
	{GPIO0,HELIOS_GPIO0},
	{GPIO1,HELIOS_GPIO1},
	{GPIO2,HELIOS_GPIO2},
	{GPIO3,HELIOS_GPIO3},
	{GPIO4,HELIOS_GPIO4},
	{GPIO5,HELIOS_GPIO5},
	{GPIO6,HELIOS_GPIO6},
	{GPIO7,HELIOS_GPIO7},
	{GPIO8,HELIOS_GPIO8},
	{GPIO9,HELIOS_GPIO9},
	{GPIO10,HELIOS_GPIO10},
	{GPIO11,HELIOS_GPIO11},
	{GPIO12,HELIOS_GPIO12},
	{GPIO13,HELIOS_GPIO13},
	{GPIO14,HELIOS_GPIO14},
	{GPIO15,HELIOS_GPIO15},
	{GPIO16,HELIOS_GPIO16},
	{GPIO17,HELIOS_GPIO17},
	{GPIO18,HELIOS_GPIO18},
	{GPIO19,HELIOS_GPIO19},
	{GPIO20,HELIOS_GPIO20},
	{GPIO21,HELIOS_GPIO21},
	{GPIO22,HELIOS_GPIO22},
	{GPIO23,HELIOS_GPIO23},
	{GPIO24,HELIOS_GPIO24},
	{GPIO25,HELIOS_GPIO25},
	{GPIO26,HELIOS_GPIO26},
	{GPIO27,HELIOS_GPIO27},
	{GPIO28,HELIOS_GPIO28},
	{GPIO29,HELIOS_GPIO29},
	{GPIO30,HELIOS_GPIO30},
	{GPIO31,HELIOS_GPIO31},
	{GPIOMAX,HELIOS_GPIOMAX},

};



STATIC machine_pin_obj_t machine_pin_obj[HELIOS_GPIOMAX] = {
	{{&machine_pin_type}, HELIOS_GPIO0, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO1, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO2, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO3, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO4, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO5, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO6, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO7, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO8, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO9, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO10, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO11, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO12, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO13, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO14, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO15, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO16, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO17, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO18, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO19, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO20, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO21, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO22, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO23, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO24, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO25, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO26, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO27, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO28, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO29, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO30, 0, 0, 0},
	{{&machine_pin_type}, HELIOS_GPIO31, 0, 0, 0},
};


STATIC void machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pin_obj_t *self = self_in;
    mp_printf(print, "<Pin(%u)>", self->pin);
}

// pin.init(mode, pull=None, *, value)
STATIC mp_obj_t machine_pin_obj_init_helper(machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_dir, ARG_pull, ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_dir, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_pull, MP_ARG_INT, {.u_int = -1}},
        { MP_QSTR_value, MP_ARG_INT, {.u_int = -1}},
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	// get dir mode
	switch (args[ARG_dir].u_int) {
		case -1:
			break;
		case HELIOS_GPIO_INPUT:
		case HELIOS_GPIO_OUTPUT:
			self->dir = args[ARG_dir].u_int;
			break;
		default:
			mp_raise_ValueError("invalid pin dir, range{0:dir in, 1:dir out}");
			break;
	}
    // get pull mode
	switch (args[ARG_pull].u_int) {
		case -1:
			break;
		case HELIOS_PULL_NONE:
		case HELIOS_PULL_UP:
		case HELIOS_PULL_DOWN:
			self->pull = args[ARG_pull].u_int;
			break;
		default:
			mp_raise_ValueError("invalid pin pull, range{0:PIN_PULL_DISABLE, 1:PIN_PULL_PU, 2:PIN_PULL_PD}");
			break;
	}
	// get initial value
	switch (args[ARG_value].u_int) {
		case -1:
			break;
		case HELIOS_LVL_LOW:
		case HELIOS_LVL_HIGH:
			self->value = args[ARG_value].u_int;
			break;
		default:
			mp_raise_ValueError("invalid pin value, range{0:PIN_LEVEL_LOW, 1:PIN_LEVEL_HIGH}");
			break;
	}
	Helios_GPIOInitStruct gpio_struct = {0};
	gpio_struct.dir = self->dir;
	gpio_struct.pull = self->pull;
	gpio_struct.value = self->value;
	HELIOS_PIN_LOG("pin = %d dir = %d, pull = %d value=%d\n",self->pin, self->dir, self->pull, self->value);
	int ret = Helios_GPIO_Init((Helios_GPIONum) self->pin, &gpio_struct);
	HELIOS_PIN_LOG("gpio init ret = %d\n",ret);
	
    return mp_const_none;
}

// constructor(drv_name, pin, ...)
mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
	Helios_Debug_Enable();

	// get the wanted pin object
	unsigned int i = 0;
	int tmp_pin = 0,wanted_pin = -1;
	tmp_pin = mp_obj_get_int(args[0]);
	//gpio map from export to inter
	for (i=0;i< sizeof(pin_map);i++){
		if(pin_map[i].export_pin == tmp_pin){
			wanted_pin=pin_map[i].internal_pin;
			break;
		}
	}
	if(wanted_pin == -1) {
		mp_raise_ValueError("invalid pin");
	}

	HELIOS_PIN_LOG("wanted_pin = %d\n",wanted_pin);
	machine_pin_obj_t *self = NULL;
    if (HELIOS_GPIO0 <= wanted_pin && wanted_pin < HELIOS_GPIOMAX) {
        self = (machine_pin_obj_t *)&machine_pin_obj[wanted_pin];
    }
    if (self == NULL || self->base.type == NULL) {
        mp_raise_ValueError("invalid pin");
    }
	
	// default settings
	self->dir = HELIOS_GPIO_OUTPUT;
	self->pull = HELIOS_PULL_NONE;
	self->value = HELIOS_LVL_LOW;
	HELIOS_PIN_LOG("dir = %d, pull = %d value=%d\n",self->dir, self->pull, self->value);

    if (n_args > 1 || n_kw > 0) {
        // pin mode given, so configure this GPIO
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
		HELIOS_PIN_LOG("n_args = %d\n",n_args);
        machine_pin_obj_init_helper(self, n_args - 1, args + 1, &kw_args);
		HELIOS_PIN_LOG("n_kw = %d\n",n_kw);
    } else {
		
		Helios_GPIOInitStruct gpio_struct = {0};
		gpio_struct.dir = self->dir;
		gpio_struct.pull = self->pull;
		gpio_struct.value = self->value;
		Helios_GPIO_Init((Helios_GPIONum) self->pin, &gpio_struct);
	}
	
	HELIOS_PIN_LOG("pin init end\n");
    return MP_OBJ_FROM_PTR(self);
}

// fast method for getting/setting pin value
STATIC mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    machine_pin_obj_t *self = self_in;
    if (n_args == 0) {
		int pin_val = Helios_GPIO_GetLevel((Helios_GPIONum) self->pin);
        return MP_OBJ_NEW_SMALL_INT(pin_val);
    } else {
		Helios_GPIO_SetLevel((Helios_GPIONum) self->pin, (Helios_LvlMode) mp_obj_is_true(args[0]));
        return mp_const_none;
    }
}

// pin.init(mode, pull)
STATIC mp_obj_t machine_pin_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_pin_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 1, machine_pin_obj_init);

// pin.value([value])
STATIC mp_obj_t machine_pin_value(size_t n_args, const mp_obj_t *args) {
    return machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, machine_pin_value);

STATIC mp_obj_t machine_pin_off(mp_obj_t self_in) {
    machine_pin_obj_t *self = self_in;
	Helios_GPIO_SetLevel((Helios_GPIONum) self->pin, (Helios_LvlMode) HELIOS_LVL_LOW);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_off_obj, machine_pin_off);

STATIC mp_obj_t machine_pin_on(mp_obj_t self_in) {
    machine_pin_obj_t *self = self_in;
	Helios_GPIO_SetLevel((Helios_GPIONum) self->pin, (Helios_LvlMode) HELIOS_LVL_HIGH);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_on_obj, machine_pin_on);

STATIC mp_obj_t machine_pin_write(mp_obj_t self_in, mp_obj_t value) 
{
    machine_pin_obj_t *self = self_in;
    int pin_value = mp_obj_get_int(value);
	int ret = Helios_GPIO_SetLevel((Helios_GPIONum) self->pin, (Helios_LvlMode) pin_value);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_pin_write_obj, machine_pin_write);

STATIC mp_obj_t machine_pin_read(size_t n_args, const mp_obj_t *args) 
{
    return machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_read_obj, 1, 2, machine_pin_read);


STATIC mp_uint_t machine_pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    (void)errcode;
    machine_pin_obj_t *self = self_in;

    switch (request) {
        case MP_PIN_READ: {
			int pin_val = Helios_GPIO_GetLevel((Helios_GPIONum) self->pin);
            return pin_val;
        }
        case MP_PIN_WRITE: {
			Helios_GPIO_SetLevel((Helios_GPIONum) self->pin, (Helios_LvlMode) arg);
            return 0;
        }
    }
    return -1;
}

STATIC const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init),    MP_ROM_PTR(&machine_pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value),   MP_ROM_PTR(&machine_pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_off),     MP_ROM_PTR(&machine_pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_on),      MP_ROM_PTR(&machine_pin_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_write),   MP_ROM_PTR(&machine_pin_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),    MP_ROM_PTR(&machine_pin_read_obj) },
    // class constants 
    //GPIO DEFINE
    { MP_ROM_QSTR(MP_QSTR_IN),        MP_ROM_INT(HELIOS_GPIO_INPUT) },
    { MP_ROM_QSTR(MP_QSTR_OUT),       MP_ROM_INT(HELIOS_GPIO_OUTPUT) },
    { MP_ROM_QSTR(MP_QSTR_PULL_PU),   MP_ROM_INT(HELIOS_PULL_UP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_PD),   MP_ROM_INT(HELIOS_PULL_DOWN) },
	{ MP_ROM_QSTR(MP_QSTR_PULL_DISABLE), MP_ROM_INT(HELIOS_PULL_NONE) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO0), MP_ROM_INT(HELIOS_GPIO0) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO1), MP_ROM_INT(HELIOS_GPIO1) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO2), MP_ROM_INT(HELIOS_GPIO2) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO3), MP_ROM_INT(HELIOS_GPIO3) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO4), MP_ROM_INT(HELIOS_GPIO4) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO5), MP_ROM_INT(HELIOS_GPIO5) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO6), MP_ROM_INT(HELIOS_GPIO6) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO7), MP_ROM_INT(HELIOS_GPIO7) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO8), MP_ROM_INT(HELIOS_GPIO8) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO9), MP_ROM_INT(HELIOS_GPIO9) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO10), MP_ROM_INT(HELIOS_GPIO10) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO11), MP_ROM_INT(HELIOS_GPIO11) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO12), MP_ROM_INT(HELIOS_GPIO12) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO13), MP_ROM_INT(HELIOS_GPIO13) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO14), MP_ROM_INT(HELIOS_GPIO14) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO15), MP_ROM_INT(HELIOS_GPIO15) },
#if 0
	{ MP_ROM_QSTR(MP_QSTR_GPIO16), MP_ROM_INT(GPIO16) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO17), MP_ROM_INT(GPIO17) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO18), MP_ROM_INT(GPIO18) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO19), MP_ROM_INT(GPIO19) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO20), MP_ROM_INT(GPIO20) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO21), MP_ROM_INT(GPIO21) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO22), MP_ROM_INT(GPIO22) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO23), MP_ROM_INT(GPIO23) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO24), MP_ROM_INT(GPIO24) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO25), MP_ROM_INT(GPIO25) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO26), MP_ROM_INT(GPIO26) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO27), MP_ROM_INT(GPIO27) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO28), MP_ROM_INT(GPIO28) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO29), MP_ROM_INT(GPIO29) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO30), MP_ROM_INT(GPIO30) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO31), MP_ROM_INT(GPIO31) },
#endif
};

STATIC MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);

STATIC const mp_pin_p_t machine_pin_pin_p = {
    .ioctl = machine_pin_ioctl,
};

const mp_obj_type_t machine_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_Pin,
    .print = machine_pin_print,
    .make_new = mp_pin_make_new,
    .call = machine_pin_call,
    .protocol = &machine_pin_pin_p,
    .locals_dict = (mp_obj_t)&machine_pin_locals_dict,
};
