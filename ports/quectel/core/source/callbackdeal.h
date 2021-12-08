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

// empty file
#ifndef __CALLBACKDEAL_H__
#define __CALLBACKDEAL_H__

#include "py/runtime.h"

#if MICROPY_ENABLE_CALLBACK_DEAL
typedef struct{
    uint8_t callback_type_id;
    void *msg;
}st_CallBack_Deal;

enum {
    CALLBACK_TYPE_ID_NONE,
    CALLBACK_TYPE_ID_EXTINT,
    CALLBACK_TYPE_ID_OSTIMER,
    CALLBACK_TYPE_ID_HWTIMER,
    CALLBACK_TYPE_ID_AUDIO_AUDIO,
    CALLBACK_TYPE_ID_AUDIO_RECORD,
    CALLBACK_TYPE_ID_AUDIO_TTS,
    CALLBACK_TYPE_ID_CAM_CAP,
    CALLBACK_TYPE_ID_VOICECALL_RECORD,
};

typedef struct{
    uint8_t pin_no;
    uint8_t edge;
    c_callback_t callback;
}st_CallBack_Extint;

enum {RECORE_TYPE_FILE,RECORE_TYPE_STREAM};
typedef struct{
    int record_type;
    char * p_data;
    int len;
    int res;
    c_callback_t callback;
}st_CallBack_AudioRecord;

void qpy_callback_deal_init(void);
void qpy_callback_deal_deinit(void);
void qpy_send_msg_to_callback_deal_thread(uint8_t id, void *msg);

#endif
#endif //__CALLBACKDEAL_H__