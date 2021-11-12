#include <stdlib.h>
#include <string.h>
#include "obj.h"
#include "objstr.h"
#include "runtime.h"
#include "binary.h"
#include "objarray.h"

#include "poc.h"
#include "helios_debug.h"

#define POC_LOG(msg, ...)      custom_log(poc, msg, ##__VA_ARGS__)


typedef struct _poc_obj_t 
{
	mp_obj_base_t base;
	unsigned char in_group;
	mp_obj_t init_callback;
	mp_obj_t login_callback;
	mp_obj_t calluser_callback;
	mp_obj_t audio_callback;
	mp_obj_t join_group_callback;
	mp_obj_t listupdate_callback;
	mp_obj_t member_change_callback;
	mp_obj_t upgrade_callback;
} poc_obj_t;

static poc_obj_t poc_obj = {0};


//init
void qpy_poc_isr(int ret) {

    if(poc_obj.init_callback && mp_obj_is_callable(poc_obj.init_callback)){
    	mp_sched_schedule(poc_obj.init_callback, MP_OBJ_FROM_PTR(mp_obj_new_int(ret)));
	}
}

int qpy_poc_login_callback(int online) {
    if(poc_obj.login_callback && mp_obj_is_callable(poc_obj.login_callback)){
        mp_sched_schedule(poc_obj.login_callback, MP_OBJ_FROM_PTR(mp_obj_new_int(online)));  
    }
    return online;
}

int qpy_poc_calluser_callback(int ret) {
    if(poc_obj.calluser_callback && mp_obj_is_callable(poc_obj.calluser_callback)){
        mp_sched_schedule(poc_obj.calluser_callback, MP_OBJ_FROM_PTR(mp_obj_new_int(ret)));  
    }
    return ret;
}

void qpy_poc_register_audio_callback(AUDIO_STATE state, uid_t uid, const char* name, int flag) {
     mp_obj_t audio_cb[4] = {
         mp_obj_new_int(state),
         mp_obj_new_int(uid),
         mp_obj_new_str(name,strlen(name)),
         mp_obj_new_int(flag)
     };
    if(poc_obj.audio_callback && mp_obj_is_callable(poc_obj.audio_callback)){
    	mp_sched_schedule(poc_obj.audio_callback, MP_OBJ_FROM_PTR(mp_obj_new_list(4, audio_cb)));
	}
}

void qpy_poc_register_join_group_callback(const char* groupname, gid_t gid) {
     mp_obj_t audio_cb[2] = {
         mp_obj_new_str(groupname,strlen(groupname)),
         mp_obj_new_int(gid)
     };
	poc_obj.in_group = 1;
    if(poc_obj.join_group_callback && mp_obj_is_callable(poc_obj.join_group_callback)){
    	mp_sched_schedule(poc_obj.join_group_callback, MP_OBJ_FROM_PTR(mp_obj_new_list(2, audio_cb)));
	}
}

void qpy_poc_register_listupdate_callback(int flag) {
    if(poc_obj.listupdate_callback && mp_obj_is_callable(poc_obj.listupdate_callback)){
    	mp_sched_schedule(poc_obj.listupdate_callback, MP_OBJ_FROM_PTR(mp_obj_new_int(flag)));
	}
}

void qpy_poc_register_member_change_callback(int flag, int nun, uid_t* uids) {
    mp_obj_t member_change[4] = {
         mp_obj_new_int(flag),
         mp_obj_new_int(nun),
         mp_obj_new_str(uids,strlen(uids)),
     };
    if(poc_obj.member_change_callback && mp_obj_is_callable(poc_obj.member_change_callback)){
    	mp_sched_schedule(poc_obj.member_change_callback, MP_OBJ_FROM_PTR(mp_obj_new_list(4,member_change)));
	}
}

void qpy_poc_egister_upgrade_callback(int ret) {
    if(poc_obj.upgrade_callback && mp_obj_is_callable(poc_obj.upgrade_callback)){
    	mp_sched_schedule(poc_obj.upgrade_callback, MP_OBJ_FROM_PTR(mp_obj_new_int(ret)));
	}
}


STATIC mp_obj_t qpy_poc_init(mp_obj_t arg)
{
	poc_ops_t *poc_ops = poc_get_ops();
	
    poc_obj.init_callback = arg;

	POC_LOG("poc init\n");
    poc_ops->init(qpy_poc_isr);
	
    poc_ops->register_audio_cb(qpy_poc_register_audio_callback);
	
    poc_ops->register_join_group_cb(qpy_poc_register_join_group_callback);
	
    poc_ops->register_listupdate_cb(qpy_poc_register_listupdate_callback);

	
    poc_ops->register_upgrade_cb(qpy_poc_egister_upgrade_callback);

	
    poc_ops->register_member_change_cb(qpy_poc_register_member_change_callback);
	

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_init_obj, qpy_poc_init);
//init end


//free
STATIC mp_obj_t qpy_poc_free()
{
	POC_LOG("poc free\n");
	poc_ops_t *poc_ops = poc_get_ops();
    poc_ops->free();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_poc_free_obj, qpy_poc_free);
//free end

// log
STATIC mp_obj_t qpy_poc_log(mp_obj_t value) 
{
	
	poc_ops_t *poc_ops = poc_get_ops();
    int log_enable = mp_obj_get_int(value);
	POC_LOG("poc log enable = %d\n",log_enable);
	poc_ops->log((boolean)log_enable);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_log_obj, qpy_poc_log);
//log end


//login

STATIC mp_obj_t qpy_poc_login(mp_obj_t args)
{
	
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    poc_obj.login_callback = args;
	POC_LOG("poc login \n");
    ret = poc_ops->login(qpy_poc_login_callback);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_login_obj, qpy_poc_login);
//login end

//logout 
STATIC mp_obj_t qpy_poc_logout()
{
	
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = poc_ops->logout();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_poc_logout_obj, qpy_poc_logout);
//logout end

//joingroup
STATIC mp_obj_t qpy_poc_joingroup(mp_obj_t args)
{
	
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = -1;
	poc_obj.in_group = 0;
    unsigned int gid = mp_obj_get_int(args);

	POC_LOG("poc joingroup\n");
    ret = poc_ops->joingroup(gid);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_joingroup_obj, qpy_poc_joingroup);
//joingroup end

//leavegroup 
STATIC mp_obj_t qpy_poc_leavegroup()
{
	
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    ret = poc_ops->leavegroup();
	if(ret== 0)
		poc_obj.in_group = 0;
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_poc_leavegroup_obj, qpy_poc_leavegroup);
//leavegroup end

//speak
STATIC mp_obj_t qpy_poc_speak(mp_obj_t value) 
{
	
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    int speak = mp_obj_get_int(value);
	ret = poc_ops->speak(speak);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_speak_obj, qpy_poc_speak);
//speak end

//calluser

STATIC mp_obj_t qpy_poc_calluser(mp_obj_t poc_uid, mp_obj_t poc_cb)
{
	poc_ops_t *poc_ops = poc_get_ops();

    int ret = 0;
	
    int uid = mp_obj_get_int(poc_uid);
	POC_LOG("poc call user uid %d",uid);
    poc_obj.calluser_callback = poc_cb;

    ret = poc_ops->calluser(uid,qpy_poc_calluser_callback);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_poc_calluser_obj, qpy_poc_calluser);
//calluser end

//group_getbyindexr
STATIC mp_obj_t qpy_poc_group_getbyindex(mp_obj_t value) 
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    group_t group_info = {0};
    int index = mp_obj_get_int(value);
	ret = poc_ops->group_getbyindex(index, &group_info);
	
	POC_LOG("qpy_poc_group_getbyindex ret = %d",ret);
	if(ret != 0 || group_info.name == NULL) return mp_obj_new_int(-1);
    
    mp_obj_t group_info_list[4] = {
			mp_obj_new_int(group_info.gid), 
			mp_obj_new_str(group_info.name, strlen(group_info.name)), 
			mp_obj_new_int(group_info.type), 
			mp_obj_new_int(group_info.index)};

    return mp_obj_new_list(4, group_info_list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_group_getbyindex_obj, qpy_poc_group_getbyindex);
//group_getbyindexr end

//group_getbyid
STATIC mp_obj_t qpy_poc_group_getbyid(mp_obj_t value) 
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    group_t group_info = {0};
    int id = mp_obj_get_int(value);

	if(id == 0 && poc_obj.in_group == 0) return mp_obj_new_int(-1);
	
	ret = poc_ops->group_getbyid(id, &group_info);
	POC_LOG("qpy_poc_group_getbyid ret = %d",ret);
	if(ret != 0 || group_info.name == NULL) return mp_obj_new_int(-1);
    
    mp_obj_t group_info_list[4] = {
			mp_obj_new_int(group_info.gid), 
			mp_obj_new_str(group_info.name, strlen(group_info.name)), 
			mp_obj_new_int(group_info.type), 
			mp_obj_new_int(group_info.index)};

    return mp_obj_new_list(4, group_info_list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_group_getbyid_obj, qpy_poc_group_getbyid);
//group_getbyid end

//member_getbyid
STATIC mp_obj_t qpy_poc_member_getbyid(mp_obj_t value) 
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    member_t member_info = {0};
    int uid = mp_obj_get_int(value);
	ret = poc_ops->member_getbyid(uid, &member_info);
	
	POC_LOG("qpy_poc_member_getbyid ret = %d",ret);
	if(ret != 0 || member_info.name == NULL) return mp_obj_new_int(-1);
    
    mp_obj_t member_info_list[4] = {
			mp_obj_new_int(member_info.uid), 
			mp_obj_new_str(member_info.name, strlen(member_info.name)), 
			mp_obj_new_int(member_info.state), 
			mp_obj_new_int(member_info.index)};

    return mp_obj_new_list(4, member_info_list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_member_getbyid_obj, qpy_poc_member_getbyid);
//member_getbyid end

//get_loginstate 
STATIC mp_obj_t qpy_poc_get_loginstate()
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    ret = poc_ops->get_loginstate();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_poc_get_loginstate_obj, qpy_poc_get_loginstate);
//get_loginstate end

//get_groupcount 
STATIC mp_obj_t qpy_poc_get_groupcount()
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    ret = poc_ops->get_groupcount();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_poc_get_groupcount_obj, qpy_poc_get_groupcount);
//get_groupcount end
//get_grouplist
STATIC mp_obj_t qpy_poc_get_grouplist(mp_obj_t poc_index_begin, mp_obj_t poc_count)
{
	poc_ops_t *poc_ops = poc_get_ops();
	int ret = 0;
    int actual_read_num = 0;
    int i = 0;
	
    int index_begin = mp_obj_get_int(poc_index_begin);
    int count = mp_obj_get_int(poc_count);
    static group_t *group_list = NULL;
	if(group_list != NULL) {
		free(group_list);
		group_list = NULL;
	}
    int group_num = poc_ops->get_groupcount();
    group_list = malloc(sizeof(group_t)*group_num);
	memset(group_list, 0, sizeof(group_t)*group_num);

    actual_read_num = poc_ops->get_grouplist(group_list, sizeof(group_t)*group_num, index_begin, count);
    static mp_obj_t *group_list_py = NULL;
	static mp_obj_t *tuple_return = NULL;
	if(group_list_py != NULL) {
		free(group_list_py);
		group_list_py = NULL;
	}
	if(tuple_return != NULL) {
		free(tuple_return);
		tuple_return = NULL;
	}
	
    group_list_py = malloc(sizeof(mp_obj_t)*actual_read_num*4);
	memset(group_list_py,0,sizeof(mp_obj_t)*actual_read_num*4);
    tuple_return = malloc(sizeof(mp_obj_t)*actual_read_num);
	memset(tuple_return,0,sizeof(mp_obj_t)*actual_read_num);
    if(actual_read_num > 0) {
        for(i = 0; i < actual_read_num; i++) {
            group_list_py[i*4] = mp_obj_new_int(group_list[i].gid);
            group_list_py[i*4+1] = mp_obj_new_str(group_list[i].name, strlen(group_list[i].name));
            group_list_py[i*4+2] = mp_obj_new_int(group_list[i].type);
            group_list_py[i*4+3] = mp_obj_new_int(group_list[i].index);
            tuple_return[i] = mp_obj_new_list(4, group_list_py+4*i);
        }
        return mp_obj_new_tuple(actual_read_num, tuple_return);
    }else {
        return mp_obj_new_int(-1);
    }
	
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_poc_get_grouplist_obj, qpy_poc_get_grouplist);
//get_grouplist end

//get_membercount
STATIC mp_obj_t qpy_poc_get_membercount(mp_obj_t value) 
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    int gid = mp_obj_get_int(value);
	ret = poc_ops->get_membercount(gid);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_get_membercount_obj, qpy_poc_get_membercount);
//get_membercount end

//get_memberlist
STATIC mp_obj_t qpy_poc_get_memberlist(mp_obj_t poc_gid, mp_obj_t poc_index_begin, mp_obj_t poc_count)
{
	poc_ops_t *poc_ops = poc_get_ops();
	int ret = 0;
    int actual_read_num = 0;
    int i = 0;
	
    int gid = mp_obj_get_int(poc_gid);
    int index_begin = mp_obj_get_int(poc_index_begin);
    int count = mp_obj_get_int(poc_count);

    static member_t *member_list = NULL;
	if(member_list != NULL) {
		free(member_list);
		member_list = NULL;
	}
    int member_num = poc_ops->get_membercount(gid);
    member_list = malloc(sizeof(member_t)*member_num);
	memset(member_list, 0, sizeof(member_t)*member_num);

    actual_read_num = poc_ops->get_memberlist(gid, member_list, (sizeof(member_t)*member_num), index_begin, count);
    static mp_obj_t *member_list_py = NULL;
	static mp_obj_t *tuple_return = NULL;
	if(member_list_py != NULL) {
		free(member_list_py);
		member_list_py = NULL;
	}
	if(tuple_return != NULL) {
		free(tuple_return);
		tuple_return = NULL;
	}
	
    member_list_py = malloc(sizeof(mp_obj_t)*actual_read_num*5);
    tuple_return = malloc(sizeof(mp_obj_t)*actual_read_num);
	memset(member_list_py,0,sizeof(mp_obj_t)*actual_read_num*5);
	memset(tuple_return,0,sizeof(mp_obj_t)*actual_read_num);
    if(actual_read_num > 0) {
        for(i = 0; i < actual_read_num; i++) {
            member_list_py[i*5] = mp_obj_new_int(member_list[i].uid);
            member_list_py[i*5+1] = mp_obj_new_str(member_list[i].name, strlen(member_list[i].name));
            member_list_py[i*5+2] = mp_obj_new_int(member_list[i].state);
            member_list_py[i*5+3] = mp_obj_new_int(member_list[i].prior);
            member_list_py[i*5+4] = mp_obj_new_int(member_list[i].index);
            tuple_return[i] = mp_obj_new_list(5, member_list_py+5*i);
        }
        return mp_obj_new_tuple(actual_read_num, tuple_return);
    }else {
        return mp_obj_new_int(-1);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(qpy_poc_get_memberlist_obj, qpy_poc_get_memberlist);
//get_memberlist end

//get_audiostate 
STATIC mp_obj_t qpy_poc_get_audiostate()
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    ret = poc_ops->get_audiostate();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_poc_get_audiostate_obj, qpy_poc_get_audiostate);
//get_audiostate end

//play_tts
STATIC mp_obj_t qpy_poc_play_tts(mp_obj_t poc_tts, mp_obj_t poc_interrupt)
{
	poc_ops_t *poc_ops = poc_get_ops();
	int ret = 0;

    char *path = (char*)mp_obj_str_get_str(poc_tts);
    int interrupt = mp_obj_get_int(poc_interrupt);
    ret = poc_ops->play_tts(path, interrupt);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_poc_play_tts_obj, qpy_poc_play_tts);
//play_tts end

//send_ping 
STATIC mp_obj_t qpy_poc_send_ping()
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    ret = poc_ops->send_ping();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_poc_send_ping_obj, qpy_poc_send_ping);
//send_ping end

//get_version 
STATIC mp_obj_t qpy_poc_get_version()
{
	poc_ops_t *poc_ops = poc_get_ops();
    char version[64] = {0};
    poc_ops->get_version(version);
	return mp_obj_new_str(version, strlen(version));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_poc_get_version_obj, qpy_poc_get_version);
//get_version end

//set_tts_enable
STATIC mp_obj_t qpy_poc_set_tts_enable(mp_obj_t value) 
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    int enable = mp_obj_get_int(value);
	ret = poc_ops->set_tts_enable(enable);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_set_tts_enable_obj, qpy_poc_set_tts_enable);
//set_tts_enable end

//register_audio_cb 

STATIC mp_obj_t qpy_poc_register_audio_cb(mp_obj_t args)
{
	poc_ops_t *poc_ops = poc_get_ops();
    poc_obj.audio_callback = args;
	
	POC_LOG("poc register audio cb\n");

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_register_audio_cb_obj, qpy_poc_register_audio_cb);
//register_audio_cb end

//register_join_group_cb 

STATIC mp_obj_t qpy_poc_register_join_group_cb(mp_obj_t args)
{
	poc_ops_t *poc_ops = poc_get_ops();
    poc_obj.join_group_callback = args;
	POC_LOG("poc register join ggroup cb\n");


    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_register_join_group_cb_obj, qpy_poc_register_join_group_cb);
//register_join_group_cb end

//register_listupdate_cb 

STATIC mp_obj_t qpy_poc_register_listupdate_cb(mp_obj_t args)
{
	poc_ops_t *poc_ops = poc_get_ops();
    poc_obj.listupdate_callback = args;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_register_listupdate_cb_obj, qpy_poc_register_listupdate_cb);
//register_listupdate_cb end

//register_member_change_cb 

STATIC mp_obj_t qpy_poc_register_member_change_cb(mp_obj_t args)
{
	poc_ops_t *poc_ops = poc_get_ops();
    poc_obj.member_change_callback = args;


    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_register_member_change_cb_obj, qpy_poc_register_member_change_cb);
//register_member_change_cb end

//set_call_end_timeptr
STATIC mp_obj_t qpy_poc_set_call_end_timeptr(mp_obj_t value) 
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    int timeptr = mp_obj_get_int(value);
	POC_LOG("poc set call end timeptr = %d\n",timeptr);
	ret = poc_ops->set_call_end_timeptr(timeptr);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_set_call_end_timeptr_obj, qpy_poc_set_call_end_timeptr);
//set_call_end_timeptr end

//set_call_auto_leave_temp_group
STATIC mp_obj_t qpy_poc_set_call_auto_leave_temp_group(mp_obj_t args) 
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    int flag = mp_obj_get_int(args);
	ret = poc_ops->set_call_auto_leave_temp_group(flag);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_set_call_auto_leave_temp_group_obj, qpy_poc_set_call_auto_leave_temp_group);
//set_call_auto_leave_temp_group end

//register_upgrade_cb 

STATIC mp_obj_t qpy_poc_register_upgrade_cb(const mp_obj_t args)
{
    poc_obj.upgrade_callback = args;
	POC_LOG("poc register upgrade_cb\n");


    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_register_upgrade_cb_obj, qpy_poc_register_upgrade_cb);
//register_upgrade_cb end

//set_notify_mode
STATIC mp_obj_t qpy_poc_set_notify_mode(mp_obj_t value) 
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    int flags = mp_obj_get_int(value);
	
	POC_LOG("qpy_poc_set_notify_mode flag = %d\n",flags);
	poc_ops->set_notify_mode(flags);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_set_notify_mode_obj, qpy_poc_set_notify_mode);
//set_notify_mode end

//set_solution
STATIC mp_obj_t qpy_poc_set_solution(const mp_obj_t arg) 
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    char *solution = mp_obj_str_get_str(arg);
	POC_LOG("qpy_poc_set_solution solution = %s\n",solution);
	ret = poc_ops->set_solution(solution);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_set_solution_obj, qpy_poc_set_solution);
//set_solution end

//set_solution_version
STATIC mp_obj_t qpy_poc_set_solution_version(const mp_obj_t arg) 
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    char *version = mp_obj_str_get_str(arg);
	POC_LOG("poc version = %s\n",version);
	ret = poc_ops->set_solution_version(version);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_set_solution_version_obj, qpy_poc_set_solution_version);
//set_solution_version end

//set_productInfo
STATIC mp_obj_t qpy_poc_set_productInfo(const mp_obj_t arg) 
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    char *productInfo = mp_obj_str_get_str(arg);
	POC_LOG("poc productinfo = %s\n", productInfo);
	ret = poc_ops->set_productInfo(productInfo);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_set_productInfo_obj, qpy_poc_set_productInfo);
//set_productInfo end

//set_manufacturer
STATIC mp_obj_t qpy_poc_set_manufacturer(const mp_obj_t arg) 
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    char *manufacturer = mp_obj_str_get_str(arg);
	POC_LOG("poc manufacturer = %s\n",manufacturer);
	ret = poc_ops->set_manufacturer(manufacturer);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_poc_set_manufacturer_obj, qpy_poc_set_manufacturer);
//set_manufacturer end

//get_init_status 
STATIC mp_obj_t qpy_poc_get_init_status()
{
	poc_ops_t *poc_ops = poc_get_ops();
    int ret = 0;
    ret = poc_ops->get_init_status();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_poc_get_init_status_obj, qpy_poc_get_init_status);
//get_init_status end


STATIC const mp_rom_map_elem_t mp_module_poc_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_poc) },

	{ MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&qpy_poc_init_obj) },
	{ MP_ROM_QSTR(MP_QSTR_free), MP_ROM_PTR(&qpy_poc_free_obj) },
	{ MP_ROM_QSTR(MP_QSTR_log), MP_ROM_PTR(&qpy_poc_log_obj) },
#if 1
    { MP_ROM_QSTR(MP_QSTR_login), MP_ROM_PTR(&qpy_poc_login_obj) },
    { MP_ROM_QSTR(MP_QSTR_logout), MP_ROM_PTR(&qpy_poc_logout_obj) },
    { MP_ROM_QSTR(MP_QSTR_joingroup), MP_ROM_PTR(&qpy_poc_joingroup_obj) },
    { MP_ROM_QSTR(MP_QSTR_leavegroup), MP_ROM_PTR(&qpy_poc_leavegroup_obj) },
    { MP_ROM_QSTR(MP_QSTR_speak), MP_ROM_PTR(&qpy_poc_speak_obj) },
    { MP_ROM_QSTR(MP_QSTR_calluser), MP_ROM_PTR(&qpy_poc_calluser_obj) },
    { MP_ROM_QSTR(MP_QSTR_group_getbyindex), MP_ROM_PTR(&qpy_poc_group_getbyindex_obj) },
    { MP_ROM_QSTR(MP_QSTR_group_getbyid), MP_ROM_PTR(&qpy_poc_group_getbyid_obj) },
    { MP_ROM_QSTR(MP_QSTR_member_getbyid), MP_ROM_PTR(&qpy_poc_member_getbyid_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_loginstate), MP_ROM_PTR(&qpy_poc_get_loginstate_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_groupcount), MP_ROM_PTR(&qpy_poc_get_groupcount_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_grouplist), MP_ROM_PTR(&qpy_poc_get_grouplist_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_membercount), MP_ROM_PTR(&qpy_poc_get_membercount_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_memberlist), MP_ROM_PTR(&qpy_poc_get_memberlist_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_audiostate), MP_ROM_PTR(&qpy_poc_get_audiostate_obj) },
    { MP_ROM_QSTR(MP_QSTR_play_tts), MP_ROM_PTR(&qpy_poc_play_tts_obj) },
    { MP_ROM_QSTR(MP_QSTR_send_ping), MP_ROM_PTR(&qpy_poc_send_ping_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_send_gpsinfo), MP_ROM_PTR(&qpy_poc_send_gpsinfo_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_version), MP_ROM_PTR(&qpy_poc_get_version_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_current_zone_time), MP_ROM_PTR(&qpy_poc_current_zone_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_tts_enable), MP_ROM_PTR(&qpy_poc_set_tts_enable_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_set_vol), MP_ROM_PTR(&qpy_poc_set_vol_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_get_vol), MP_ROM_PTR(&qpy_poc_get_vol_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_get_account_day), MP_ROM_PTR(&qpy_poc_get_account_day_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_set_rec_audio_mode), MP_ROM_PTR(&qpy_poc_set_rec_audio_mode_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_get_rec_audio_coun), MP_ROM_PTR(&qpy_poc_get_rec_audio_coun_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_get_rec_audio_info), MP_ROM_PTR(&qpy_poc_get_rec_audio_info_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_play_rec_audio), MP_ROM_PTR(&qpy_poc_play_rec_audio_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_stop_play_rec_audio), MP_ROM_PTR(&qpy_poc_stop_play_rec_audio_obj) },

    { MP_ROM_QSTR(MP_QSTR_register_audio_cb), MP_ROM_PTR(&qpy_poc_register_audio_cb_obj) },
    { MP_ROM_QSTR(MP_QSTR_register_join_group_cb), MP_ROM_PTR(&qpy_poc_register_join_group_cb_obj) },
    { MP_ROM_QSTR(MP_QSTR_register_listupdate_cb), MP_ROM_PTR(&qpy_poc_register_listupdate_cb_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_call_auto_leave_temp_group), MP_ROM_PTR(&qpy_poc_set_call_auto_leave_temp_group_obj) },
    //...
    { MP_ROM_QSTR(MP_QSTR_set_call_end_timeptr), MP_ROM_PTR(&qpy_poc_set_call_end_timeptr_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_set_call_end_timeptr), MP_ROM_PTR(&qpy_poc_set_call_end_timeptr_obj) },
    //...
    { MP_ROM_QSTR(MP_QSTR_register_upgrade_cb), MP_ROM_PTR(&qpy_poc_register_upgrade_cb_obj) },
    //...
    { MP_ROM_QSTR(MP_QSTR_set_notify_mode), MP_ROM_PTR(&qpy_poc_set_notify_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_solution), MP_ROM_PTR(&qpy_poc_set_solution_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_solution_version), MP_ROM_PTR(&qpy_poc_set_solution_version_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_productInfo), MP_ROM_PTR(&qpy_poc_set_productInfo_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_manufacturer), MP_ROM_PTR(&qpy_poc_set_manufacturer_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_init_status), MP_ROM_PTR(&qpy_poc_get_init_status_obj) },
 #endif
};
STATIC MP_DEFINE_CONST_DICT(mp_module_poc_globals, mp_module_poc_globals_table);


const mp_obj_module_t mp_module_poc = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_poc_globals,
};




