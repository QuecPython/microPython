/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2021, QUECTEL  
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */



#include "stdio.h"
#include "stdlib.h"
#include "obj.h"
#include <string.h>
#include "runtime.h"
#include "mphalport.h"
#include "helios_os.h"
#include "helios_debug.h"
#include "helios_gpio.h"


#define MOD_SN95500_LOG(msg, ...)      custom_log(SN95500, msg, ##__VA_ARGS__)


typedef struct _sensor_sn95500_obj_t {
    mp_obj_base_t base;
    Helios_GPIONum clk_pin;
	Helios_GPIONum sdio_pin;
	unsigned int recv_data1;
	unsigned int recv_data2;
	Helios_Mutex_t data_mutex;
} sensor_sn95500_obj_t;

static sensor_sn95500_obj_t sn95500_obj = {0};


static int wakeup_oid(void);
static void recv_opt_data(void);
static int check_and_read_optical_data(void);
static int transmit_cmd(unsigned char cmd);
static void send_cmd(unsigned char cmd);
static unsigned short recv_ack(void);


//不准确的us延时函数，2us延时比较准确，延时时间越久，误差越大
//超过10us,实际延时时间估计只有期望值的56%~60%
static void delay_us(unsigned int us)
{
	int cnt = 26;
	volatile int i = 0;
	while (us--) {
		i = cnt;
		while (i--);
	}
}

static void delay_ms(unsigned int ms)
{
	Helios_msleep((uint32_t)ms);
}

static unsigned int read_sdio_pin_level(void)
{
	int pin_val = 0;
	
	pin_val = Helios_GPIO_GetLevel(sn95500_obj.sdio_pin);
	return pin_val;
}

static void set_clk_high(void)
{
	Helios_GPIO_SetLevel(sn95500_obj.clk_pin, HELIOS_LVL_HIGH);
}

static void set_clk_low(void)
{
	Helios_GPIO_SetLevel(sn95500_obj.clk_pin, HELIOS_LVL_LOW);
}

#if 0
static void set_sdio_high(void)
{
	Helios_GPIO_SetLevel(sn95500_obj.sdio_pin, HELIOS_LVL_HIGH);
}
#endif

static void set_sdio_low(void)
{
	Helios_GPIO_SetLevel(sn95500_obj.sdio_pin, HELIOS_LVL_LOW);
}

static void set_sdio_output(void)
{
	Helios_GPIO_SetDirection(sn95500_obj.sdio_pin, HELIOS_GPIO_OUTPUT);
}

static void set_sdio_input(void)
{
	Helios_GPIO_SetDirection(sn95500_obj.sdio_pin, HELIOS_GPIO_INPUT);
}


/*=============================================================================*/
/* FUNCTION: wakeup_oid                                                        */
/*=============================================================================*/
/*!@brief 		: wakeup SN95500
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		:
 *        -  1--success
 *        -  0--failed
 */
/*=============================================================================*/
static int wakeup_oid(void)
{
	unsigned int oid_det = 0;
	unsigned short int i = 0;
	set_clk_high();
	delay_ms(30);
	set_clk_low();

	while (1)
	{
		delay_ms(20);
		i += 1;
		if (i >= 200)
		{
			break;
		}

		oid_det = read_sdio_pin_level();
		if (!oid_det)
		{
			//MOD_SN95500_LOG("## oid_det = %u\n", oid_det);
			recv_opt_data();
			if (sn95500_obj.recv_data2 == 0x0000FFF8)
			{
				i = 0xFFFF;
				break;
			}
			break;
		}
	}
	if (i == 0xFFFF)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/*=============================================================================*/
/* FUNCTION: recv_opt_data                                                     */
/*=============================================================================*/
/*!@brief 		: receive data from sn95500
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		: None
 */
/*=============================================================================*/
static void recv_opt_data(void)
{
	int i = 0; 
	unsigned int oid_det = 0;
	unsigned int recv_data1 = 0;
	unsigned int recv_data2 = 0;
	
	uint32_t critical = Helios_Critical_Enter();

	set_clk_high();
	set_sdio_output();
	set_sdio_low();
	delay_us(20); //实际延时11.5~20us
	set_clk_low();
	delay_us(20); //实际延时11.5~20us

	for (i=0; i<64; i++)
	{
		set_clk_high();
		set_sdio_input();
		delay_us(20);
		set_clk_low();

		oid_det = read_sdio_pin_level();
		if (oid_det)
		{
			if (i < 32)
			{
				recv_data1 += 1; //recv_data1保存高32bit数据
			}
			else
			{
				recv_data2 += 1; //recv_data2保存低32bit数据
			}
		}

		if (i < 31)
		{
			recv_data1 <<= 1;
		}
		if ((i > 31) && (i < 63))
		{
			recv_data2 <<= 1;
		}

		delay_us(20); //实际延时11.5~20us
	}
	delay_us(160); //实际延时86us左右

	Helios_Mutex_Lock(sn95500_obj.data_mutex, HELIOS_WAIT_FOREVER);
	sn95500_obj.recv_data1 = recv_data1;
	sn95500_obj.recv_data2 = recv_data2;
	Helios_Mutex_Unlock(sn95500_obj.data_mutex);
	Helios_Critical_Exit(critical);
}


/*=============================================================================*/
/* FUNCTION: get_opt_data                                                      */
/*=============================================================================*/
/*!@brief 		: get data
 * @param[in] 	: data1-保存高32bit数据;data2-保存低32bit数据;
 * @param[out] 	: 
 * @return		:
 *        -  1--success
 *        -  0--failed
 */
/*=============================================================================*/
static int get_opt_data(unsigned int *data1, unsigned int *data2)
{
	Helios_Mutex_Lock(sn95500_obj.data_mutex, HELIOS_WAIT_FOREVER);
	*data1 = sn95500_obj.recv_data1;
	*data2 = sn95500_obj.recv_data2;
	Helios_Mutex_Unlock(sn95500_obj.data_mutex);
	return 1;
}


/*=============================================================================*/
/* FUNCTION: check_and_read_optical_data                                       */
/*=============================================================================*/
/*!@brief 		: 
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		:
 *        -  1--success
 *        -  0--no data can be read
 */
/*=============================================================================*/
static int check_and_read_optical_data(void)
{
	int ret = 0;
	unsigned int oid_det = 0;
	oid_det = read_sdio_pin_level();
	if (!oid_det)
	{
		recv_opt_data();
		ret = 1;
	}
	return ret;
}


/*=============================================================================*/
/* FUNCTION: transmit_cmd                                                      */
/*=============================================================================*/
/*!@brief 		: write cmd to sn95500
 * @param[in] 	: cmd
 * @param[out] 	: 
 * @return		:
 *        -  1--write succeed
 *        -  0--write failed
 */
/*=============================================================================*/
static int transmit_cmd(unsigned char cmd)
{
	unsigned int oid_det = 0;
	unsigned char tx_cmd = 0;
	unsigned char rx_ack_cnt = 0;
	unsigned short ack = 0;

	oid_det = read_sdio_pin_level();
	if (!oid_det)  //侦测SN95500是否有数据要传输
	{
		recv_opt_data(); //有传输需求，先接收数据
	}
	
	tx_cmd = cmd;
	send_cmd(tx_cmd);
	rx_ack_cnt = 0;
	
	while (1)
	{
		oid_det = read_sdio_pin_level();
		if (!oid_det)
		{
			ack = recv_ack();
			//tx_cmd = (unsigned char)recv_ack();
			MOD_SN95500_LOG("transmit_cmd : cmd = %02x, ack = %04x\n", cmd, ack);
			tx_cmd = ack & 0xFF;
			tx_cmd -= 1;
			if (tx_cmd == cmd) //ACK的数值=发送的命令+1
			{
				return 1;	   //收到正确的ACK，命令发送成功
			}
			else
			{
				return 0;      //收到的ACK不正确，命令发送失败，需重新发送
			}
		}
		
		delay_ms(30);
		rx_ack_cnt += 1;
		if (rx_ack_cnt >= 10)
		{
			//等待ACK的time out时间设置为300ms，实际ACK一般在
			//50ms内回复；没有侦测到ACK，命令发送失败，需重发
			//MOD_SN95500_LOG("## wait ack time out\n");
			return 2;
		}
	}
}


static void send_cmd(unsigned char cmd)
{
	unsigned char i = 0;

	uint32_t critical = Helios_Critical_Enter();
	
	set_clk_high();
	set_sdio_input();
	//set_sdio_output();
	//set_sdio_high();
	delay_us(20); //实际延时11.5~20us
	set_clk_low();
	delay_us(20); //实际延时11.5~20us

	for (i=0; i<8; i++)
	{
		set_clk_high();

		if (cmd & 0x80)
		{
			set_sdio_input();
			//set_sdio_high();
		}
		else
		{
			set_sdio_output();
			set_sdio_low();
		}

		delay_us(20);
		set_clk_low();

		if (i == 7)
		{
			//set_sdio_high();
			delay_us(20);
			set_sdio_input();
		}
		
		delay_us(20);

		cmd <<= 1;
	}
	
	delay_us(160);
	Helios_Critical_Exit(critical);
}


/*=============================================================================*/
/* FUNCTION: recv_ack                                                          */
/*=============================================================================*/
/*!@brief 		: receive ack from SN95500
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		: ack
 *    
 */
/*=============================================================================*/
static unsigned short recv_ack(void)
{
	int i = 0;
	unsigned int oid_det = 0;
	unsigned char oid[16] = {0};
	unsigned short ack = 0;
	char tmpbuf1[70] = {0};
	char tmpbuf2[5] = {0};
	
	uint32_t critical = Helios_Critical_Enter();
	
	set_clk_high();
	set_sdio_output();
	set_sdio_low();
	delay_us(20); //实际延时11.5~20us
	set_clk_low();
	delay_us(20); //实际延时11.5~20us

	for (i=15; i>=0; i--)
	{
		set_clk_high();
		set_sdio_input();
		delay_us(20);
		set_clk_low();

		oid_det = read_sdio_pin_level();
		oid[15-i] = (unsigned char)oid_det;
		if (oid_det)
		{
			ack = ack | (1 << i);
		}

		delay_us(20);
	}
	delay_us(160); //实际延时86us左右

	Helios_Critical_Exit(critical);
	//下面这段代码为调试打印,可以屏蔽
	sprintf(tmpbuf1, "recv_ack: ack=0x%04x, sdio:", ack);
	for (i=0; i<16; i++)
	{
		sprintf(tmpbuf2, "%hhu ", oid[i]);
		strcat(tmpbuf1, tmpbuf2);
		memset(tmpbuf2, 0, 5);
	}
	MOD_SN95500_LOG("%s\r\n", tmpbuf1);
	
	return ack;
}


static void exit_suspend_mode(void)
{
	set_clk_high();
	delay_us(4);
	set_clk_low();
}
/*========================================================================================*/
/*========================================================================================*/


STATIC mp_obj_t qpy_wakeup_oid(mp_obj_t self_in)
{
	int ret = wakeup_oid();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_wakeup_oid_obj, qpy_wakeup_oid);

#if 0
STATIC mp_obj_t qpy_recv_opt_data(mp_obj_t self_in)
{
	recv_opt_data();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_recv_opt_data_obj, qpy_recv_opt_data);


STATIC mp_obj_t qpy_get_opt_data(mp_obj_t self_in)
{
	unsigned int data1 = 0, data2 = 0;
	
	get_opt_data(&data1, &data2);
	
	mp_obj_t tuple[2] = {
		mp_obj_new_int_from_uint(data1),
		mp_obj_new_int_from_uint(data2),
	};
			
	return mp_obj_new_tuple(2, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_get_opt_data_obj, qpy_get_opt_data);
#endif

STATIC mp_obj_t qpy_check_and_read(mp_obj_t self_in)
{
	unsigned int data1 = 0, data2 = 0;
	int ret = check_and_read_optical_data();
	if (ret == 1)
	{
		get_opt_data(&data1, &data2);
		mp_obj_t tuple[2] = {
			mp_obj_new_int_from_uint(data1),
			mp_obj_new_int_from_uint(data2),
		};
		return mp_obj_new_tuple(2, tuple);
	}
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_check_and_read_obj, qpy_check_and_read);


STATIC mp_obj_t qpy_transmit_cmd(mp_obj_t self_in, mp_obj_t cmd)
{
	unsigned char tmp = (unsigned char)mp_obj_get_int(cmd);
	
	int ret = transmit_cmd(tmp);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_transmit_cmd_obj, qpy_transmit_cmd);

#if 0

STATIC mp_obj_t qpy_send_cmd(mp_obj_t self_in, mp_obj_t cmd)
{
	unsigned char tmp = (unsigned char)mp_obj_get_int(cmd);
	MOD_SN95500_LOG("send_cmd: cmd = 0x%02x\n", tmp);
	send_cmd(tmp);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_send_cmd_obj, qpy_send_cmd);


STATIC mp_obj_t qpy_recv_ack(mp_obj_t self_in)
{
	unsigned short ack = 0;
	
	ack = recv_ack();
	MOD_SN95500_LOG("recv_ack: ack = 0x%04x\n", ack);
	return mp_obj_new_int(ack);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_recv_ack_obj, qpy_recv_ack);


STATIC mp_obj_t qpy_read_sdio_pin(mp_obj_t self_in)
{
	unsigned int val = 0;
	
	val = read_sdio_pin_level();
	return mp_obj_new_int(val);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_read_sdio_pin_obj, qpy_read_sdio_pin);
#endif

STATIC mp_obj_t qpy_exit_suspend_mode(mp_obj_t self_in)
{
	exit_suspend_mode();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_exit_suspend_mode_obj, qpy_exit_suspend_mode);


STATIC const mp_rom_map_elem_t sensor_sn95500_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_wakeup), MP_ROM_PTR(&qpy_wakeup_oid_obj) },
	//{ MP_ROM_QSTR(MP_QSTR_recvData), MP_ROM_PTR(&qpy_recv_opt_data_obj) },
	//{ MP_ROM_QSTR(MP_QSTR_getData), MP_ROM_PTR(&qpy_get_opt_data_obj) },
	{ MP_ROM_QSTR(MP_QSTR_checkAndRead), MP_ROM_PTR(&qpy_check_and_read_obj) },
	{ MP_ROM_QSTR(MP_QSTR_transCmd), MP_ROM_PTR(&qpy_transmit_cmd_obj) },
	//{ MP_ROM_QSTR(MP_QSTR_sendCmd), MP_ROM_PTR(&qpy_send_cmd_obj) },
	//{ MP_ROM_QSTR(MP_QSTR_recvAck), MP_ROM_PTR(&qpy_recv_ack_obj) },
	{ MP_ROM_QSTR(MP_QSTR_exitSuspendMode), MP_ROM_PTR(&qpy_exit_suspend_mode_obj) },
	//{ MP_ROM_QSTR(MP_QSTR_readSdioPin), MP_ROM_PTR(&qpy_read_sdio_pin_obj) },
   
    { MP_ROM_QSTR(MP_QSTR_GPIO1),  MP_ROM_INT(HELIOS_GPIO1) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO2),  MP_ROM_INT(HELIOS_GPIO2) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO3),  MP_ROM_INT(HELIOS_GPIO3) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO4),  MP_ROM_INT(HELIOS_GPIO4) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO5),  MP_ROM_INT(HELIOS_GPIO5) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO6),  MP_ROM_INT(HELIOS_GPIO6) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO7),  MP_ROM_INT(HELIOS_GPIO7) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO8),  MP_ROM_INT(HELIOS_GPIO8) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO9),  MP_ROM_INT(HELIOS_GPIO9) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO10), MP_ROM_INT(HELIOS_GPIO10) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO11), MP_ROM_INT(HELIOS_GPIO11) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO12), MP_ROM_INT(HELIOS_GPIO12) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO13), MP_ROM_INT(HELIOS_GPIO13) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO14), MP_ROM_INT(HELIOS_GPIO14) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO15), MP_ROM_INT(HELIOS_GPIO15) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO16), MP_ROM_INT(HELIOS_GPIO16) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO17), MP_ROM_INT(HELIOS_GPIO17) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO18), MP_ROM_INT(HELIOS_GPIO18) },
	{ MP_ROM_QSTR(MP_QSTR_GPIO19), MP_ROM_INT(HELIOS_GPIO19) },
};
STATIC MP_DEFINE_CONST_DICT(sensor_sn95500_locals_dict, sensor_sn95500_locals_dict_table);

STATIC mp_obj_t sensor_sn95500_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) ;

const mp_obj_type_t sensor_sn95500_type = {
    { &mp_type_type },
    .name = MP_QSTR_SN95500,
    .make_new = sensor_sn95500_make_new,
    .locals_dict = (mp_obj_dict_t *)&sensor_sn95500_locals_dict,
};

STATIC mp_obj_t sensor_sn95500_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) 
{
    mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, true);
 	
	Helios_GPIOInitStruct clk_struct = {0};
	Helios_GPIOInitStruct sdio_struct = {0};
    sensor_sn95500_obj_t *self = (sensor_sn95500_obj_t *)&sn95500_obj;
    self->base.type = &sensor_sn95500_type;
	int clk_pin = mp_obj_get_int(args[0]);
	int sdio_pin = mp_obj_get_int(args[1]);

	//MOD_SN95500_LOG("### clk_pin=%d, sdio_pin=%d\n", self->clk_pin, self->sdio_pin);
	self->clk_pin = (Helios_GPIONum)clk_pin;
	self->sdio_pin = (Helios_GPIONum)sdio_pin;

	clk_struct.dir = HELIOS_GPIO_OUTPUT;
	clk_struct.pull = HELIOS_PULL_DOWN;
	clk_struct.value = HELIOS_LVL_LOW;
	Helios_GPIO_Init((Helios_GPIONum)clk_pin, &clk_struct);

	sdio_struct.dir = HELIOS_GPIO_INPUT;
	sdio_struct.pull = HELIOS_PULL_UP;
	sdio_struct.value = HELIOS_LVL_HIGH;
	Helios_GPIO_Init((Helios_GPIONum)sdio_pin, &sdio_struct);

	self->recv_data1 = 0;
	self->recv_data2 = 0;
	self->data_mutex = 0;

	self->data_mutex = Helios_Mutex_Create();

    return MP_OBJ_FROM_PTR(self);
}




