#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "helios_dev.h"
#include "helios_debug.h"
#include "helios_wifiscan.h"

#include "nlr.h"
#include "objlist.h"
#include "objstr.h"
#include "runtime.h"
#include "mperrno.h"

#define SAFE_FREE(p)            {if(p){free(p); (p)=NULL;}}

/* 短整型大小端互换*/
#define BigLittleSwap16(A)  ((((uint16_t)(A) & 0xff00) >> 8) | (((uint16_t)(A) & 0x00ff) << 8))

/* 长整型大小端互换*/
#define BigLittleSwap32(A)  ((((uint32_t)(A) & 0xff000000) >> 24) | \
                              (((uint32_t)(A) & 0x00ff0000) >> 8) | \
                              (((uint32_t)(A) & 0x0000ff00) << 8) | \
                              (((uint32_t)(A) & 0x000000ff) << 24))

/* 服务类型*/
#define Q_SERV_TYPE_LOC     0x01
#define Q_SERV_TYPE_LPOS    0x02
#define Q_SERV_TYPE_APS     0x03
#define Q_SERV_TYPE_USER    0x04

/*基站定位请求TAG宏定义*/
#define BER_LOC_TAG               0x01  /* 基站定位请求，嵌套类型*/
#define BER_LOC_BASIC_TAG         0x01  /* 基本信息*/
#define BER_LOC_AUTH_TAG          0x02  /* 鉴权信息*/

#define BER_LOC_WIFI1_TAG         0x14  /*WIFI信息1*/
#define BER_LOC_WIFI2_TAG         0x15  /*WIFI信息2*/
#define BER_LOC_WIFI3_TAG         0x16  /*WIFI信息3*/
#define BER_LOC_WIFI4_TAG         0x17  /*WIFI信息4*/
#define BER_LOC_WIFI5_TAG         0x18  /*WIFI信息5*/
#define BER_LOC_WIFI6_TAG         0x19  /*WIFI信息6*/

#define BER_LOC_MD5_TAG           0x09  /* MD5*/
#define BER_LOC_SRV_LIST_VER_TAG  0x0A /*固化服务器表version*/

/* 服务类型字符串*/
#define Q_SERV_STR_LOC      ("q_loc")
#define Q_SERV_STR_LPOS     ("q_lpos")
#define Q_SERV_STR_APS      ("q_agps")
#define Q_SERV_STR_USER     ("q_user")

/* 基站定位的服务类型*/
#define Q_LOC_MOD_WIFI      0x06 //wifi定位

/* 加密类型*/
#define Q_ENCRIPT_XOR       0x01

/* 密钥索引 (暂先固定为0x01)*/
#define Q_LOC_KEY_INDEX  0x01

/* 返回信息是否包含地址*/
#define Q_ADDR_NONE         0x01
#define Q_ADDR_INCLUDE      0x02

#define MAX_RAW_KEY_NUM     (8) /*加扰key的总个数*/
#define XOR_KEY_LEN         (8) /*异或加解密的KEY长度*/

static uint8_t xor_key[MAX_RAW_KEY_NUM][XOR_KEY_LEN] =
{ 
    {0x01,0x03,0xef,0xfe,0x12,0x34,0x66,0x88},
    {0x32,0x47,0xdf,0x3e,0x33,0x69,0x76,0x92},
    {0x23,0x28,0x12,0xba,0xa5,0x12,0x38,0x2a},
    {0x43,0x82,0x11,0xdb,0xb9,0xa2,0x83,0x1c},
    {0xda,0x28,0xda,0x29,0x7a,0x2c,0x71,0x7c},
    {0x1b,0xf3,0x32,0x21,0xee,0xc2,0xea,0x36},
    {0x2f,0x25,0x27,0x83,0xed,0xec,0xa2,0x2f},
    {0x3a,0xcd,0xef,0x37,0x2a,0x3b,0x45,0x9a}
};


/* 数据包类型：数据包格式，01为不包含地址的回复，02为包含地址的回复*/
#define Q_LOC_DATA_METHOD  0x01


#define BER_CONSTRUCTOR                   (0x20)
#define BER_LONG_LEN                      (0x80)
#define BER_MORE_TAG                      (0x80)
#define BER_EXTENSION_ID                  (0x1F)

//wifi 
#define MAX_WIFI_INFO_SIZE 58

#define BER_LOC_WIFI1_TAG         0x14  /*WIFI信息1*/
#define BER_LOC_WIFI2_TAG         0x15  /*WIFI信息2*/
#define BER_LOC_WIFI3_TAG         0x16  /*WIFI信息3*/
#define BER_LOC_WIFI4_TAG         0x17  /*WIFI信息4*/
#define BER_LOC_WIFI5_TAG         0x18  /*WIFI信息5*/
#define BER_LOC_WIFI6_TAG         0x19  /*WIFI信息6*/

#define MAX_MAC_ADDR_STR_LEN        18
#define MAX_WIFI_AP_SSID_LEN        32

#define AUTH_MD5_LEN   8 /*MD5值长度*/
#define MAX_SIGLE_TLV_LEN  (2046) /*单个TLV长度最大为2046字节*/

#define MAX_SERVER_LIST_VERSION_SIZE       (20)

typedef enum QuectelBerClass_Tag{
    BER_TAG_CLASS_UNIVERSAL        = 0,      /* 0b00 */
    BER_TAG_CLASS_APPLICATION    = 1,      /* 0b01 */
    BER_TAG_CLASS_CONTEXT            = 2,      /* 0b10 */
    BER_TAG_CLASS_PRIVATE             = 3	       /* 0b11 */
}ber_class_tag;

typedef enum QuectelBerConstruct_Tag{
    BER_TAG_PRIMITIVE,
    BER_TAG_CONSTRUCTED
}ber_construct_tag;

typedef struct QuectelBerTlv_Tag
{
    int32_t tag_value;
    ber_construct_tag construct_tag;
    ber_class_tag class_tag;
}ber_tlv_tag;

/*retun loc*/
typedef struct
{
    uint8_t serve_type;       /*服务类型*/
    uint8_t encript_type;     /*加密类型*/
    uint8_t key_index;         /*密钥索引*/
    uint8_t pos_data_type;  /*数据包类型*/
    uint8_t loc_method;        /*定位类型*/
}tlv_baseinfo_struct;

#define MAX_BASIC_INFO_SIZE 5

/*请求的原始数据打包解包*/
#define AUTH_NAME_LEN 16
#define AUTH_PWD_LEN 16
#define AUTH_IMEI_LEN 15
#define AUTH_TOTKEN_LEN 16
#define AUTH_FIXINFO_LEN        17 /* 固定长度：IMEI(15)+RAND(2)=17*/
#define MAX_AUTH_INFO_SIZE  (AUTH_NAME_LEN+AUTH_PWD_LEN+AUTH_FIXINFO_LEN)

typedef struct  QTlvAuthInfo_Tag
{
    uint8_t name[AUTH_NAME_LEN+1];                 /*用户名*/
    uint8_t pwd[AUTH_PWD_LEN+1];                   /*密码*/
    uint8_t imei[AUTH_IMEI_LEN+1];                 /*IMEI*/
    uint8_t token[AUTH_TOTKEN_LEN*2+1];            /*密钥*/
    int16_t rand;                                  /*随机数*/
}QTlvAuthInfo;

typedef struct
{
  uint8_t       mac[MAX_MAC_ADDR_STR_LEN];   // eg. "44:6a:2e:12:08:01"
  uint8_t       ssid[MAX_WIFI_AP_SSID_LEN];  // eg. "Quectel-hf"
  int16_t       rssi;  // eg. "-30
}tlv_wifiinfo_struct;


/*返回位置信息*/
#define BER_POS_TAG                         0x02/* 复合结构*/
#define BER_POS_STAT_TAG                    0x01
#define BER_POS_BASIC_TAG                   0x02
#define BER_POS_RAND_TAG                    0x03
#define BER_POS_POS1_TAG                    0x04 
#define BER_POS_POS2_TAG                    0x05
#define BER_POS_POS3_TAG                    0x06
#define BER_POS_POS4_TAG                    0x07 
#define BER_POS_POS5_TAG                    0x08 
#define BER_POS_POS6_TAG                    0x09 
#define BER_POS_MD5_TAG                     0x0A  /* MD5*/

#define BER_POS_REDIRECT1_TAG               0x0B /*重定向表数据1*/
#define BER_POS_REDIRECT2_TAG               0x0C /*重定向表数据2*/
#define BER_POS_REDIRECT3_TAG               0x0D /*重定向表数据3*/
#define BER_POS_SRVLIST_VER_TAG             0x0E /*固化服务器表版本信息*/
#define BER_POS_SRVLIST1_TAG                0x0F /*固化服务器表数据1*/
#define BER_POS_SRVLIST2_TAG                0x10
#define BER_POS_SRVLIST3_TAG                0x11
#define BER_POS_SRVLIST4_TAG                0x12
#define BER_POS_SRVLIST5_TAG                0x13
#define BER_POS_SRVLIST6_TAG                0x14
#define BER_POS_SRVLIST7_TAG                0x15
#define BER_POS_SRVLIST8_TAG                0x16
#define BER_POS_SRVLIST9_TAG                0x17
#define BER_POS_SRVLIST10_TAG               0x18 /*固化服务器表数据10*/
#define BER_POS_ADDRESS_TAG                 0x19 /*街道信息*/
#define BER_POS_TRIANGLE_FRE_TAG            0x1A/*获取服务器返回的三角定位访问频率*/
#define BER_POS_ACCURACY_LEVEL_TAG          0x1B  /*服务器返回是否是精准定位*/

/*status: 
0 = 成功定位
1 = 用户信息验证失败
2 = 请求格式不正确
3= 基站信息未发现*/
/* 返回的状态类型*/
typedef enum{
    POS_POS_NUM_0 =0,
    POS_POS_NUM_1,
    POS_POS_NUM_2,
    POS_POS_NUM_3,
    POS_POS_NUM_4,
    POS_POS_NUM_5,
    POS_POS_NUM_6, 
    POS_POS_NUM_10 = 10, //表示该token不存在或绑定的设备超过最大值
    POS_POS_NUM_11 = 11, //11表示设备的每天定位超过最大访问次数
    POS_POS_NUM_12 = 12, //token定位超过最大访问次数
    POS_POS_NUM_13 = 13, //token过期
    POS_POS_NUM_14 = 14, //该IMEI号不可访问服务器
    POS_POS_NUM_15 = 15, //token的每天定位超过最大访问次数
    POS_POS_NUM_16 = 16, //token的周期内定位超过最大访问次数
    POS_POS_NUM_17 = 17, //IMEI号不合法
    POS_POS_NUM_18 = 18, //token绑定设备超过最大值 
}QTlvRetPosStat;

//encode TLV info
typedef struct QTLVNode_Tag {
    //q_link_type  link_ptr;
    ber_tlv_tag tag;                     /*标记值*/
    uint32_t length;                   /* 数据长度*/
    uint8_t* value;                  /* 数据*/
    struct QTLVNode_Tag* next;                  /*兄弟*/
    struct QTLVNode_Tag* child;                 /*孩子*/

    struct QTLVNode_Tag*(*AddBrother)(struct QTLVNode_Tag* curNode, struct QTLVNode_Tag* newNode);
    struct QTLVNode_Tag*(*AddChild)(struct QTLVNode_Tag* curNode, struct QTLVNode_Tag* newNode);
}QTLVNode;

static QTLVNode* QTLVNode_Create(void);

