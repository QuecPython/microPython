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

#include <stdio.h>
#include <stdint.h>
#include "stdlib.h"
#include "mpconfigport.h"
#include "obj.h"
#include "compile.h"
#include "runtime.h"
#include "repl.h"
#include "mperrno.h"
#include "modmisc.h"

#include "helios_power.h"

STATIC mp_obj_t misc_power_reset()
{
	int ret = 1;
	Helios_Power_Reset(ret);
	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(misc_power_reset_obj, misc_power_reset);


STATIC mp_obj_t misc_power_down()
{
	int ret = 1;
	Helios_Power_Down(ret);
	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(misc_power_down_obj, misc_power_down);


STATIC mp_obj_t misc_power_get_down_reason()
{
	int ret;
	ret = Helios_Power_GetDownReason();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(misc_power_get_down_reason_obj, misc_power_get_down_reason);


STATIC mp_obj_t misc_power_get_up_reason()
{
	int ret;
	ret = Helios_Power_GetUpReason();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(misc_power_get_up_reason_obj, misc_power_get_up_reason);


STATIC mp_obj_t misc_power_get_batt()
{
	unsigned int ret;
	ret = Helios_Power_GetBatteryVol();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(misc_power_get_batt_obj, misc_power_get_batt);


STATIC const mp_rom_map_elem_t misc_power_locals_dict_table[] = {
	{ MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_misc) },
	{ MP_ROM_QSTR(MP_QSTR_powerRestart), MP_ROM_PTR(&misc_power_reset_obj) },
	{ MP_ROM_QSTR(MP_QSTR_powerDown), MP_ROM_PTR(&misc_power_down_obj) },
	{ MP_ROM_QSTR(MP_QSTR_powerOnReason), MP_ROM_PTR(&misc_power_get_up_reason_obj) },
	{ MP_ROM_QSTR(MP_QSTR_powerDownReason), MP_ROM_PTR(&misc_power_get_down_reason_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getVbatt), MP_ROM_PTR(&misc_power_get_batt_obj) },
};

STATIC MP_DEFINE_CONST_DICT(misc_power_locals_dict, misc_power_locals_dict_table);

const mp_obj_type_t misc_power_type = {
    { &mp_type_type },
    .name = MP_QSTR_Power,
    .locals_dict = (mp_obj_t)&misc_power_locals_dict,
};