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

/**
 ******************************************************************************
 * @file    modmachine.c
 * @author  xxx
 * @version V1.0.0
 * @date    2019/04/16
 * @brief   xxx
 ******************************************************************************
 */

#include <stdio.h>
#include <stdint.h>
#include "mpconfigport.h"

#include "compile.h"
#include "runtime.h"
#include "repl.h"
#include "mperrno.h"

#include "modmachine.h"



#ifdef MICROPY_PY_MACHINE




STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
	{ MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_machine) },
	{ MP_ROM_QSTR(MP_QSTR_LCD), MP_ROM_PTR(&machine_lcd_type) },
	{ MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&machine_pin_type) },
	{ MP_ROM_QSTR(MP_QSTR_UART), MP_ROM_PTR(&machine_uart_type) },
	{ MP_ROM_QSTR(MP_QSTR_ExtInt), MP_ROM_PTR(&machine_extint_type) },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_Timer), MP_ROM_PTR(&machine_timer_type) },
	{ MP_ROM_QSTR(MP_QSTR_RTC), MP_ROM_PTR(&machine_rtc_type) },
	{ MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&machine_hard_i2c_type) },
	{ MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&machine_hard_spi_type) },
	{ MP_ROM_QSTR(MP_QSTR_WDT), MP_ROM_PTR(&machine_wdt_type) },
#ifdef CONFIG_SPINAND
	{ MP_ROM_QSTR(MP_QSTR_NANDFLASH), MP_ROM_PTR(&machine_nandflash_type) },
#endif
};

STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

const mp_obj_module_t mp_module_machine = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&machine_module_globals,
};
    
#endif /* MICROPY_PY_MACHINE */

