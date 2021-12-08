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

#include "helios_datacall.h"
#include "helios_celllocator.h"
#include "helios_debug.h"
#include "quos_md5.h"

#include "modwifilocator.h"

#define QPY_MODLBS_LOG(msg, ...)      custom_log(wifilocator, msg, ##__VA_ARGS__)
#define WIFI_LOWER_CASE( c ) ( ((c) >= 'A' && (c) <= 'Z') ? ((c) + 0x20) : (c) )

STATIC Helios_LBSInfoStruct position_rec = {0};

STATIC bool checkCPUendian()
{
    union{
        uint32_t i;
        uint8_t s[4];
    }u_test;
    u_test.i = 0x12345678;
    return (0x12 == u_test.s[0]);
}

STATIC uint16_t HtoN_uint16(uint16_t h)
{
    return checkCPUendian() ? h : BigLittleSwap16(h);
}

STATIC int16_t HtoN_int16(int16_t h)
{
    return (int16_t)checkCPUendian() ? h : BigLittleSwap16(h);
}

STATIC uint8_t* ber_length_serialize(int32_t tlen, uint8_t *bufptr, int32_t* len)
{
    uint8_t *start_data = bufptr;
    int32_t len_size = *len;

    //TLV_ASSERT(len != 0 && bufptr != 0);

    /* no indefinite lengths sent */
    if (tlen < 0x80)
    {
        if (len_size < 1)
        {
            return NULL;
        }
        *bufptr++ = (uint8_t)tlen;
    }
    else if (tlen <= 0xFF) 
    {
        if (len_size < 2)
        {
            return NULL;
        }
        *bufptr++ = (uint8_t)(0x01 | BER_LONG_LEN);
        *bufptr++ = (uint8_t)tlen;
    }
    else if (tlen <= 0xFFFF)
    { /* 0xFF < tlen <= 0xFFFF */
        if (len_size < 3)
        {
            return NULL;
        }
        *bufptr++ = (uint8_t)(0x02 | BER_LONG_LEN);
        *bufptr++ = (uint8_t)((tlen >> 8) & 0xFF);
        *bufptr++ = (uint8_t)(tlen & 0xFF);
    }
    else if (tlen <= 0xFFFFFF)
    { /* 0xFF < tlen <= 0xFFFF */
        if (len_size < 4)
        {
            return NULL;
        }
        *bufptr++ = (uint8_t)(0x03 | BER_LONG_LEN);
        *bufptr++ = (uint8_t)((tlen >> 16) & 0xFF);
        *bufptr++ = (uint8_t)((tlen >> 8) & 0xFF);
        *bufptr++ = (uint8_t)(tlen & 0xFF);
    }
    else
    {
        if (len_size < 5)
        {
            return NULL;
        }
        *bufptr++ = (uint8_t)(0x04 | BER_LONG_LEN);
        *bufptr++ = (uint8_t)((tlen >> 24) & 0xFF);
        *bufptr++ = (uint8_t)((tlen >> 16) & 0xFF);
        *bufptr++ = (uint8_t)((tlen >> 8) & 0xFF);
        *bufptr++ = (uint8_t)(tlen & 0xFF);
    }

    *len = (int32_t)(bufptr-start_data);
    return bufptr;
}

STATIC uint8_t* ber_tag_serialize(ber_class_tag tclass, ber_construct_tag tconstructed, uint32_t tval, uint8_t *bufp, int32_t* len)
{
    uint8_t *buf = (uint8_t *)bufp;
    uint8_t *end = NULL;
    int32_t tag_len = *len;
    int32_t required_size = 1;
    int32_t i = 0;

    if(len == 0 || bufp == 0)
    {
        return NULL;
    }

    if(tval <= 30)
    {
        /* Encoded in 1 octet */
        if(tag_len)
            buf[0] = (tclass << 6) | (tconstructed << 5) | tval;
        *len = 1;
        return bufp;
    }
    else if(tag_len)
    {
        *buf++ = (tclass << 6) | (tconstructed << 5) |0x1F;
        tag_len--;
    }

    /*
    * Compute the size of the subsequent bytes.
    */
    int32_t size = (int32_t)8 * sizeof(tval);
    for(required_size = 1, i = 7; i < size; i += 7) {
        if(tval >> i)
            required_size++;
        else
            break;
    }

    if(tag_len < required_size)
        return NULL;

    /*
    * Fill in the buffer, space permitting.
    */
    end = buf + required_size - 1;
    for(i -= 7; buf < end; i -= 7, buf++)
        *buf = 0x80 | ((tval >> i) & 0x7F);
    *buf = (tval & 0x7F);   /* Last octet without high bit */

    *len = required_size + 1;
    return buf;
}

STATIC int32_t tlv_serialize_tlv_buffer(const ber_tlv_tag* tlvtag, int32_t tlvlen,const uint8_t* tlvvalue,
                                           int32_t valuesize, uint8_t** retbuf)
{
    uint8_t tagbuf[32] = {0};
    int32_t taglen = sizeof(tagbuf);
    uint8_t lenbuf[32] = {0};
    int32_t lensize = sizeof(lenbuf);
    uint8_t* alloc_buf = NULL;

    if(!ber_tag_serialize(tlvtag->class_tag, tlvtag->construct_tag, tlvtag->tag_value, tagbuf, &taglen))
    {
        QPY_MODLBS_LOG("ber_tag_serialize Fail \r\n");
        return -1;
    }

    if(!ber_length_serialize(tlvlen,lenbuf,&lensize))
    {
        QPY_MODLBS_LOG("ber_length_serialize Fail \r\n");
        return -1;
    }

    *retbuf = (uint8_t*)malloc(taglen+lensize+valuesize);
    if(!*retbuf)
    {
        QPY_MODLBS_LOG("apptcpip_mem_malloc Fail \r\n");
        return -1;
    }
    alloc_buf = *retbuf;
    memcpy(alloc_buf,tagbuf,taglen);
    alloc_buf += taglen;
    memcpy(alloc_buf,lenbuf,lensize);
    alloc_buf += lensize;
    memcpy(alloc_buf,tlvvalue,valuesize);
    return(taglen+lensize+valuesize);
}

STATIC uint32_t QTlv_Serialize_BaseInfo_Buf(tlv_baseinfo_struct* pBaseInfo, uint8_t *retBuffer)
{
    uint32_t data_len = 0;

    if(pBaseInfo == NULL || retBuffer == NULL)
    {
        return 0;
    }

    memcpy(retBuffer, &(pBaseInfo->serve_type),sizeof(pBaseInfo->serve_type));
    data_len += sizeof(pBaseInfo->serve_type);
    memcpy(retBuffer+data_len, &(pBaseInfo->encript_type),sizeof(pBaseInfo->encript_type));
    data_len += sizeof(pBaseInfo->encript_type);
    memcpy(retBuffer+data_len, &(pBaseInfo->key_index),sizeof(pBaseInfo->key_index));
    data_len += sizeof(pBaseInfo->key_index);
    memcpy(retBuffer+data_len, &(pBaseInfo->pos_data_type),sizeof(pBaseInfo->pos_data_type));
    data_len += sizeof(pBaseInfo->pos_data_type);
    memcpy(retBuffer+data_len, &(pBaseInfo->loc_method),sizeof(pBaseInfo->loc_method));
    data_len += sizeof(pBaseInfo->loc_method);

    return data_len;
}

STATIC uint32_t QTlv_Serialize_AuthInfo_Buf(bool bWithPwd, uint8_t *retBuffer, QTlvAuthInfo *authInfo)
{
    int16_t net_order = 0;
    uint32_t data_len = 0;
    uint32_t tmpLen = 0;

    tmpLen = strlen((const char *)authInfo->name);
    memcpy(retBuffer, authInfo->name, tmpLen);
    data_len = tmpLen;

    if(1 == bWithPwd)
    {
        tmpLen = strlen((const char *)authInfo->pwd);
        memcpy(retBuffer+data_len, authInfo->pwd, tmpLen);
        data_len += tmpLen;
    }

    /*  IMEI*/
    tmpLen = AUTH_IMEI_LEN;
    memcpy(retBuffer+data_len, authInfo->imei, tmpLen);
    data_len += tmpLen;

    if(strlen((const char *)authInfo->token) > 0)
    {
        memcpy(retBuffer+data_len, authInfo->token, AUTH_TOTKEN_LEN);
        data_len += AUTH_TOTKEN_LEN;
    }

    net_order = (int16_t)HtoN_uint16((uint16_t)(authInfo->rand));
    memcpy(retBuffer+data_len,&net_order,sizeof(net_order));
    data_len += sizeof(net_order);

    return data_len;
}

STATIC void QTlv_data_encryption_xor(uint8_t keyIndex, const uint8_t* src, uint8_t* dest, uint32_t len)
{
    uint32_t keyRewind = 0;
    uint32_t i = 0;

    for (i=0; i < len; i++)
    {
        if(keyRewind >= XOR_KEY_LEN)
            keyRewind = 0;
        dest[i] = src[i] ^ xor_key[keyIndex][keyRewind++];
    }
}

STATIC uint8_t* tlv_realloc_memory(uint8_t* buf1, uint32_t buf1Len, uint8_t* buf2, uint32_t buf2Len)
{
    uint8_t* newBuf = NULL;

    newBuf = (uint8_t*)malloc(buf1Len+buf2Len);
    if (!newBuf)
    {
        return NULL;
    }
    // 合并内存
    memcpy(newBuf, buf1, buf1Len);
    memcpy(newBuf+buf1Len, buf2, buf2Len);
    // 释放内存
    free((void *)buf1);
    free((void *)buf2);
    buf2 = NULL;
    buf1 = newBuf;

    return newBuf;
}

STATIC uint32_t QTlv_Serialize_WifiInfo_Buf(tlv_wifiinfo_struct *wifiInfo, uint8_t *retBuffer)
{
    uint32_t      data_len = 0;
    uint32_t      tmpLen = 0;
    uint16_t      net_order_16 = 0;

    tmpLen = 17;
    memcpy(retBuffer,wifiInfo->mac,tmpLen);
    data_len += tmpLen;
    tmpLen = strlen((const char*)wifiInfo->ssid);
    memcpy(retBuffer+data_len,wifiInfo->ssid,tmpLen);
    data_len += tmpLen;

    net_order_16 = HtoN_int16(wifiInfo->rssi);
    memcpy(retBuffer+data_len,&net_order_16,sizeof(net_order_16));
    data_len += sizeof(net_order_16);
    return data_len;
}

STATIC void wifi_hex_to_str(unsigned char *pbDest, unsigned char *pbSrc, int nLen)
{
    char ddl,ddh;
    int i;

    for (i=0; i<nLen; i++)
    {
        ddh = 48 + pbSrc[i] / 16;
        ddl = 48 + pbSrc[i] % 16;
        if (ddh > 57) ddh = ddh + 7;
        if (ddl > 57) ddl = ddl + 7;
        pbDest[i*2] = WIFI_LOWER_CASE(ddh);
        pbDest[i*2+1] = WIFI_LOWER_CASE(ddl);
    }
    pbDest[nLen*2] = '\0';
}

STATIC mp_obj_t wifi_locreqencode(size_t n_args, const mp_obj_t *args)
{
    uint8_t i = 0;
    uint8_t* buf_1 = NULL;
    uint8_t* buf_2 = NULL;
    int buf_len_1 = 0;
    int buf_len_2 = 0;
    int len = 0;
    int ret = 0;

    ber_tlv_tag tag;
    tag.class_tag = BER_TAG_CLASS_UNIVERSAL;
    tag.construct_tag = BER_TAG_PRIMITIVE;
    tlv_baseinfo_struct baseinfo_req;
    uint8_t basicBuf[MAX_BASIC_INFO_SIZE];

    /* base info encode*/
    baseinfo_req.serve_type = Q_SERV_TYPE_LOC;
    baseinfo_req.encript_type = Q_ENCRIPT_XOR;
    baseinfo_req.key_index = Q_LOC_KEY_INDEX;//rand() % MAX_RAW_KEY_NUM;
    baseinfo_req.pos_data_type = Q_LOC_DATA_METHOD;
    baseinfo_req.loc_method = Q_LOC_MOD_WIFI;

    len = QTlv_Serialize_BaseInfo_Buf(&(baseinfo_req), basicBuf);
    tag.tag_value = BER_LOC_BASIC_TAG;
    buf_len_1 = tlv_serialize_tlv_buffer(&tag, len, basicBuf,len, &buf_1);
    if(buf_len_1 <= 0)
    {
        QPY_MODLBS_LOG("serialize buffer Fail \r\n");
        return mp_obj_new_int(-1);
    }

    /* auth info encode*/
    QTlvAuthInfo authInfo;
    memset(&authInfo, 0, sizeof(QTlvAuthInfo));
    memcpy(authInfo.name, "quectel", strlen("quectel"));
    memcpy(authInfo.pwd, "123456", strlen("123456"));

    ret = Helios_Dev_GetIMEI((void *)authInfo.imei, AUTH_IMEI_LEN+1);
    QPY_MODLBS_LOG("wifi_locreq:  get imei [%s]\r\n", authInfo.imei);

    mp_buffer_info_t tokeninfo = {0};
    mp_get_buffer_raise(args[0], &tokeninfo, MP_BUFFER_READ);

    uint8_t digest1[16] = {0};
    md5_state_t md5_ctx1;
    memset(&md5_ctx1, 0, sizeof(md5_state_t));
    Quos_md5Init(&md5_ctx1);
    Quos_md5Append(&md5_ctx1, (uint8_t *)tokeninfo.buf, strlen((char *)tokeninfo.buf));
    Quos_md5Finish(&md5_ctx1, digest1);

    uint8_t secretinfo[64] = {0};
    wifi_hex_to_str(secretinfo, digest1, 16);
    QPY_MODLBS_LOG("MD5 secretinfo tokeninfo.buf[%s] digest1 [%s]\r\n", tokeninfo.buf, secretinfo);

    memcpy(authInfo.token, secretinfo, AUTH_TOTKEN_LEN);

    authInfo.rand = 356;
    uint8_t authBuf[MAX_AUTH_INFO_SIZE];
    len = QTlv_Serialize_AuthInfo_Buf(0, authBuf, &authInfo);
    QTlv_data_encryption_xor(baseinfo_req.key_index, authBuf, authBuf, len);
    tag.tag_value = BER_LOC_AUTH_TAG;
    buf_len_2 = tlv_serialize_tlv_buffer(&tag, len, authBuf, len, &buf_2);
    if(buf_len_2 <=0)
    {
        SAFE_FREE(buf_1);
        QPY_MODLBS_LOG("wifi_locreq: AuthInfo error\r\n");
        return mp_obj_new_int(-1);
    }
    else
    {
        /*合并内存*/
        buf_1 = tlv_realloc_memory(buf_1,buf_len_1,buf_2,buf_len_2);
        if (!buf_1){
            SAFE_FREE(buf_1);
            SAFE_FREE(buf_2);
            return mp_obj_new_int(-1);
        }
        buf_len_1 += buf_len_2;
    }

    /* wifi info encode*/
    ret = Helios_WifiScan_Support();
    if (ret != 1)
        mp_raise_ValueError("WiFi-Scan is not support.");

    uint8_t status = 0;
    Helios_WifiScan_GetStatus(&status);
    if (status != 1)
    {
        ret = Helios_WifiScan_Open();
        if (ret != 0)
            mp_raise_ValueError("WiFi-Scan open Fail.");
    }

    Helios_WifiScanAPInfoStruct info = {0};
    tlv_wifiinfo_struct tlv_wifiinfo = {0};

    MP_THREAD_GIL_EXIT();
    ret = Helios_WifiScan_SyncStart(&info);
    MP_THREAD_GIL_ENTER();

    if (ret == 0)
    {
        if (info.ap_nums == 0)
            mp_raise_ValueError("WiFi-Scan wifi Scan timeout.");

        for (i = 0; i < info.ap_nums; i++)
        {
            if (i >= 5)
                break;
            uint8_t wifiBuf[MAX_WIFI_INFO_SIZE];
            char wifimac_str[MAX_MAC_ADDR_STR_LEN] = {0};
            sprintf(wifimac_str, "%02X:%02X:%02X:%02X:%02X:%02X", \
                                info.ap[i].bssid[0],info.ap[i].bssid[1],\
                                info.ap[i].bssid[2],info.ap[i].bssid[3],\
                                info.ap[i].bssid[4],info.ap[i].bssid[5]);

            memcpy(&tlv_wifiinfo.mac, wifimac_str, strlen((const char*)wifimac_str));
            tlv_wifiinfo.rssi = info.ap[i].rssi;
            memcpy(&tlv_wifiinfo.ssid, "test", strlen((const char*)"test"));
            len = QTlv_Serialize_WifiInfo_Buf(&(tlv_wifiinfo), wifiBuf);

            QTlv_data_encryption_xor(baseinfo_req.key_index, wifiBuf, wifiBuf, len);
            tag.tag_value = BER_LOC_WIFI1_TAG+i;
            buf_len_2 = tlv_serialize_tlv_buffer(&tag, len, wifiBuf, len, &buf_2);
            if(buf_len_2 <=0)
            {
                SAFE_FREE(buf_1);
                mp_raise_ValueError("WiFi-Scan tlv_buffer Fail.");
            }
            else
            {
                buf_1 = tlv_realloc_memory(buf_1,buf_len_1,buf_2,buf_len_2);
                if (!buf_1)
                {
                    SAFE_FREE(buf_1);
                    SAFE_FREE(buf_2);
                    mp_raise_ValueError("WiFi-Scan memory Fail.");
                }
                buf_len_1 += buf_len_2;
            }
        }
    }
    else
    {
        mp_raise_ValueError("WiFi-Scan wifi Scan Fail.");
    }

    /*MD5 info encode*/
    uint8_t md5Str[256] = {0};
    uint8_t serialBuf[64];
    uint8_t pDigest[16] = {0};
    int retLen = 0;
    int offset = 0;

    memset(md5Str, 0, sizeof(md5Str));
    memset(pDigest, 0, sizeof(pDigest));

    retLen = QTlv_Serialize_BaseInfo_Buf(&(baseinfo_req), serialBuf);
    memcpy(md5Str+offset, serialBuf, retLen);
    offset += retLen;

    retLen = QTlv_Serialize_AuthInfo_Buf(1, serialBuf, &authInfo);
    memcpy(md5Str+offset, serialBuf, retLen);
    offset += retLen;

    tlv_wifiinfo_struct tlv_md5_wifiinfo = {0};
    char md5_wifimac_str[MAX_MAC_ADDR_STR_LEN] = {0};
    sprintf(md5_wifimac_str, "%02X:%02X:%02X:%02X:%02X:%02X", \
                        info.ap[0].bssid[0],info.ap[0].bssid[1],\
                        info.ap[0].bssid[2],info.ap[0].bssid[3],\
                        info.ap[0].bssid[4],info.ap[0].bssid[5]);
    memcpy(&tlv_md5_wifiinfo.mac, md5_wifimac_str, strlen((const char*)md5_wifimac_str));
    memcpy(&tlv_md5_wifiinfo.ssid, "test", strlen((const char*)"test"));
    tlv_md5_wifiinfo.rssi = info.ap[0].rssi;
    retLen = QTlv_Serialize_WifiInfo_Buf(&(tlv_md5_wifiinfo), serialBuf);
    memcpy(md5Str+offset, serialBuf, retLen);
    offset += retLen;

    uint8_t digest[16] = {0};
    md5_state_t md5_ctx;
    memset(&md5_ctx, 0, sizeof(md5_state_t));
    Quos_md5Init(&md5_ctx);
    Quos_md5Append(&md5_ctx, md5Str, offset);
    Quos_md5Finish(&md5_ctx, digest);

    uint8_t md5Buf[AUTH_MD5_LEN];
    QTlv_data_encryption_xor(baseinfo_req.key_index, digest, md5Buf, AUTH_MD5_LEN);
    tag.tag_value = BER_LOC_MD5_TAG;
    buf_len_2 = tlv_serialize_tlv_buffer(&tag, AUTH_MD5_LEN, md5Buf, AUTH_MD5_LEN, &buf_2);
    if(buf_len_2 <=0)
    {
        SAFE_FREE(buf_1);
        mp_raise_ValueError("WiFi-Scan MD5 Fail.");
    }
    else
    {
        /*合并内存*/
        buf_1 = tlv_realloc_memory(buf_1,buf_len_1,buf_2,buf_len_2);
        if (!buf_1)
        {
            SAFE_FREE(buf_1);
            SAFE_FREE(buf_2);
            mp_raise_ValueError("WiFi-Scan MD5 memory Fail.");
        }
        buf_len_1 += buf_len_2;
    }

    /*version info encode*/
    uint8_t srvListVer[MAX_SERVER_LIST_VERSION_SIZE];
    uint8_t version[15] = "2021";

    QTlv_data_encryption_xor(baseinfo_req.key_index, version, srvListVer, len);
    tag.tag_value = BER_LOC_SRV_LIST_VER_TAG;
    buf_len_2 = tlv_serialize_tlv_buffer(&tag, len, srvListVer, len, &buf_2);
    if(buf_len_2 <=0)
    {
        SAFE_FREE(buf_1);
        mp_raise_ValueError("WiFi-Scan version tlv_buffer Fail.");
    }
    else
    {
        /*合并内存*/
        buf_1 = tlv_realloc_memory(buf_1,buf_len_1,buf_2,buf_len_2);
        if (!buf_1){
            SAFE_FREE(buf_1);
            SAFE_FREE(buf_2);
            mp_raise_ValueError("WiFi-Scan version memory Fail.");
        }
        buf_len_1 += buf_len_2;
    }

    ///*序列化整个Auth信息*/
    retLen = 0;
    uint8_t* retBuffer = NULL;
    tag.tag_value = BER_LOC_TAG;
    tag.construct_tag = BER_TAG_CONSTRUCTED;
    retLen = tlv_serialize_tlv_buffer(&tag, buf_len_1, buf_1, buf_len_1, &retBuffer);

    SAFE_FREE(buf_1);
    if(retLen <= 0)
    {
        mp_raise_ValueError("WiFi-Scan TLV data Fail.");
    }

    mp_obj_t codeinfo_objs[2] = {
                mp_obj_new_int(retLen),
                mp_obj_new_str((char *)retBuffer, retLen),
            };

    return mp_obj_new_tuple(2, codeinfo_objs);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_lbs_get_wifi_locreqencode_obj, 1, 6, wifi_locreqencode);


STATIC QTLVNode* QTLVNode_AddBrother(QTLVNode* curNode, QTLVNode* newNode)
{
    curNode->next = newNode;
    return curNode->next;
}

STATIC QTLVNode* QTLVNode_AddChild(QTLVNode* curNode, QTLVNode* newNode)
{
    curNode->child= newNode;
    return curNode->child;
}

STATIC int32_t ber_fetch_tag(const uint8_t *ptr, int32_t size, ber_tlv_tag* rettag)
{
    int32_t skipped = 0;
    uint32_t val = *(const uint8_t *)ptr;

    if(size == 0)
        return (0);

    rettag->class_tag = (ber_class_tag)(val >> 6); // 获取类别
    rettag->construct_tag = (ber_construct_tag)((val >>5 ) & 0x01); // 获得结构类型

    if((val &= BER_EXTENSION_ID) != BER_EXTENSION_ID)
    {
        rettag->tag_value = (val & BER_EXTENSION_ID);
        return (1);
    }

    val = 0;
    for(ptr = ((const uint8_t *)ptr) + 1, skipped = 2; skipped <= size; ptr = ((const uint8_t *)ptr) + 1, skipped++) 
    {
        uint32_t oct = *(const uint8_t *)ptr;
        if(oct & BER_MORE_TAG)
        {
            val = (val << 7) | (oct & 0x7F);
            if(val >> ((8 * sizeof(val)) - 9))
            {
                return (-1);
            }
        }
        else
        {
            val = (val << 7) | oct;
            rettag->tag_value = val;
            return (skipped);
        }
    }
    return 0;
}

STATIC int32_t ber_fetch_length(const uint8_t *bufptr, int32_t size, uint32_t *len_r)
{
    const uint8_t *buf = (const uint8_t *)bufptr;
    int32_t oct = 0;

    if(size == 0)
        return 0;

    oct = *(const uint8_t *)buf;
    if((oct & BER_LONG_LEN) == 0)
    {
        *len_r = oct;
        return 1;
    }
    else
    {
        int32_t len = 0;
        int32_t skipped = 0;

        if(oct == 0xff)
        {
            return -1;
        }

        oct &= 0x7F;
        for(len = 0, buf++, skipped = 1; oct && (++skipped <= size); buf++, oct--)
        {
            len = (len << 8) | *buf;
            if(len < 0 || (len > MAX_SIGLE_TLV_LEN))
            {
                return -1;
            }
        }

        if(oct == 0)
        {
            *len_r = len;
            return skipped;
        }
        return 0;	/* Want more */
    }
}

STATIC uint16_t NtoH_uint16(uint16_t n)
{
    return checkCPUendian() ? n : BigLittleSwap16(n);
}

STATIC uint32_t NtoH_uint32(uint32_t n)
{
    /* 若本机为大端，与网络字节序同，直接返回；若本机为小端，网络数据转换成小端再返回*/
    return checkCPUendian() ? n : BigLittleSwap32(n);
}

STATIC float NtoH_float(float n)
{
    uint32_t host_uint32 = NtoH_uint32(*((uint32_t*)&n));
    return *((float*)&host_uint32);
}

STATIC int32_t tlv_parse_to_tree(const uint8_t* buffer, uint32_t bufSize, QTLVNode** root)
{
    QTLVNode* insertNode = *root;
    QTLVNode* ktlvNode = NULL;
    uint32_t currentIndex = 0;
    uint8_t currentReadTag = 'T';
    int32_t bFirst = 1;
    int32_t isConstruected = 0;
    ber_tlv_tag tag;
    int32_t tagused  = 0;
    int32_t lenused = 0;
    int32_t objectUsed = 0;
    QTLVNode* ktlvChildNode = NULL;
    tlv_baseinfo_struct baseinfo_rec;
    uint8_t decodeLen = 0;

    memset(&tag, 0, sizeof(ber_tlv_tag));
    memset(&baseinfo_rec, 0, sizeof(tlv_baseinfo_struct));
    while(currentIndex < bufSize)
    {
        isConstruected = 0;
        ktlvNode = NULL;
        if (!bFirst)
            ktlvNode = QTLVNode_Create();
        else
            ktlvNode = insertNode;

        do 
        {
            if (currentReadTag == 'T')
            {
                tagused = ber_fetch_tag(buffer+currentIndex,bufSize-currentIndex,&(tag));
                if (0 >= tagused)
                {
                    mp_raise_ValueError("WiFi-Scan TLV ber_fetch_tag Fail.");
                }
                else
                {
                    ktlvNode->tag.tag_value = tag.tag_value;
                    ktlvNode->tag.class_tag = tag.class_tag;
                    isConstruected = ktlvNode->tag.construct_tag = tag.construct_tag;
                    currentIndex += tagused;
                    currentReadTag = 'L';
                }
            }

            if (currentReadTag == 'L')
            {
                lenused = ber_fetch_length(buffer+currentIndex, bufSize-currentIndex, &(ktlvNode->length));
                if (0 >= lenused)
                {
                    mp_raise_ValueError("WiFi-Scan TLV ber_fetch_length Fail.");
                }
                else
                {
                    currentIndex += lenused;
                    currentReadTag = 'V';
                }
            }

            if (currentReadTag == 'V')
            {
                if (!isConstruected)
                {
                    ktlvNode->value= (uint8_t*)malloc(ktlvNode->length);
                    if (!ktlvNode->value)
                    {
                        mp_raise_ValueError("WiFi-Scan TLV malloc Fail.");
                    }

                    memcpy(ktlvNode->value, buffer+currentIndex, ktlvNode->length);
                    QPY_MODLBS_LOG("get ktlvNode->tag.tag_value [%d]\r\n", ktlvNode->tag.tag_value);
                    if (ktlvNode->tag.tag_value == BER_POS_STAT_TAG)
                    {
                        QTlvRetPosStat start =  *(QTlvRetPosStat*)ktlvNode->value;
                        QPY_MODLBS_LOG("********BER_POS_STAT_TAG ktlvNode->value :******** [%d]\r\n", start);
                        if (start == 0)
                        {
                            mp_raise_ValueError("WiFi-Scan TLV server status Fail.");
                        }
                        else if (start >= POS_POS_NUM_10)
                        {
                            char ret_buf[60];
                            sprintf(ret_buf, "WiFi-Scan token %d Fail.",start);
                            mp_raise_ValueError(ret_buf);
                        }
                    }
                    else if (ktlvNode->tag.tag_value == BER_POS_BASIC_TAG)
                    {
                        baseinfo_rec.serve_type = *ktlvNode->value;
                        baseinfo_rec.encript_type = *(ktlvNode->value+1);
                        baseinfo_rec.key_index = *(ktlvNode->value+2);
                        baseinfo_rec.pos_data_type = *(ktlvNode->value+3);
                        baseinfo_rec.loc_method = *(ktlvNode->value+4);
                        //QPY_MODLBS_LOG("[xjin]:********BER_POS_BASIC_TAG  [%d][%d][%d][%d] [%d]\r\n", baseinfo_rec.serve_type,
                        //               baseinfo_rec.encript_type,baseinfo_rec.key_index,baseinfo_rec.pos_data_type,baseinfo_rec.loc_method);
                    }
                    else if (ktlvNode->tag.tag_value == BER_POS_RAND_TAG)
                    {
                        uint16_t rec_rand = 0;
                        QTlv_data_encryption_xor(baseinfo_rec.key_index, ktlvNode->value, ktlvNode->value, ktlvNode->length);
                        rec_rand = NtoH_uint16(*((uint16_t*)ktlvNode->value));
                        QPY_MODLBS_LOG("********BER_POS_RAND_TAG rand :******** [%d]\r\n", rec_rand);
                    }
                    else if (ktlvNode->tag.tag_value == BER_POS_MD5_TAG)
                    {
                        //QTlv_data_encryption_xor(baseinfo_rec.key_index, ktlvNode->value, ktlvNode->value, ktlvNode->length);
                    }
                    else if (ktlvNode->tag.tag_value == BER_POS_POS1_TAG)
                    {
                        decodeLen = 0;
                        QTlv_data_encryption_xor(baseinfo_rec.key_index, ktlvNode->value, ktlvNode->value, ktlvNode->length);
                        position_rec.longitude = NtoH_float(*(float*)(ktlvNode->value + decodeLen));
                        decodeLen += sizeof(float);
                        position_rec.latitude  = NtoH_float(*(float*)(ktlvNode->value + decodeLen));
                        decodeLen += sizeof(float);
                        position_rec.accuracy  = NtoH_uint16(*(uint16_t*)(ktlvNode->value + decodeLen));
                        decodeLen += sizeof(uint16_t);
                        //QPY_MODLBS_LOG("position.longitude [%f] latitude [%f]\r\n", position_rec.longitude, position_rec.latitude);
                        //QPY_MODLBS_LOG("position.accuracy [%d]\r\n", position_rec.accuracy);
                        return 0;
                    }

                    if (!bFirst)
                    {
                        insertNode = insertNode->AddBrother(insertNode, ktlvNode);
                    }
                    currentIndex += ktlvNode->length;
                    currentReadTag = 'T';
                    bFirst = 0;
                }
                else
                {
                    if (!bFirst)
                    {
                        insertNode = insertNode->AddBrother(insertNode, ktlvNode);
                    }
                    ktlvChildNode = QTLVNode_Create();
                    insertNode->AddChild(insertNode, ktlvChildNode);
                    objectUsed = tlv_parse_to_tree(buffer+currentIndex,ktlvNode->length,&insertNode->child);
                    currentIndex += objectUsed;
                    currentReadTag = 'T';
                    bFirst = 0;
                }
            }
        } while (0);
    }

    return -1;
}


STATIC QTLVNode* QTLVNode_Create(void)
{
    QTLVNode *curNode = (QTLVNode*)malloc(sizeof(QTLVNode));
    if(curNode == NULL)
    {
        mp_raise_ValueError("WiFi-Scan malloc Fail.");
    }
    memset(curNode, 0, sizeof(QTLVNode));
    curNode->AddBrother = QTLVNode_AddBrother;
    curNode->AddChild = QTLVNode_AddChild;
    return curNode;
}


STATIC mp_obj_t encode_wifi_locreq_tlvinfo(size_t n_args, const mp_obj_t *args)
{
    QTLVNode* firstNode = NULL;
    int32_t retValue = 0;
    uint8_t buffer[512];

    firstNode = QTLVNode_Create();
    mp_buffer_info_t datainfo = {0};
    mp_get_buffer_raise(args[0], &datainfo, MP_BUFFER_READ);
    memcpy(buffer, datainfo.buf, datainfo.len);

    retValue = tlv_parse_to_tree(buffer, (uint32_t)strlen((char *)buffer), &firstNode);
    if (retValue == 0)
    {
        mp_obj_t codeinfo_objs[3] = {
                mp_obj_new_float(position_rec.longitude),
                mp_obj_new_float(position_rec.latitude),
                mp_obj_new_int(position_rec.accuracy),
                };
        return mp_obj_new_tuple(3, codeinfo_objs);
    }
    else
    {
        return mp_obj_new_int(-1);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_lbs_encode_wifi_tlvinfo_obj, 1, 6, encode_wifi_locreq_tlvinfo);

STATIC const mp_rom_map_elem_t mp_module_wifilocator_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR___wifiLocator) },
    { MP_ROM_QSTR(MP_QSTR_getWifilocreq), MP_ROM_PTR(&qpy_lbs_get_wifi_locreqencode_obj) },
    { MP_ROM_QSTR(MP_QSTR_encodeWifilocreq), MP_ROM_PTR(&qpy_lbs_encode_wifi_tlvinfo_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_wifilocator_globals, mp_module_wifilocator_globals_table);

const mp_obj_module_t mp_module_wifilocator = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_wifilocator_globals,
};


