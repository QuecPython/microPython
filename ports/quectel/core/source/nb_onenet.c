
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "runtime.h"
#include "gc.h"
#include "mphal.h"
#include "mperrno.h"

#include "mphalport.h"
#include "helios_ril_onenet.h"
#if  MICROPY_PY_NB_ONENET


#define ONENET_ADD_OBJECT_CNT      6
#define ONENET_OBSERVER_RESP_LIST  3
#define ONENET_CONFIG_LIST         7
#define ONENET_DISCOVER_RESP_LIST  6
#define ONENET_WRITE_RESP_LIST     3
#define ONENET_READ_RESP_LIST      11
#define ONENET_NOTIFY_LIST         10

static c_callback_t *onenet_callback_cur = NULL;

//s32 Helios_ONENET_Create(void);
STATIC mp_obj_t qpy_ONENET_Create()
{
	int ret = -1;
	ret = Helios_ONENET_Create();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_ONENET_Create_obj, qpy_ONENET_Create);

//typedef struct{
// u32 ref;         // Instance ID of OneNET communication suite..
// u32 obj_id;       //Object identifier. If the object ID is not existed, the module will return error..        
// u32 ins_count;   //Length of data sent.
// u8* insbitmap;   //Instance bitmap. A string which should be marked with double quotation marks  For example,
//                  //if <insCount>=4, and the <insBitmap>="1101", it means the instance ID 0, 1, 3 will be registered, and the instance ID 2 will not be registered.
// u32 attrcount;   //Attribute count, which indicate the count of readable and/or writeable resources.
// u32 actcount;    //Action count, which indicate the count of executable resources.
//}Helios_ONENET_Obj_Param_t;
static s32 resolve_ONENET_Addobj_list(mp_obj_t mp_data, Helios_ONENET_Obj_Param_t *onenet_obj_param)
{
	size_t len = 0;
    mp_obj_t *items = NULL;
    mp_obj_list_get(mp_data, &len, &items);
	if (len < ONENET_ADD_OBJECT_CNT ) 
	{
		return -1;
	}
	for (u32 i = 0; i < len; i++) 
	{
		mp_obj_t value = items[i];
		switch ( i ) {
			case 0:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_obj_param->ref = mp_obj_get_int_truncated(value);
				break;
			case 1:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_obj_param->obj_id = mp_obj_get_int_truncated(value);
				break;
			case 2:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_obj_param->ins_count = mp_obj_get_int_truncated(value);
				break;
			case 3:
				if ( !mp_obj_is_str(value) ) return -1;
				mp_buffer_info_t data = {0};
				mp_get_buffer_raise(value, &data, MP_BUFFER_READ);
				onenet_obj_param->insbitmap = data.buf;
				break;
			case 4:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_obj_param->attrcount = mp_obj_get_int_truncated(value);
				break;
			case 5:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_obj_param->actcount = mp_obj_get_int_truncated(value);
				break;
			default:
				return -1;
		}
	}
	return 0;
}
//s32 Helios_ONENET_Addobj(Helios_ONENET_Obj_Param_t *onenet_obj_param_t);
STATIC mp_obj_t qpy_ONENET_Addobj(mp_obj_t mp_data)
{
	int ret = -1;
	Helios_ONENET_Obj_Param_t onenet_obj_param;
	memset(&onenet_obj_param, 0x00, sizeof(onenet_obj_param));
	ret = resolve_ONENET_Addobj_list(mp_data, &onenet_obj_param);
	if( ret != 0 ) {
		return mp_obj_new_int(ret);
	}
	
	ret = Helios_ONENET_Addobj(&onenet_obj_param);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ONENET_Addobj_obj, qpy_ONENET_Addobj);


//s32 Helios_ONENET_Delobj(u32 ref,u32 obj_id);
STATIC mp_obj_t qpy_ONENET_Delobj(mp_obj_t mp_ref, mp_obj_t mp_obj_id)
{
	int ret = -1;
	if ( !mp_obj_is_int(mp_ref) || !mp_obj_is_int(mp_obj_id) ) return mp_obj_new_int(ret);
	
	u32 ref = mp_obj_get_int_truncated(mp_ref);
	u32 obj_id = mp_obj_get_int_truncated(mp_obj_id);
	
	ret = Helios_ONENET_Delobj(ref, obj_id);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_ONENET_Delobj_obj, qpy_ONENET_Delobj);

static void qpy_callback_onenet_req(u8* buffer,u32 length)
{
	if(onenet_callback_cur == NULL) {
		return;
	}
	
    mp_sched_schedule_ex(onenet_callback_cur, MP_OBJ_FROM_PTR(mp_obj_new_str((char *)buffer, length)));
}
//s32 Helios_ONENET_Open(u32 ref, u32 lifetime, void (*callback_onenet_req)(u8* buffer,u32 length) );
STATIC mp_obj_t qpy_ONENET_Open(mp_obj_t mp_ref, mp_obj_t mp_lifetime, mp_obj_t mp_callback)
{
	int ret = -1;
	if ( !mp_obj_is_int(mp_ref) || !mp_obj_is_int(mp_lifetime) ) return mp_obj_new_int(ret);
	u32 ref = mp_obj_get_int_truncated(mp_ref);
	u32 lifetime = mp_obj_get_int_truncated(mp_lifetime);
	
	static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
	onenet_callback_cur = &cb;
	mp_sched_schedule_callback_register(onenet_callback_cur, mp_callback);
	
	ret = Helios_ONENET_Open(ref, lifetime, qpy_callback_onenet_req);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(qpy_ONENET_Open_obj, qpy_ONENET_Open);

//typedef struct{
//u32 ref;    //Instance ID of OneNET communication suite.
//u32 msgid;  //The message identifier, which comes from the URC "+ MIPLOBSERVE:".
//Helios_Enum_ONENET_Observe_Result  obderve_result;// The result of observe.
//u8  raimode;
//}Helios_ONENET_Observe_Param_t;
static s32 resolve_ONENET_Observer_Rsp_list(mp_obj_t mp_data, Helios_ONENET_Observe_Param_t *onenet_observe_param)
{
	size_t len = 0;
    mp_obj_t *items = NULL;
    mp_obj_list_get(mp_data, &len, &items);
	if (len < ONENET_OBSERVER_RESP_LIST ) 
	{
		return -1;
	}
	for (u32 i = 0; i < len; i++) 
	{
		mp_obj_t value = items[i];
		switch ( i ) {
			case 0:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_observe_param->ref = mp_obj_get_int_truncated(value);
				break;
			case 1:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_observe_param->msgid = mp_obj_get_int_truncated(value);
				break;
			case 2:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_observe_param->obderve_result = mp_obj_get_int_truncated(value);
				break;
			case 3:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_observe_param->raimode = mp_obj_get_int_truncated(value);
				break;
			default:
				return -1;
		}
	}
	return 0;
}
//s32 Helios_ONENET_Observer_Rsp(Helios_ONENET_Observe_Param_t* onenet_observe_param_t);
STATIC mp_obj_t qpy_ONENET_Observer_Rsp(mp_obj_t mp_data)
{
	int ret = -1;
	Helios_ONENET_Observe_Param_t onenet_observe_param;
	memset(&onenet_observe_param, 0x00, sizeof(onenet_observe_param));
	ret = resolve_ONENET_Observer_Rsp_list(mp_data, &onenet_observe_param);
	if( ret != 0 ) {
		return mp_obj_new_int(ret);
	}
	
	ret = Helios_ONENET_Observer_Rsp(&onenet_observe_param);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ONENET_Observer_Rsp_obj, qpy_ONENET_Observer_Rsp);

//typedef struct{
// Helios_Enum_ONENET_Bs_Mode      onenet_bs_mode;      
// u8* ip;   
// u32 port; 
// u8 ack_timeout;
// bool obs_autoack;
// bool auto_update;
// bool save_state;
//}Helios_ONENET_Config_Param_t;
static s32 resolve_ONENET_Config_list(mp_obj_t mp_data, Helios_ONENET_Config_Param_t* onenet_config_param)
{
	size_t len = 0;
    mp_obj_t *items = NULL;
    mp_obj_list_get(mp_data, &len, &items);
	if (len < ONENET_CONFIG_LIST ) 
	{
		return -1;
	}
	for (u32 i = 0; i < len; i++) 
	{
		mp_obj_t value = items[i];
		switch ( i ) {
			case 0:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_config_param->onenet_bs_mode = mp_obj_get_int_truncated(value);
				break;
			case 1:
				if ( !mp_obj_is_str(value) ) return -1;
				mp_buffer_info_t data = {0};
				mp_get_buffer_raise(value, &data, MP_BUFFER_READ);
				onenet_config_param->ip = data.buf;
				break;
			case 2:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_config_param->port = mp_obj_get_int_truncated(value);
				break;
			case 3:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_config_param->ack_timeout = mp_obj_get_int_truncated(value);
				break;
			case 4:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_config_param->obs_autoack = mp_obj_get_int_truncated(value);
				break;
			case 5:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_config_param->auto_update = mp_obj_get_int_truncated(value);
				break;
			case 6:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_config_param->save_state = mp_obj_get_int_truncated(value);
				break;
			default:
				return -1;
		}
	}
	return 0;
}
//s32 Helios_ONENET_Config(Helios_ONENET_Config_Param_t* onenet_config_param_t,Helios_Enum_ONENET_Conf_Flag config_flag);
STATIC mp_obj_t qpy_ONENET_Config(mp_obj_t mp_data, mp_obj_t mp_config_flag)
{
	int ret = -1;
	if ( !mp_obj_is_int(mp_config_flag) ) return mp_obj_new_int(ret);
	s32 config_flag = mp_obj_get_int(mp_config_flag);
	
	Helios_ONENET_Config_Param_t onenet_config_param;
	memset(&onenet_config_param, 0x00, sizeof(onenet_config_param));
	ret = resolve_ONENET_Config_list(mp_data, &onenet_config_param);
	if( ret != 0 ) {
		return mp_obj_new_int(ret);
	}
	
	ret = Helios_ONENET_Config(&onenet_config_param, config_flag);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_ONENET_Config_obj, qpy_ONENET_Config);

//typedef struct{
// u32 ref;		 // Instance ID of OneNET communication suite..
// u32 msgid;      //The message identifier, which comes from the URC "+ MIPLDISCOVER:"
// u32 result;     //The result of discover operate,
// u32 length;     //The length of <valuestring>.
// u8* value_string; //A string which includes the attributes of the object and should be marked with double quotation marks. 
// Helios_Enum_ONENET_Rai_Mode  raimode;
//}Helios_ONENET_Discover_Rsp_Param_t;
static s32 resolve_ONENET_Discover_Rsp_list(mp_obj_t mp_data, Helios_ONENET_Discover_Rsp_Param_t* onenet_discover_rsp_param)
{
	size_t len = 0;
    mp_obj_t *items = NULL;
    mp_obj_list_get(mp_data, &len, &items);
	if (len < ONENET_DISCOVER_RESP_LIST ) 
	{
		return -1;
	}
	for (u32 i = 0; i < len; i++) 
	{
		mp_obj_t value = items[i];
		switch ( i ) {
			case 0:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_discover_rsp_param->ref = mp_obj_get_int_truncated(value);
				break;
			case 1:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_discover_rsp_param->msgid = mp_obj_get_int_truncated(value);
				break;
			case 2:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_discover_rsp_param->result = mp_obj_get_int_truncated(value);
				break;
			case 3:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_discover_rsp_param->length = mp_obj_get_int_truncated(value);
				break;
			case 4:
				if ( !mp_obj_is_str(value) ) return -1;
				mp_buffer_info_t data = {0};
				mp_get_buffer_raise(value, &data, MP_BUFFER_READ);
				onenet_discover_rsp_param->value_string = data.buf;
				break;
			case 5:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_discover_rsp_param->raimode = mp_obj_get_int_truncated(value);
				break;
			default:
				return -1;
		}
	}
	return 0;
}
//s32 Helios_ONENET_Discover_Rsp(Helios_ONENET_Discover_Rsp_Param_t *onenet_discover_rsp_param_t);
STATIC mp_obj_t qpy_ONENET_Discover_Rsp(mp_obj_t mp_data)
{
	int ret = -1;
	
	Helios_ONENET_Discover_Rsp_Param_t onenet_discover_rsp_param;
	memset(&onenet_discover_rsp_param, 0x00, sizeof(onenet_discover_rsp_param));
	ret = resolve_ONENET_Discover_Rsp_list(mp_data, &onenet_discover_rsp_param);
	if( ret != 0 ) {
		return mp_obj_new_int(ret);
	}
	
	ret = Helios_ONENET_Discover_Rsp(&onenet_discover_rsp_param);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ONENET_Discover_Rsp_obj, qpy_ONENET_Discover_Rsp);

//typedef struct{
//u32 ref;	//Instance ID of OneNET communication suite.
//u32 msgid;	//The message identifier, which comes from the URC "+MIPLWRITE:".
//Helios_Enum_ONENET_Observe_Result	result;// The result of response.
//Helios_Enum_ONENET_Rai_Mode	raimode;
//}Helios_ONENET_Write_Rsp_Param_t;
static s32 resolve_ONENET_Write_Rsp_list(mp_obj_t mp_data, Helios_ONENET_Write_Rsp_Param_t *onenet_write_rsp_param)
{
	size_t len = 0;
    mp_obj_t *items = NULL;
    mp_obj_list_get(mp_data, &len, &items);
	if (len < ONENET_WRITE_RESP_LIST ) 
	{
		return -1;
	}
	for (u32 i = 0; i < len; i++) 
	{
		mp_obj_t value = items[i];
		switch ( i ) {
			case 0:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_write_rsp_param->ref = mp_obj_get_int_truncated(value);
				break;
			case 1:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_write_rsp_param->msgid = mp_obj_get_int_truncated(value);
				break;
			case 2:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_write_rsp_param->result = mp_obj_get_int_truncated(value);
				break;
			case 3:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_write_rsp_param->raimode = mp_obj_get_int_truncated(value);
				break;
			default:
				return -1;
		}
	}
	return 0;
}
//s32 Helios_ONENET_Write_Rsp(Helios_ONENET_Write_Rsp_Param_t* onenet_write_rsp_param_t);
STATIC mp_obj_t qpy_ONENET_Write_Rsp(mp_obj_t mp_data)
{
	int ret = -1;
	
	Helios_ONENET_Write_Rsp_Param_t onenet_write_rsp_param;
	memset(&onenet_write_rsp_param, 0x00, sizeof(onenet_write_rsp_param));
	ret = resolve_ONENET_Write_Rsp_list(mp_data, &onenet_write_rsp_param);
	if( ret != 0 ) {
		return mp_obj_new_int(ret);
	}
	
	ret = Helios_ONENET_Write_Rsp(&onenet_write_rsp_param);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ONENET_Write_Rsp_obj, qpy_ONENET_Write_Rsp);

//typedef struct{
// u32 ref;		 // Instance ID of OneNET communication suite..
// u32 msgid;      //The message identifier, which comes from the URC "+MIPLREAD:"
// Helios_Enum_ONENET_Observe_Result  result;// The result of response.
// u32 objid;      //Object identifier.
// u32 insid;      //The instance identifier, which comes from the URC "+MIPLOBSERVE :"
// u32 resid;      //The resource identifier, which comes from the URC "+MIPLOBSERVE :".
// Helios_Enum_ONENET_Value_Type value_type; //The value type.
// u32 len;         //The value length.
// u8* value;
// u32 index;      //The index number of the data.
// u32 flag;       //The message indication. The range is 0-2. 
// Helios_Enum_ONENET_Rai_Mode raimode;     //Integer type. Just for raimode 
//}Helios_ONENET_Read_Rsp_Param_t;
static s32 resolve_ONENET_Read_Rsp_list(mp_obj_t mp_data, Helios_ONENET_Read_Rsp_Param_t* onenet_read_param)
{
	size_t len = 0;
    mp_obj_t *items = NULL;
    mp_obj_list_get(mp_data, &len, &items);
	if (len < ONENET_READ_RESP_LIST ) 
	{
		return -1;
	}
	for (u32 i = 0; i < len; i++) 
	{
		mp_obj_t value = items[i];
		switch ( i ) {
			case 0:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_read_param->ref = mp_obj_get_int_truncated(value);
				break;
			case 1:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_read_param->msgid = mp_obj_get_int_truncated(value);
				break;
			case 2:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_read_param->result = mp_obj_get_int_truncated(value);
				break;
			case 3:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_read_param->objid = mp_obj_get_int_truncated(value);
				break;
			case 4:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_read_param->insid = mp_obj_get_int_truncated(value);
				break;
			case 5:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_read_param->resid = mp_obj_get_int_truncated(value);
				break;
			case 6:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_read_param->value_type = mp_obj_get_int_truncated(value);
				break;
			case 7:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_read_param->len = mp_obj_get_int_truncated(value);
				break;
			case 8:
				if ( !mp_obj_is_str(value) ) return -1;
				mp_buffer_info_t data = {0};
				mp_get_buffer_raise(value, &data, MP_BUFFER_READ);
				onenet_read_param->value = data.buf;
				break;
			case 9:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_read_param->index = mp_obj_get_int_truncated(value);
				break;
			case 10:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_read_param->flag = mp_obj_get_int_truncated(value);
				break;
			case 11:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_read_param->raimode = mp_obj_get_int_truncated(value);
				break;
			default:
				return -1;
		}
	}
	return 0;
}
//s32 Helios_ONENET_Read_Rsp(Helios_ONENET_Read_Rsp_Param_t* onenet_read_param_t);
STATIC mp_obj_t qpy_ONENET_Read_Rsp(mp_obj_t mp_data)
{
	int ret = -1;
	
	Helios_ONENET_Read_Rsp_Param_t onenet_read_param;
	memset(&onenet_read_param, 0x00, sizeof(onenet_read_param));
	ret = resolve_ONENET_Read_Rsp_list(mp_data, &onenet_read_param);
	if( ret != 0 ) {
		return mp_obj_new_int(ret);
	}
	
	ret = Helios_ONENET_Read_Rsp(&onenet_read_param);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ONENET_Read_Rsp_obj, qpy_ONENET_Read_Rsp);


//s32 Helios_ONENET_Execute_Rsp(Helios_ONENET_Write_Rsp_Param_t* onenet_write_rsp_param_t);
STATIC mp_obj_t qpy_ONENET_Execute_Rsp(mp_obj_t mp_data)
{
	int ret = -1;
	
	Helios_ONENET_Write_Rsp_Param_t onenet_write_rsp_param;
	memset(&onenet_write_rsp_param, 0x00, sizeof(onenet_write_rsp_param));
	ret = resolve_ONENET_Write_Rsp_list(mp_data, &onenet_write_rsp_param);
	if( ret != 0 ) {
		return mp_obj_new_int(ret);
	}
	
	ret = Helios_ONENET_Execute_Rsp(&onenet_write_rsp_param);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ONENET_Execute_Rsp_obj, qpy_ONENET_Execute_Rsp);

//typedef struct{
// u32 ref;		 // Instance ID of OneNET communication suite..
// u32 msgid;      //The message identifier, which comes from the URC "+MIPLDISCOVER:"
// u32 objid;      //Object identifier.
// u32 insid;      //The instance identifier, which comes from the URC "+MIPLOBSERVE :"
// u32 resid;      //The resource identifier, which comes from the URC "+MIPLOBSERVE :".
// Helios_Enum_ONENET_Value_Type value_type; //The value type.
// u32 len;         //The value length.
// u8* value;
// u32 index;      //The index number of the data.
// u32 flag;       //The message indication. The range is 0-2. 
// u32 ackid;      //Integer type, range: 0-65535
// u32 result;     //Integer type. Just for Read Response
// Helios_Enum_ONENET_Rai_Mode raimode;     //Integer type. Just for raimode 
//}Helios_ONENET_Notify_Param_t;
static s32 resolve_ONENET_Notify_list(mp_obj_t mp_data, Helios_ONENET_Notify_Param_t* onenet_notify_param)
{
	size_t len = 0;
    mp_obj_t *items = NULL;
    mp_obj_list_get(mp_data, &len, &items);
	if (len < ONENET_CONFIG_LIST ) 
	{
		return -1;
	}
	for (u32 i = 0; i < len; i++) 
	{
		mp_obj_t value = items[i];
		switch ( i ) {
			case 0:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_notify_param->ref = mp_obj_get_int_truncated(value);
				break;
			case 1:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_notify_param->msgid = mp_obj_get_int_truncated(value);
				break;
			case 2:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_notify_param->objid = mp_obj_get_int_truncated(value);
				break;
			case 3:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_notify_param->insid = mp_obj_get_int_truncated(value);
				break;
			case 4:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_notify_param->resid = mp_obj_get_int_truncated(value);
				break;
			case 5:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_notify_param->value_type = mp_obj_get_int_truncated(value);
				break;
			case 6:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_notify_param->len = mp_obj_get_int_truncated(value);
				break;
			case 7:
				if ( !mp_obj_is_str(value) ) return -1;
				mp_buffer_info_t data = {0};
				mp_get_buffer_raise(value, &data, MP_BUFFER_READ);
				onenet_notify_param->value = data.buf;
				break;
			case 8:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_notify_param->index = mp_obj_get_int_truncated(value);
				break;
			case 9:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_notify_param->flag = mp_obj_get_int_truncated(value);
				break;
			case 10:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_notify_param->ackid = mp_obj_get_int_truncated(value);
				break;
			case 11:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_notify_param->result = mp_obj_get_int_truncated(value);
				break;
			case 12:
				if ( !mp_obj_is_int(value) ) return -1;
				onenet_notify_param->raimode = mp_obj_get_int_truncated(value);
				break;
			default:
				return -1;
		}
	}
	return 0;
}
//s32 Helios_ONENET_Notify(Helios_ONENET_Notify_Param_t* onenet_notify_param_t,bool ack_flag);
STATIC mp_obj_t qpy_ONENET_Notify(mp_obj_t mp_data, mp_obj_t mp_ack_flag)
{
	int ret = -1;
	if ( !mp_obj_is_int(mp_ack_flag) ) return mp_obj_new_int(ret);
	s32 ack_flag = mp_obj_get_int(mp_ack_flag);
	
	Helios_ONENET_Notify_Param_t onenet_notify_param;
	memset(&onenet_notify_param, 0x00, sizeof(onenet_notify_param));
	ret = resolve_ONENET_Notify_list(mp_data, &onenet_notify_param);
	if( ret != 0 ) {
		return mp_obj_new_int(ret);
	}
	
	ret = Helios_ONENET_Notify(&onenet_notify_param, ack_flag);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_ONENET_Notify_obj, qpy_ONENET_Notify);


//s32 Helios_ONENET_Update(u32 ref,u32 lifetime,Helios_Enum_ONENET_Obj_Flag obj_flag,Helios_Enum_ONENET_Rai_Mode raimode);
STATIC mp_obj_t qpy_ONENET_Update(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	int ret = 0;
	static const mp_arg_t allowed_args[] = 
	{
		{ MP_QSTR_ref, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_lifetime, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_obj_flag, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_raimode, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
	};
	mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
	
	u32 ref = vals[0].u_int;
	u32 lifetime = vals[1].u_int;
	u32 obj_flag = vals[2].u_int;
	u32 raimode = vals[3].u_int;

	ret = Helios_ONENET_Update(ref, lifetime, obj_flag, raimode);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(qpy_ONENET_Update_obj, 2, qpy_ONENET_Update);


//s32 Helios_ONENET_CLOSE(u32 ref);
STATIC mp_obj_t qpy_ONENET_CLOSE(mp_obj_t mp_data)
{
	int ret = -1;
	if ( !mp_obj_is_int(mp_data) ) return mp_obj_new_int(ret);
	
	s32 ref = mp_obj_get_int(mp_data);
	ret = Helios_ONENET_CLOSE(ref);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ONENET_CLOSE_obj, qpy_ONENET_CLOSE);


//s32 Helios_ONENET_DELETE(u32 ref);
STATIC mp_obj_t qpy_ONENET_DELETE(mp_obj_t mp_data)
{
	int ret = -1;
	if ( !mp_obj_is_int(mp_data) ) return mp_obj_new_int(ret);
	
	s32 ref = mp_obj_get_int(mp_data);
	ret = Helios_ONENET_DELETE(ref);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ONENET_DELETE_obj, qpy_ONENET_DELETE);

STATIC const mp_rom_map_elem_t onenet_module_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_create), MP_ROM_PTR(&qpy_ONENET_Create_obj) },
    { MP_ROM_QSTR(MP_QSTR_add_object), MP_ROM_PTR(&qpy_ONENET_Addobj_obj) },
	{ MP_ROM_QSTR(MP_QSTR_delete_object), MP_ROM_PTR(&qpy_ONENET_Delobj_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&qpy_ONENET_Open_obj) },
    { MP_ROM_QSTR(MP_QSTR_observer_resp), MP_ROM_PTR(&qpy_ONENET_Observer_Rsp_obj) },
    { MP_ROM_QSTR(MP_QSTR_config), MP_ROM_PTR(&qpy_ONENET_Config_obj) },
	{ MP_ROM_QSTR(MP_QSTR_discover_resp), MP_ROM_PTR(&qpy_ONENET_Discover_Rsp_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_resp), MP_ROM_PTR(&qpy_ONENET_Write_Rsp_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_resp), MP_ROM_PTR(&qpy_ONENET_Read_Rsp_obj) },
    { MP_ROM_QSTR(MP_QSTR_execute_resp), MP_ROM_PTR(&qpy_ONENET_Execute_Rsp_obj) },
	{ MP_ROM_QSTR(MP_QSTR_notify), MP_ROM_PTR(&qpy_ONENET_Notify_obj) },
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&qpy_ONENET_Update_obj) },
	{ MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&qpy_ONENET_CLOSE_obj) },
    { MP_ROM_QSTR(MP_QSTR_destroy), MP_ROM_PTR(&qpy_ONENET_DELETE_obj) },
};
STATIC MP_DEFINE_CONST_DICT(onenet_module_dict, onenet_module_dict_table);

const mp_obj_module_t module_onenet = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&onenet_module_dict,
};

#endif /*MICROPY_PY_NB_ONENET*/