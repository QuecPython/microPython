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
#include <stddef.h>
#include <string.h>

#include "runtime.h"
#include "gc.h"
#include "mphal.h"

#include "helios_extint.h"
#include "helios_debug.h"
#include "callbackdeal.h"

#define EXTINT_LOG(msg, ...)      custom_log("extint", msg, ##__VA_ARGS__)


extern int Helios_GPIO_GetLevel(Helios_GPIONum gpio_num);


typedef struct {
    mp_obj_base_t base;
    mp_int_t line;
	mp_int_t mode;
	mp_int_t pull;
	c_callback_t callback;
} extint_obj_t;

typedef struct {
	mp_int_t rising_count;
	mp_int_t falling_count;
} extint_count_t;

typedef void(*eint_handler_t)(void);

static extint_obj_t *extint_obj[HELIOS_GPIOMAX];
static extint_count_t extint_count[HELIOS_GPIOMAX] = {0};

enum {
	HELIOS_EXTINT_RISING,
	HELIOS_EXTINT_FALLING,
};

#define EINT_HANDLER_DEF(n) handler##n
#define PLAT_EINT_HANDLER_DEF(n) BOOST_PP_REPEAT_1(n,EINT_HANDLER_DEF)


/*
** Jayceon-20200908:
** Replace function mp_call_function_1_protected() with mp_sched_schedule_ex to slove the dump problem.
*/
#if MICROPY_ENABLE_CALLBACK_DEAL
#define EXTINT_CALLBACK_OP(X, edge)     do{                                                                                     \
                                            st_CallBack_Extint *extint = malloc(sizeof(st_CallBack_Extint));                    \
                                    	    if(NULL != extint) {                                                                \
                                        	    extint->pin_no = X;                                                             \
                                        	    extint->edge = edge;                                                            \
                                        	    extint->callback = extint_obj[X]->callback;                                     \
                                        	    qpy_send_msg_to_callback_deal_thread(CALLBACK_TYPE_ID_EXTINT, extint);          \
                                        	}                                                                                   \
                                        }while(0)
#else
#define EXTINT_CALLBACK_OP(X, edge)     do{                                                                                     \
                                            mp_obj_t extint_list[2] = {                                                         \
                                                mp_obj_new_int(X),                                                              \
                                                mp_obj_new_int(edge),                                                           \
                                            };                                                                                  \
                                            mp_sched_schedule_ex(&extint_obj[X]->callback,  mp_obj_new_list(2, extint_list));   \
                                        }while(0)

#endif

#define HANDLER_FUN(X) 															            \
static void handler##X(void)	                                                            \
{	                                                                                        \
	int edge = HELIOS_EXTINT_RISING;											            \
	if(Helios_GPIO_GetLevel((Helios_GPIONum)X) == 0) 						                \
		{edge = HELIOS_EXTINT_FALLING; extint_count[X].falling_count++;}		            \
	else {extint_count[X].rising_count++;}									                \
	if (extint_obj[X]->callback.cb != mp_const_none &&                                      \
	            ((mp_sched_num_pending() < MICROPY_SCHEDULER_DEPTH))) {			            \
		        EXTINT_CALLBACK_OP(X, edge);                                                \
	}																			            \
	Helios_ExtInt_Enable(extint_obj[X]->line);                                              \
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
HANDLER_FUN(32)
HANDLER_FUN(33)
HANDLER_FUN(34)
HANDLER_FUN(35)
HANDLER_FUN(36)
HANDLER_FUN(37)
HANDLER_FUN(38)
HANDLER_FUN(39)
HANDLER_FUN(40)
HANDLER_FUN(41)
HANDLER_FUN(42)
HANDLER_FUN(43)
HANDLER_FUN(44)
HANDLER_FUN(45)
HANDLER_FUN(46)
HANDLER_FUN(47)
HANDLER_FUN(48)
HANDLER_FUN(49)
HANDLER_FUN(50)



eint_handler_t eint_handler[HELIOS_GPIOMAX] = {
	handler0, PLAT_EINT_HANDLER_DEF(PLAT_GPIO_NUM)
};

static void extint_count_reset(int offset) {
	extint_count[offset].falling_count = 0;
	extint_count[offset].rising_count = 0;
}

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
    int ret = Helios_ExtInt_Enable((Helios_GPIONum) self->line);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(extint_obj_enable_obj, extint_obj_enable);

/// \method disable()
/// Disable the interrupt associated with the ExtInt object.
/// This could be useful for debouncing.
STATIC mp_obj_t extint_obj_disable(mp_obj_t self_in) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int ret = Helios_ExtInt_Disable((Helios_GPIONum) self->line);
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

	if (extint_obj[vals[0].u_int] == NULL)
	{
		extint_obj[vals[0].u_int] = m_new_obj_with_finaliser(extint_obj_t);
	}
	extint_obj_t *self = extint_obj[vals[0].u_int];
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
	
    mp_sched_schedule_callback_register(&self->callback, vals[3].u_obj);
	
	Helios_ExtIntStruct extint_struct = {0};
	extint_struct.gpio_trigger = HELIOS_EDGE_TRIGGER;
	extint_struct.gpio_edge = self->mode;
	extint_struct.gpio_debounce = HELIOS_DEBOUNCE_EN;
	extint_struct.gpio_pull = self->pull;
	extint_struct.eint_cb = NULL;
	extint_struct.wakeup_eint_cb = eint_handler[self->line];
	
	if(0 != Helios_ExtInt_Init((Helios_GPIONum) self->line,  &extint_struct)) {
		mp_raise_ValueError("Interrupt initialization failed");
	}

	extint_count_reset(self->line);
	
    return MP_OBJ_FROM_PTR(self);
}

STATIC void extint_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<ExtInt line=%u>", self->line);
}

STATIC mp_obj_t extint_obj_read_count(mp_obj_t self_in, mp_obj_t is_reset) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int reset_flag = mp_obj_get_int(is_reset);

	if(reset_flag != 0 && reset_flag != 1) {
		mp_raise_ValueError("invalid is_reset value, must in [0,1]");
	} 
	
	mp_obj_t extint_list[2] = { 
			mp_obj_new_int(extint_count[self->line].rising_count),
			mp_obj_new_int(extint_count[self->line].falling_count),
	};
			
	if(1 == reset_flag) {
		extint_count_reset(self->line);
	}
    return mp_obj_new_list(2, extint_list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(extint_obj_read_count_obj, extint_obj_read_count);

STATIC mp_obj_t extint_obj_count_reset(mp_obj_t self_in) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
	extint_count_reset(self->line);

	return mp_obj_new_int(0);
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(extint_obj_count_reset_obj, extint_obj_count_reset);

STATIC mp_obj_t extint_obj_Deinit(mp_obj_t self_in) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int ret = -1;
	//EXTINT_LOG("extint deinit");

	extint_obj[self->line] = NULL;
	extint_count_reset(self->line);

	if((0 == Helios_ExtInt_Disable((Helios_GPIONum) self->line)) && (0 == Helios_ExtInt_Deinit((Helios_GPIONum) self->line))) {
		ret = 0;
	}
	
	return mp_obj_new_int(ret);
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(extint__del__obj, extint_obj_Deinit);


STATIC const mp_rom_map_elem_t extint_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), 	MP_ROM_PTR(&extint__del__obj) },
    { MP_ROM_QSTR(MP_QSTR_line),    MP_ROM_PTR(&extint_obj_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable),  MP_ROM_PTR(&extint_obj_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable), MP_ROM_PTR(&extint_obj_disable_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_count), MP_ROM_PTR(&extint_obj_read_count_obj) },
    { MP_ROM_QSTR(MP_QSTR_count_reset), MP_ROM_PTR(&extint_obj_count_reset_obj) },
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
#if defined(PLAT_Qualcomm)	
	{ MP_ROM_QSTR(MP_QSTR_GPIO2), MP_ROM_INT(HELIOS_GPIO2) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO3), MP_ROM_INT(HELIOS_GPIO3) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO6), MP_ROM_INT(HELIOS_GPIO6) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO7), MP_ROM_INT(HELIOS_GPIO7) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO8), MP_ROM_INT(HELIOS_GPIO8) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO9), MP_ROM_INT(HELIOS_GPIO9) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO11), MP_ROM_INT(HELIOS_GPIO11) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO12), MP_ROM_INT(HELIOS_GPIO12) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO14), MP_ROM_INT(HELIOS_GPIO14) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO16), MP_ROM_INT(HELIOS_GPIO16) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO17), MP_ROM_INT(HELIOS_GPIO17) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO18), MP_ROM_INT(HELIOS_GPIO18) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO19), MP_ROM_INT(HELIOS_GPIO19) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO20), MP_ROM_INT(HELIOS_GPIO20) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO21), MP_ROM_INT(HELIOS_GPIO21) },
#else
	PLAT_GPIO_DEF(PLAT_GPIO_NUM),
#endif
};

STATIC MP_DEFINE_CONST_DICT(extint_locals_dict, extint_locals_dict_table);

const mp_obj_type_t machine_extint_type = {
    { &mp_type_type },
    .name = MP_QSTR_ExtInt,
    .print = extint_obj_print,
    .make_new = extint_make_new,
    .locals_dict = (mp_obj_dict_t *)&extint_locals_dict,
};
