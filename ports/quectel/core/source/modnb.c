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
 * @file    modnb.h
 * @author  burols.wang
 * @version V1.0.0
 * @date    2021/08/23
 * @brief   IOT data interaction function module
 ******************************************************************************
 */

#if defined(PLAT_RDA)
#include <stdio.h>
#include <stdint.h>


#include "compile.h"
#include "runtime.h"
#include "repl.h"
#include "mperrno.h"

#include "modnb.h"



#if MICROPY_PY_NB

STATIC const mp_rom_map_elem_t nb_module_globals_table[] = {
	{ MP_OBJ_NEW_QSTR(MP_QSTR___NB__), MP_ROM_QSTR(MP_QSTR_NB) },

	#if MICROPY_PY_NB_OC
	{ MP_ROM_QSTR(MP_QSTR_OC), MP_ROM_PTR(&nb_oc_type) },
	#endif
	#if  MICROPY_PY_NB_AEP
	{ MP_OBJ_NEW_QSTR(MP_QSTR_AEP), MP_ROM_PTR(&nb_aep_type) },
	#endif

	#if  MICROPY_PY_NB_ONENET
	{ MP_ROM_QSTR(MP_QSTR_ONENET), MP_ROM_PTR(&module_onenet) },
    #endif
};

STATIC MP_DEFINE_CONST_DICT(nb_module_globals, nb_module_globals_table);

const mp_obj_module_t mp_module_nb = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&nb_module_globals,
};

#endif /* MICROPY_PY_NB */
#endif /* END PLAT_RDA*/
