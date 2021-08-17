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
#include "helios_sms.h"


#define QPY_MODSMS_LOG(msg, ...)      custom_log("SMS", msg, ##__VA_ARGS__)

static mp_obj_t g_sms_user_callback;

STATIC mp_obj_t qpy_sms_pdu_decode(mp_obj_t data, mp_obj_t datalen)
{
	char *pdu_data = (char *)mp_obj_str_get_str(data);
    int pdu_datalen = mp_obj_get_int(datalen);

	if ((pdu_data == NULL) || (pdu_datalen <= 0))
	{
		mp_raise_ValueError("invalid value.");
	}
    Helios_SMSStatusInfo info = {0};
	int ret = Helios_SMS_DecodePdu(&info, pdu_data);
	if (ret == 0)
	{
		mp_obj_t msg_info[4] = 
		{
			mp_obj_new_str(info.number, strlen(info.number)),
			mp_obj_new_str(info.body, strlen(info.body)),
			mp_obj_new_str(info.time, strlen(info.time)),
			mp_obj_new_int(info.body_len),
		};
		return mp_obj_new_tuple(4, msg_info);
	}

    return mp_obj_new_int(-1); 
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_sms_pdu_decode_obj, qpy_sms_pdu_decode);

/*=============================================================================*/
/* FUNCTION: qpy_sms_send_text_msg                                             */
/*=============================================================================*/
/*!@brief: send text message
 *	
 * @phone_num	[in] 	phone number
 * @msg_data	[in] 	text messages to send
 * @code_mode	[in] 	GSM, UCS2
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sms_send_text_msg(mp_obj_t phone_num, mp_obj_t msg_data, mp_obj_t code_mode)
{
	Helios_SMSSendMsgInfoStruct info = {0};
	mp_buffer_info_t phninfo = {0};
	mp_buffer_info_t msginfo = {0};
 	mp_get_buffer_raise(phone_num, &phninfo, MP_BUFFER_READ);
	mp_get_buffer_raise(msg_data, &msginfo, MP_BUFFER_READ);
	char *codemode = (char *)mp_obj_str_get_str(code_mode);

	if (phninfo.len > 20)
	{
		mp_raise_ValueError("invalid value, the length of phone number should be no more than 20 bytes.");
	}
	if (msginfo.len > 140)
	{
		mp_raise_ValueError("invalid value, the length of msg should be no more than 140 bytes.");
	}
	
	char *p = codemode;
	while (*p)
	{
		if ((*p >= 'a') && (*p <= 'z'))
		{
			*p = *p - 32;
		}
		p++;
	}

	if (strcmp(codemode, "UCS2") == 0)
	{
		info.code = HELIOS_UCS2;
	}
	else if (strcmp(codemode, "GSM") == 0)
	{
		info.code = HELIOS_GSM;
	}
	else
	{
		mp_raise_ValueError("invalid value, codemode can only be UCS2 or GSM.");
	}

	info.phone_num = (char *)phninfo.buf;
	info.msg = (char *)msginfo.buf;
	
	int ret = Helios_SMS_SendTextMsg(0, &info);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(qpy_sms_send_text_msg_obj, qpy_sms_send_text_msg);


/*=============================================================================*/
/* FUNCTION: qpy_sms_send_pdu_msg                                              */
/*=============================================================================*/
/*!@brief: send message by pdu.Temporarily does not support long SMS sending.
 *
 * @phone_num	[in]	phone number
 * @msg_data	[in]	text messages to send
 * @code_mode	[in]	GSM, UCS2
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sms_send_pdu_msg(mp_obj_t phone_num, mp_obj_t msg_data, mp_obj_t code_mode)
{
	Helios_SMSSendMsgInfoStruct info = {0};
	mp_buffer_info_t phninfo = {0};
	mp_buffer_info_t msginfo = {0};
 	mp_get_buffer_raise(phone_num, &phninfo, MP_BUFFER_READ);
	mp_get_buffer_raise(msg_data, &msginfo, MP_BUFFER_READ);
	char *codemode = (char *)mp_obj_str_get_str(code_mode);

	if (phninfo.len > 20)
	{
		mp_raise_ValueError("invalid value, the length of phone number should be no more than 20 bytes.");
	}
	if (msginfo.len > 140)
	{
		mp_raise_ValueError("invalid value, the length of msg should be no more than 140 bytes.");
	}
	
	char *p = codemode;
	while (*p)
	{
		if ((*p >= 'a') && (*p <= 'z'))
		{
			*p = *p - 32;
		}
		p++;
	}

	if (strcmp(codemode, "UCS2") == 0)
	{
		info.code = HELIOS_UCS2;
	}
	else if (strcmp(codemode, "GSM") == 0)
	{
		info.code = HELIOS_GSM;
	}
	else
	{
		mp_raise_ValueError("invalid value, codemode can only be UCS2 or GSM.");
	}
	
	info.phone_num = (char *)phninfo.buf;
	info.msg = (char *)msginfo.buf;

	int ret = Helios_SMS_SendPDUMsg(0, &info);
	
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(qpy_sms_send_pdu_msg_obj, qpy_sms_send_pdu_msg);


/*=============================================================================*/
/* FUNCTION: qpy_sms_delete_msg                                                */
/*=============================================================================*/
/*!@brief: delete message by index.
 *
 * @index	[in]	index number for message
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sms_delete_msg(mp_obj_t index)
{
	int msgindex = mp_obj_get_int(index);
	if (msgindex < 0 || msgindex > 255)
	{
		mp_raise_ValueError("invalid value, index must be greater than or equal to 0.");
	}
	
	uint8_t mindex = msgindex;
	int ret = Helios_SMS_DeleteMsg(0, mindex);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_sms_delete_msg_obj, qpy_sms_delete_msg);


/*=============================================================================*/
/* FUNCTION: qpy_sms_get_center_address                                        */
/*=============================================================================*/
/*!@brief: get the SMS center address.
 *
 * @return:
 *     if get successfully, return to SMS center address
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sms_get_center_address(void)
{
	char address[30] = {0};

	int ret = Helios_SMS_GetCenterAddress(0, (void *)address, sizeof(address));
	if (ret == 0)
	{
		return mp_obj_new_str(address, strlen(address));
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_sms_get_center_address_obj, qpy_sms_get_center_address);


/*=============================================================================*/
/* FUNCTION: qpy_sms_set_center_address                                        */
/*=============================================================================*/
/*!@brief: set the SMS center address.
 *
 * @address		[in]	SMS center address
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sms_set_center_address(mp_obj_t address)
{
	char *addr = (char *)mp_obj_str_get_str(address);
	if (strlen(addr) > 30)
	{
		mp_raise_ValueError("invalid value, the length of center addr should be no more than 30 bytes.");
	}

	int ret = Helios_SMS_SetCenterAddress(0, (void *)addr);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_sms_set_center_address_obj, qpy_sms_set_center_address);


/*=============================================================================*/
/* FUNCTION: qpy_sms_set_save_location                                         */
/*=============================================================================*/
/*!@brief: set the location of SMS storage.
 *
 * @mem1		[in]	The location where messages are read and deleted
 * @mem2		[in]	The location where messages are written and sent
 * @mem3		[in]	The location where the received message is stored
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sms_set_save_location(mp_obj_t mem1, mp_obj_t mem2, mp_obj_t mem3)
{
	Helios_SMSMemSetInfoStruct info = {0};
	char *addr1 = (char *)mp_obj_str_get_str(mem1);
	char *addr2 = (char *)mp_obj_str_get_str(mem2);
	char *addr3 = (char *)mp_obj_str_get_str(mem3);

	if (((strcmp(addr1,"SM") != 0) && (strcmp(addr1,"ME") != 0)) \
		|| ((strcmp(addr2,"SM") != 0) && (strcmp(addr2,"ME") != 0)) \
		|| ((strcmp(addr3,"SM") != 0) && (strcmp(addr3,"ME") != 0)))
	{
		mp_raise_ValueError("invalid value, mem can only be SM, ME.");
	}
		
	strncpy(info.mem1, addr1, strlen(addr1));
	strncpy(info.mem2, addr2, strlen(addr2));
	strncpy(info.mem3, addr3, strlen(addr3));
	
	int ret = Helios_SMS_SetSavingLocation(0, &info);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(qpy_sms_set_save_location_obj, qpy_sms_set_save_location);


/*=============================================================================*/
/* FUNCTION: qpy_sms_set_save_location                                         */
/*=============================================================================*/
/*!@brief: get the storage information of SM, ME, MT.
 *
 * @return:
 *     If get successfully, return the tuple containing stored information
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sms_get_save_location(void)
{
	Helios_SMSMemGetInfoStruct info = {0};

	int ret = Helios_SMS_GetSavingLocation(0, &info);
	if (ret == 0)
	{
		mp_obj_t mem1[3] = 
		{
			mp_obj_new_str(info.mem1.mem, strlen(info.mem1.mem)),
			mp_obj_new_int(info.mem1.current_nums),
			mp_obj_new_int(info.mem1.max_nums),
		};
		
		mp_obj_t mem2[3] = 
		{
			mp_obj_new_str(info.mem2.mem, strlen(info.mem2.mem)),
			mp_obj_new_int(info.mem2.current_nums),
			mp_obj_new_int(info.mem2.max_nums),
		};
		
		mp_obj_t mem3[3] = 
		{
			mp_obj_new_str(info.mem3.mem, strlen(info.mem3.mem)),
			mp_obj_new_int(info.mem3.current_nums),
			mp_obj_new_int(info.mem3.max_nums),
		};

		mp_obj_t tuple[3] = {
			mp_obj_new_list(3, mem1), 
			mp_obj_new_list(3, mem2),
			mp_obj_new_list(3, mem3)
			};
			
		return mp_obj_new_tuple(3, tuple);
	}
	else
	{
		ret = -1;
	}
	return mp_obj_new_int(ret);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_sms_get_save_location_obj, qpy_sms_get_save_location);


/*=============================================================================*/
/* FUNCTION: qpy_sms_read_text_msg                                             */
/*=============================================================================*/
/*!@brief: read a single short message by text format.
 *
 * @index		[in]	The location where messages are read and deleted
 * @return:
 *     If search successfully,return the tuple containing text message information.
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sms_read_text_msg(mp_obj_t index)
{
	int msgindex = mp_obj_get_int(index);
	Helios_SMSRecvMsgInfoStruct text_msg = {0};
	char data[400] = {0};
	text_msg.msg_buf = data;
	text_msg.buf_len = 400;
	
	if (msgindex < 0 || msgindex > 255)
	{
		mp_raise_ValueError("invalid value, index must be greater than or equal to 0.");
	}
	uint8_t mindex = msgindex;
	
	int ret = Helios_SMS_ReadTextMsg(0, mindex, &text_msg);
	if (ret == 0)
	{
		mp_obj_t msg_info[3] = 
		{
			mp_obj_new_str(text_msg.phone_num, strlen(text_msg.phone_num)),
			mp_obj_new_str(text_msg.msg_buf, strlen(text_msg.msg_buf)),
			mp_obj_new_int(strlen(text_msg.msg_buf)),
		};
		return mp_obj_new_tuple(3, msg_info);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_sms_read_text_msg_obj, qpy_sms_read_text_msg);


/*=============================================================================*/
/* FUNCTION: qpy_sms_read_pdu_msg                                              */
/*=============================================================================*/
/*!@brief: read a single short message by PDU format.
 *
 * @index		[in]	The location where messages are read and deleted
 * @return:
 *     If search successfully,return the PDU message.
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sms_read_pdu_msg(mp_obj_t index)
{
	int msgindex = mp_obj_get_int(index);
	Helios_SMSRecvMsgInfoStruct pdu_msg = {0};
	char data[512] = {0};
	pdu_msg.msg_buf = data;
	pdu_msg.buf_len = 512;

	if (msgindex < 0 || msgindex > 255)
	{
		mp_raise_ValueError("invalid value, index must be greater than or equal to 0.");
	}
	uint8_t mindex = msgindex;
	
	int ret = Helios_SMS_ReadPDUMsg(0, mindex, &pdu_msg);
	if (ret == 0)
	{
		return mp_obj_new_str(pdu_msg.msg_buf, strlen(pdu_msg.msg_buf));
	}

	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_sms_read_pdu_msg_obj, qpy_sms_read_pdu_msg);


/*=============================================================================*/
/* FUNCTION: qpy_sms_get_pdu_length                                            */
/*=============================================================================*/
/*!@brief: get length of PDU message.
 *
 * @data		[in]	The location where messages are read and deleted
 * @return: return the length of the pdu,it will return -1 if error.
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sms_get_pdu_length(mp_obj_t data)
{
	char *pdumsg = (char *)mp_obj_str_get_str(data);
	
	int ret = Helios_SMS_GetPDUMsgLength((void *)pdumsg);
	if (ret != -1)
	{
		return mp_obj_new_int(ret);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_sms_get_pdu_length_obj, qpy_sms_get_pdu_length);


/*=============================================================================*/
/* FUNCTION: qpy_sms_get_msg_nums                                              */
/*=============================================================================*/
/*!@brief: read a single short message by PDU format.
 *
 * @return:
 *     If search successfully,return the message numbers.
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sms_get_msg_nums(void)
{
	int ret = Helios_SMS_GetMsgIndex(0);
	if (ret != -1)
	{
		return mp_obj_new_int(ret);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_sms_get_msg_nums_obj, qpy_sms_get_msg_nums);


static void qpy_sms_event_handler(uint8_t sim_id, int32_t event_id, void *ctx)
{
	if (event_id == HELIOS_SMS_NEW_MSG_IND)
	{
		Helios_SMSNewMsgInfoStruct *pctx = (Helios_SMSNewMsgInfoStruct *)ctx;
		uint8_t index  = pctx->index;
		char storage[5] = {0};

		strncpy(storage, pctx->mem, strlen(pctx->mem));

		mp_obj_t tuple[3] = 
		{
			mp_obj_new_int(sim_id),
			mp_obj_new_int(index),
			mp_obj_new_str(storage, strlen(storage))
		};
		
		if (g_sms_user_callback)
		{
			QPY_MODSMS_LOG("[SMS] callback start.\r\n");
			mp_sched_schedule(g_sms_user_callback, mp_obj_new_tuple(3, tuple));
			QPY_MODSMS_LOG("[SMS] callback end.\r\n");
		}
	}
}

/*=============================================================================*/
/* FUNCTION: qpy_sms_read_pdu_msg                                              */
/*=============================================================================*/
/*!@brief: read a single short message by PDU format.
 *
 * @handler		[in]	the callback function of user
 * @return:
 *     0	-	success
 *    -1	-	error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_sms_add_event_handler(mp_obj_t handler)
{
	Helios_SMSInitStruct info = {0};
	
	info.user_cb = qpy_sms_event_handler;
	g_sms_user_callback = handler;
	Helios_SMS_Init(&info);
	
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_sms_add_event_handler_obj, qpy_sms_add_event_handler);



STATIC const mp_rom_map_elem_t mp_module_sms_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),           MP_ROM_QSTR(MP_QSTR_sms) },
    { MP_ROM_QSTR(MP_QSTR_decodePdu),          MP_ROM_PTR(&qpy_sms_pdu_decode_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendTextMsg),        MP_ROM_PTR(&qpy_sms_send_text_msg_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendPduMsg),         MP_ROM_PTR(&qpy_sms_send_pdu_msg_obj) },
    { MP_ROM_QSTR(MP_QSTR_deleteMsg),          MP_ROM_PTR(&qpy_sms_delete_msg_obj) },
    { MP_ROM_QSTR(MP_QSTR_getCenterAddr),      MP_ROM_PTR(&qpy_sms_get_center_address_obj) },
    { MP_ROM_QSTR(MP_QSTR_setCenterAddr),      MP_ROM_PTR(&qpy_sms_set_center_address_obj) },
    { MP_ROM_QSTR(MP_QSTR_setSaveLoc),         MP_ROM_PTR(&qpy_sms_set_save_location_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getSaveLoc),         MP_ROM_PTR(&qpy_sms_get_save_location_obj) },
    { MP_ROM_QSTR(MP_QSTR_searchTextMsg),      MP_ROM_PTR(&qpy_sms_read_text_msg_obj) },
    { MP_ROM_QSTR(MP_QSTR_searchPduMsg),       MP_ROM_PTR(&qpy_sms_read_pdu_msg_obj) },
    { MP_ROM_QSTR(MP_QSTR_getPduLength),       MP_ROM_PTR(&qpy_sms_get_pdu_length_obj) },
    { MP_ROM_QSTR(MP_QSTR_getMsgNums),         MP_ROM_PTR(&qpy_sms_get_msg_nums_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setCallback),        MP_ROM_PTR(&qpy_sms_add_event_handler_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_sms_globals, mp_module_sms_globals_table);


const mp_obj_module_t mp_module_sms = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_sms_globals,
};




