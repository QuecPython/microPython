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

#ifndef __MOD_NB_H_
#define __MOD_NB_H_

#include "mpconfigport.h"
#include "obj.h"

#if defined(MICROPY_PY_NB_OC)
extern const mp_obj_type_t nb_oc_type;
#endif

#if  defined(MICROPY_PY_NB_AEP)
extern const mp_obj_type_t nb_aep_type;
#endif

#if  defined(MICROPY_PY_NB_ONENET)
extern const mp_obj_module_t module_onenet;
#endif

#endif /* __MOD_NB_H_ */

