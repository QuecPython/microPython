/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2021, QUECTEL  
 * Copyright (c) Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
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


/*===========================================================================
 * include files
 ===========================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "obj.h"
#include "runtime.h"
#include "mphalport.h"
#include "ql_api_osi.h"
#include "ql_log.h"
#include "modbma250.h"
#include "ql_uart.h"
#include "ql_i2c.h"
#include "ql_type.h"
#include "ql_gpio.h"


#define QL_MODGSENSOR_LOG(msg, ...)    QL_LOG(QL_LOG_LEVEL_INFO, "bma250", msg, ##__VA_ARGS__)


/*===========================================================================
 * Variate
 ===========================================================================*/
ql_task_t gsensor_task = NULL;
ql_task_t gsensor_tumble_task = NULL;

BMA250_INFO_t  BMA250_INFO={0};
int8_t  BMA250E_MeasureRange = 8;
int GsensorIntStatus=0;
int Tumble_detect_stage=0;



/*===========================================================================
 * Functions
 ===========================================================================*/

void BMA250E_WriteByte(u8 reg_addr,u8 data)
{
    int ret;
    
    //printf("### OPT3001_WriteHalfWord wdata=%#x,%#x\r\n",wdata[0],wdata[1]);
    ret=ql_I2cWrite(EC600S_IIC_BMA250_CHL, BMA250E_IIC_ADDR, reg_addr, &data,1);
    if(ret != 0)
        QL_MODGSENSOR_LOG("### error BMA250E_WriteByte ret=%d\r\n",ret);

}


uint8_t  BMA250E_ReadByte(u8 reg_addr)
{
    u8 data=0xff;
    int ret;
    
    ret=ql_I2cRead(EC600S_IIC_BMA250_CHL,BMA250E_IIC_ADDR, reg_addr,&data,1);
    if(ret<0)
        QL_MODGSENSOR_LOG("error BMA250E_ReadByte ret=%d\r\n",ret);

    return data;
}

void BMA250E_Init(void)
{
    BMA250E_WriteByte(BMA250E_REG_SOFTRESET, 0xB6);         
    ql_rtos_task_sleep_ms(50);
    switch(BMA250E_MeasureRange)
    {
        case 2:
            BMA250E_WriteByte(BMA250E_REG_PMU_RANGE,0x03);          
            BMA250E_MeasureRange = 4;
        break;
        
        default:
        case 4:
            BMA250E_WriteByte(BMA250E_REG_PMU_RANGE,0x05);         
            BMA250E_MeasureRange = 8;
        break;
        
        case 8:
            BMA250E_WriteByte(BMA250E_REG_PMU_RANGE,0x08);          
            BMA250E_MeasureRange = 16;
        break;
        
        case 16:
            BMA250E_WriteByte(BMA250E_REG_PMU_RANGE,0x0C);         
            BMA250E_MeasureRange = 32;
        break;
    }
    
    
    BMA250E_WriteByte(BMA250E_REG_INT_RST_LATCH,0x8d);      
//  BMA250E_WriteByte(BMA250E_REG_INT_OUT_CTRL, 0x0A);      
    BMA250E_WriteByte(BMA250E_REG_INT_OUT_CTRL, 0x05);      
    BMA250E_WriteByte(BMA250E_REG_INT_MAP_0, 0x0D);         
    
    BMA250E_WriteByte(BMA250E_REG_INT_0, 0x00);             
    BMA250E_WriteByte(BMA250E_REG_INT_1, 0x5D);             
    
    BMA250E_WriteByte(BMA250E_REG_INT_2, 0x04);             
    BMA250E_WriteByte(BMA250E_REG_INT_3, 0x0f);             
    BMA250E_WriteByte(BMA250E_REG_INT_4, 0x40);             
    
    BMA250E_WriteByte(BMA250E_REG_INT_5, 0x03);             
    BMA250E_WriteByte(BMA250E_REG_INT_6, 0x14);             
    BMA250E_WriteByte(BMA250E_REG_INT_7, 0x14);             
    
    BMA250E_WriteByte(BMA250E_REG_INT_EN_0, 0x07);        
    BMA250E_WriteByte(BMA250E_REG_INT_EN_1, 0x0F);         
    BMA250E_WriteByte(BMA250E_REG_INT_EN_2, 0x0f);         
    
    BMA250E_ReadByte(BMA250E_REG_INT_STATUS_0);
    strcpy(BMA250_INFO.GoodsState,"Init.."); 
    
    GsensorIntStatus=0;
    Tumble_detect_stage=0;
    
    BMA250E_GetChipID(&BMA250_INFO);
}

void BMA250E_GetChipID(BMA250_INFO_t *BMA250_INFO)
{
    BMA250_INFO->chipid = BMA250E_ReadByte(BMA250E_REG_BGW_CHIPID);
}
void BMA250E_GetIntStatus(BMA250_INFO_t *BMA250_INFO)
{
    uint8_t status;
    bool     int_state_low_g;
    bool     int_state_high_g;
    bool     int_state_no_motion;
    bool     int_state_motion;
    
    status = BMA250E_ReadByte(BMA250E_REG_INT_STATUS_0);

    int_state_no_motion = status & (1<<3);
    int_state_motion    = status & (1<<2);
    int_state_high_g    = status & (1<<1);
    int_state_low_g     = status & (1<<0);

    if(int_state_no_motion != 0)
    {
        GsensorIntStatus = GsensorInt_no_motion;
        BMA250E_WriteByte(BMA250E_REG_INT_EN_0, 0x07); 
        BMA250E_WriteByte(BMA250E_REG_INT_EN_2, 0x00);         
        BMA250E_ReadByte(BMA250E_REG_INT_STATUS_0);
        strcpy(BMA250_INFO->GoodsState,"Motionless"); 
    }
    if(int_state_motion != 0)
    {
        GsensorIntStatus = GsensorInt_motion;
//      BMA250E_WriteByte(BMA250E_REG_INT_EN_0, 0x00);          
        BMA250E_WriteByte(BMA250E_REG_INT_EN_2, 0x0f);          
        BMA250E_ReadByte(BMA250E_REG_INT_STATUS_0);
        strcpy(BMA250_INFO->GoodsState,"Motion");
    }
    if(int_state_high_g != 0)
    {
        GsensorIntStatus = GsensorInt_high_g;
        BMA250E_WriteByte(BMA250E_REG_INT_EN_0, 0x07);          
        BMA250E_WriteByte(BMA250E_REG_INT_EN_2, 0x0f); 
        BMA250E_ReadByte(BMA250E_REG_INT_STATUS_0);
        strcpy(BMA250_INFO->GoodsState,"Crash");
    } 
    
    if(int_state_low_g != 0)
    {
        GsensorIntStatus = GsensorInt_freefall;
        BMA250E_WriteByte(BMA250E_REG_INT_EN_0, 0x07); 
        BMA250E_WriteByte(BMA250E_REG_INT_EN_2, 0x0f); 
        BMA250E_ReadByte(BMA250E_REG_INT_STATUS_0);
        strcpy(BMA250_INFO->GoodsState,"Fall");
    } 

    QL_MODGSENSOR_LOG("BMA250E_GetInfo int status:%s,%#x",BMA250_INFO->GoodsState,status);
}
void BMA250E_GetInfo(BMA250_INFO_t *BMA250_INFO)
{
    BMA250_INFO->accd_x = BMA250E_Get_ACCD(x);
    BMA250_INFO->accd_y = BMA250E_Get_ACCD(y);
    BMA250_INFO->accd_z = BMA250E_Get_ACCD(z);
}

float BMA250E_Get_ACCD(ACCD_ChannelEnum_t channel)
{
    int16_t accd_lsb=0,accd_msb=0;
    int16_t   accd_value=0;
    int16_t accd_l=0,accd_m=0;
    float   accd_f;
    
    if(channel == x)
    {
        accd_lsb = BMA250E_REG_ACCD_X_LSB;
        accd_msb = BMA250E_REG_ACCD_X_MSB;
    }
    else if(channel == y)
    {
        accd_lsb = BMA250E_REG_ACCD_Y_LSB;
        accd_msb = BMA250E_REG_ACCD_Y_MSB;
    }
    else if(channel == z)
    {
        accd_lsb = BMA250E_REG_ACCD_Z_LSB;
        accd_msb = BMA250E_REG_ACCD_Z_MSB;
    }
    
    accd_l = (BMA250E_ReadByte(accd_lsb)>>2);
    accd_m = BMA250E_ReadByte(accd_msb);
    
    accd_value = (accd_l>>6) + (accd_m<<2);
    
    accd_value &= 0x03ff;
    
    if(accd_value >> 9)
    {
        accd_value &= 0x01ff;
        accd_value  = ~accd_value;
        accd_value += 1;
        accd_value &= 0x01ff;
        accd_value  = - accd_value;
        accd_f      = (accd_value*BMA250E_MeasureRange/1024.0);
    }
    else
    {
        accd_value = accd_value;
        accd_f     = (accd_value*BMA250E_MeasureRange/1024.0);
    }
    
    return accd_f;
}

bool BMA250_TumbleCheckDemo(void)
{
    int cnt=0;
    
    if(GsensorIntStatus == GsensorInt_freefall)
    {
       Tumble_detect_stage = Tumble_high_g;
       goto detect_crash;
    }
    else
    {
       return false;
    }
    
detect_crash:
        while(cnt < Tumble_high_g_time)   // 200ms
        {
            if(GsensorIntStatus == GsensorInt_high_g)
            {
                Tumble_detect_stage = Tumble_no_motion;
                cnt=0;
                goto detect_no_motion;
            }
            cnt++;
            ql_rtos_task_sleep_ms(1);

        }
        return false;
    
detect_no_motion:
        while(cnt < Tumble_no_motion_time)  //3.5s
        {
            if(GsensorIntStatus == GsensorInt_no_motion)
            {
                return true;
            }
            cnt++;
            ql_rtos_task_sleep_ms(1);

        }
        return false;

}

void mod_i2c_bma250_int_handler(void)
{
    
    ql_int_disable(BMA250_INT_GPIO);
    
    QL_MODGSENSOR_LOG("############### APP BMA250 MONTION INT ###############\r\n");
    BMA250E_GetIntStatus(&BMA250_INFO);
    
    ql_int_enable(BMA250_INT_GPIO);
}

static void mod_gsensor_bma250_thread(void *param)
{
    int ret=0;
    QL_MODGSENSOR_LOG("gsensor demo thread enter, param 0x%x", param);
    ret=ql_I2cInit(i2c_1, STANDARD_MODE);
    QL_MODGSENSOR_LOG("ql_I2cInit ret:%d",ret);
    
    ql_gpio_deinit(BMA250_INT_GPIO);
    ql_int_register(BMA250_INT_GPIO,  EDGE_TRIGGER, DEBOUNCE_EN,EDGE_FALLING,PULL_NONE,mod_i2c_bma250_int_handler ,NULL);
    ql_int_enable(BMA250_INT_GPIO);
    
    BMA250E_Init();

    while(1)
    { 
        BMA250E_GetInfo(&BMA250_INFO);
        BMA250E_GetIntStatus(&BMA250_INFO);
        QL_MODGSENSOR_LOG("***task BMA250,accd_x:%f,\t accd_y:%f,\t accd_z:%f\r\n",BMA250_INFO.accd_x,BMA250_INFO.accd_y,BMA250_INFO.accd_z); 

        ql_rtos_task_sleep_ms(10);
    }
    

    QL_MODGSENSOR_LOG("gsensor demo thread exit, param 0x%x", param);
    ql_rtos_task_delete(NULL);
}

STATIC mp_obj_t mod_gsensor_bma250_app_init(void)
{
    QlOSStatus err = QL_OSI_SUCCESS;
    ql_rtos_task_sleep_s(3);

    err = ql_rtos_task_create(&gsensor_task, 4096, APP_PRIORITY_NORMAL, "gsensordemo", mod_gsensor_bma250_thread, NULL, 5);
    if( err != QL_OSI_SUCCESS )
    {
        QL_MODGSENSOR_LOG("gsensor demo task created failed");
        return mp_obj_new_int(-1);
    }
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_gsensor_bma250_app_init_obj, mod_gsensor_bma250_app_init);

STATIC mp_obj_t mod_bma250_TumbleCheckDemo(void)
{
    bool flag = BMA250_TumbleCheckDemo();

    if ( flag )
    {
        QL_MODGSENSOR_LOG("*** gsensor_tumble check true \n");
        return mp_obj_new_int(1);
    }
    QL_MODGSENSOR_LOG("*** gsensor_tumble check false \n");
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_bma250_TumbleCheckDemo_obj, mod_bma250_TumbleCheckDemo);

STATIC mp_obj_t mod_bma250_getInfo(void)
{
	float accd_x = 0.00;
	float accd_y = 0.00;
	float accd_z = 0.00;

    accd_x = BMA250_INFO.accd_x;
	accd_y = BMA250_INFO.accd_y;
	accd_z = BMA250_INFO.accd_z;

	mp_obj_t tuple[3] = {mp_obj_new_float(accd_x), mp_obj_new_float(accd_y), mp_obj_new_float(accd_z)};

	return mp_obj_new_tuple(3, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_bma250_getInfo_obj, mod_bma250_getInfo);


STATIC const mp_rom_map_elem_t bma250_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_bma250) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&mod_gsensor_bma250_app_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_getTumble), MP_ROM_PTR(&mod_bma250_TumbleCheckDemo_obj) },
    { MP_ROM_QSTR(MP_QSTR_getInfo), MP_ROM_PTR(&mod_bma250_getInfo_obj) },
};
STATIC MP_DEFINE_CONST_DICT(bma250_module_globals, bma250_module_globals_table);

const mp_obj_module_t mp_module_bma250 = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&bma250_module_globals,
};







