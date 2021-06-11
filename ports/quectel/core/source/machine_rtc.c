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
#include <string.h>

//#include "ql_rtc.h"

#include "nlr.h"
#include "obj.h"
#include "runtime.h"
#include "mphal.h"
//#include "timeutils.h"

#include "modmachine.h"

#include "helios_rtc.h"

typedef struct _machine_rtc_obj_t {
    mp_obj_base_t base;
} machine_rtc_obj_t;



// singleton RTC object
STATIC const machine_rtc_obj_t machine_rtc_obj = {{&machine_rtc_type}};



STATIC mp_obj_t machine_rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // return constant object
    return (mp_obj_t)&machine_rtc_obj;
}



STATIC mp_obj_t machine_rtc_datetime_helper(mp_uint_t n_args, const mp_obj_t *args) {
	//ql_rtc_time_t tm = {0}; //Jayceon-2020/08/27:Resolved that calling the init() function would cause system dump
    Helios_RTCTime tm = {0};
	if (n_args == 1) {
        // Get time

        //struct timeval tv;

       // gettimeofday(&tv, NULL);
        //timeutils_struct_time_t tm;

        //timeutils_seconds_since_2000_to_struct_time(tv.tv_sec, &tm);
		
		//ql_rtc_get_time(&tm);
		// Pawn 2021/4/28 Solve the problem of different initial time zones between RDA and ASR
		// Pawn Fix the bug SW1PQUECPYTHON-119
		Helios_RTC_GetLocalTime(&tm);
		// Pawn Fix the bug SW1PQUECPYTHON-119 end
        mp_obj_t tuple[8] = {
            mp_obj_new_int(tm.tm_year),
            mp_obj_new_int(tm.tm_mon),
            mp_obj_new_int(tm.tm_mday),
            mp_obj_new_int(tm.tm_wday),
            mp_obj_new_int(tm.tm_hour),
            mp_obj_new_int(tm.tm_min),
            mp_obj_new_int(tm.tm_sec),
            mp_obj_new_int(0)
        };

        return mp_obj_new_tuple(8, tuple);
    } else {
        // Set time

        mp_obj_t *items;
        mp_obj_get_array_fixed_n(args[1], 8, &items);
		tm.tm_year = mp_obj_get_int(items[0]);
		tm.tm_mon = mp_obj_get_int(items[1]);
		tm.tm_mday = mp_obj_get_int(items[2]);
		tm.tm_hour = mp_obj_get_int(items[4]);
		tm.tm_min = mp_obj_get_int(items[5]);
		tm.tm_sec = mp_obj_get_int(items[6]);

		//int ret = ql_rtc_set_time(&tm);
		
		// Pawn Fix the bug SW1PQUECPYTHON-119
		int ret = Helios_RTC_NtpSetTime(&tm, 1);
		// Pawn Fix the bug SW1PQUECPYTHON-119 end
		
        //struct timeval tv = {0};
        //tv.tv_sec = timeutils_seconds_since_2000(mp_obj_get_int(items[0]), mp_obj_get_int(items[1]), mp_obj_get_int(items[2]), mp_obj_get_int(items[4]), mp_obj_get_int(items[5]), mp_obj_get_int(items[6]));
        //tv.tv_usec = mp_obj_get_int(items[7]);
        //settimeofday(&tv, NULL);

        return mp_obj_new_int(ret);
    }
}


STATIC mp_obj_t machine_rtc_datetime(mp_uint_t n_args, const mp_obj_t *args) {
    return machine_rtc_datetime_helper(n_args, args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_rtc_datetime_obj, 1, 2, (mp_obj_t)machine_rtc_datetime);


STATIC mp_obj_t machine_rtc_init(mp_obj_t self_in, mp_obj_t date) {
    mp_obj_t args[2] = {self_in, date};
    machine_rtc_datetime_helper(2, args);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_rtc_init_obj, machine_rtc_init);

STATIC const mp_rom_map_elem_t machine_rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_rtc_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&machine_rtc_datetime_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_rtc_locals_dict, machine_rtc_locals_dict_table);

const mp_obj_type_t machine_rtc_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTC,
    .make_new = machine_rtc_make_new,
    .locals_dict = (mp_obj_t)&machine_rtc_locals_dict,
};

