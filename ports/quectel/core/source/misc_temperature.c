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
#if defined(PLAT_RDA)
#include "stdio.h"
#include "stdlib.h"
//#include "timer.h"
#include "obj.h"
#include <string.h>
#include "runtime.h"
#include "mphalport.h"

#include "helios_adc.h"

typedef struct _misc_tempr_obj_t {
    mp_obj_base_t base;
    int temper;
} misc_tempr_obj_t;


/*=============================================================================*/
/* FUNCTION: Helios_adc_deinit                                                    */
/*=============================================================================*/
/*!@brief 		: adc deinit
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		: 0
 *    
 */
/*=============================================================================*/
STATIC mp_obj_t Helios_Get_temperature(mp_obj_t self_in)
{
	int ret = Helios_Get_Temperature();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(Helios_Get_temperature_obj, Helios_Get_temperature);

STATIC const mp_rom_map_elem_t misc_temper_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_temperature), MP_ROM_PTR(&Helios_Get_temperature_obj) },
};
STATIC MP_DEFINE_CONST_DICT(misc_temper_locals_dict, misc_temper_locals_dict_table);

STATIC mp_obj_t misc_temper_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) ;

const mp_obj_type_t machine_temperature_type = {
    { &mp_type_type },
    .name = MP_QSTR_Temperature,
    .make_new = misc_temper_make_new,
    .locals_dict = (mp_obj_dict_t *)&misc_temper_locals_dict,
};

STATIC mp_obj_t misc_temper_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) 
{
    mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, true);
    //unsigned int pin_id = mp_obj_get_int(args[0]);

    // create ADC object
    misc_tempr_obj_t *self = m_new_obj(misc_tempr_obj_t);
    self->base.type = &machine_temperature_type;
    //self->pin = pin_id;

    return MP_OBJ_FROM_PTR(self);
}

#endif

