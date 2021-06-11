/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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
#include <stddef.h>
#include <string.h>

#include "runtime.h"
#include "gc.h"
#include "mphal.h"

#include "helios_extint.h"

extern int Helios_GPIO_GetLevel(Helios_GPIONUM gpio_num);

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

typedef struct {
    mp_obj_base_t base;
    mp_int_t line;
	mp_int_t mode;
	mp_int_t pull;
	mp_obj_t callback;
} extint_obj_t;

typedef void(*eint_handler_t)(void);

static extint_obj_t extint_obj[HELIOS_GPIOMAX];

enum {
	HELIOS_EXTINT_RISING,
	HELIOS_EXTINT_FALLING,
};

/*
** Jayceon-20200908:
** Replace function mp_call_function_1_protected() with mp_sched_schedule to slove the dump problem.
*/
#define HANDLER_FUN(X) 															\
static void handler##X(void)													\
{	\
	int edge = HELIOS_EXTINT_RISING;											\
	if (extint_obj[X].callback != mp_const_none) {								\
		if(Helios_GPIO_GetLevel((Helios_GPIONUM)X) == 0) 						\
			{edge = HELIOS_EXTINT_FALLING;}										\
		mp_obj_t extint_list[2] = {												\
				mp_obj_new_int(X),												\
				mp_obj_new_int(edge),											\
			};																	\
		mp_sched_schedule(extint_obj[X].callback,  mp_obj_new_list(2, extint_list));	\
	}																			\
	Helios_ExtInt_Enable(extint_obj[X].line);                    \
} 


HANDLER_FUN(0)
HANDLER_FUN(1)
HANDLER_FUN(2)
HANDLER_FUN(3)
HANDLER_FUN(4)
HANDLER_FUN(5)
HANDLER_FUN(6)
HANDLER_FUN(7)
HANDLER_FUN(8)
HANDLER_FUN(9)
HANDLER_FUN(10)
HANDLER_FUN(11)
HANDLER_FUN(12)
HANDLER_FUN(13)
HANDLER_FUN(14)
HANDLER_FUN(15)
HANDLER_FUN(16)
HANDLER_FUN(17)
HANDLER_FUN(18)
HANDLER_FUN(19)
HANDLER_FUN(20)
HANDLER_FUN(21)
HANDLER_FUN(22)
HANDLER_FUN(23)
HANDLER_FUN(24)
HANDLER_FUN(25)
HANDLER_FUN(26)
HANDLER_FUN(27)
HANDLER_FUN(28)
HANDLER_FUN(29)
HANDLER_FUN(30)
HANDLER_FUN(31)



eint_handler_t eint_handler[HELIOS_GPIOMAX] = {
	handler0, handler1, handler2, handler3, handler4, handler5, handler6, handler7, handler8, handler9,
	handler10, handler11, handler12, handler13, handler14, handler15, handler16, handler17, handler18, handler19,
	handler20, handler21, handler22, handler23, handler24, handler25, handler26, handler27, handler28, handler29,
	handler30, handler31,
};



/// \method line()
/// Return the line number that the pin is mapped to.
STATIC mp_obj_t extint_obj_line(mp_obj_t self_in) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(self->line);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(extint_obj_line_obj, extint_obj_line);

/// \method enable()
/// Enable a disabled interrupt.
STATIC mp_obj_t extint_obj_enable(mp_obj_t self_in) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int ret = Helios_ExtInt_Enable((Helios_GPIONUM) self->line);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(extint_obj_enable_obj, extint_obj_enable);

/// \method disable()
/// Disable the interrupt associated with the ExtInt object.
/// This could be useful for debouncing.
STATIC mp_obj_t extint_obj_disable(mp_obj_t self_in) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int ret = Helios_ExtInt_Disable((Helios_GPIONUM) self->line);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(extint_obj_disable_obj, extint_obj_disable);

/// \classmethod \constructor(pin, mode, pull, callback)
/// Create an ExtInt object:
///
///   - `pin` is the pin on which to enable the interrupt (can be a pin object or any valid pin name).
///   - `mode` can be one of:
///     - `ExtInt.IRQ_RISING` - trigger on a rising edge;
///     - `ExtInt.IRQ_FALLING` - trigger on a falling edge;
///     - `ExtInt.IRQ_RISING_FALLING` - trigger on a rising or falling edge.
///   - `pull` can be one of:
///     - `pyb.Pin.PULL_NONE` - no pull up or down resistors;
///     - `pyb.Pin.PULL_UP` - enable the pull-up resistor;
///     - `pyb.Pin.PULL_DOWN` - enable the pull-down resistor.
///   - `callback` is the function to call when the interrupt triggers.  The
///   callback function must accept exactly 1 argument, which is the line that
///   triggered the interrupt.
STATIC const mp_arg_t pyb_extint_make_new_args[] = {
    { MP_QSTR_pin,      MP_ARG_REQUIRED | MP_ARG_INT, {.u_obj = 0} },
    { MP_QSTR_mode,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_pull,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_callback, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
};
#define PYB_EXTINT_MAKE_NEW_NUM_ARGS MP_ARRAY_SIZE(pyb_extint_make_new_args)

STATIC mp_obj_t extint_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // type_in == extint_obj_type

    // parse args
    mp_arg_val_t vals[PYB_EXTINT_MAKE_NEW_NUM_ARGS];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, PYB_EXTINT_MAKE_NEW_NUM_ARGS, pyb_extint_make_new_args, vals);

	if (vals[0].u_int < HELIOS_GPIO0 || vals[0].u_int > HELIOS_GPIOMAX) {
		mp_raise_ValueError("invalid pin value");
	}
	extint_obj_t *self = &extint_obj[vals[0].u_int];
	self->base.type = type;
	self->line = vals[0].u_int;
	
	self->mode = vals[1].u_int;
	if (self->mode < HELIOS_EDGE_RISING || self->mode > HELIOS_EDGE_BOTH) {
		mp_raise_ValueError("invalid mode value");
	}
	
	self->pull = vals[2].u_int;
	if (self->pull < HELIOS_PULL_NONE || self->pull > HELIOS_PULL_DOWN) {
		mp_raise_ValueError("invalid pull value");
	}
	if (vals[3].u_obj == mp_const_none) {
		mp_raise_ValueError("callback is none");
	}
	self->callback = vals[3].u_obj;
	
	Helios_ExtIntStruct extint_struct = {0};
	extint_struct.gpio_trigger = HELIOS_EDGE_TRIGGER;
	extint_struct.gpio_edge = self->mode;
	extint_struct.gpio_debounce = HELIOS_DEBOUNCE_EN;
	extint_struct.gpio_pull = self->pull;
	extint_struct.eint_cb = eint_handler[self->line];
	extint_struct.wakeup_eint_cb = NULL;
	
	Helios_ExtInt_Init((Helios_GPIONUM) self->line,  &extint_struct);
	
    return MP_OBJ_FROM_PTR(self);
}

STATIC void extint_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<ExtInt line=%u>", self->line);
}

STATIC const mp_rom_map_elem_t extint_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_line),    MP_ROM_PTR(&extint_obj_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable),  MP_ROM_PTR(&extint_obj_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable), MP_ROM_PTR(&extint_obj_disable_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_swint),   MP_ROM_PTR(&extint_obj_swint_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_regs),    MP_ROM_PTR(&extint_regs_obj) },

    // class constants
    /// \constant IRQ_RISING - interrupt on a rising edge
    /// \constant IRQ_FALLING - interrupt on a falling edge
    /// \constant IRQ_RISING_FALLING - interrupt on a rising or falling edge
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING),         MP_ROM_INT(HELIOS_EDGE_RISING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_FALLING),        MP_ROM_INT(HELIOS_EDGE_FALLING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING_FALLING), MP_ROM_INT(HELIOS_EDGE_BOTH) },
	{ MP_ROM_QSTR(MP_QSTR_PULL_DISABLE), MP_ROM_INT(HELIOS_PULL_NONE) },
	{ MP_ROM_QSTR(MP_QSTR_PULL_PU),   MP_ROM_INT(HELIOS_PULL_UP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_PD), MP_ROM_INT(HELIOS_PULL_DOWN) },
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
    //{ MP_ROM_QSTR(MP_QSTR_EVT_RISING),         MP_ROM_INT(GPIO_MODE_EVT_RISING) },
    //{ MP_ROM_QSTR(MP_QSTR_EVT_FALLING),        MP_ROM_INT(GPIO_MODE_EVT_FALLING) },
    //{ MP_ROM_QSTR(MP_QSTR_EVT_RISING_FALLING), MP_ROM_INT(GPIO_MODE_EVT_RISING_FALLING) },
};

STATIC MP_DEFINE_CONST_DICT(extint_locals_dict, extint_locals_dict_table);

const mp_obj_type_t machine_extint_type = {
    { &mp_type_type },
    .name = MP_QSTR_ExtInt,
    .print = extint_obj_print,
    .make_new = extint_make_new,
    .locals_dict = (mp_obj_dict_t *)&extint_locals_dict,
};
