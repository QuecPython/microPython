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
 * @file    modlpm.c
 * @author  Pawn.zhou
 * @version V1.0.0
 * @date    2020/10/24
 * @brief   Low power dependent API
 ******************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "obj.h"
#include "mpconfigport.h"


#include "helios_lpm.h"


/*=============================================================================*/
/* FUNCTION: Helios_sleep_enable_mp                                             */
/*=============================================================================*/
/*!@brief     : Set the system sleep switch
 * @param[in]   : s_flag, 0-false, 1-true
 * @param[out]  : 
 * @return    :
 *        -  0--successful
 */
/*=============================================================================*/
STATIC mp_obj_t Helios_sleep_enable_mp(const mp_obj_t s_flag)
{
  int ret = 0;
  int sleep_flag;
  
  sleep_flag = mp_obj_get_int(s_flag);
  if(sleep_flag == 0 || sleep_flag ==1){
	ret = Helios_LPM_AutoSleepEnable((uint32_t) sleep_flag);
    return mp_obj_new_int(ret);
  }else{
	return mp_obj_new_int(-1);
  }

}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(Helios_sleep_enable_mp_obj, Helios_sleep_enable_mp);


/*=============================================================================*/
/* FUNCTION: Helios_lpm_wakelock_lock_mp                                             */
/*=============================================================================*/
/*!@brief     : lock
 * @param[in]   : 
 * @param[out]  : 
 * @return    :
 *        -  0--successful
 *        -  -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t Helios_lpm_wakelock_lock_mp(const mp_obj_t lpm_fd)
{
  int ret = 0;
  int g_lpm_fd;
  
  g_lpm_fd = mp_obj_get_int(lpm_fd);
  
  ret = Helios_LPM_WakeLockAcquire(g_lpm_fd);
  if ( ret == 0 )
  {
    return mp_obj_new_int(0);
  }
  
  return mp_obj_new_int(-1);
}
#if !defined(PLAT_RDA)

STATIC MP_DEFINE_CONST_FUN_OBJ_1(Helios_lpm_wakelock_lock_mp_obj, Helios_lpm_wakelock_lock_mp);
#endif


/*=============================================================================*/
/* FUNCTION: Helios_lpm_wakelock_unlock_mp                                             */
/*=============================================================================*/
/*!@brief     : unlock
 * @param[in]   : 
 * @param[out]  : 
 * @return    :
 *        -  0--successful
  *       -  -1--error
 */
/*=============================================================================*/
STATIC mp_obj_t Helios_lpm_wakelock_unlock_mp(const mp_obj_t lpm_fd)
{
  int ret = 0;
  int g_lpm_fd;
  
  g_lpm_fd = mp_obj_get_int(lpm_fd);
  ret = Helios_LPM_WakeLockRelease(g_lpm_fd);
  if ( ret == 0 )
  {
    return mp_obj_new_int(0);
  }
  
  return mp_obj_new_int(-1);
}
#if !defined(PLAT_RDA)

STATIC MP_DEFINE_CONST_FUN_OBJ_1(Helios_lpm_wakelock_unlock_mp_obj, Helios_lpm_wakelock_unlock_mp);
#endif

/*=============================================================================*/
/* FUNCTION: Helios_lpm_wakelock_create_mp                                             */
/*=============================================================================*/
/*!@brief     : create wakelock
 * @param[in]   : lock_name
 * @param[in]   : name_size
 * @param[out]  : 
 * @return    :
 *        -  wakelock_fd
 */
/*=============================================================================*/
STATIC mp_obj_t Helios_lpm_wakelock_create_mp(size_t n_args, const mp_obj_t *args)
{
  int wakelock_fd;
  int name_size;
  
  char *lock_name = (char *)mp_obj_str_get_str(args[0]);
  name_size = strlen(lock_name);
  
#if 0   //Aim At  EC100Y by kingka
  ret = quec_lpm_task_init();
  if (ret != 0)
  {
    return -1;
  }
#endif

  if((NULL == lock_name) || (0 == name_size))
  {
	return mp_obj_new_int(-1);
  }
  	
  wakelock_fd = Helios_LPM_WakeLockInit(lock_name, name_size);

  
  /*if ( wakelock_fd < 1 || wakelock_fd > 32 )
  {
    return mp_obj_new_int(-1);
  }*/
  // Helios_autosleep_enable(1);
  
  return mp_obj_new_int(wakelock_fd);
}
#if !defined(PLAT_RDA)

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(Helios_lpm_wakelock_create_mp_obj, 1, 2, Helios_lpm_wakelock_create_mp);
#endif

/*=============================================================================*/
/* FUNCTION: Helios_lpm_wakelock_delete_mp                                             */
/*=============================================================================*/
/*!@brief     : delete wakelock
 * @param[in]   : wakelock_fd
 * @param[out]  : 
 * @return    :
 *        -  0
 */
/*=============================================================================*/
STATIC mp_obj_t Helios_lpm_wakelock_delete_mp(const mp_obj_t lpm_fd)
{
  int ret = 0;
  int g_lpm_fd;
  
  g_lpm_fd = mp_obj_get_int(lpm_fd);
  ret = Helios_LPM_WakeLockDeInit(g_lpm_fd);
  return mp_obj_new_int(ret);
}
#if !defined(PLAT_RDA)
STATIC MP_DEFINE_CONST_FUN_OBJ_1(Helios_lpm_wakelock_delete_mp_obj, Helios_lpm_wakelock_delete_mp);
#endif

/*=============================================================================*/
/* FUNCTION: Helios_lpm_get_wakelock_num_mp                                             */
/*=============================================================================*/
/*!@brief     : get wakelock num
 * @param[in]   : 
 * @param[out]  : 
 * @return    :
 *        -  (int)wakelock num
 */
/*=============================================================================*/

STATIC mp_obj_t Helios_lpm_get_wakelock_num_mp()
{
  int num;
  
  num = Helios_LPM_GetWakeLockNum();
  
  return mp_obj_new_int(num);
}
#if !defined(PLAT_RDA)
STATIC MP_DEFINE_CONST_FUN_OBJ_0(Helios_lpm_get_wakelock_num_mp_obj, Helios_lpm_get_wakelock_num_mp);
#endif

/*=============================================================================*/
/* FUNCTION: Helios_lpm_ex_set_mp                                             */
/*=============================================================================*/
/*!@brief     : set power ex
 * @param[in]   : lpm seconds
 * @param[out]  :
 * @return    :
 *        0     success
 *		  else  fail
 */
/*=============================================================================*/
#if defined(PLAT_ASR)
STATIC mp_obj_t Helios_lpm_ex_set_mp(const mp_obj_t lpm_seconds)
{
  int ret = 0;
  int g_lpm_seconds;
  
  g_lpm_seconds = mp_obj_get_int(lpm_seconds);
  ret = Helios_LPM_EXSet(g_lpm_seconds);
  if ( ret == 0 )
  {
    return mp_obj_new_int(0);
  }
  
  return mp_obj_new_int(-1);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(Helios_lpm_ex_set_mp_obj, Helios_lpm_ex_set_mp);
#endif

/*
STATIC mp_obj_t mp_get_free_size()
{
  int num;
  
  num = Helios_rtos_get_free_heap_size();
  
  return mp_obj_new_int(num);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_free_size_obj, mp_get_free_size);
*/


STATIC const mp_rom_map_elem_t pm_module_globals_table[] = {
  { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_pm) },
#if !defined(PLAT_RDA)
  { MP_ROM_QSTR(MP_QSTR_create_wakelock), MP_ROM_PTR(&Helios_lpm_wakelock_create_mp_obj) },
  { MP_ROM_QSTR(MP_QSTR_delete_wakelock), MP_ROM_PTR(&Helios_lpm_wakelock_delete_mp_obj) },
  { MP_ROM_QSTR(MP_QSTR_wakelock_lock), MP_ROM_PTR(&Helios_lpm_wakelock_lock_mp_obj) },
  { MP_ROM_QSTR(MP_QSTR_wakelock_unlock), MP_ROM_PTR(&Helios_lpm_wakelock_unlock_mp_obj) },
  { MP_ROM_QSTR(MP_QSTR_get_wakelock_num), MP_ROM_PTR(&Helios_lpm_get_wakelock_num_mp_obj) },
#endif
  { MP_ROM_QSTR(MP_QSTR_autosleep), MP_ROM_PTR(&Helios_sleep_enable_mp_obj) },
  // { MP_ROM_QSTR(MP_QSTR_getFreeSize), MP_ROM_PTR(&mp_get_free_size_obj) },
#if defined(PLAT_ASR)
  { MP_ROM_QSTR(MP_QSTR_sclkEx_set), MP_ROM_PTR(&Helios_lpm_ex_set_mp_obj) },
#endif
};
STATIC MP_DEFINE_CONST_DICT(pm_module_globals, pm_module_globals_table);

const mp_obj_module_t mp_module_pm = {
  .base = { &mp_type_module },
  .globals = (mp_obj_dict_t *)&pm_module_globals,
};
