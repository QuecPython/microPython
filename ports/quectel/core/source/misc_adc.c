/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2021, QUECTEL  
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


#include "stdio.h"
#include "stdlib.h"
//#include "timer.h"
#include "obj.h"
#include <string.h>
#include "runtime.h"
#include "mphalport.h"

#include "helios_adc.h"

typedef enum
{
	ADC0 = 0,
	ADC1 = 1,
	ADC2 = 2,
	ADC3 = 3,
}ADCn;

typedef struct _misc_adc_obj_t {
    mp_obj_base_t base;
    unsigned int pin;
} misc_adc_obj_t;


/*=============================================================================*/
/* FUNCTION: Helios__adc_init                                                      */
/*=============================================================================*/
/*!@brief 		: ADC driver init
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		:
 *        -  0--success
 *        - -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t Helios_adc_init(mp_obj_t self_in)
{
	int ret = Helios_ADC_Init();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(Helios_adc_init_obj, Helios_adc_init);


/*=============================================================================*/
/* FUNCTION: Helios_get_cur_source_vol                                            */
/*=============================================================================*/
/*!@brief 		: Read ADC channel voltage
 * @param[in] 	: adc_channel - 0 or 1
 * @param[out] 	: 
 * @return		:
 *        -  If successful,return voltage(mV)
 *        - -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t Helios_adc_read(mp_obj_t self_in, mp_obj_t adc_channel)
{
	int ret = -1;
	int channel = mp_obj_get_int(adc_channel);
	if ((channel == 0) || (channel == 1)|| (channel == 2))
	{
		unsigned int chl = channel;
		//ret = ql_adc_read(chl, &batvol);
		
		ret = Helios_ADC_Read((Helios_ADCNum) chl);
		return mp_obj_new_int(ret);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(Helios_adc_read_obj, Helios_adc_read);


/*=============================================================================*/
/* FUNCTION: Helios_get_cur_source_vol                                            */
/*=============================================================================*/
/*!@brief 		: read adc1 Voltage
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		: Voltage
 *    
 */
/*=============================================================================*/
/*STATIC mp_obj_t Helios_get_cur_source_vol(mp_obj_t self_in)
{
	int vol = ql_get_cur_source_vol();
	return mp_obj_new_int(vol);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(Helios_get_cur_source_vol_obj, qpy_get_cur_source_vol);
*/


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
STATIC mp_obj_t Helios_adc_deinit(mp_obj_t self_in)
{
	int ret = Helios_ADC_Deinit();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(Helios_adc_deinit_obj, Helios_adc_deinit);



STATIC const mp_rom_map_elem_t misc_adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&Helios_adc_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&Helios_adc_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&Helios_adc_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_ADC0), MP_ROM_INT(ADC0) },
    { MP_ROM_QSTR(MP_QSTR_ADC1), MP_ROM_INT(ADC1) },
    { MP_ROM_QSTR(MP_QSTR_ADC2), MP_ROM_INT(ADC2) },
    { MP_ROM_QSTR(MP_QSTR_ADC3), MP_ROM_INT(ADC3) },
};
STATIC MP_DEFINE_CONST_DICT(misc_adc_locals_dict, misc_adc_locals_dict_table);

STATIC mp_obj_t misc_adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) ;

const mp_obj_type_t misc_adc_type = {
    { &mp_type_type },
    .name = MP_QSTR_ADC,
    .make_new = misc_adc_make_new,
    .locals_dict = (mp_obj_dict_t *)&misc_adc_locals_dict,
};

STATIC mp_obj_t misc_adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) 
{
    mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, true);
    //unsigned int pin_id = mp_obj_get_int(args[0]);

    // create ADC object
    misc_adc_obj_t *self = m_new_obj(misc_adc_obj_t);
    self->base.type = &misc_adc_type;
    //self->pin = pin_id;

    return MP_OBJ_FROM_PTR(self);
}



