/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2021, QUECTEL  
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

/**
 ******************************************************************************
 * @file    modmisc.c
 * @author  Pawn
 * @version V1.0.0
 * @date    2020/08/18
 * @brief   misc
 ******************************************************************************
 */

#include <stdio.h>
#include <stdint.h>
#include "mpconfigport.h"

#include "compile.h"
#include "runtime.h"
#include "repl.h"
#include "mperrno.h"

#include "modmisc.h"
#include "obj.h"  // Pawn 2020-12-19 Add replEnable

#ifdef MICROPY_PY_MISC

extern int repl_protect_enable;

STATIC mp_obj_t qpy_misc_set_replEnable(size_t n_args, const mp_obj_t *args)
{
	int flag;
	
	flag = mp_obj_get_int(args[0]);
	if (flag != 0 && flag != 1)
	{
		return mp_obj_new_int(-1);
	}
	
	repl_protect_enable = flag;

	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_misc_set_replEnable_obj, 1, 2, qpy_misc_set_replEnable);


STATIC const mp_rom_map_elem_t misc_module_globals_table[] = {
	{ MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_misc) },
	{ MP_ROM_QSTR(MP_QSTR_Power), MP_ROM_PTR(&misc_power_type) },
	{ MP_ROM_QSTR(MP_QSTR_PowerKey), MP_ROM_PTR(&misc_powerkey_type) },
	{ MP_ROM_QSTR(MP_QSTR_PWM), MP_ROM_PTR(&misc_pwm_type) },
	{ MP_ROM_QSTR(MP_QSTR_ADC), MP_ROM_PTR(&misc_adc_type) },
	{ MP_ROM_QSTR(MP_QSTR_USB), MP_ROM_PTR(&misc_usb_type) },
	{ MP_ROM_QSTR(MP_QSTR_replEnable), MP_ROM_PTR(&qpy_misc_set_replEnable_obj) },
};

STATIC MP_DEFINE_CONST_DICT(misc_module_globals, misc_module_globals_table);

const mp_obj_module_t mp_module_misc = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&misc_module_globals,
};

#endif /* MICROPY_PY_MISC */


