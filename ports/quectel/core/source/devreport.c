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
 
#include "py/runtime.h"
#if MICROPY_ENABLE_DEVICE_REPORT

#include "helios.h"
#include "helios_debug.h"
#include "helios_os.h"
#include "helios_dev.h"
#include "helios_nw.h"
#include "helios_sim.h"
#if !defined (PLAT_Qualcomm) 
#include "sockets.h"
#include "netif.h"
#include "netdb.h"
#include "inet.h"
#endif
#include "helios_datacall.h"

#include <string.h>

#define DEVREPORT_STACK_SIZE 3
#define app_debug(fmt, ...) custom_log(DEVREPORT, fmt, ##__VA_ARGS__)


#define STATION_CODE_CN                     "460"                   // 国内识别码

#define DEVICE_INFO_REPORT_SERVER_CN       "iot-south.quectel.com"  // 平台地址
#define DEVICE_INFO_REPORT_PORT           6789                      // 平台端口号 

#define DEVICE_REPORT_TRY_CNT  3       // 上报重试次数
#define DEVICE_MSG_HEAD    0x8001      // 上报数据定义头 

// 失败次数计数
static int g_failure_cnt = 0;
// 轮询等待时间，单位秒
static int g_loop_sleep_sec = 5;


typedef enum {
	SSS_DEVICE_IMSI = 0,         //  读取imsi状态
	SSS_DEVICE_IMEI,             //  读取imei状态
	SSS_DEVICE_NETWORK_CHECK,    //  检查网络状态
	SSS_DEVICE_REPORT,           //  设备信息上报 
	SSS_DEVICE_STATION_END       //  状态结束
} REPORT_STATUS;

struct device_config {
	char imei[32];
	char imsi[32];
	int profile_idx;
	REPORT_STATUS status;
};
struct device_config devConfig;

#pragma pack(1)
struct report_msg {
	unsigned short head;
	unsigned short len;
	unsigned char value[32];
};
#pragma pack()


#define DR_HASH256_ENABLE 1
#if DR_HASH256_ENABLE
typedef struct {
	uint8_t  buf[64];
	uint32_t hash[8];
	uint32_t bits[2];
	uint32_t len;
} dr_sha256_context;


#define FN_ inline static

static const uint32_t K[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#ifdef MINIMIZE_STACK_IMPACT
static uint32_t W[64];
#endif

/* -------------------------------------------------------------------------- */
FN_ uint8_t _shb(uint32_t x, uint32_t n)
{
	return ( (x >> (n & 31)) & 0xff );
} /* _shb */

/* -------------------------------------------------------------------------- */
FN_ uint32_t _shw(uint32_t x, uint32_t n)
{
	return ( (x << (n & 31)) & 0xffffffff );
} /* _shw */

/* -------------------------------------------------------------------------- */
FN_ uint32_t _r(uint32_t x, uint8_t n)
{
	return ( (x >> n) | _shw(x, 32 - n) );
} /* _r */

/* -------------------------------------------------------------------------- */
FN_ uint32_t _Ch(uint32_t x, uint32_t y, uint32_t z)
{
	return ( (x & y) ^ ((~x) & z) );
} /* _Ch */

/* -------------------------------------------------------------------------- */
FN_ uint32_t _Ma(uint32_t x, uint32_t y, uint32_t z)
{
	return ( (x & y) ^ (x & z) ^ (y & z) );
} /* _Ma */

/* -------------------------------------------------------------------------- */
FN_ uint32_t _S0(uint32_t x)
{
	return ( _r(x, 2) ^ _r(x, 13) ^ _r(x, 22) );
} /* _S0 */

/* -------------------------------------------------------------------------- */
FN_ uint32_t _S1(uint32_t x)
{
	return ( _r(x, 6) ^ _r(x, 11) ^ _r(x, 25) );
} /* _S1 */

/* -------------------------------------------------------------------------- */
FN_ uint32_t _G0(uint32_t x)
{
	return ( _r(x, 7) ^ _r(x, 18) ^ (x >> 3) );
} /* _G0 */

/* -------------------------------------------------------------------------- */
FN_ uint32_t _G1(uint32_t x)
{
	return ( _r(x, 17) ^ _r(x, 19) ^ (x >> 10) );
} /* _G1 */

/* -------------------------------------------------------------------------- */
FN_ uint32_t _word(uint8_t *c)
{
	return ( _shw(c[0], 24) | _shw(c[1], 16) | _shw(c[2], 8) | (c[3]) );
} /* _word */

/* -------------------------------------------------------------------------- */
FN_ void  _addbits(dr_sha256_context *ctx, uint32_t n)
{
	if ( ctx->bits[0] > (0xffffffff - n) )
		ctx->bits[1] = (ctx->bits[1] + 1) & 0xFFFFFFFF;
	ctx->bits[0] = (ctx->bits[0] + n) & 0xFFFFFFFF;
} /* _addbits */

/* -------------------------------------------------------------------------- */
static void _hash(dr_sha256_context *ctx)
{
	register uint32_t a, b, c, d, e, f, g, h, i;
	uint32_t t[2];
#ifndef MINIMIZE_STACK_IMPACT
	uint32_t W[64];
#endif

	a = ctx->hash[0];
	b = ctx->hash[1];
	c = ctx->hash[2];
	d = ctx->hash[3];
	e = ctx->hash[4];
	f = ctx->hash[5];
	g = ctx->hash[6];
	h = ctx->hash[7];

	for (i = 0; i < 64; i++) {
		if ( i < 16 )
			W[i] = _word(&ctx->buf[_shw(i, 2)]);
		else
			W[i] = _G1(W[i - 2]) + W[i - 7] + _G0(W[i - 15]) + W[i - 16];

		t[0] = h + _S1(e) + _Ch(e, f, g) + K[i] + W[i];
		t[1] = _S0(a) + _Ma(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + t[0];
		d = c;
		c = b;
		b = a;
		a = t[0] + t[1];
	}

	ctx->hash[0] += a;
	ctx->hash[1] += b;
	ctx->hash[2] += c;
	ctx->hash[3] += d;
	ctx->hash[4] += e;
	ctx->hash[5] += f;
	ctx->hash[6] += g;
	ctx->hash[7] += h;
} /* _hash */

/* -------------------------------------------------------------------------- */
static void dr_sha256_init(dr_sha256_context *ctx)
{
	if ( ctx != NULL ) {
		ctx->bits[0]  = ctx->bits[1] = 0;
		ctx->len      = 0;
		ctx->hash[0] = 0x6a09e667;
		ctx->hash[1] = 0xbb67ae85;
		ctx->hash[2] = 0x3c6ef372;
		ctx->hash[3] = 0xa54ff53a;
		ctx->hash[4] = 0x510e527f;
		ctx->hash[5] = 0x9b05688c;
		ctx->hash[6] = 0x1f83d9ab;
		ctx->hash[7] = 0x5be0cd19;
	}
} /* sha256_init */

/* -------------------------------------------------------------------------- */
static void dr_sha256_hash(dr_sha256_context *ctx, const void *data, size_t len)
{
	register size_t i;
	const uint8_t *bytes = (const uint8_t *)data;

	if ( (ctx != NULL) && (bytes != NULL) )
		for (i = 0; i < len; i++) {
			ctx->buf[ctx->len] = bytes[i];
			ctx->len++;
			if (ctx->len == sizeof(ctx->buf) ) {
				_hash(ctx);
				_addbits(ctx, sizeof(ctx->buf) * 8);
				ctx->len = 0;
			}
		}
} /* sha256_hash */

/* -------------------------------------------------------------------------- */
static void dr_sha256_done(dr_sha256_context *ctx, uint8_t *hash)
{
	register uint32_t i, j;

	if ( ctx != NULL ) {
		j = ctx->len % sizeof(ctx->buf);
		ctx->buf[j] = 0x80;
		for (i = j + 1; i < sizeof(ctx->buf); i++)
			ctx->buf[i] = 0x00;

		if ( ctx->len > 55 ) {
			_hash(ctx);
			for (j = 0; j < sizeof(ctx->buf); j++)
				ctx->buf[j] = 0x00;
		}

		_addbits(ctx, ctx->len * 8);
		ctx->buf[63] = _shb(ctx->bits[0],  0);
		ctx->buf[62] = _shb(ctx->bits[0],  8);
		ctx->buf[61] = _shb(ctx->bits[0], 16);
		ctx->buf[60] = _shb(ctx->bits[0], 24);
		ctx->buf[59] = _shb(ctx->bits[1],  0);
		ctx->buf[58] = _shb(ctx->bits[1],  8);
		ctx->buf[57] = _shb(ctx->bits[1], 16);
		ctx->buf[56] = _shb(ctx->bits[1], 24);
		_hash(ctx);

		if ( hash != NULL )
			for (i = 0, j = 24; i < 4; i++, j -= 8) {
				hash[i     ] = _shb(ctx->hash[0], j);
				hash[i +  4] = _shb(ctx->hash[1], j);
				hash[i +  8] = _shb(ctx->hash[2], j);
				hash[i + 12] = _shb(ctx->hash[3], j);
				hash[i + 16] = _shb(ctx->hash[4], j);
				hash[i + 20] = _shb(ctx->hash[5], j);
				hash[i + 24] = _shb(ctx->hash[6], j);
				hash[i + 28] = _shb(ctx->hash[7], j);
			}
	}
} /* sha256_done */
// sha256算法实现
static void dr_sha256(const void *data, size_t len, uint8_t *hash)
{
	dr_sha256_context ctx;

	dr_sha256_init(&ctx);
	dr_sha256_hash(&ctx, data, len);
	dr_sha256_done(&ctx, hash);
} /* sha256 */
#endif
// 建立udp连接，上报数据
static int dr_udp_client_send(int cid, const char *host, unsigned char *sendData, int sendLength)
{
    int ret = 0;
	struct netif *netif_cfg = NULL;
	struct sockaddr_in server_addr;
	struct addrinfo hints;
	struct sockaddr_in * sin_pres;
	struct addrinfo *dev_pres = NULL;
	int sockfd;
	char recvUdpData[128];
	
	socklen_t len;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
	// 域名解析
	ret = getaddrinfo_with_pcid(host, NULL, &hints, &dev_pres, cid);
	if ( ret != 0 || dev_pres == NULL ) {
			return -1;
	}
#if !defined (PLAT_Qualcomm)
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
#endif
	if(sockfd < 0) {
		 freeaddrinfo(dev_pres);
         return -1;
    }
	struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
	// 设置接收超时，3s
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#if defined(PLAT_RDA)
        int pdp = 1;
        netif_cfg = netif_get_by_cid(pdp);
#else
        int pdp = Helios_DataCall_GetCurrentPDP();
#if defined (PLAT_Unisoc)
        netif_cfg = netif_get_by_cid(pdp);
#elif defined (PLAT_ASR)
        netif_cfg = netif_get_by_cid(pdp - 1);
#elif defined (PLAT_Qualcomm)
		netif_cfg = NULL;
		(void)pdp;
#endif
#endif
#if !defined (PLAT_Qualcomm)
	struct sockaddr_in local4;
    if(netif_cfg) {
 	    local4.sin_family = AF_INET;
 		local4.sin_port = DEVICE_INFO_REPORT_PORT;
#if defined (PLAT_Unisoc) || defined(PLAT_RDA)
        local4.sin_addr.s_addr = netif_cfg->ip_addr.u_addr.ip4.addr;
#elif defined (PLAT_ASR)
        local4.sin_addr.s_addr = netif_cfg->ip_addr.addr;
#endif
 	    if(bind(sockfd,(struct sockaddr *)&local4,sizeof(struct sockaddr))) {
		   freeaddrinfo(dev_pres);
 			return -1;
 	    }
    }
#endif
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEVICE_INFO_REPORT_PORT);
    sin_pres = (struct sockaddr_in *)dev_pres->ai_addr;
    server_addr.sin_addr = sin_pres->sin_addr;
    len = sizeof(server_addr);
	ret = sendto(sockfd, sendData, sendLength, 0, (struct sockaddr *)(&server_addr), len);
    if(ret < 0) {
     	close(sockfd);
		freeaddrinfo(dev_pres);
     	return -1;
    }
    ret = recvfrom(sockfd, recvUdpData, 100, 0, (struct sockaddr *)&server_addr, &len);
	if( ret != 5 || recvUdpData[4] != 0x30 ) {
		close(sockfd);
		freeaddrinfo(dev_pres);
		return -1;
	}
	close(sockfd);
	freeaddrinfo(dev_pres);
    return 0;
}
// 大端对齐转换为小端对齐
static short _big2littles(short be)
{
	short swapped = (0xff00 & be << 8) | (0x00ff & be >> 8);
    return swapped;
}
// 上报设备信息
static void dr_report_dev_info(int profile_idx)
{
	struct report_msg msg;
	msg.head = DEVICE_MSG_HEAD;
	int data_length = 0;
	int i = 0;
	//char sendhex[256] = {0};
	int ret = 0;
#if DR_HASH256_ENABLE
	unsigned char imei_hash[32] = {0};
	dr_sha256(devConfig.imei, strlen(devConfig.imei), imei_hash);
	data_length = 32;
	msg.len = data_length;
	memcpy(msg.value, imei_hash, msg.len);
#else
	data_length = strlen(devConfig.imei);
	msg.len = data_length;
	memcpy(msg.value, devConfig.imei, data_length);
#endif
	msg.head = _big2littles(msg.head);
	msg.len = _big2littles(msg.len);
	for( i = 0; i < DEVICE_REPORT_TRY_CNT; i++ ) {
		ret = dr_udp_client_send(profile_idx, DEVICE_INFO_REPORT_SERVER_CN, (unsigned char *)&msg, data_length+4);
		if( ret == 0 ) {
			break;
		}
		Helios_sleep(1);
	}
	devConfig.status = SSS_DEVICE_STATION_END;
}
// 读取设备imsi
static void dr_read_device_imsi()
{
	int ret = 0;
	int i = 0;
	for( i = 0; i < 2; i++ ) {
		ret = Helios_SIM_GetIMSI(i, devConfig.imsi, sizeof(devConfig.imsi)-1);
#if !defined (PLAT_Qualcomm) 
		if( ret == 0 ) {
#else
		if( ret == 0 && strncmp(devConfig.imsi, "NULL", 4)) {
#endif
			if( !strncmp(devConfig.imsi, STATION_CODE_CN, 3) ) {
				devConfig.status = SSS_DEVICE_IMEI;
			} else {
				devConfig.status = SSS_DEVICE_STATION_END;
			}
			g_failure_cnt = 0;
			g_loop_sleep_sec = 5;
			return;
		}
	}
	g_failure_cnt += 1;
	if( g_failure_cnt == 6 ) g_loop_sleep_sec = 30;
	Helios_sleep(g_loop_sleep_sec);
}
// 读取设备imei
static void dr_read_device_imei()
{
	int ret = 0;
	ret = Helios_Dev_GetIMEI(devConfig.imei, sizeof(devConfig.imei)-1);
	if( ret != 0 ) {
		devConfig.status = SSS_DEVICE_STATION_END;
		return;
	}
	devConfig.status = SSS_DEVICE_NETWORK_CHECK;
}
// 检查网络连接
static void dr_network_check()
{
	int i;
	int ret = 0;
	Helios_DataCallInfoStruct info;
	memset(&info, 0x00, sizeof(info));
	for(i = (int)HELIOS_PROFILE_IDX_MIN; i < (int)HELIOS_PROFILE_IDX_MAX + 1; i++) {
		ret = Helios_DataCall_GetInfo(i, 0, &info);
		if( ret == 0 ) {
			if( info.v4.state == 1 && info.v4.addr.ip.s_addr != 0 ) {
				devConfig.profile_idx = info.profile_idx;
				devConfig.status = SSS_DEVICE_REPORT;
				return;
			}
		}
	}
	g_failure_cnt += 1;
	if( g_failure_cnt == 6 ) g_loop_sleep_sec = 30;
	Helios_sleep(g_loop_sleep_sec);
}

/*
* 设备信息上报状态机
* 读取imsi-> 读取imei -> 检查网络状态 -> 设备信息上报 -> 结束
*/
static void devReport(void *argv)
{
	(void)argv;
	Helios_sleep(15);
	memset(&devConfig, 0x00, sizeof(devConfig));
	do {
		switch( devConfig.status ) {
			case SSS_DEVICE_IMSI:
				dr_read_device_imsi();
				break;
			case SSS_DEVICE_IMEI:
				dr_read_device_imei();
				break;
			case SSS_DEVICE_NETWORK_CHECK:
				dr_network_check();
				break;
			case SSS_DEVICE_REPORT:
				dr_report_dev_info(devConfig.profile_idx);
				break;
			case SSS_DEVICE_STATION_END:
			default:
				Helios_Thread_Exit();
		}
	} while(1);
	Helios_Thread_Exit();
}

application_init(devReport, "devReport", DEVREPORT_STACK_SIZE, 0);
#endif
