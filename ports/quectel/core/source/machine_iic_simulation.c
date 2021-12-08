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
#include "gc.h"
#include "mphal.h"
#include "mperrno.h"
#include "helios_os.h"

#include "helios_gpio.h"
#include "mphalport.h"
#include "helios_iic.h"
#include "helios_debug.h"

#define HELIOS_IIC_SIMULATION_LOG(msg, ...)      custom_log("machine_iic_simu", msg, ##__VA_ARGS__)

#if defined (PLAT_ASR)
#define MS_REFERENCE_VALUE 43	//ASR时间 该时间由EC600NCN_LC实际测试所得

#elif defined (PLAT_Unisoc)
#define MS_REFERENCE_VALUE 76	//展锐时间，该时间由EC600UCN_LB实际测试所得 76

#elif defined (PLAT_RDA)
#define MS_REFERENCE_VALUE 6	//NB时间，该时间由BC25PA实际测试所得 

#elif defined (PLAT_Qualcomm)
#define MS_REFERENCE_VALUE 122	//高通时间，该时间由BG95M3实际测试所得  (不可靠)


#else
	#error "This platform does not support"
#endif

const mp_obj_type_t machine_simulation_i2c_type;

typedef struct _machine_simu_i2c_obj_t
{
    mp_obj_base_t base;
    Helios_GPIONum scl_gpio;
    Helios_GPIONum sda_gpio;
    uint32_t i2c_freq;
	uint32_t sleep_us;
	

} machine_simu_i2c_obj_t;

STATIC machine_simu_i2c_obj_t *cur_i2c = NULL;

#define IIC_SDA(X) (Helios_GPIO_SetLevel((Helios_GPIONum)cur_i2c->sda_gpio, X))
#define IIC_SCL(X) (Helios_GPIO_SetLevel((Helios_GPIONum)cur_i2c->scl_gpio, X))

#define READ_SCL (Helios_GPIO_GetLevel((Helios_GPIONum)cur_i2c->scl_gpio))
#define READ_SDA (Helios_GPIO_GetLevel((Helios_GPIONum)cur_i2c->sda_gpio))

#define SDA_IN	(Helios_GPIO_SetDirection((Helios_GPIONum) cur_i2c->sda_gpio, HELIOS_GPIO_INPUT))
#define SDA_OUT	(Helios_GPIO_SetDirection((Helios_GPIONum) cur_i2c->sda_gpio, HELIOS_GPIO_OUTPUT))
#define SCL_OUT	(Helios_GPIO_SetDirection((Helios_GPIONum) cur_i2c->scl_gpio, HELIOS_GPIO_OUTPUT))



STATIC void uudelay(uint32_t us)
{
    volatile uint32_t i;
    for (i = 0; i < us * MS_REFERENCE_VALUE; i++) {
        i = i + 1;
	}

}

#define SLEEP (uudelay(cur_i2c->sleep_us))





//产生IIC起始信号
//1.先拉高SDA，再拉高SCL，空闲状态
//2.拉低SDA
STATIC void IIC_Start() //启动信号

{
	SDA_OUT;
	SCL_OUT;
    IIC_SDA(1); //确保SDA线为高电平
    IIC_SCL(1); //确保SCL高电平

    SLEEP;

    IIC_SDA(0); //在SCL为高时拉低SDA线，即为起始信号

    SLEEP;
}

//产生IIC停止信号
//1.先拉低SDA，再拉低SCL
//2.拉高SCL
//3.拉高SDA
//4.停止接收数据
STATIC void IIC_Stop(void)
{
	SDA_OUT;
    IIC_SCL(0);
    IIC_SDA(0); //STOP:当SCL高时，数据由低变高
    SLEEP;
    IIC_SCL(1);
    IIC_SDA(1); //发送I2C总线结束信号
    SLEEP;
}

//主机产生应答信号ACK
//1.先拉低SCL，再拉低SDA
//2.拉高SCL
//3.拉低SCL## 标题
STATIC void I2C_Ack(void)
{
	SDA_OUT;
    IIC_SCL(0); //先拉低SCL，使得SDA数据可以发生改变
    IIC_SDA(0);
    SLEEP;
    IIC_SCL(1);
    SLEEP;
    IIC_SCL(0);
	SLEEP;
}

//主机不产生应答信号NACK
//1.先拉低SCL，再拉高SDA
//2.拉高SCL
//3.拉低SCL
STATIC void I2C_NAck(void)
{
	SDA_OUT;
    IIC_SCL(0); //先拉低SCL，使得SDA数据可以发生改变
    IIC_SDA(1); //拉高SDA，不产生应答信号
    SLEEP;
    IIC_SCL(1);
    SLEEP;
    IIC_SCL(0);
	SLEEP;
}

STATIC uint8_t I2C_Wait_Ack(void)  
{  
    int ErrTime=0;  
    int ReadAck=0;  
    SDA_IN; // Config SDA GPIO as Input
    SLEEP;  
    IIC_SCL(1);   // Set the SCL and wait for ACK  
    while(1)  
    {  
         ReadAck = READ_SDA ;  // Read the input  
         if(ReadAck)  
        {   
             ErrTime++;  
             if(ErrTime>1000)  
            {  
                 //Error handler:Set error flag, retry or stop.  
                //Define by users  
                 return 1;  
             }  
        }  
         if(ReadAck==0)  // Receive a ACK
         {  
             SLEEP;  
             IIC_SCL(0);   // Clear the SCL for Next Transmit  
             return 0;  
         }  
     } 
}

STATIC uint8_t I2C_read_ack()  
{  
	uint8_t r;  
	SDA_IN;           //设置SDA方向为输入  
	IIC_SCL(0);              // SCL变低  
	r = READ_SDA;                //读取ACK信号  
	SLEEP;  
	IIC_SCL(1);              // SCL变高  
	SLEEP;  
	return r;  
} 


//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答

//IIC_SCL=0;
//在SCL上升沿时准备好数据，进行传送数据时，拉高拉低SDA，因为传输一个字节，一个SCL脉冲里传输一个位。
//数据传输过程中，数据传输保持稳定（在SCL高电平期间，SDA一直保持稳定，没有跳变）
//只有当SCL被拉低后，SDA才能被改变
//总结：在SCL为高电平期间，发送数据，发送8次数据，数据为1,SDA被拉高，数据为0，SDA被拉低。
//传输期间保持传输稳定，所以数据线仅可以在时钟SCL为低电平时改变。
STATIC uint8_t IIC_Send_Byte(uint8_t txd)
{
    uint8_t t;
	SLEEP;
    SDA_OUT;
	
	IIC_SCL(0);
	SLEEP;
    for (t = 0; t < 8; t++)
    {
        //IIC_SDA=(txd&0x80)>>7;   //获取最高位
        //获取数据的最高位，然后左移7位
        //如果某位为1，则SDA为1，否则相反
        //IIC_SCL(0);
		//SLEEP;
        if ((txd & 0x80) >> 7)
            IIC_SDA(1);
        else
            IIC_SDA(0);
        txd <<= 1;
		SLEEP;
		

        IIC_SCL(1);
		SLEEP;
		IIC_SCL(0);
        //SLEEP;
    }

	return I2C_Wait_Ack();
}

//读1个字节，ack=1时，发送ACK，ack=0，发送nACK
STATIC uint8_t IIC_Read_Byte(uint8_t ack)
{
    uint8_t i, receive = 0;
	SLEEP;
	SDA_IN; //SDA设置为输入	
	SLEEP;
    for (i = 0; i < 8; i++)
    {
        IIC_SCL(1);
        
        receive <<= 1;
        if (READ_SDA) {
            receive++;
		}
        SLEEP;
        IIC_SCL(0);
		SLEEP;
    }
    if (!ack)
        I2C_NAck(); //发送nACK
    else
        I2C_Ack(); //发送ACK
    //SLEEP;
    return receive;
}

/*  
I2C读操作  
addr：目标设备地址  
buf：读缓冲区  
len：读入字节的长度  
*/
/* 通过模拟I2C向从站写数据 */

#define IIC_SEND_BYTE(x) do { \
	if(0 != IIC_Send_Byte(x)) {	\
		return -1;	\
	}	\
}while(0)

STATIC int i2c_read(uint8_t slaveaddress, uint8_t *regaddr, size_t addrlen, uint8_t *data, size_t datalen,uint32_t wait_times)
{
    uint32_t i;
    uint8_t t;

	if(addrlen > 0) {
		IIC_Start(); //起始条件，开始数据通信
		//发送地址和数据读写方向
		t = (slaveaddress << 1) | 0; //低位为0，表示写数据
		IIC_SEND_BYTE(t);
		//写入数据
		for (i = 0; i < addrlen; i++)
			IIC_SEND_BYTE(regaddr[i]);
	}

	//Helios_msleep(5);
	
    IIC_Start(); //起始条件，开始数据通信
    //发送地址和数据读写方向
    t = (slaveaddress << 1) | 1; //低位为1，表示读数据
    IIC_SEND_BYTE(t);
    //读入数据
    for (i = 0; i < datalen-1; i++) {
        data[i] = IIC_Read_Byte(1);
	}	
	data[i] = IIC_Read_Byte(0);
    IIC_Stop(); //终止条件，结束数据通信
    return 0;
}
/*  
I2C写操作  
addr：目标设备地址  
buf：写缓冲区  
len：写入字节的长度  
*/
STATIC int i2c_write(uint8_t slaveaddress, uint8_t *regaddr,size_t addrlen,uint8_t *data, size_t datalen)
{
    uint32_t i;
    uint8_t t;
	HELIOS_IIC_SIMULATION_LOG("i2c_write1\n");
    IIC_Start(); //起始条件，开始数据通信
    HELIOS_IIC_SIMULATION_LOG("i2c_write14\n");
    //发送地址和数据读写方向
    t = (slaveaddress << 1) | 0; //低位为0，表示写数据
    IIC_SEND_BYTE(t);
	HELIOS_IIC_SIMULATION_LOG("i2c_write2\n");
    //写入数据
    for (i = 0; i < addrlen; i++)
        IIC_SEND_BYTE(regaddr[i]);
	HELIOS_IIC_SIMULATION_LOG("i2c_write3\n");
	
    for (i = 0; i < datalen; i++)
        IIC_SEND_BYTE(data[i]);
	HELIOS_IIC_SIMULATION_LOG("i2c_write4\n");
    IIC_Stop(); //终止条件，结束数据通信
    return 0;
}

STATIC void machine_simu_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    machine_simu_i2c_obj_t *self = self_in;
    mp_printf(print, "I2C scl_gpio:%d, sda_gpio:%d, freq:%dHz\n", self->scl_gpio, self->sda_gpio, self->i2c_freq);
}

mp_obj_t machine_simu_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum
    {
        ARG_scl,
        ARG_sda,
        ARG_freq
    };
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_scl, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = HELIOS_GPIO1}},
        {MP_QSTR_sda, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = HELIOS_GPIO2}},
        {MP_QSTR_frq, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 100}},
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint32_t i2c_sda = args[ARG_sda].u_int;
    uint32_t i2c_scl = args[ARG_scl].u_int;
    uint32_t freq = args[ARG_freq].u_int;

	HELIOS_IIC_SIMULATION_LOG("i2c_sda:%d, i2c_scl:%d, freq:%d\n", i2c_sda, i2c_scl, freq);

    if (freq > 1000000 || freq == 0 ||i2c_sda >= HELIOS_GPIOMAX || i2c_scl >= HELIOS_GPIOMAX)
    {
        mp_raise_ValueError("Parameter setting error, please check");
    }

    Helios_GPIOInitStruct gpio_struct = {0};
    gpio_struct.dir = HELIOS_GPIO_OUTPUT;
    gpio_struct.pull = HELIOS_PULL_NONE;
    gpio_struct.value = HELIOS_LVL_HIGH;
	if(0 != Helios_GPIO_Init((Helios_GPIONum)i2c_sda, &gpio_struct)) {
        mp_raise_ValueError("GPIO(SDA) initialization failed. Please try again or replace another GPIO");
	}
	Helios_msleep(5);
    if (0 != Helios_GPIO_Init((Helios_GPIONum)i2c_scl, &gpio_struct))
    {
        mp_raise_ValueError("GPIO(SCL) initialization failed. Please try again or replace another GPIO");
    }

    machine_simu_i2c_obj_t *self = m_new_obj(machine_simu_i2c_obj_t);
    cur_i2c = self;

    self->base.type = &machine_simulation_i2c_type;
    self->scl_gpio = i2c_scl;
    self->sda_gpio = i2c_sda;
    self->i2c_freq = freq;

	self->sleep_us = (uint32_t)(500000/freq);
	
	HELIOS_IIC_SIMULATION_LOG("sleep_us:%d\n", self->sleep_us);

    return MP_OBJ_FROM_PTR(self);
}
STATIC const mp_arg_t machine_i2c_mem_allowed_args[] = {
    {MP_QSTR_slaveaddr, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0}},
    {MP_QSTR_regaddr, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
    {MP_QSTR_regaddr_len, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0}},
    {MP_QSTR_databuf, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
    {MP_QSTR_datasize, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 8}},
};
STATIC mp_obj_t machine_i2c_write_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum
    {
        ARG_slaveaddr,
        ARG_regaddr,
        ARG_regaddr_len,
        ARG_databuf,
        ARG_datasize
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_mem_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
                     MP_ARRAY_SIZE(machine_i2c_mem_allowed_args), machine_i2c_mem_allowed_args, args);

    machine_simu_i2c_obj_t *self = (machine_simu_i2c_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);

    mp_buffer_info_t regaddr_bufinfo;
    mp_get_buffer_raise(args[ARG_regaddr].u_obj, &regaddr_bufinfo, MP_BUFFER_READ);
    int regaddr_length = args[ARG_regaddr_len].u_int;

    // get the buffer to write the data from
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_databuf].u_obj, &bufinfo, MP_BUFFER_READ);
    int length = (size_t)args[ARG_datasize].u_int > bufinfo.len ? bufinfo.len : (size_t)args[ARG_datasize].u_int;
    //uart_printf("i2c wirte: busid:%x slaveddr:%x, regaddr:%x, data:%s datalen:%d\n", self->bus_id, args[ARG_slaveaddr].u_int, args[ARG_regaddr].u_int,
    //							bufinfo.buf, args[ARG_datasize].u_int);
    // do the transfer
	
    int ret = i2c_write((uint8_t)args[ARG_slaveaddr].u_int, (uint8_t *)regaddr_bufinfo.buf, (size_t)regaddr_length, (void *)bufinfo.buf, (size_t)length);
    if (ret < 0)
    {
        ret = -1;
        return mp_obj_new_int(ret);
    }

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_write_obj, 1, machine_i2c_write_mem);

STATIC const mp_arg_t machine_i2c_mem_allowed_read_args[] = {
    {MP_QSTR_slaveaddr, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0}},
    {MP_QSTR_regaddr, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
    {MP_QSTR_regaddr_len, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0}},
    {MP_QSTR_databuf, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
    {MP_QSTR_datasize, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 8}},
    {MP_QSTR_dalay, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 8}},
};
STATIC mp_obj_t machine_i2c_read_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum
    {
        ARG_slaveaddr,
        ARG_regaddr,
        ARG_regaddr_len,
        ARG_databuf,
        ARG_datasize,
        ARG_dalay
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_mem_allowed_read_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
                     MP_ARRAY_SIZE(machine_i2c_mem_allowed_read_args), machine_i2c_mem_allowed_read_args, args);

    machine_simu_i2c_obj_t *self = (machine_simu_i2c_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);

    mp_buffer_info_t regaddr_bufinfo;
    mp_get_buffer_raise(args[ARG_regaddr].u_obj, &regaddr_bufinfo, MP_BUFFER_READ);
    int regaddr_length = args[ARG_regaddr_len].u_int;

    // get the buffer to read the data into
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_databuf].u_obj, &bufinfo, MP_BUFFER_WRITE);

    int length = (size_t)args[ARG_datasize].u_int > bufinfo.len ? bufinfo.len : (size_t)args[ARG_datasize].u_int;

    //uart_printf("i2c read: busid:%x slaveddr:%x, regaddr:%x, datalen:%d\n", self->bus_id, args[ARG_slaveaddr].u_int, args[ARG_regaddr].u_int,
    //							args[ARG_datasize].u_int);
    // do the transfer
    int ret = i2c_read((uint8_t)args[ARG_slaveaddr].u_int, (uint8_t *)regaddr_bufinfo.buf, (size_t)regaddr_length, (uint8_t *)bufinfo.buf, (size_t)length, (uint32_t)args[ARG_dalay].u_int);
    if (ret < 0)
    {
        ret = -1;
        return mp_obj_new_int(ret);
    }

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_read_obj, 1, machine_i2c_read_mem);

STATIC const mp_rom_map_elem_t machine_i2c_locals_dict_table1[] = {
    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&machine_i2c_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&machine_i2c_write_obj)},
    // class constants
    {MP_ROM_QSTR(MP_QSTR_STANDARD_MODE), MP_ROM_INT(HELIOS_STANDARD_MODE)},
    {MP_ROM_QSTR(MP_QSTR_FAST_MODE), MP_ROM_INT(HELIOS_FAST_MODE)},
    PLAT_GPIO_DEF(PLAT_GPIO_NUM),
};

MP_DEFINE_CONST_DICT(mp_machine_simu_i2c_locals_dict, machine_i2c_locals_dict_table1);

const mp_obj_type_t machine_simulation_i2c_type = {
    {&mp_type_type},
    .name = MP_QSTR_I2C_SIMU,
    .print = machine_simu_i2c_print,
    .make_new = machine_simu_i2c_make_new,
    .locals_dict = (mp_obj_dict_t *)&mp_machine_simu_i2c_locals_dict,
};
