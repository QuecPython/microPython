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

#include "obj.h"
#include "runtime.h"
#include "mphalport.h"
#include "helios_debug.h"
#include "helios_nw.h"
#include "helios_dev.h"
#if !defined(PLAT_RDA)
#include "helios_sim.h"
#endif

#define QPY_NET_LOG(msg, ...)      custom_log(modnet, msg, ##__VA_ARGS__)


/*=============================================================================*/
/* FUNCTION: qpy_net_set_mode                                                  */
/*=============================================================================*/
/*!@brief: set network mode
 *	
 * @mode	[in] 	network mode
 * @roaming	[in] 	enable or disable roaming
 *
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_set_configuration(size_t n_args, const mp_obj_t *args)
{
	int mode;
	int roaming;
	int ret = 0;
	Helios_NwConfigInfoStruct config_info = {0};

	mode = mp_obj_get_int(args[0]);
	QPY_NET_LOG("[network] set config, mode=%d\r\n", mode);
	if (n_args == 2)
	{
		roaming = mp_obj_get_int(args[1]);
		if((roaming != 0) && (roaming != 1))
		{
			QPY_NET_LOG("[network] invalid roaming value, roaming=%d\r\n", roaming);
			return mp_obj_new_int(-1);
		}
		config_info.roaming_switch = roaming;
		QPY_NET_LOG("[network] set config, roaming=%d\r\n", roaming);
	}

	config_info.net_mode = mode;

	ret = Helios_Nw_SetConfiguration(0, &config_info);
	return mp_obj_new_int(ret); 
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_net_set_configuration_obj, 1, 2, qpy_net_set_configuration);


/*=============================================================================*/
/* FUNCTION: qpy_net_get_mode                                                  */
/*=============================================================================*/
/*!@brief: get network mode
 *
 * @return:
 *     returns net mode on success.
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_configuration(void)
{
	int ret = 0;
	Helios_NwConfigInfoStruct config_info = {0};
	
	ret = Helios_Nw_GetConfiguration(0, &config_info);
	if(ret == 0)
	{
		mp_obj_t tuple[2] = {mp_obj_new_int(config_info.net_mode), mp_obj_new_bool(config_info.roaming_switch)};
		return mp_obj_new_tuple(2, tuple);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_configuration_obj, qpy_net_get_configuration);


/*=============================================================================*/
/* FUNCTION: qpy_net_get_csq                                                  */
/*=============================================================================*/
/*!@brief: get CSQ signal strength
 *
 * @return:
 *     returns CSQ on success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_csq(void)
{
	int ret = 0;
	int csq = -1;
	
#if !defined(PLAT_RDA)
	Helios_SIM_Status_e status = 0;
	ret = Helios_SIM_GetCardStatus(0, &status);
	if (ret == 0)
	{
		if (status == 0)
		{
			return mp_obj_new_int(99);
		}
	}
	else
	{
		return mp_obj_new_int(-1);
	}
#endif
	csq = Helios_Nw_GetCSQ(0);
	if(csq != -1)
	{
		return mp_obj_new_int(csq);
	}

	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_csq_obj, qpy_net_get_csq);


/*=============================================================================*/
/* FUNCTION: qpy_net_get_nitz_time                                             */
/*=============================================================================*/
/*!@brief: get nitz time
 *
 * @return:
 *     returns nitz time on success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_nitz_time(void)
{
	int ret = 0; 
	Helios_NwNITZTimeInfoStruct info = {0};
	
	ret = Helios_Nw_GetNITZTime(&info);
	if (ret == 0)
	{
		QPY_NET_LOG("nitz_time:%s\r\n",info.nitz_time);
		mp_obj_t tuple[3] = {
			mp_obj_new_str(info.nitz_time, strlen(info.nitz_time)), 
			mp_obj_new_int_from_ull(info.abs_time), 
			mp_obj_new_int(info.leap_sec)};

        return mp_obj_new_tuple(3, tuple);
	}

	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_nitz_time_obj, qpy_net_get_nitz_time);


/*=============================================================================*/
/* FUNCTION: qpy_net_get_operator_name                                         */
/*=============================================================================*/
/*!@brief: get operator name
 *
 * @return:
 *     returns operator name on success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_operator_name(void)
{
	int ret = 0; 
	Helios_NwOperatorInfoStruct info = {0};
	Helios_NwRegisterStatusInfoStruct reg_info = {0};
	
	ret = Helios_Nw_GetRegisterStatus(0, &reg_info);
	if (ret == 0)
	{
		if ((reg_info.data_reg.status != 1) && (reg_info.data_reg.status != 5))
		{
			QPY_NET_LOG("nw is not registered\r\n");
			return mp_obj_new_int(-1);
		}
	}
	else
	{
		QPY_NET_LOG("get nw register status failed.\r\n");
		return mp_obj_new_int(-1);
	}
	
	ret = Helios_Nw_GetOperatorName(0, &info);
	if (ret == 0)
	{
		mp_obj_t tuple[4] = {
			mp_obj_new_str(info.long_name, strlen(info.long_name)), 
			mp_obj_new_str(info.short_name, strlen(info.short_name)), 
			mp_obj_new_str(info.mcc, strlen(info.mcc)),
			mp_obj_new_str(info.mnc, strlen(info.mnc))};
			
		return mp_obj_new_tuple(4, tuple);
	}
	
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_operator_name_obj, qpy_net_get_operator_name);

#if 0
/*=============================================================================*/
/* FUNCTION: qpy_net_set_selection                                             */
/*=============================================================================*/
/*!@brief: set network selection
 *	
 * @mode	[in] 	0 - automatic, 1 - manual
 * @mcc		[in] 	mobile device country code
 * @mnc		[in] 	mobile device network code
 * @act		[in] 	network mode
 *
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_set_selection(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	Helios_NwSelectionInfoStruct info = {0};
	mp_buffer_info_t bufinfo_mc = {0};
	mp_buffer_info_t bufinfo_mn = {0};
	mp_int_t mode = 0;
	mp_int_t nw_act = 0;

	mode = mp_obj_get_int(args[0]);
	nw_act = mp_obj_get_int(args[3]);
	mp_get_buffer_raise(args[1], &bufinfo_mc, MP_BUFFER_READ);
	mp_get_buffer_raise(args[2], &bufinfo_mn, MP_BUFFER_READ);
	
	if ((mode != 0) && (mode != 1))
	{
		QPY_NET_LOG("[network] set selection failed, mode=%d\r\n", mode);
		return mp_obj_new_int(-1);
	}
	
	if ((nw_act < 0) || (nw_act > 7))
	{
		QPY_NET_LOG("[network] set selection failed, act=%d\r\n", nw_act);
		return mp_obj_new_int(-1);
	}
	
	if (strlen(bufinfo_mc.buf) > 4)
	{
		QPY_NET_LOG("[network] set selection failed, mcc=%s\r\n", bufinfo_mc.buf);
		return mp_obj_new_int(-1);
	}
	
	if (strlen(bufinfo_mn.buf) > 4)
	{
		QPY_NET_LOG("[network] set selection failed, mnc=%s\r\n", bufinfo_mn.buf);
		return mp_obj_new_int(-1);
	}
	info.nw_selection_mode = mode;
	strcpy(info.mcc, bufinfo_mc.buf);
	strcpy(info.mnc, bufinfo_mn.buf);
	info.act = nw_act;

	ret = Helios_Nw_SetSelection(0, &info);
	if (ret == 0)
	{
		return mp_obj_new_int(0);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_net_set_selection_obj, 3, 4, qpy_net_set_selection);
#endif

/*=============================================================================*/
/* FUNCTION: qpy_net_get_selection                                             */
/*=============================================================================*/
/*!@brief: get network selection
 *
 * @return:
 *     returns (nw_selection,mcc, mnc, act) on success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_selection(void)
{
	int ret = 0;
	Helios_NwSelectionInfoStruct info = {0};
	
	ret = Helios_Nw_GetSelection(0, &info);
	if (ret == 0)
	{
		mp_obj_t tuple[4] = {
			mp_obj_new_int(info.nw_selection_mode), 
			mp_obj_new_str(info.mcc, strlen(info.mcc)), 
			mp_obj_new_str(info.mnc, strlen(info.mnc)), 
			mp_obj_new_int(info.act)};
			
		return mp_obj_new_tuple(4, tuple);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_selection_obj, qpy_net_get_selection);


/*=============================================================================*/
/* FUNCTION: qpy_net_get_reg_status                                            */
/*=============================================================================*/
/*!@brief: get information about network registration
 *
 * @return:
 *     returns information about network registration on success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_reg_status(void)
{
	int ret = 0;
	Helios_NwRegisterStatusInfoStruct info = {0};
	
	ret = Helios_Nw_GetRegisterStatus(0, &info);
	if (ret == 0)
	{
		mp_obj_t voice_list[6] = {
			mp_obj_new_int(info.voice_reg.status), 
			mp_obj_new_int(info.voice_reg.lac), 
			mp_obj_new_int(info.voice_reg.cid), 
			mp_obj_new_int(info.voice_reg.act), 
			mp_obj_new_int(info.voice_reg.reject_cause), 
			mp_obj_new_int(info.voice_reg.psc)
			};
		
		mp_obj_t data_list[6] = {
			mp_obj_new_int(info.data_reg.status),
			mp_obj_new_int(info.data_reg.lac), 
			mp_obj_new_int(info.data_reg.cid), 
			mp_obj_new_int(info.data_reg.act), 
			mp_obj_new_int(info.data_reg.reject_cause), 
			mp_obj_new_int(info.data_reg.psc)
			};
		
		mp_obj_t tuple[2] = {
			mp_obj_new_list(6,voice_list), 
			mp_obj_new_list(6,data_list)};
		return mp_obj_new_tuple(2, tuple);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_reg_status_obj, qpy_net_get_reg_status);


/*=============================================================================*/
/* FUNCTION: qpy_net_get_signal_strength                                       */
/*=============================================================================*/
/*!@brief: get signal strength
 *	
 * @return:
 *     returns information about signal strength on success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_signal_strength(void)
{
	int ret = 0;
	Helios_NwSignalStrengthInfoStruct info = {0};
	
	ret = Helios_Nw_GetSignalStrength(0, &info);
	if (ret == 0)
	{
		mp_obj_t gw_list[4] = {
			mp_obj_new_int(info.gw_signal_strength.rssi), 
			mp_obj_new_int(info.gw_signal_strength.bit_error_rate), 
			mp_obj_new_int(info.gw_signal_strength.rscp), 
			mp_obj_new_int(info.gw_signal_strength.ecno)};
		
		mp_obj_t lte_list[4] = {
			mp_obj_new_int(info.lte_signal_strength.rssi), 
			mp_obj_new_int(info.lte_signal_strength.rsrp), 
			mp_obj_new_int(info.lte_signal_strength.rsrq), 
			mp_obj_new_int(info.lte_signal_strength.cqi)};
		
		mp_obj_t tuple[2] = {
			mp_obj_new_list(4,gw_list), 
			mp_obj_new_list(4,lte_list)};
			
		return mp_obj_new_tuple(2, tuple);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_signal_strength_obj, qpy_net_get_signal_strength);


/*=============================================================================*/
/* FUNCTION: qpy_net_get_mnc                                                   */
/*=============================================================================*/
/*!@brief: get mnc
 *	
 * @return:
 *     returns mnc on success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_mnc(void)
{
	int ret = 0;
	int i = 0;
   	mp_obj_t mnc_list = mp_obj_new_list(0, NULL);
	Helios_NwCellInfoStruct info = {0};
	
	ret = Helios_Nw_GetCellInfo(0, &info);
    if (ret == 0)
    {
		if (info.gsm_info_num > 0)
		{
			for (i=0; i<info.gsm_info_num; i++)
			{					
				mp_obj_list_append(mnc_list, mp_obj_new_int(info.gsm_info[i].mnc));
			}
		}

		if (info.umts_info_num > 0)
		{
			for (i=0; i<info.umts_info_num; i++)
			{					
				mp_obj_list_append(mnc_list, mp_obj_new_int(info.umts_info[i].mnc));
			}
		}

		if (info.lte_info_num > 0)
		{
			for (i=0; i<info.lte_info_num; i++)
			{					
				mp_obj_list_append(mnc_list, mp_obj_new_int(info.lte_info[i].mnc));
			}
		}

		return mnc_list;
	}

	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_mnc_obj, qpy_net_get_mnc);


/*=============================================================================*/
/* FUNCTION: qpy_net_get_mcc                                                   */
/*=============================================================================*/
/*!@brief: get mcc
 *	
 * @return:
 *     returns mnc on success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_mcc(void)
{
	int ret = 0;
	int i = 0;
   	mp_obj_t mcc_list = mp_obj_new_list(0, NULL);
	Helios_NwCellInfoStruct info = {0};
	
	ret = Helios_Nw_GetCellInfo(0, &info);
    if (ret == 0)
    {
		if (info.gsm_info_num > 0)
		{
			for (i=0; i<info.gsm_info_num; i++)
			{					
				mp_obj_list_append(mcc_list, mp_obj_new_int(info.gsm_info[i].mcc));
			}
		}

		if (info.umts_info_num > 0)
		{
			for (i=0; i<info.umts_info_num; i++)
			{					
				mp_obj_list_append(mcc_list, mp_obj_new_int(info.umts_info[i].mcc));
			}
		}

		if (info.lte_info_num > 0)
		{
			for (i=0; i<info.lte_info_num; i++)
			{					
				mp_obj_list_append(mcc_list, mp_obj_new_int(info.lte_info[i].mcc));
			}
		}

		return mcc_list;
	}

	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_mcc_obj, qpy_net_get_mcc);


/*=============================================================================*/
/* FUNCTION: qpy_net_get_lac                                                   */
/*=============================================================================*/
/*!@brief: get lac
 *	
 * @return:
 *     returns lac on success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_lac(void)
{
	int ret = 0;
	int i = 0;
   	mp_obj_t lac_list = mp_obj_new_list(0, NULL);
	Helios_NwCellInfoStruct info = {0};
	
	ret = Helios_Nw_GetCellInfo(0, &info);
    if (ret == 0)
    {
		if (info.gsm_info_num > 0)
		{
			for (i=0; i<info.gsm_info_num; i++)
			{					
				mp_obj_list_append(lac_list, mp_obj_new_int(info.gsm_info[i].lac));
			}
		}
		
		if (info.umts_info_num > 0)
		{
			for (i=0; i<info.umts_info_num; i++)
			{					
				mp_obj_list_append(lac_list, mp_obj_new_int(info.umts_info[i].lac));
			}
		}
		
		if (info.lte_info_num > 0)
		{
			for (i=0; i<info.lte_info_num; i++)
			{					
				mp_obj_list_append(lac_list, mp_obj_new_int(info.lte_info[i].tac));
			}
		}

		return lac_list;
	}

	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_lac_obj, qpy_net_get_lac);

/*=============================================================================*/
/* FUNCTION: qpy_net_get_cid                                                   */
/*=============================================================================*/
/*!@brief: get cid
 *	
 * @return:
 *     returns cid on success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_cid(void)
{
	int ret = 0;
	int i = 0;
   	mp_obj_t cid_list = mp_obj_new_list(0, NULL);
	Helios_NwCellInfoStruct info = {0};
	
	ret = Helios_Nw_GetCellInfo(0, &info);
    if (ret == 0)
    {
		if (info.gsm_info_num > 0)
		{
			for (i=0; i<info.gsm_info_num; i++)
			{					
				mp_obj_list_append(cid_list, mp_obj_new_int(info.gsm_info[i].cid));
			}
		}
		
		if (info.umts_info_num > 0)
		{
			for (i=0; i<info.umts_info_num; i++)
			{					
				mp_obj_list_append(cid_list, mp_obj_new_int(info.umts_info[i].cid));
			}
		}
		
		if (info.lte_info_num > 0)
		{
			for (i=0; i<info.lte_info_num; i++)
			{					
				mp_obj_list_append(cid_list, mp_obj_new_int(info.lte_info[i].cid));
			}
		}

		return cid_list;
	}

	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_cid_obj, qpy_net_get_cid);


/*=============================================================================*/
/* FUNCTION: qpy_net_get_cell_info                                             */
/*=============================================================================*/
/*!@brief: get cell informations 
 *	
 * @return:
 *     returns cell informations on success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_cell_info(void)
{
	int i = 0;
	int ret = 0;
	mp_obj_t list_gsm = mp_obj_new_list(0, NULL);
	mp_obj_t list_umts = mp_obj_new_list(0, NULL);
	mp_obj_t list_lte = mp_obj_new_list(0, NULL);
	Helios_NwCellInfoStruct info = {0};

    MP_THREAD_GIL_EXIT();
	ret = Helios_Nw_GetCellInfo(0, &info);
    MP_THREAD_GIL_ENTER();
    if (ret == 0)
    {
		if (info.gsm_info_num > 0)
		{
			for (i=0; i<info.gsm_info_num; i++)
			{
				mp_obj_t gsm[8] = {
					mp_obj_new_int(info.gsm_info[i].flag), 
					mp_obj_new_int(info.gsm_info[i].cid),
					mp_obj_new_int(info.gsm_info[i].mcc),
					mp_obj_new_int(info.gsm_info[i].mnc),
					mp_obj_new_int(info.gsm_info[i].lac),
					mp_obj_new_int(info.gsm_info[i].arfcn),
					mp_obj_new_int(info.gsm_info[i].bsic), 
					//Pawn.zhou Edit 2020/12/18
					mp_obj_new_int(info.gsm_info[i].rssi)};

				mp_obj_list_append(list_gsm, mp_obj_new_tuple(8, gsm));
			}			
		}
	
		if (info.umts_info_num > 0)
		{
			for (i=0; i<info.lte_info_num; i++)
			{
				mp_obj_t umts[9] = {
						mp_obj_new_int(info.umts_info[i].flag), 
						mp_obj_new_int(info.umts_info[i].cid),
						mp_obj_new_int(info.umts_info[i].lcid),
						mp_obj_new_int(info.umts_info[i].mcc),
						mp_obj_new_int(info.umts_info[i].mnc),
						mp_obj_new_int(info.umts_info[i].lac),
						mp_obj_new_int(info.umts_info[i].uarfcn),
						mp_obj_new_int(info.umts_info[i].psc),
						//Pawn.zhou Edit 2020/12/18
						mp_obj_new_int(info.umts_info[i].rssi)};
				
				mp_obj_list_append(list_umts, mp_obj_new_tuple(9, umts));
			}
		}
	
		if (info.lte_info_num > 0)
		{
			for (i=0; i<info.lte_info_num; i++)
			{
				//Pawn.zhou Edit 2020/12/18
				mp_obj_t lte[8] = {
					mp_obj_new_int(info.lte_info[i].flag),
					mp_obj_new_int(info.lte_info[i].cid),
					mp_obj_new_int(info.lte_info[i].mcc),
					mp_obj_new_int(info.lte_info[i].mnc),
					mp_obj_new_int(info.lte_info[i].pci),
					mp_obj_new_int(info.lte_info[i].tac),
					mp_obj_new_int(info.lte_info[i].earfcn),
					mp_obj_new_int(info.lte_info[i].rssi)};

				mp_obj_list_append(list_lte, mp_obj_new_tuple(8, lte));
			}
		}

		mp_obj_t list[3] = {list_gsm, list_umts, list_lte};

		return mp_obj_new_tuple(3, list);
    }
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_cell_info_obj, qpy_net_get_cell_info);


/*=============================================================================*/
/* FUNCTION: qpy_net_get_modem_fun                                             */
/*=============================================================================*/
/*!@brief: get the current mode of the SIM
 *	
 * @return:
 *     returns mode on success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_get_modem_fun(void)
{
	int ret = 0;
	Helios_DevModemFunction current_fun = 0;
	
	ret = Helios_Dev_GetModemFunction(&current_fun);
	if (ret == 0)
	{
		return mp_obj_new_int(current_fun);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_net_get_modem_fun_obj, qpy_net_get_modem_fun);


/*=============================================================================*/
/* FUNCTION: qpy_net_get_modem_fun                                             */
/*=============================================================================*/
/*!@brief: turn on or off flight mode
 *	
 * @modem_fun	[in] 	1 - turn off flight mode, 4 - turn on flight mode
 * @rst			[in] 	0 - effective immediately, 1 - restart to take effect
 *
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_set_modem_fun(size_t n_args, const mp_obj_t *args)
{
	int modem_fun;
	int rst = 0;
	int ret = 0;
	
	if ( n_args > 1 )
	{
		rst = mp_obj_get_int(args[1]);
	}
	
	modem_fun = (uint8_t)mp_obj_get_int(args[0]);
	
	ret = Helios_Dev_SetModemFunction(modem_fun, rst);
	if (ret == 0)
	{
		return mp_obj_new_int(0);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_net_set_modem_fun_obj, 1, 2, qpy_net_set_modem_fun);


static mp_obj_t g_net_user_callback = NULL;

static void qpy_net_event_handler(uint8_t sim_id, int32_t event_id, void *ctx)
{
	switch (event_id)
	{
		case HELIOS_NW_DATA_REG_STATUS_IND:
		{
			Helios_NwRegisterInfoStruct *nw_register_status = (Helios_NwRegisterInfoStruct *)ctx;
			mp_obj_t tuple[5] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(nw_register_status->status),
				mp_obj_new_int(nw_register_status->lac),
				mp_obj_new_int(nw_register_status->cid),
				mp_obj_new_int(nw_register_status->act),
			};
			
			if (g_net_user_callback)
			{
				QPY_NET_LOG("[net] callback start.\r\n");
				mp_sched_schedule(g_net_user_callback, mp_obj_new_tuple(5, tuple));
				QPY_NET_LOG("[net] callback end.\r\n");
			}
			break;
		}
		case HELIOS_NW_VOICE_REG_STATUS_IND:
			/* ... */
			QPY_NET_LOG("[net] ind_flag = %x\r\n", event_id);
			break;
		case HELIOS_NW_NITZ_TIME_UPDATE_IND:
			/* ... */
			QPY_NET_LOG("[net] ind_flag = %x\r\n", event_id);
			break;
		case HELIOS_NW_SIGNAL_QUALITY_IND:
			/* ... */
			QPY_NET_LOG("[net] ind_flag = %x\r\n", event_id);
			break;
		default:
			QPY_NET_LOG("[net] event handler, ind=%x\r\n", event_id);
			break;
	}
}

/*=============================================================================*/
/* FUNCTION: qpy_net_add_event_handler                                         */
/*=============================================================================*/
/*!@brief: registered user callback function
 *	
 * @handler	[in] 	callback function
 *
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_net_add_event_handler(mp_obj_t handler)
{
	g_net_user_callback = handler;
	Helios_NwInitStruct info = {0};

	info.user_cb = qpy_net_event_handler;
	Helios_Nw_Init(&info);
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_net_add_event_handler_obj, qpy_net_add_event_handler);



STATIC const mp_rom_map_elem_t net_module_globals_table[] = {
	{ MP_OBJ_NEW_QSTR(MP_QSTR___name__), 	MP_ROM_QSTR(MP_QSTR_net) 					},
	{ MP_ROM_QSTR(MP_QSTR_csqQueryPoll), 	MP_ROM_PTR(&qpy_net_get_csq_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_getState), 		MP_ROM_PTR(&qpy_net_get_reg_status_obj) 	},
	{ MP_ROM_QSTR(MP_QSTR_getConfig), 		MP_ROM_PTR(&qpy_net_get_configuration_obj) 	},
    { MP_ROM_QSTR(MP_QSTR_setConfig), 		MP_ROM_PTR(&qpy_net_set_configuration_obj) 	},
	{ MP_ROM_QSTR(MP_QSTR_nitzTime), 		MP_ROM_PTR(&qpy_net_get_nitz_time_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_operatorName), 	MP_ROM_PTR(&qpy_net_get_operator_name_obj) 	},
	//{ MP_ROM_QSTR(MP_QSTR_setNetMode), 		MP_ROM_PTR(&qpy_net_set_selection_obj) 		},	
	{ MP_ROM_QSTR(MP_QSTR_getNetMode), 		MP_ROM_PTR(&qpy_net_get_selection_obj) 		},	
	{ MP_ROM_QSTR(MP_QSTR_getSignal), 		MP_ROM_PTR(&qpy_net_get_signal_strength_obj)},	
	{ MP_ROM_QSTR(MP_QSTR_getCellInfo), 	MP_ROM_PTR(&qpy_net_get_cell_info_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_getCi), 			MP_ROM_PTR(&qpy_net_get_cid_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_getLac), 			MP_ROM_PTR(&qpy_net_get_lac_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_getMnc), 			MP_ROM_PTR(&qpy_net_get_mnc_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_getMcc), 			MP_ROM_PTR(&qpy_net_get_mcc_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_setModemFun), 	MP_ROM_PTR(&qpy_net_set_modem_fun_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_getModemFun), 	MP_ROM_PTR(&qpy_net_get_modem_fun_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_setCallback), 	MP_ROM_PTR(&qpy_net_add_event_handler_obj) 	},

};

STATIC MP_DEFINE_CONST_DICT(net_module_globals, net_module_globals_table);

const mp_obj_module_t mp_module_net = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&net_module_globals,
};
