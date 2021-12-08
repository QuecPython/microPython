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

#include "stdio.h"
#include "stdlib.h"
#include "obj.h"
#include "runtime.h"
#include "mphalport.h"
#include "helios_debug.h"
#include "helios_sim.h"

#define QPY_MODSIM_LOG(msg, ...)      custom_log("SIM", msg, ##__VA_ARGS__)

/*=============================================================================*/
/* FUNCTION: qpy_sim_get_imsi                                                  */
/*=============================================================================*/
/*!@brief: Get the IMSI of the SIM card
 *	
 * @return:
 *     if get successfully, return the IMSI
 *    -1	-	error
 */
/*=============================================================================*/
//可变参函数
//无入参情况下，默认获取当前卡的信息
//有入参情况下，获取指定卡的信息
STATIC mp_obj_t qpy_sim_get_imsi(size_t n_args, const mp_obj_t *args)
{
	char imsi_str[19] = {0};
    int ret = 0;

    if (n_args == 1)
    {
        int sim_id = mp_obj_get_int(args[0]);
        if ((sim_id != 0) && (sim_id != 1))
    	{
    		mp_raise_ValueError("invalid value, sim id should be 0/1.");
    	}
        
        ret = Helios_SIM_GetIMSI(sim_id, (void *)imsi_str, sizeof(imsi_str));
    	if (ret == 0)
    	{
    		return mp_obj_new_str(imsi_str, strlen(imsi_str));
    	}
    }
    else
    {
    #if defined (PLAT_ASR)
        HELIOS_SimID_ex helios_simid = HELIOS_SIM_0;
        HELIOS_SIM_ERRORCODE err = Helios_SIM_Get_Current_Simid(helios_simid);
        if (err == HELIOS_SIM_NOT_SUPPORT) {
            ret = Helios_SIM_GetIMSI(0, (void *)imsi_str, sizeof(imsi_str));
        } else {
            ret = Helios_SIM_GetIMSI(helios_simid, (void *)imsi_str, sizeof(imsi_str));
        }
    #else
        ret = Helios_SIM_GetIMSI(0, (void *)imsi_str, sizeof(imsi_str));
    #endif
        if (ret == 0)
    	{
    		return mp_obj_new_str(imsi_str, strlen(imsi_str));
    	}
    }
	
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_sim_get_imsi_obj, 0, 1, qpy_sim_get_imsi);


/*=============================================================================*/
/* FUNCTION: qpy_sim_get_iccid                                                 */
/*=============================================================================*/
/*!@brief: Get the ICCID of the SIM card
 *	
 * @return:
 *     if get successfully, return the ICCID
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sim_get_iccid(void)
{
	char iccid_str[23] = {0};
	uint8_t status = 0;
	int ret = 0;

	ret = Helios_SIM_GetCardStatus(0, (Helios_SIM_Status_e *)&status);
	if (ret == 0)
	{
		if (status != 1)
		{
			return mp_obj_new_int(-1);
		}
	}
	else
	{
		return mp_obj_new_int(-1);
	}

	ret = Helios_SIM_GetICCID(0, (void *)iccid_str, sizeof(iccid_str));
	if (ret == 0)
	{
		return mp_obj_new_str(iccid_str, strlen(iccid_str));
	}

	/*if (n_args == 1)
    {
        int sim_id = mp_obj_get_int(args[0]);
        if ((sim_id != 0) && (sim_id != 1))
    	{
    		mp_raise_ValueError("invalid value, sim id should be 0/1.");
    	}

    	ret = Helios_SIM_GetCardStatus(0, (Helios_SIM_Status_e *)&status);
    	if (ret == 0)
    	{
    		if (status != 1)
    		{
    			return mp_obj_new_int(-1);
    		}
    	}
    	else
    	{
    		return mp_obj_new_int(-1);
    	}
        
        ret = Helios_SIM_GetIMSI(sim_id, (void *)imsi_str, sizeof(imsi_str));
    	if (ret == 0)
    	{
    		return mp_obj_new_str(imsi_str, strlen(imsi_str));
    	}
    }
    else
    {
    #if defined (PLAT_ASR)
        HELIOS_SimID_ex helios_simid = HELIOS_SIM_0;
        HELIOS_SIM_ERRORCODE err = Helios_SIM_Get_Current_Simid(helios_simid);
        if (err == HELIOS_SIM_NOT_SUPPORT) {
            ret = Helios_SIM_GetIMSI(0, (void *)imsi_str, sizeof(imsi_str));
        } else {
            ret = Helios_SIM_GetIMSI(helios_simid, (void *)imsi_str, sizeof(imsi_str));
        }
    #else
        ret = Helios_SIM_GetIMSI(0, (void *)imsi_str, sizeof(imsi_str));
    #endif
        if (ret == 0)
    	{
    		return mp_obj_new_str(imsi_str, strlen(imsi_str));
    	}
    }*/

	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_sim_get_iccid_obj, qpy_sim_get_iccid);


/*=============================================================================*/
/* FUNCTION: qpy_sim_get_phonenumber                                           */
/*=============================================================================*/
/*!@brief: Get the phone number of the SIM card
 *	
 * @return:
 *     if get successfully, return the phone number
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sim_get_phonenumber(void)
{
	char phone_number[20+1] = {0};

	int ret = Helios_SIM_GetPhoneNumber(0, (void *)phone_number, sizeof(phone_number));
	if (ret == 0)
	{
		return mp_obj_new_str(phone_number, strlen(phone_number));
	}

	return mp_obj_new_int(-1);
}
#if !defined (PLAT_RDA)
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_sim_get_phonenumber_obj, qpy_sim_get_phonenumber);
#endif

/*=============================================================================*/
/* FUNCTION: qpy_sim_enable_pin                                                */
/*=============================================================================*/
/*!@brief: enable SIM card PIN code verification, and restart will take effect
 *	
 * @pin		[in] 	PIN code
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sim_enable_pin(const mp_obj_t pin)
{
	Helios_SIMPinInfoStruct info = {0};
	mp_buffer_info_t bufinfo = {0};
 	mp_get_buffer_raise(pin, &bufinfo, MP_BUFFER_READ);

	if (bufinfo.len > 15)
	{
		mp_raise_ValueError("invalid value, the length of pin should be no more than 15 bytes.");
	}
	
	strncpy((char *)info.pin, (const char *)bufinfo.buf, bufinfo.len);
	
	int ret = Helios_SIM_PINEnable(0, &info);
	if (ret == 0)
	{
		return mp_obj_new_int(ret);
	}
	
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_sim_enable_pin_obj, qpy_sim_enable_pin);


/*=============================================================================*/
/* FUNCTION: qpy_sim_disable_pin                                               */
/*=============================================================================*/
/*!@brief: disable SIM card PIN code verification
 *	
 * @pin		[in] 	PIN code
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sim_disable_pin(const mp_obj_t pin)
{
	Helios_SIMPinInfoStruct info = {0};
	mp_buffer_info_t bufinfo = {0};
 	mp_get_buffer_raise(pin, &bufinfo, MP_BUFFER_READ);

	if (bufinfo.len > 15)
	{
		mp_raise_ValueError("invalid value, the length of pin should be no more than 15 bytes.");
	}
	
	strncpy((char *)info.pin, (const char *)bufinfo.buf, bufinfo.len);
	
	int ret = Helios_SIM_PINDisable(0, &info);
	if (ret == 0)
	{
		return mp_obj_new_int(ret);
	}
	
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_sim_disable_pin_obj, qpy_sim_disable_pin);


/*=============================================================================*/
/* FUNCTION: qpy_sim_verify_pin                                                */
/*=============================================================================*/
/*!@brief: when the SIM state is requested PIN/PIN2, enter the PIN/PIN2 code to verify
 *	
 * @pin		[in] 	PIN code
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sim_verify_pin(const mp_obj_t pin)
{
	Helios_SIMPinInfoStruct info = {0};
	mp_buffer_info_t bufinfo = {0};
 	mp_get_buffer_raise(pin, &bufinfo, MP_BUFFER_READ);

	if (bufinfo.len > 15)
	{
		mp_raise_ValueError("invalid value, the length of pin should be no more than 15 bytes.");
	}
	
	strncpy((char *)info.pin, (const char *)bufinfo.buf, bufinfo.len);
	int ret = Helios_SIM_PINVerify(0, &info);
	if (ret == 0)
	{
		return mp_obj_new_int(ret);
	}
	
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_sim_verify_pin_obj, qpy_sim_verify_pin);


/*=============================================================================*/
/* FUNCTION: qpy_sim_change_pin                                                */
/*=============================================================================*/
/*!@brief: After enabling SIM card PIN verification, change the SIM card PIN
 *	
 * @old_pin		[in] 	old PIN
 * @new_pin		[in] 	new PIN
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sim_change_pin(const mp_obj_t old_pin, const mp_obj_t new_pin)
{
	Helios_SIMChangePinInfoStruct info = {0};
	mp_buffer_info_t bufinfo[2] = {0};
 	mp_get_buffer_raise(old_pin, &bufinfo[0], MP_BUFFER_READ);
 	mp_get_buffer_raise(new_pin, &bufinfo[1], MP_BUFFER_READ);

	if ((bufinfo[0].len > 15) || (bufinfo[1].len > 15))
	{
		mp_raise_ValueError("invalid value, the length of pin should be no more than 15 bytes.");
	}
	
 	strncpy((char *)info.old_pin, (const char *)bufinfo[0].buf, bufinfo[0].len);
 	strncpy((char *)info.new_pin, (const char *)bufinfo[1].buf, bufinfo[1].len);
 
	int ret = Helios_SIM_PINChange(0, &info);
	if (ret == 0)
	{
		return mp_obj_new_int(ret);
	}
	
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_sim_change_pin_obj, qpy_sim_change_pin);


/*=============================================================================*/
/* FUNCTION: qpy_sim_unblock_pin                                               */
/*=============================================================================*/
/*!@brief: When the SIM card status is requested PUK/PUK2 after multiple incorrect input
 *		   of PIN/PIN2 code, input PUK/PUK2 code and a new PIN/PIN2 code to unlock
 *	
 * @puk			[in] 	PUK/PUK2
 * @new_pin		[in] 	new PIN/PIN2
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sim_unblock_pin(const mp_obj_t puk, const mp_obj_t new_pin)
{
	Helios_SIMUnlockPinInfoStruct info = {0};
	mp_buffer_info_t bufinfo[2] = {0};
 	mp_get_buffer_raise(puk, &bufinfo[0], MP_BUFFER_READ);
 	mp_get_buffer_raise(new_pin, &bufinfo[1], MP_BUFFER_READ);

	if (bufinfo[0].len > 15)
	{
		mp_raise_ValueError("invalid value, the length of puk should be no more than 15 bytes.");
	}
	if (bufinfo[1].len > 15)
	{
		mp_raise_ValueError("invalid value, the length of pin should be no more than 15 bytes.");
	}
	
 	strncpy((char *)info.puk, (const char *)bufinfo[0].buf, bufinfo[0].len);
 	strncpy((char *)info.new_pin, (const char *)bufinfo[1].buf, bufinfo[1].len);
 	
	int ret = Helios_SIM_PINUnlock(0, &info);
	if (ret == 0)
	{
		return mp_obj_new_int(ret);
	}
	
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_sim_unblock_pin_obj, qpy_sim_unblock_pin);


/*=============================================================================*/
/* FUNCTION: sim_get_card_status                                               */
/*=============================================================================*/
/*!@brief: Get the status of the SIM card.
 *
 * @return :
 *        - 0		SIM was removed.
 *	      - 1		SIM is ready.
 *	      - 2		Expecting the universal PIN./SIM is locked, waiting for a CHV1 password.
 *	      - 3		Expecting code to unblock the universal PIN./SIM is blocked, CHV1 unblocking password is required.
 *	      - 4		SIM is locked due to a SIM/USIM personalization check failure.
 *	      - 5		SIM is blocked due to an incorrect PCK; an MEP unblocking password is required.
 *	      - 6		Expecting key for hidden phone book entries.
 *	      - 7		Expecting code to unblock the hidden key.
 *	      - 8		SIM is locked; waiting for a CHV2 password.
 *	      - 9		SIM is blocked; CHV2 unblocking password is required.
 *	      - 10		SIM is locked due to a network personalization check failure.
 *	      - 11		SIM is blocked due to an incorrect NCK; an MEP unblocking password is required.
 *	      - 12		
SIM is locked due to a network subset personalization check failure.
 *	      - 13		SIM is blocked due to an incorrect NSCK; an MEP unblocking password is required.
 *	      - 14		SIM is locked due to a service provider personalization check failure.
 *	      - 15		SIM is blocked due to an incorrect SPCK; an MEP unblocking password is required.
 *	      - 16		SIM is locked due to a corporate personalization check failure.
 *	      - 17		SIM is blocked due to an incorrect CCK; an MEP unblocking password is required.
 *	      - 18		SIM is being initialized; waiting for completion.
 *	      - 19		Use of CHV1/CHV2/universal PIN/code to unblock the CHV1/code to unblock the CHV2/code to unblock the universal PIN/ is blocked.
 *	      - 20		Unuseful status.
 *		  - 21		Unknow status.
 */
 /*=============================================================================*/
STATIC mp_obj_t qpy_sim_get_card_status(void)
{
	Helios_SIM_Status_e status = 0;

	int ret = Helios_SIM_GetCardStatus(0, &status);
	if (ret == 0)
	{
		return mp_obj_new_int(status);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_sim_get_card_status_obj, qpy_sim_get_card_status);


/*=============================================================================*/
/* FUNCTION: qpy_sim_read_phonebook_record                                 */
/*=============================================================================*/
/*!@brief : Read the phone book.
 *
 * @args[1]		[in]	the storage position of the phone book.
 * @args[2]		[in]	start_index
 * @args[3]		[in]	end_index
 * @args[4]		[in]	username
 * @return :
 *     -1	-	error
 * If it reads successfully, the results are returned in the following format.
 * (record_number, [(index, username, phone_number), ... , (index, username, phone_number)])
 * For example:
 *	(2, [(1, 'zhangsan', '18122483511'), (2, 'lisi', '18122483542')])
 */
/*=============================================================================*/
#if defined(PLAT_ASR)

STATIC mp_obj_t qpy_sim_read_phonebook_record(size_t n_args, const mp_obj_t *args)
{
	uint8_t i = 0;
	int32_t storage = mp_obj_get_int(args[0]);
	Helios_SIMReadPhoneBookInfoStruct records = {0};
	mp_obj_t list_records = mp_obj_new_list(0, NULL);

	records.start_index = mp_obj_get_int(args[1]);
	records.end_index   = mp_obj_get_int(args[2]);

	mp_buffer_info_t nameinfo = {0};
 	mp_get_buffer_raise(args[3], &nameinfo, MP_BUFFER_READ);

	if ((storage < 0) || (storage > 15))
	{
		mp_raise_ValueError("invalid value, storage should be in (0,15).");
	}
	if (records.end_index < records.start_index)
	{
		mp_raise_ValueError("invalid value, end >= start.");
	}
	if ((records.end_index - records.start_index) > 20)
	{
		mp_raise_ValueError("invalid value, end - start <= 20.");
	}

	if (nameinfo.len > 30)
	{
		mp_raise_ValueError("invalid value, the length of username should be no more than 30 bytes.");
	}
	records.user_name = (char *)nameinfo.buf;

	int ret = Helios_SIM_ReadPhonebookRecord(0, storage, &records);
	if (ret == 0)
	{
		for (i=0; i<records.phonebook.count; ++i)
		{
			mp_obj_t record_info[3] = 
			{
				mp_obj_new_int(records.phonebook.records[i].index),
				mp_obj_new_str(records.phonebook.records[i].user_name, strlen(records.phonebook.records[i].user_name)),
				mp_obj_new_str(records.phonebook.records[i].phone_num, strlen(records.phonebook.records[i].phone_num)),
			};
			mp_obj_list_append(list_records, mp_obj_new_tuple(3, record_info));
		}

		mp_obj_t tuple_records[2] = 
		{
			mp_obj_new_int(records.phonebook.count),
			list_records,
		};

		return mp_obj_new_tuple(2, tuple_records);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_sim_read_phonebook_record_obj, 3, 4, qpy_sim_read_phonebook_record);


/*=============================================================================*/
/* FUNCTION: qpy_sim_write_phonebook_record                                */
/*=============================================================================*/
/*!@brief 		: Write the information of phone number to the phone book.
 * @args[1]		[in]	the storage position of the phone book.
 * @args[2]		[in]	the record index in phone book.
 * @args[3]		[in]	username
 * @args[4]		[in]	Phone number, it can include '+'.
 * @return		:
 *        -  0--success
 *        - -1--error
 */
/*=============================================================================*/

STATIC mp_obj_t qpy_sim_write_phonebook_record(size_t n_args, const mp_obj_t *args)
{
	int32_t storage = 0;
	mp_buffer_info_t bufinfo[2] = {0};
	Helios_SIMPhoneInfoStruct record = {0};
	
	storage = mp_obj_get_int(args[0]);
	record.index = mp_obj_get_int(args[1]);
 	mp_get_buffer_raise(args[2], &bufinfo[0], MP_BUFFER_READ);
 	mp_get_buffer_raise(args[3], &bufinfo[1], MP_BUFFER_READ);

	if ((storage < 0) || (storage > 15))
	{
		mp_raise_ValueError("invalid value, storage should be in (0,15).");
	}
	if ((record.index < 1) || (record.index > 500))
	{
		mp_raise_ValueError("invalid value, index should be in (1,500).");
	}
	if (bufinfo[0].len > 30)
	{
		mp_raise_ValueError("invalid value, the length of username should be no more than 30 bytes.");
	}
	if (bufinfo[1].len > 20)
	{
		mp_raise_ValueError("invalid value, the length of phonenumber should be no more than 20 bytes.");
	}

 	strncpy(record.user_name, bufinfo[0].buf, bufinfo[0].len);
 	strncpy(record.phone_num, bufinfo[1].buf, bufinfo[1].len);
 	
	int ret = Helios_SIM_WritePhonebookRecord(0, storage, &record);
 	if (ret == 0)
 	{
		return mp_obj_new_int(0);
 	}
 	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_sim_write_phonebook_record_obj, 3, 4, qpy_sim_write_phonebook_record);
#endif

STATIC mp_obj_t qpy_sim_set_simdet(const mp_obj_t obj_detenable, const mp_obj_t obj_insertlevel)
{
	int ret = 0;
	int detenable = mp_obj_get_int(obj_detenable);
    int insertlevel = mp_obj_get_int(obj_insertlevel);

	if (detenable != 0 && detenable != 1)
        mp_raise_ValueError("invalid value, detenable should be in (0,1).");
    if (insertlevel != 0 && insertlevel != 1)
        mp_raise_ValueError("invalid value, insertlevel should be in (0,1).");

    ret = Helios_SIM_SetSimDet(0, detenable, insertlevel);
    if (ret == 0)
	{
		return mp_obj_new_int(ret);
	}
	
	return mp_obj_new_int(-1);
}

#if !defined (PLAT_RDA)
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_sim_set_simdet_obj, qpy_sim_set_simdet);
#endif

STATIC mp_obj_t qpy_sim_get_simdet(void)
{
	int ret = 0;
	int detenable = 99;
    int insertlevel = 99;

    ret = Helios_SIM_GetSimDet(0, &detenable, &insertlevel);
    if (ret == 0)
	{
	    mp_obj_t tuple[2] = 
		{
			mp_obj_new_int(detenable),
			mp_obj_new_int(insertlevel),
		};
		return mp_obj_new_tuple(2, tuple);
	}
	
	return mp_obj_new_int(-1);
}
#if !defined (PLAT_RDA)
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_sim_get_simdet_obj, qpy_sim_get_simdet);
#endif
static c_callback_t *g_sim_user_callback;
#if defined (PLAT_ASR)
static c_callback_t * g_switchsim_done_callback;
#endif
static void qpy_sim_event_handler(uint8_t sim_id, unsigned int event_id, void *ctx)
{
	if(g_sim_user_callback)
	{
	    QPY_MODSIM_LOG("[SIM] callback start.\r\n");
	    mp_sched_schedule_ex(g_sim_user_callback, mp_obj_new_int(event_id));
	    QPY_MODSIM_LOG("[SIM] callback end.\r\n");
	}
}

STATIC mp_obj_t qpy_sim_add_event_handler(mp_obj_t handler)
{
    static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
    g_sim_user_callback = &cb;
    mp_sched_schedule_callback_register(g_sim_user_callback, handler);

	Helios_SIM_Add_Event_Handler(qpy_sim_event_handler);
	
	return mp_obj_new_int(0);
}
#if !defined (PLAT_RDA)
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_sim_add_event_handler_obj, qpy_sim_add_event_handler);
#endif

STATIC mp_obj_t qpy_module_sim_deinit(void)
{
	g_sim_user_callback = NULL;
    #if defined (PLAT_ASR)
    g_switchsim_done_callback = NULL;
    #endif
	Helios_SIM_Add_Event_Handler(NULL);
	return mp_obj_new_int(0);
}
#if !defined (PLAT_RDA)
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_module_sim_deinit_obj, qpy_module_sim_deinit);
#endif

#if defined (PLAT_ASR)
STATIC mp_obj_t qpy_sim_genericaccess(const mp_obj_t sim_id, const mp_obj_t cmd)
{
	Helios_SIMGenericAccesStruct info = {0};
    int simid = mp_obj_get_int(sim_id);
	mp_buffer_info_t bufinfo = {0};
 	mp_get_buffer_raise(cmd, &bufinfo, MP_BUFFER_READ);

	if (bufinfo.len <= 0)
	{
		mp_raise_ValueError("invalid value, the length of cmd should be more than 0 bytes.");
	}

    if (simid > 1 || simid < 0)
    {
        mp_raise_ValueError("invalid value, simid should be 0 or 1.");
    }

 	strncpy((char *)info.cmd, (const char *)bufinfo.buf, bufinfo.len);
 	info.len = bufinfo.len;
	int ret = Helios_SIM_GenericAccess(simid, &info);
	if (ret == 0)
	{
	    mp_obj_t tuple[2] = 
		{
			mp_obj_new_int(strlen(info.resp)),
			mp_obj_new_str(info.resp, strlen(info.resp)),
		};
		return mp_obj_new_tuple(2, tuple);
		//return mp_obj_new_str(info.resp, strlen(info.resp));
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_sim_genericaccess_obj, qpy_sim_genericaccess);

//typedef void (*SwitchSimDoneCb)(UINT8);

static void switch_sim_done_callback(uint8_t state)
{		
	if (g_switchsim_done_callback)
	{
		mp_sched_schedule_ex(g_switchsim_done_callback, mp_obj_new_int(state));
	}
}

STATIC mp_obj_t qpy_sim_switch_card(mp_obj_t sim_id, mp_obj_t switchsim_callback)
{
    int simid = mp_obj_get_int(sim_id);
	static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
	g_switchsim_done_callback = &cb;
	mp_sched_schedule_callback_register(g_switchsim_done_callback, switchsim_callback);
	int ret = Helios_SIM_Switch_Card(simid, switch_sim_done_callback);
	if (ret == HELIOS_SIM_GENERIC_FAILURE)
	{
		ret = -1;
	}
    else if (ret == HELIOS_SIM_NOT_SUPPORT)
	{
		return mp_obj_new_str("NOT SUPPORT", strlen("NOT SUPPORT"));
	}
	
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_sim_switch_card_obj, qpy_sim_switch_card);
#endif

STATIC const mp_rom_map_elem_t mp_module_sim_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), 		MP_ROM_QSTR(MP_QSTR_sim) 			},
    { MP_ROM_QSTR(MP_QSTR_getImsi), 		MP_ROM_PTR(&qpy_sim_get_imsi_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_getIccid), 		MP_ROM_PTR(&qpy_sim_get_iccid_obj) 		},

	{ MP_ROM_QSTR(MP_QSTR_verifyPin), 		MP_ROM_PTR(&qpy_sim_verify_pin_obj) 	},
	{ MP_ROM_QSTR(MP_QSTR_changePin), 		MP_ROM_PTR(&qpy_sim_change_pin_obj) 	},
	{ MP_ROM_QSTR(MP_QSTR_unblockPin), 		MP_ROM_PTR(&qpy_sim_unblock_pin_obj) 	},
	{ MP_ROM_QSTR(MP_QSTR_enablePin), 		MP_ROM_PTR(&qpy_sim_enable_pin_obj) 	},
	{ MP_ROM_QSTR(MP_QSTR_disablePin), 		MP_ROM_PTR(&qpy_sim_disable_pin_obj) 	},
	{ MP_ROM_QSTR(MP_QSTR_getStatus), 		MP_ROM_PTR(&qpy_sim_get_card_status_obj)},
#if defined (PLAT_ASR)
	{ MP_ROM_QSTR(MP_QSTR_readPhonebook),   MP_ROM_PTR(&qpy_sim_read_phonebook_record_obj) },
	{ MP_ROM_QSTR(MP_QSTR_writePhonebook),  MP_ROM_PTR(&qpy_sim_write_phonebook_record_obj) },
	{ MP_ROM_QSTR(MP_QSTR_genericAccess),  MP_ROM_PTR(&qpy_sim_genericaccess_obj) },
	{ MP_ROM_QSTR(MP_QSTR_switchCard),      MP_ROM_PTR(&qpy_sim_switch_card_obj) },
#endif
#if !defined (PLAT_RDA)
    { MP_ROM_QSTR(MP_QSTR_setSimDet),       MP_ROM_PTR(&qpy_sim_set_simdet_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getSimDet),       MP_ROM_PTR(&qpy_sim_get_simdet_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getPhoneNumber), 	MP_ROM_PTR(&qpy_sim_get_phonenumber_obj)},
	{ MP_ROM_QSTR(MP_QSTR_setCallback),     MP_ROM_PTR(&qpy_sim_add_event_handler_obj) },
	{ MP_ROM_QSTR(MP_QSTR___qpy_module_deinit__),   MP_ROM_PTR(&qpy_module_sim_deinit_obj) },
#endif
};
STATIC MP_DEFINE_CONST_DICT(mp_module_sim_globals, mp_module_sim_globals_table);


const mp_obj_module_t mp_module_sim = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_sim_globals,
};




