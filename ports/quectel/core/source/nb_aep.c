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



#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "runtime.h"
#include "stream.h"
#include "mperrno.h"
#include "modnb.h"
#include "helios_ril_aep.h"

#if  MICROPY_PY_NB_AEP

typedef struct _nb_aep_obj_t {
    mp_obj_base_t       base;
    Helios_NB_AEP_Info   nb_aep_info;
} nb_aep_obj_t;

STATIC mp_obj_t nb_aep_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);


STATIC void nb_aep_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  //  nb_aep_obj_t *self = MP_OBJ_TO_PTR(self_in);
}

STATIC mp_obj_t nb_aep_connect(mp_obj_t self_in){
    nb_aep_obj_t * self = MP_OBJ_TO_PTR(self_in);
    int ret = Helios_AEP_Connect(self->nb_aep_info.ip,self->nb_aep_info.port);
    if (  ret != -1 ){
        return mp_obj_new_int(ret);
    }
    return  mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nb_aep_connect_obj, nb_aep_connect);

STATIC mp_obj_t nb_aep_close(mp_obj_t self_in){
    mp_uint_t ret = 0;
    ret = Helios_AEP_DisConnect();
    if ( !ret ){
        return mp_const_true;
    }else{
        return mp_const_false;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nb_aep_close_obj,  nb_aep_close);

STATIC mp_obj_t nb_aep_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    mp_uint_t ret = 0;

    enum { ARG_data_len,ARG_data_buf, ARG_type};


    const mp_arg_t allowed_args[] = {
        { MP_QSTR_data_len  , MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_data_buf  , MP_ARG_REQUIRED | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_type      , MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 0 } }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];

    //mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t data_len    = args[ARG_data_len].u_int;
    mp_int_t type        = args[ARG_type].u_int;
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_data_buf].u_obj, &bufinfo, MP_BUFFER_READ);
	data_len = (data_len < (mp_int_t)bufinfo.len ? data_len : (mp_int_t)bufinfo.len );
    ret = Helios_AEP_SendData(( uint32_t )data_len,(uint8_t *)bufinfo.buf,type);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(nb_aep_send_obj, 2, nb_aep_send);

STATIC mp_obj_t nb_aep_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {


    enum { ARG_data_len,ARG_data_buf };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_data_len  , MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_data_buf  , MP_ARG_REQUIRED | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_data_buf].u_obj, &bufinfo, MP_BUFFER_READ);
    mp_int_t data_len    = args[ARG_data_len].u_int;
    data_len = (int32_t)(data_len) > (int32_t)bufinfo.len ? (int32_t)bufinfo.len : (int32_t)(data_len);
    if ( data_len < 0 ){
        mp_raise_OSError(MP_EINVAL);
    }else if ( data_len == 0 ){
        data_len = (int32_t)bufinfo.len;
    }

    Helios_NB_AEP_databuffer data_buffer;
    data_buffer.buffer      = (uint8_t *)malloc(data_len+1);
    if ( data_buffer.buffer == NULL )
    {
        mp_raise_OSError(MP_ENOMEM);
    }
    data_buffer.capacity    = data_len;
    memset(data_buffer.buffer,0x00,data_len+1);
    mp_uint_t ret = Helios_AEP_RecvData(&data_buffer);

    if ( ret != 0 )
    {
        free(data_buffer.buffer);
        return mp_obj_new_int(ret);
    }
    strncpy((char *)bufinfo.buf, (char *)data_buffer.buffer,data_len);

    free(data_buffer.buffer);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(nb_aep_recv_obj, 1, nb_aep_recv);

STATIC const mp_rom_map_elem_t nb_aep_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&nb_aep_connect_obj)  },
    { MP_ROM_QSTR(MP_QSTR_close),   MP_ROM_PTR(&nb_aep_close_obj)    },
    { MP_ROM_QSTR(MP_QSTR_recv),    MP_ROM_PTR(&nb_aep_recv_obj)     },
    { MP_ROM_QSTR(MP_QSTR_send),    MP_ROM_PTR(&nb_aep_send_obj)     },
};

STATIC MP_DEFINE_CONST_DICT(nb_aep_locals_dict, nb_aep_locals_dict_table);

const mp_obj_type_t nb_aep_type = {
    { &mp_type_type },
    .name = MP_QSTR_AEP,
    .print = nb_aep_print,
    .make_new = nb_aep_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .locals_dict = (mp_obj_dict_t *)&nb_aep_locals_dict,
};

STATIC mp_obj_t nb_aep_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 2, MP_OBJ_FUN_ARGS_MAX, true);
    // create instance
    nb_aep_obj_t *self = m_new_obj(nb_aep_obj_t);
    if ( self == NULL ){
        mp_raise_OSError(MP_ENOMEM);
    }
   
    memset((void *)&self->nb_aep_info,0x00,sizeof(self->nb_aep_info));
    self->base.type = &nb_aep_type;
    const char * ip          = mp_obj_str_get_str(args[0]);
    const char * port        = mp_obj_str_get_str(args[1]);
	memcpy(self->nb_aep_info.ip    ,ip   ,strlen(ip   ));
    memcpy(self->nb_aep_info.port  ,port  ,strlen(port  ));
    return MP_OBJ_FROM_PTR(self);
}
#endif /*end MICROPY_PY_NB_AEP*/