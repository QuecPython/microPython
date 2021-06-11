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

#include "helios_uart.h"

typedef struct _machine_uart_obj_t {
    mp_obj_base_t base;
    unsigned int uart_num;
    Helios_UARTConfig config;
} machine_uart_obj_t;

static mp_obj_t callback_cur[4] = {0};


const mp_obj_type_t machine_uart_type;
STATIC const char *_parity_name[] = {"None", "1", "0"};

/******************************************************************************/
// MicroPython bindings for UART

STATIC void machine_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "UART(%u, baudrate=%u, bits=%u, parity=%s, stop=%u, flow=%u",
        self->uart_num, self->config.baudrate, self->config.data_bit, _parity_name[self->config.parity_bit],
        self->config.stop_bit, self->config.flow_ctrl);
    mp_printf(print, ")");
}

STATIC void machine_uart_init_helper(machine_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_bits, ARG_parity, ARG_stop,  ARG_flow};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_parity, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_stop, MP_ARG_INT, {.u_int = -1} },
		{ MP_QSTR_flow, MP_ARG_INT, {.u_int = -1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	// get baudrate bits
    switch (args[ARG_baudrate].u_int) {
        case 0:
            break;
        case HELIOS_UART_BAUD_300:
		case HELIOS_UART_BAUD_600:
		case HELIOS_UART_BAUD_1200:
		case HELIOS_UART_BAUD_2400:
		case HELIOS_UART_BAUD_3600:
		case HELIOS_UART_BAUD_4800:
		case HELIOS_UART_BAUD_7200:
		case HELIOS_UART_BAUD_9600:
		case HELIOS_UART_BAUD_14400:
		case HELIOS_UART_BAUD_19200:
		case HELIOS_UART_BAUD_28800:
		case HELIOS_UART_BAUD_33600:
		case HELIOS_UART_BAUD_38400:
		case HELIOS_UART_BAUD_57600:
		case HELIOS_UART_BAUD_115200:
		case HELIOS_UART_BAUD_230400:
		case HELIOS_UART_BAUD_460800:
		case HELIOS_UART_BAUD_921600:
		case HELIOS_UART_BAUD_1000000:
		case HELIOS_UART_BAUD_1842000:
		case HELIOS_UART_BAUD_3686400:
		case HELIOS_UART_BAUD_4468750:
            self->config.baudrate = args[ARG_baudrate].u_int;
            break;
        default:
            mp_raise_ValueError("invalid baudrate");
            break;
    }

    // get data bits
    switch (args[ARG_bits].u_int) {
        case 0:
            break;
        case 5:
            self->config.data_bit = HELIOS_UART_DATABIT_5;
            break;
        case 6:
            self->config.data_bit = HELIOS_UART_DATABIT_6;
            break;
        case 7:
            self->config.data_bit = HELIOS_UART_DATABIT_7;
            break;
        case 8:
            self->config.data_bit = HELIOS_UART_DATABIT_8;
            break;
        default:
            mp_raise_ValueError("invalid data bits");
            break;
    }

	// get parity bits
    switch (args[ARG_parity].u_int) {
		case -1:
			break;
        case 0:
			self->config.parity_bit = HELIOS_UART_PARITY_NONE;
            break;
        case 1:
            self->config.parity_bit = HELIOS_UART_PARITY_EVEN;
            break;
        case 2:
            self->config.parity_bit = HELIOS_UART_PARITY_ODD;
            break;
        default:
            mp_raise_ValueError("invalid parity bits");
            break;
    }

    // get stop bits
    switch (args[ARG_stop].u_int) {
		case -1:
			break;
        case 0:
            break;
        case 1:
            self->config.stop_bit = HELIOS_UART_STOP_1;
            break;
        case 2:
            self->config.stop_bit = HELIOS_UART_STOP_2;
            break;
        default:
            mp_raise_ValueError("invalid stop bits");
            break;
    }

	// get flow bits
    switch (args[ARG_flow].u_int) {
		case -1:
			break;
        case 0:
			self->config.flow_ctrl = HELIOS_UART_FC_NONE;
            break;
        case 1:
            self->config.flow_ctrl = HELIOS_UART_FC_HW;
            break;
        default:
            mp_raise_ValueError("invalid flow bits");
            break;
    }
	Helios_UARTInitStruct uart_para = {0};
	Helios_UARTConfig uart_config = {0};
	uart_para.uart_config = &uart_config;
	
	memcpy((void*)&uart_config,(void*)&self->config, sizeof(Helios_UARTConfig));
	
	Helios_UART_Init((Helios_UARTNum) self->uart_num, &uart_para);
}

STATIC mp_obj_t machine_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get uart id
    mp_int_t uart_num = mp_obj_get_int(args[0]);
    if (uart_num < HELIOS_UART0 || uart_num > HELIOS_UARTMAX) {
        mp_raise_msg_varg(&mp_type_ValueError, "UART(%d) does not exist", uart_num);
    }

    // create instance
    machine_uart_obj_t *self = m_new_obj(machine_uart_obj_t);
	
    self->base.type = &machine_uart_type;
    self->uart_num = uart_num;
	self->config.baudrate = HELIOS_UART_BAUD_115200;
	self->config.data_bit = HELIOS_UART_DATABIT_8;
	self->config.stop_bit = HELIOS_UART_STOP_1;
	self->config.parity_bit = HELIOS_UART_PARITY_NONE;
	self->config.flow_ctrl = HELIOS_UART_FC_NONE;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_uart_init_helper(self, n_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_uart_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_uart_init_helper(args[0], n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_uart_init_obj, 1, machine_uart_init);

STATIC mp_obj_t machine_uart_deinit(mp_obj_t self_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
	
	int ret = Helios_UART_Deinit((Helios_UARTNum) self->uart_num);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_deinit_obj, machine_uart_deinit);


STATIC mp_obj_t machine_uart_any(mp_obj_t self_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    size_t rxbufsize;
    rxbufsize = Helios_UART_Any((Helios_UARTNum) self->uart_num);
    return MP_OBJ_NEW_SMALL_INT(rxbufsize);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_any_obj, machine_uart_any);


/*
STATIC mp_obj_t machine_uart_sendbreak(mp_obj_t self_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    char buf[1] = {0};
	qpy_uart_write(self->uart_num, (unsigned char*)buf, 1);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_sendbreak_obj, machine_uart_sendbreak);
*/


void helios_uart_callback_to_python(uint64_t ind_type, Helios_UARTNum port, uint64_t size)
{
	mp_obj_t decode_cb[3] = {
		mp_obj_new_int(ind_type),
	 	mp_obj_new_int(port),
        mp_obj_new_int(size),
     };
	
	if(callback_cur[port] == NULL) {
		return;
	}
    if(mp_obj_is_callable(callback_cur[port])){
    	mp_sched_schedule(callback_cur[port], MP_OBJ_FROM_PTR(mp_obj_new_list(3, decode_cb)));
	}
}


STATIC mp_obj_t helios_uart_set_callback(mp_obj_t self_in, mp_obj_t callback)
{
	machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
	callback_cur[self->uart_num] = callback;
	
	int ret = Helios_UART_SetCallback((Helios_UARTNum) self->uart_num, (Helios_UARTCallback) helios_uart_callback_to_python);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_uart_set_callback_obj, helios_uart_set_callback);



STATIC const mp_rom_map_elem_t machine_uart_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_uart_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&machine_uart_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&machine_uart_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_callback), MP_ROM_PTR(&machine_uart_set_callback_obj) },
   // { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
   // { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
  //  { MP_ROM_QSTR(MP_QSTR_sendbreak), MP_ROM_PTR(&machine_uart_sendbreak_obj) },
    { MP_ROM_QSTR(MP_QSTR_UART0), MP_ROM_INT(HELIOS_UART0) },
    { MP_ROM_QSTR(MP_QSTR_UART1), MP_ROM_INT(HELIOS_UART1) },
    { MP_ROM_QSTR(MP_QSTR_UART2), MP_ROM_INT(HELIOS_UART2) },
    { MP_ROM_QSTR(MP_QSTR_UART3), MP_ROM_INT(HELIOS_UART3) },
    //{ MP_ROM_QSTR(MP_QSTR_UART4), MP_ROM_INT(HELIOS_UART4) },
   // { MP_ROM_QSTR(MP_QSTR_UART5), MP_ROM_INT(HELIOS_UART5) },
};

STATIC MP_DEFINE_CONST_DICT(machine_uart_locals_dict, machine_uart_locals_dict_table);

STATIC mp_uint_t machine_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    //int bytes_read = ql_uart_read(self->uart_num, buf_in, size);
    
	int bytes_read = Helios_UART_Read((Helios_UARTNum) self->uart_num, buf_in, size);
    if (bytes_read < 0) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    return bytes_read;
}

STATIC mp_uint_t machine_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int bytes_written = Helios_UART_Write((Helios_UARTNum) self->uart_num, (void*)buf_in, size);
    if (bytes_written < 0) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    // return number of bytes written
    return bytes_written;
}

STATIC mp_uint_t machine_uart_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    //machine_uart_obj_t *self = self_in;
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = (mp_uint_t)arg;
        ret = 0;
        size_t rxbufsize = 1;
        //rxbufsize = ql_uart_get_rx_data_len(self->uart_num);
        if ((flags & MP_STREAM_POLL_RD) && rxbufsize > 0) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR) && 1) { // FIXME: uart_tx_any_room(self->uart_num)
            ret |= MP_STREAM_POLL_WR;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

STATIC const mp_stream_p_t uart_stream_p = {
    .read = machine_uart_read,
    .write = machine_uart_write,
    .ioctl = machine_uart_ioctl,
    .is_text = false,
};

const mp_obj_type_t machine_uart_type = {
    { &mp_type_type },
    .name = MP_QSTR_UART,
    .print = machine_uart_print,
    .make_new = machine_uart_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &uart_stream_p,
    .locals_dict = (mp_obj_dict_t *)&machine_uart_locals_dict,
};
