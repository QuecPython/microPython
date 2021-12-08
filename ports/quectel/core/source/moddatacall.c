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

#include <stdlib.h>
#include <string.h>

#include "nlr.h"
#include "objlist.h"
#include "objstr.h"
#include "runtime.h"
#include "mperrno.h"
#include "helios_debug.h"
#include "helios_datacall.h"

#define MOD_DATACALL_LOG(msg, ...)      custom_log(DataCall, msg, ##__VA_ARGS__)



/*=============================================================================*/
/* FUNCTION: qpy_datacall_start                                                */
/*=============================================================================*/
/*!@brief 		: set APN and datacall.
 * @profile_idx	[in] 	profile_idx, 1~HELIOS_PROFILE_IDX_MAX
 * @ip_type		[in] 	0-IPV4, 1-IPV6, 2-IPV4 and IPV6
 * @apn_name	[in] 	anp name
 * @usr_name	[in] 	user name
 * @password	[in] 	password
 * @auth_type	[in] 	auth_type, 0-None, 1-PAP, 2-CHAP
 * @return		:
 *        -  0--successful
 *        - -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_datacall_start(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int profile_id = mp_obj_get_int(args[0]);
	int ip_type = mp_obj_get_int(args[1]);
	int auth_type = mp_obj_get_int(args[5]);
	
	mp_buffer_info_t apninfo = {0};
	mp_buffer_info_t usrinfo = {0};
	mp_buffer_info_t pwdinfo = {0};
	mp_get_buffer_raise(args[2], &apninfo, MP_BUFFER_READ);
	mp_get_buffer_raise(args[3], &usrinfo, MP_BUFFER_READ);
	mp_get_buffer_raise(args[4], &pwdinfo, MP_BUFFER_READ);

	int min_profile_id = (int)HELIOS_PROFILE_IDX_MIN;
	int max_profile_id = (int)HELIOS_PROFILE_IDX_MAX;
	
	if ((profile_id < min_profile_id) || (profile_id > max_profile_id))
	{
		mp_raise_msg_varg(&mp_type_ValueError, "invalid value, profileIdx should be in [%d,%d].", min_profile_id, max_profile_id);
	}
	if ((ip_type < 0) || (ip_type > 2))
	{
		mp_raise_ValueError("invalid value, ipTpye should be in [0,2].");
	}
	if ((auth_type < 0) || (auth_type > 2))
	{
		mp_raise_ValueError("invalid value, authType should be in [0,2].");
	}
	if (apninfo.len > 63)
	{
		mp_raise_ValueError("invalid value, the length of apn should be no more than 63 bytes.");
	}
	if (usrinfo.len > 15)
	{
		mp_raise_ValueError("invalid value, the length of username should be no more than 15 bytes.");
	}
	if (pwdinfo.len > 15)
	{
		mp_raise_ValueError("invalid value, the length of password should be no more than 15 bytes.");
	}

	MOD_DATACALL_LOG("[datacall] profile_idx=%d, ip_version=%d, auth_type=%d\r\n", profile_id, ip_type, auth_type);
	MOD_DATACALL_LOG("[datacall] anp_name=%s, usr_name=%s, password=%s\r\n", apninfo.buf, usrinfo.buf, pwdinfo.buf);
	
    Helios_DataCallStartStruct DataCallStartStruct = {0};
    DataCallStartStruct.ip_type = (int32_t)ip_type;
    DataCallStartStruct.auth = (int32_t)auth_type;
    snprintf(DataCallStartStruct.apn, sizeof(DataCallStartStruct.apn), "%s", (char *)apninfo.buf);
    snprintf(DataCallStartStruct.user, sizeof(DataCallStartStruct.user), "%s", (char *)usrinfo.buf);
    snprintf(DataCallStartStruct.pwd, sizeof(DataCallStartStruct.pwd), "%s", (char *)pwdinfo.buf);
    
    MP_THREAD_GIL_EXIT();
    ret = Helios_DataCall_Start(profile_id, 0, &DataCallStartStruct);
    MP_THREAD_GIL_ENTER();
    
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_datacall_start_obj, 5, 6, qpy_datacall_start);


/*=============================================================================*/
/* FUNCTION: qpy_datacall_record_apn                                           */
/*=============================================================================*/
/*!@brief 		: record APN for poweron datacall.
 * @profile_idx	[in] 	profile_idx, 1~7
 * @ip_type		[in] 	0-IPV4, 1-IPV6, 2-IPV4 and IPV6
 * @apn_name	[in] 	anp name
 * @usr_name	[in] 	user name
 * @password	[in] 	password
 * @auth_type	[in] 	0-None, 1-PAP, 2-CHAP
 * @apn_type	[in] 	0-default apn, 1-user's apn
 * @return		:
 *        -  0--successful
 *        - -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_datacall_record_apn(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int profile_id  = mp_obj_get_int(args[0]);
	int ip_type     = mp_obj_get_int(args[1]);
	int auth_type   = mp_obj_get_int(args[5]);
	int apn_type    = mp_obj_get_int(args[6]);

	mp_buffer_info_t apninfo = {0};
	mp_buffer_info_t usrinfo = {0};
	mp_buffer_info_t pwdinfo = {0};
	mp_get_buffer_raise(args[2], &apninfo, MP_BUFFER_READ);
	mp_get_buffer_raise(args[3], &usrinfo, MP_BUFFER_READ);
	mp_get_buffer_raise(args[4], &pwdinfo, MP_BUFFER_READ);

	int min_profile_id = (int)HELIOS_PROFILE_IDX_MIN;
	int max_profile_id = (int)HELIOS_PROFILE_IDX_MAX;
	
	if ((profile_id < min_profile_id) || (profile_id > max_profile_id))
	{
		mp_raise_msg_varg(&mp_type_ValueError, "invalid value, profileIdx should be in [%d,%d].", min_profile_id, max_profile_id);
	}
	if ((ip_type < 0) || (ip_type > 2))
	{
		mp_raise_ValueError("invalid value, ipTpye should be in [0,2].");
	}
	if ((auth_type < 0) || (auth_type > 2))
	{
		mp_raise_ValueError("invalid value, authType should be in [0,2].");
	}
	if (apninfo.len > 63)
	{
		mp_raise_ValueError("invalid value, the length of apn should be no more than 63 bytes.");
	}
	if (usrinfo.len > 15)
	{
		mp_raise_ValueError("invalid value, the length of username should be no more than 15 bytes.");
	}
	if (pwdinfo.len > 15)
	{
		mp_raise_ValueError("invalid value, the length of password should be no more than 15 bytes.");
	}

    Helios_DataCallRecordAPNStruct DataCallRecordAPNStruct = {
        .profile_idx = (int32_t)profile_id,
        .ip_type = (int32_t)ip_type,
        .auth_type = (int32_t)auth_type,
        .apn_type = (int32_t)apn_type
    };

	snprintf(DataCallRecordAPNStruct.apn, sizeof(DataCallRecordAPNStruct.apn), "%s", (char *)apninfo.buf);
    snprintf(DataCallRecordAPNStruct.user, sizeof(DataCallRecordAPNStruct.user), "%s", (char *)usrinfo.buf);
    snprintf(DataCallRecordAPNStruct.pwd, sizeof(DataCallRecordAPNStruct.pwd), "%s", (char *)pwdinfo.buf);
	
	ret = Helios_DataCall_RecordApn(0, &DataCallRecordAPNStruct);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_datacall_record_apn_obj, 5, 7, qpy_datacall_record_apn);


/*=============================================================================*/
/* FUNCTION: qpy_datacall_set_autoconnect                                      */
/*=============================================================================*/
/*!@brief 		: set auto connect
 * @profile_idx	[in] 	profile_idx, 1~7
 * @enable		[in] 	0-disable, 1-enable
 * @return		:
 *        -  0--successful
 *        - -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_datacall_set_autoconnect(size_t n_args, const mp_obj_t *args)
{	
	int ret = 0;
	int profile_idx = mp_obj_get_int(args[0]);
	bool enable = mp_obj_is_true(args[1]);
	
	ret = Helios_DataCall_SetAutoConnect((int32_t)profile_idx, 0, enable);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_datacall_set_autoconnect_obj, 1, 2, qpy_datacall_set_autoconnect);


/*=============================================================================*/
/* FUNCTION: qpy_datacall_set_asynmode                                         */
/*=============================================================================*/
/*!@brief 		: Set asynchronous mode to datacall
 * @mode		[in] 	0-disable, 1-enable
 * @return		:
 *        -  0--successful
 *        - -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_datacall_set_asynmode(mp_obj_t mode)
{	
	bool enable = mp_obj_is_true(mode);
    Helios_DataCall_SetAsynMode((int32_t)Helios_DataCall_GetCurrentPDP(), 0, enable);
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_datacall_set_asynmode_obj, qpy_datacall_set_asynmode);


STATIC mp_obj_t qpy_datacall_get_info(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int profile_id  = mp_obj_get_int(args[0]);
	int ip_type     = mp_obj_get_int(args[1]);
	char ip4_ip_addr[16] = {0};
	char ip4_pri_dns[16] = {0};
	char ip4_sec_dns[16] = {0};
	char ip6_ip_addr[64] = {0};
	char ip6_pri_dns[64] = {0};
	char ip6_sec_dns[64] = {0};
	Helios_DataCallInfoStruct info = {0};
	int min_profile_id = (int)HELIOS_PROFILE_IDX_MIN;
	int max_profile_id = (int)HELIOS_PROFILE_IDX_MAX;
	
	if ((profile_id < min_profile_id) || (profile_id > max_profile_id))
	{
		mp_raise_msg_varg(&mp_type_ValueError, "invalid value, profileIdx should be in [%d,%d].", min_profile_id, max_profile_id);
	}
	if ((ip_type < 0) || (ip_type > 2))
	{
		mp_raise_ValueError("invalid value, ipTpye should be in [0,2].");
	}
	
	ret = Helios_DataCall_GetInfo(profile_id, 0, &info);
	if (0 == ret)
	{
		inet_ntop(AF_INET, &info.v4.addr.ip, ip4_ip_addr, sizeof(ip4_ip_addr));
		inet_ntop(AF_INET, &info.v4.addr.pri_dns, ip4_pri_dns, sizeof(ip4_pri_dns));
		inet_ntop(AF_INET, &info.v4.addr.sec_dns, ip4_sec_dns, sizeof(ip4_sec_dns));
		inet_ntop(AF_INET6, &info.v6.addr.ip, ip6_ip_addr, sizeof(ip6_ip_addr));
		inet_ntop(AF_INET6, &info.v6.addr.pri_dns, ip6_pri_dns, sizeof(ip6_pri_dns));
		inet_ntop(AF_INET6, &info.v6.addr.sec_dns, ip6_sec_dns, sizeof(ip6_sec_dns));

		mp_obj_t ip4_list[5] = {
				mp_obj_new_int(info.v4.state),
				mp_obj_new_int(info.v4.reconnect),

				mp_obj_new_str(ip4_ip_addr, strlen(ip4_ip_addr)),
				mp_obj_new_str(ip4_pri_dns, strlen(ip4_pri_dns)),
				mp_obj_new_str(ip4_sec_dns, strlen(ip4_sec_dns)),
			};
		mp_obj_t ip6_list[5] = {
				mp_obj_new_int(info.v6.state),
				mp_obj_new_int(info.v6.reconnect),

				mp_obj_new_str(ip6_ip_addr, strlen(ip6_ip_addr)),
				mp_obj_new_str(ip6_pri_dns, strlen(ip6_pri_dns)),
				mp_obj_new_str(ip6_sec_dns, strlen(ip6_sec_dns)),
			};

		if (ip_type == 0)
		{
			mp_obj_t tuple[3] = {
				mp_obj_new_int(info.profile_idx),
				mp_obj_new_int(info.ip_version),
				mp_obj_new_list(5, ip4_list),
			};
			return mp_obj_new_tuple(3, tuple);
		}
		else if (ip_type == 1)
		{
			mp_obj_t tuple[3] = {
				mp_obj_new_int(info.profile_idx),
				mp_obj_new_int(info.ip_version),
				mp_obj_new_list(5, ip6_list),
			};
			return mp_obj_new_tuple(3, tuple);
		}
		else if (ip_type == 2)
		{
			mp_obj_t tuple[4] = {
				mp_obj_new_int(info.profile_idx),
				mp_obj_new_int(info.ip_version),
				mp_obj_new_list(5, ip4_list),
				mp_obj_new_list(5, ip6_list),
			};
		
			return mp_obj_new_tuple(4, tuple);
		}
	}
	return mp_obj_new_int(ret);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_datacall_get_info_obj, 1, 2, qpy_datacall_get_info);



static c_callback_t * g_usr_callback = NULL;

static void datacall_callback(int32_t profile_idx, uint8_t sim_id, int32_t nw_status)
{
	mp_obj_t tuple[3] = 
	{
		mp_obj_new_int(profile_idx), 
		mp_obj_new_int(nw_status),
		mp_obj_new_int(sim_id)
	};
	MOD_DATACALL_LOG("[datacall] pdp = %d, nwsta = %d\r\n", profile_idx, nw_status);
	if (g_usr_callback)
	{
		MOD_DATACALL_LOG("[datacall] callback start.\r\n");
		mp_sched_schedule_ex(g_usr_callback, mp_obj_new_tuple(3, tuple));
		MOD_DATACALL_LOG("[datacall] callback end.\r\n");
	}
}


/*=============================================================================*/
/* FUNCTION: qpy_datacall_register_usr_callback                                */
/*=============================================================================*/
/*!@brief 		: register the callback function for user
 * @user_cb		[in] 	callback function
 * @return		:
 *        -  0--successful
 *        - -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t qpy_datacall_register_usr_callback(mp_obj_t callback)
{	
    static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
	g_usr_callback = &cb;
	mp_sched_schedule_callback_register(g_usr_callback, callback);
	
    Helios_DataCallInitStruct DataCallInitStruct = {datacall_callback};
    Helios_DataCall_Init((int32_t)Helios_DataCall_GetCurrentPDP(), 0, &DataCallInitStruct);
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_register_usr_callback_obj, qpy_datacall_register_usr_callback);

STATIC mp_obj_t qpy_datacall_get_pdp_range(void)
{	
	uint32_t min = Helios_DataCall_GetProfileIdxMin();
	uint32_t max = Helios_DataCall_GetProfileIdxMax();
	mp_obj_t tuple[2] = 
	{
		mp_obj_new_int(min),
		mp_obj_new_int(max)
	};
		
	return mp_obj_new_tuple(2, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_get_pdp_range_obj, qpy_datacall_get_pdp_range);

//#if defined(PLAT_ASR) || defined(PLAT_Unisoc)
/*STATIC mp_obj_t qpy_datacall_get_apn(mp_obj_t simid)
{
#if defined(PLAT_ASR) || defined(PLAT_Unisoc)
    int sim_id = mp_obj_get_int(simid);
	int ret = 0;
	char apn[99+1] = {0};
	
	ret = Helios_DataCall_GetApn(sim_id, apn);
	if (ret == 0)
	{
		return mp_obj_new_str(apn, strlen(apn));
	}
	return mp_obj_new_int(-1);
#else
    return mp_obj_new_int(-1);
#endif
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_datacall_get_apn_obj, qpy_datacall_get_apn);*/
//#endif

STATIC mp_obj_t qpy_datacall_get_apn(size_t n_args, const mp_obj_t *args)
{
#if defined(PLAT_ASR) || defined(PLAT_Unisoc)
    int sim_id = mp_obj_get_int(args[0]);
    int ret = 0;
    char apn[99+1] = {0};

    if (n_args == 1)
    {
        ret = Helios_DataCall_GetApn(2, sim_id, apn);
    	if (ret == 0)
    	{
    		return mp_obj_new_str(apn, strlen(apn));
    	}
    	return mp_obj_new_int(-1);
    }
    else if (n_args == 2)
    {
        int pid = mp_obj_get_int(args[1]);
        
        ret = Helios_DataCall_GetApn(3, sim_id, apn, pid);
    	if (ret == 0)
    	{
    		return mp_obj_new_str(apn, strlen(apn));
    	}
    	return mp_obj_new_int(-1);
    }
    else
    {
        mp_raise_ValueError("invalid value, The number of parameters cannot be greater than 2.");
    }
#else
    return mp_obj_new_str("NOT SUPPORT",strlen("NOT SUPPORT"));
#endif
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_datacall_get_apn_obj, 1, 2, qpy_datacall_get_apn);
#if defined(PLAT_ASR) || defined(PLAT_Unisoc)

STATIC mp_obj_t qpy_datacall_set_dns_server(size_t n_args, const mp_obj_t *args)
{
    int profile_idx  = mp_obj_get_int(args[0]);
    int sim_id     = mp_obj_get_int(args[1]);

    mp_buffer_info_t new_pri_dns = {0};
    mp_buffer_info_t new_sec_dns = {0};
    mp_get_buffer_raise(args[2], &new_pri_dns, MP_BUFFER_READ);
    mp_get_buffer_raise(args[3], &new_sec_dns, MP_BUFFER_READ);

    char new_pri[128];
    char new_sec[128];
    memset(&new_pri, 0, sizeof(new_pri));
    memset(&new_sec, 0, sizeof(new_sec));

    memcpy(new_pri, new_pri_dns.buf, new_pri_dns.len);
    memcpy(new_sec, new_sec_dns.buf, new_sec_dns.len);

    int ret = 0;
    ret = Helios_DataCall_SetDnsServer(profile_idx, sim_id, new_pri, new_sec);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_datacall_set_dns_server_obj, 4, 4, qpy_datacall_set_dns_server);
#endif

STATIC mp_obj_t qpy_module_datacall_deinit(void)
{
	g_usr_callback = NULL;
    MOD_DATACALL_LOG("module datacall deinit.\r\n");
#if defined(PLAT_ASR) || defined(PLAT_Unisoc)
	Helios_DataCall_Deinit();
#endif
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_module_datacall_deinit_obj, qpy_module_datacall_deinit);

STATIC const mp_rom_map_elem_t mp_module_datacall_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__),		MP_ROM_QSTR(MP_QSTR_dial) },
	{ MP_ROM_QSTR(MP_QSTR___qpy_module_deinit__),   MP_ROM_PTR(&qpy_module_datacall_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_start),			MP_ROM_PTR(&qpy_datacall_start_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setAutoConnect),	MP_ROM_PTR(&qpy_datacall_set_autoconnect_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getInfo),			MP_ROM_PTR(&qpy_datacall_get_info_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setCallback),		MP_ROM_PTR(&qpy_register_usr_callback_obj) },
	{ MP_ROM_QSTR(MP_QSTR_recordApn),		MP_ROM_PTR(&qpy_datacall_record_apn_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setAsynMode),		MP_ROM_PTR(&qpy_datacall_set_asynmode_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getPdpRange),		MP_ROM_PTR(&qpy_get_pdp_range_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getApn), 	        MP_ROM_PTR(&qpy_datacall_get_apn_obj) },
	#if defined(PLAT_ASR) || defined(PLAT_Unisoc)
	{ MP_ROM_QSTR(MP_QSTR_setDnsserver),    MP_ROM_PTR(&qpy_datacall_set_dns_server_obj) },
	#endif
};
STATIC MP_DEFINE_CONST_DICT(mp_module_datacall_globals, mp_module_datacall_globals_table);


const mp_obj_module_t mp_module_dial = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_datacall_globals,
};


