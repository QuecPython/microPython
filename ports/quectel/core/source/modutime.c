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
#include "runtime.h"
#include "timeutils.h"
#include "utime_mphal.h"
#include "sockets.h"
#include "helios_debug.h"
#include "helios_rtc.h"

#define QPY_UTIME_LOG(msg, ...)      custom_log(Utime, msg, ##__VA_ARGS__)


STATIC mp_obj_t time_localtime(size_t n_args, const mp_obj_t *args) {
    timeutils_struct_time_t tm;
    mp_int_t seconds;
    if (n_args == 0 || args[0] == mp_const_none) {
        Helios_RTCTime rtc_tm;
		Helios_RTC_GetLocalTime(&rtc_tm);
		seconds = timeutils_seconds_since_2000(rtc_tm.tm_year, rtc_tm.tm_mon, rtc_tm.tm_mday, rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
		timeutils_seconds_since_2000_to_struct_time(seconds, &tm);
    } 
	else 
	{
        seconds = mp_obj_get_int(args[0]) - 946656000; // Pawn - 2021-03-17 -Fixed bug:Return time error
		// Pawn - 2020-12-10 -Fixed bug:Return time error
		timeutils_seconds_since_2000_to_struct_time(seconds, &tm);
		/*[Jayceon][2021/03/04]解决localtime()方法转换秒数为日期时，出现hour超过24的情��? --[begin]--*/
		if (tm.tm_hour >= 24)
		{
			tm.tm_mday += 1;
			tm.tm_hour = tm.tm_hour % 24;

			switch (tm.tm_mon)
			{
				case 1:
				case 3:
				case 5:
				case 7:
				case 8:
				case 10:
				case 12:
					if (tm.tm_mday > 31)
					{
						tm.tm_mon += 1;
						tm.tm_mday = tm.tm_mday % 31;
					}
					break;
				case 4:
				case 6:
				case 9:
				case 11:
					if (tm.tm_mday > 30)
					{
						tm.tm_mon += 1;
						tm.tm_mday = tm.tm_mday % 30;
					}
					break;
				case 2:
					if (((tm.tm_year%4 == 0) && (tm.tm_year%100 != 0)) || (tm.tm_year%400 == 0))
					{
						if (tm.tm_mday > 29)
						{
							tm.tm_mon += 1;
							tm.tm_mday = tm.tm_mday % 29;
						}
					}
					else
					{
						if (tm.tm_mday > 28)
						{
							tm.tm_mon += 1;
							tm.tm_mday = tm.tm_mday % 28;
						}
					}
					break;
				default:
					QPY_UTIME_LOG("the value(%d) of month is invalid.\n", tm.tm_mon);
					break;
			}

			if (tm.tm_mon > 12)
			{
				tm.tm_year += 1;
				tm.tm_mon = tm.tm_mon % 12;
			}
		}
		/*[Jayceon][2021/03/04]解决localtime()方法转换秒数为日期时，出现hour超过24的情��? --[END]--*/
    }
    mp_obj_t tuple[8] = {
        tuple[0] = mp_obj_new_int(tm.tm_year),
        tuple[1] = mp_obj_new_int(tm.tm_mon),
        tuple[2] = mp_obj_new_int(tm.tm_mday),
        tuple[3] = mp_obj_new_int(tm.tm_hour),
        tuple[4] = mp_obj_new_int(tm.tm_min),
        tuple[5] = mp_obj_new_int(tm.tm_sec),
        tuple[6] = mp_obj_new_int(tm.tm_wday),
        tuple[7] = mp_obj_new_int(tm.tm_yday),
    };
    return mp_obj_new_tuple(8, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_localtime_obj, 0, 1, time_localtime);

STATIC mp_obj_t time_mktime(mp_obj_t tuple) {
    size_t len;
    mp_obj_t *elem;
    mp_obj_get_array(tuple, &len, &elem);
	Helios_RTCTime rtc_tm;
	mp_uint_t time_unix;
    // localtime generates a tuple of len 8. CPython uses 9, so we accept both.
    if (len < 8 || len > 9) {
        mp_raise_msg_varg(&mp_type_TypeError, "mktime needs a tuple of length 8 or 9 (%d given)", len);
    }
	rtc_tm.tm_year = mp_obj_get_int(elem[0]);
	rtc_tm.tm_mon = mp_obj_get_int(elem[1]);
	rtc_tm.tm_mday = mp_obj_get_int(elem[2]);
	rtc_tm.tm_hour = mp_obj_get_int(elem[3]);
	rtc_tm.tm_min = mp_obj_get_int(elem[4]);
	rtc_tm.tm_sec = mp_obj_get_int(elem[5]);

	if (0 >= (rtc_tm.tm_mon -= 2)) {    /* 1..12 -> 11,12,1..10 */
        rtc_tm.tm_mon += 12;      /* Puts Feb last since it has leap day */
        rtc_tm.tm_year -= 1;
    }
	time_unix = (((
		( mp_uint_t ) (rtc_tm.tm_year/4 - rtc_tm.tm_year/100 + rtc_tm.tm_year/400
		+ 367*rtc_tm.tm_mon/12 + rtc_tm.tm_mday) + rtc_tm.tm_year*365 - 719499)*24
		+ rtc_tm.tm_hour)*60 + rtc_tm.tm_min)*60 + rtc_tm.tm_sec - 28800;	

	return mp_obj_new_int_from_uint(time_unix);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(time_mktime_obj, time_mktime);

/*return rtc seconds since power on*/
STATIC mp_obj_t time_time(void) 
{
    long seconds=0;
    
	seconds = Helios_RTC_GetSecond();
    return mp_obj_new_int(seconds);
}
MP_DEFINE_CONST_FUN_OBJ_0(time_time_obj, time_time);

STATIC mp_obj_t time_set_timezone(mp_obj_t tz)
{
	int offset = mp_obj_get_int(tz);
	if ((offset < -12) || (offset > 12))
	{
		mp_raise_ValueError("invalid value, timezone should be in [-12, +12].");
	}
	Helios_RTC_SetTimeZoneOffset(offset);
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(time_set_timezone_obj, time_set_timezone);

STATIC mp_obj_t time_get_timezone(void)
{
	int offset = Helios_RTC_GetTimeZoneOffset();
	return mp_obj_new_int(offset);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(time_get_timezone_obj, time_get_timezone);



STATIC const mp_rom_map_elem_t time_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_utime) },

    { MP_ROM_QSTR(MP_QSTR_localtime), MP_ROM_PTR(&time_localtime_obj) },
    { MP_ROM_QSTR(MP_QSTR_mktime), MP_ROM_PTR(&time_mktime_obj) },
    { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&time_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&mp_utime_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_ms), MP_ROM_PTR(&mp_utime_sleep_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_us), MP_ROM_PTR(&mp_utime_sleep_us_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_ms), MP_ROM_PTR(&mp_utime_ticks_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_us), MP_ROM_PTR(&mp_utime_ticks_us_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_cpu), MP_ROM_PTR(&mp_utime_ticks_cpu_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_add), MP_ROM_PTR(&mp_utime_ticks_add_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_diff), MP_ROM_PTR(&mp_utime_ticks_diff_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setTimeZone), MP_ROM_PTR(&time_set_timezone_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getTimeZone), MP_ROM_PTR(&time_get_timezone_obj) },
};

STATIC MP_DEFINE_CONST_DICT(time_module_globals, time_module_globals_table);

const mp_obj_module_t utime_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&time_module_globals,
};
