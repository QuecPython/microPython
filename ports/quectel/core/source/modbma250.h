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



#ifndef _GSENSORDEMO_H
#define _GSENSORDEMO_H


#ifdef __cplusplus
extern "C" {
#endif

#define EC600S_IIC_BMA250_CHL (0)

#define BMA250_INT_GPIO   (9)
#define BMA250E_IIC_ADDR             (0x18)

#define BMA250E_REG_BGW_CHIPID        0x00
#define BMA250E_REG_ACCD_X_LSB        0x02
#define BMA250E_REG_ACCD_X_MSB        0x03
#define BMA250E_REG_ACCD_Y_LSB        0x04
#define BMA250E_REG_ACCD_Y_MSB        0x05
#define BMA250E_REG_ACCD_Z_LSB        0x06
#define BMA250E_REG_ACCD_Z_MSB        0x07
#define BMA250E_REG_ACCD_TEMP         0x08

#define BMA250E_REG_INT_STATUS_0      0x09
#define BMA250E_REG_INT_STATUS_1      0x0A
#define BMA250E_REG_INT_STATUS_2      0x0B
#define BMA250E_REG_INT_STATUS_3      0x0C

#define BMA250E_REG_PMU_RANGE         0x0F	   // g Range
#define BMA250E_REG_PMU_BW            0x10	   // Bandwidth
#define BMA250E_REG_PMU_LPW           0x11	   // Power modes
#define BMA250E_REG_ACCD_HBW          0x13	   // Special Control Register
#define BMA250E_REG_SOFTRESET         0x14	   // Soft reset register writing 0xB6 causes reset
#define BMA250E_REG_INT_EN_0          0x16	   // Interrupt settings register 1
#define BMA250E_REG_INT_EN_1          0x17	   // Interrupt settings register 2
#define BMA250E_REG_INT_EN_2          0x18
#define BMA250E_REG_INT_MAP_0         0x19	   // Interrupt mapping register 1
#define BMA250E_REG_INT_MAP_1         0x1A	   // Interrupt mapping register 2
#define BMA250E_REG_INT_MAP_2         0x1B	   // Interrupt mapping register 3

#define BMA250E_REG_INT_SRC           0x1E

#define BMA250E_REG_INT_OUT_CTRL      0x20
#define BMA250E_REG_INT_RST_LATCH     0x21
#define BMA250E_REG_INT_0             0x22
#define BMA250E_REG_INT_1             0x23
#define BMA250E_REG_INT_2             0x24
#define BMA250E_REG_INT_3             0x25
#define BMA250E_REG_INT_4             0x26
#define BMA250E_REG_INT_5             0x27
#define BMA250E_REG_INT_6             0x28
#define BMA250E_REG_INT_7             0x29


#define Tumble_high_g_time 200 //ms
#define Tumble_no_motion_time 3500 //ms

typedef struct 
{
    uint16_t chipid;
    float    accd_x;
    float    accd_y;
    float    accd_z;
    char     GoodsState[50];
}BMA250_INFO_t;

typedef enum
{
    x = 1,
    y = 2,
    z = 3,
}ACCD_ChannelEnum_t;

typedef enum
{
    GsensorInt_none,
    GsensorInt_freefall,
    GsensorInt_high_g,
    GsensorInt_no_motion,
    GsensorInt_motion,
    
}En_GsensorInt;

typedef enum
{
    Tumble_none,
    Tumble_freefall,
    Tumble_high_g,
    Tumble_no_motion,
    
}En_Tumble_DetectStatus;

typedef unsigned char u8;

/*===========================================================================
 * Functions declaration
 ===========================================================================*/
void  BMA250E_Init(void);
void  BMA250E_GetInfo(BMA250_INFO_t *BMA250_INFO);
void  BMA250E_GetChipID(BMA250_INFO_t *BMA250_INFO);
float BMA250E_Get_ACCD(ACCD_ChannelEnum_t channel);

void     BMA250E_WriteByte(u8 reg_addr,u8 data);
uint8_t  BMA250E_ReadByte(u8 reg_addr);



#ifdef __cplusplus
} /*"C" */
#endif

#endif /* _GNSSDEMO_H */



