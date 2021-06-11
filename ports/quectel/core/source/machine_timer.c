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
#include "modmachine.h"
#include "mphalport.h"

#include "helios_timer.h"
#include "helios_debug.h"

typedef unsigned long int UINT32;
#define HELIOS_TIMER_LOG(msg, ...)      custom_log("TIMER", msg, ##__VA_ARGS__)


typedef enum
{
	Timer0 = 0,
	Timer1 = 1,
	Timer2 = 2,
	Timer3 = 3
}Timern;
    
typedef enum   /* The meaning of the API flag*/         
{
	TIMER_PERIODIC = 0x1,		/* periodic execution */
	TIMER_AUTO_DELETE = 0x2	    /* one execution */
}TIMER_FLAG;

typedef enum
{
	MP_TIMER_CREATED,
	MP_TIMER_RUNNING,
	MP_TIMER_STOP
}MP_TIMER_STATUS;

typedef struct _machine_timer_obj_t {
    mp_obj_base_t base;
	//ONE_SHOT OR PERIODIC 
    mp_uint_t mode;
    mp_uint_t period;
    mp_obj_t callback;
	mp_int_t timer_id;
	mp_int_t timer_id_real;
	MP_TIMER_STATUS timer_status;
	//for traverse all the timers, such as soft reset while input CTRL+B
    struct _machine_timer_obj_t *next;
} machine_timer_obj_t;

STATIC void machine_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_timer_obj_t *self = self_in;
    HELIOS_TIMER_LOG("period=%d\r\n", self->period);
    HELIOS_TIMER_LOG("timer_id=%d\r\n", self->timer_id);
}


STATIC void machine_timer_isr(UINT32 self_in) {

    //HELIOS_TIMER_LOG("timer cb1,self_in=%#X\r\n",self_in);

    machine_timer_obj_t *self = (void*)self_in;
    
	if((self!=NULL )&& (mp_obj_is_callable(self->callback)))
	{
        //HELIOS_TIMER_LOG("timer mp_obj_is_callable");
    	mp_sched_schedule(self->callback, self);
	}
    // mp_hal_wake_main_task_from_isr();

}


STATIC mp_obj_t machine_timer_init_helper(machine_timer_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_mode,
        ARG_callback,
        ARG_period,
        ARG_tick_hz,
        ARG_freq,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,         MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = TIMER_PERIODIC} },
        { MP_QSTR_callback,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_period,       MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0xffffffff} },
        { MP_QSTR_tick_hz,      MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
        #if MICROPY_PY_BUILTINS_FLOAT
        { MP_QSTR_freq,         MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        #else
        { MP_QSTR_freq,         MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0xffffffff} },
        #endif
    };

    int ret;

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    #if MICROPY_PY_BUILTINS_FLOAT
    if (args[ARG_freq].u_obj != mp_const_none) {
        self->period = (mp_uint_t)(1000 / mp_obj_get_float(args[ARG_freq].u_obj));
    }
    #else
    if (args[ARG_freq].u_int != 0xffffffff) {
        self->period = 1000 / ((mp_uint_t)args[ARG_freq].u_int);
    }
    #endif
    else {
        self->period = (((uint64_t)args[ARG_period].u_int) * 1000) / args[ARG_tick_hz].u_int;
    }

    self->mode = args[ARG_mode].u_int;
    self->callback = args[ARG_callback].u_obj;

    HELIOS_TIMER_LOG("mode: %d, period: %d,callback:%#X\r\n", self->mode, self->period,self->callback);

// Check whether the timer is already running, if so return 0
    if (MP_TIMER_RUNNING == self->timer_status) {
        return mp_obj_new_int(0);
    }
	
	int timer = Helios_Timer_init( (void* )machine_timer_isr, self);
	
    if(!timer) {
        return mp_const_false;
	}

	uint8_t cyclicalEn = 0;
	if(self->mode == TIMER_PERIODIC) {
		cyclicalEn = 1;
	}

	ret = Helios_Timer_Start(timer, (uint32_t) self->period, cyclicalEn);
    
    
	if(!ret)
	{
        //get time_id_real
		HELIOS_TIMER_LOG("timer_id: %#x\r\n", timer);
		self->timer_id_real = timer;
		self->timer_status = MP_TIMER_RUNNING;
	}
	else
	{
        HELIOS_TIMER_LOG("timer create failed.ret=%d\r\n",ret);
	}
    return mp_obj_new_int(0);
}



STATIC mp_obj_t machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
	mp_int_t timer_id=0;
	
	if (n_args > 0) {
			timer_id = mp_obj_get_int(args[0]);
			--n_args;
			++args;
	}
	

	// Check whether the timer is already initialized, if so return it
    for (machine_timer_obj_t *t = MP_STATE_PORT(machine_timer_obj_head); t; t = t->next) {
        if (t->timer_id == timer_id) {
            return t;
        }
    }
	if ((timer_id < 0) || (timer_id > 3))
	{
		mp_raise_ValueError("invalid value, Timern should be in (Timer0~Timer3).");
	}
	
	machine_timer_obj_t *self = m_new_obj(machine_timer_obj_t);
    self->base.type = &machine_timer_type;
	self->timer_id = timer_id;
	self->timer_status = MP_TIMER_CREATED;

    if (n_args > 0 || n_kw > 0) {
        // Start the timer
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        machine_timer_init_helper(self, n_args, args, &kw_args);
    }

	// Add the timer to the linked-list of timers
    self->next = MP_STATE_PORT(machine_timer_obj_head);
    MP_STATE_PORT(machine_timer_obj_head) = self;
	
	return MP_OBJ_FROM_PTR(self);
}


STATIC mp_obj_t machine_timer_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    return machine_timer_init_helper(self, n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_timer_init_obj, 1, machine_timer_init);



STATIC mp_obj_t machine_timer_deinit(mp_obj_t self_in) {
    int ret=0;
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);

    
    HELIOS_TIMER_LOG("timer deinit: id (%#X) \r\n",self->timer_id_real);
	if(self->timer_id_real > 0)
	{
		HELIOS_TIMER_LOG("timer Helios_Timer_Deinit (%#X) \r\n",self->timer_id_real);		
		Helios_Timer_Deinit(self->timer_id_real);
		self->timer_id_real = 0;
		self->timer_status = MP_TIMER_STOP;
		return mp_obj_new_int(0);
	}
    return mp_obj_new_int(-1);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_deinit_obj, machine_timer_deinit);


STATIC mp_obj_t machine_timer_initialize() {
    static int initialized = 0;
    if (!initialized) {
        
        HELIOS_TIMER_LOG("machine_timer_initialize\r\n");
        initialized = 1;
    }
    return mp_const_none;
}

void machine_timer_deinit_all(void) {
    // Disable, deallocate and remove all timers from list
    machine_timer_obj_t **t = &MP_STATE_PORT(machine_timer_obj_head);
    while (*t != NULL) {
        machine_timer_deinit(*t);
        machine_timer_obj_t *next = (*t)->next;
        m_del_obj(machine_timer_obj_t, *t);
        *t = next;
    }
}


STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_timer_initialize_obj, machine_timer_initialize);

STATIC const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_timer_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&machine_timer_initialize_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&machine_timer_init_obj) },
   // { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_timer_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_ONE_SHOT), MP_ROM_INT(TIMER_AUTO_DELETE) },
    { MP_ROM_QSTR(MP_QSTR_PERIODIC), MP_ROM_INT(TIMER_PERIODIC) },
    { MP_ROM_QSTR(MP_QSTR_Timer0), MP_ROM_INT(Timer0) },
    { MP_ROM_QSTR(MP_QSTR_Timer1), MP_ROM_INT(Timer1) },
    { MP_ROM_QSTR(MP_QSTR_Timer2), MP_ROM_INT(Timer2) },
    { MP_ROM_QSTR(MP_QSTR_Timer3), MP_ROM_INT(Timer3) },
};
STATIC MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);



const mp_obj_type_t machine_timer_type = {
    { &mp_type_type },
    .name = MP_QSTR_Timer,
    .print = machine_timer_print,
    .make_new = machine_timer_make_new,
    .locals_dict = (mp_obj_t)&machine_timer_locals_dict,
};

