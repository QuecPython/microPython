/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Paul Sokolovsky
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

#include <assert.h>
#include <string.h>

#include "py/runtime.h"

#if MICROPY_PY_UHASHLIB

#if MICROPY_SSL_MBEDTLS
#include "mbedtls/version.h"
#endif

#if MICROPY_PY_UHASHLIB_SHA256

#if MICROPY_SSL_MBEDTLS
#include "mbedtls/sha256.h"
#else
#include "crypto-algorithms/sha256.h"
#endif

#endif

#if MICROPY_PY_UHASHLIB_SHA1 || MICROPY_PY_UHASHLIB_MD5

#if MICROPY_SSL_AXTLS
#include "lib/axtls/crypto/crypto.h"
#endif

#if MICROPY_SSL_MBEDTLS
#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#endif

#include "modhash.h"
#include "modaes.h"

#endif

typedef struct _mp_obj_hash_t {
    mp_obj_base_t base;
    char state[0];
} mp_obj_hash_t;

#if MICROPY_PY_UHASHLIB_SHA256
STATIC mp_obj_t uhashlib_sha256_update(mp_obj_t self_in, mp_obj_t arg);

#if MICROPY_SSL_MBEDTLS

#if MBEDTLS_VERSION_NUMBER < 0x02070000
#define mbedtls_sha256_starts_ret mbedtls_sha256_starts
#define mbedtls_sha256_update_ret mbedtls_sha256_update
#define mbedtls_sha256_finish_ret mbedtls_sha256_finish
#endif

STATIC mp_obj_t uhashlib_sha256_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_obj_hash_t *o = m_new_obj_var(mp_obj_hash_t, char, sizeof(mbedtls_sha256_context));
    o->base.type = type;
    mbedtls_sha256_init((mbedtls_sha256_context *)&o->state);
    mbedtls_sha256_starts_ret((mbedtls_sha256_context *)&o->state, 0);
    if (n_args == 1) {
        uhashlib_sha256_update(MP_OBJ_FROM_PTR(o), args[0]);
    }
    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t uhashlib_sha256_update(mp_obj_t self_in, mp_obj_t arg) {
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(arg, &bufinfo, MP_BUFFER_READ);
    mbedtls_sha256_update_ret((mbedtls_sha256_context *)&self->state, bufinfo.buf, bufinfo.len);
    return mp_const_none;
}

STATIC mp_obj_t uhashlib_sha256_digest(mp_obj_t self_in) {
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    vstr_t vstr;
    vstr_init_len(&vstr, 32);
    mbedtls_sha256_finish_ret((mbedtls_sha256_context *)&self->state, (unsigned char *)vstr.buf);
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}

#else

#include "crypto-algorithms/sha256.c"

STATIC mp_obj_t uhashlib_sha256_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_obj_hash_t *o = m_new_obj_var(mp_obj_hash_t, char, sizeof(CRYAL_SHA256_CTX));
    o->base.type = type;
    sha256_init((CRYAL_SHA256_CTX *)o->state);
    if (n_args == 1) {
        uhashlib_sha256_update(MP_OBJ_FROM_PTR(o), args[0]);
    }
    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t uhashlib_sha256_update(mp_obj_t self_in, mp_obj_t arg) {
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(arg, &bufinfo, MP_BUFFER_READ);
    sha256_update((CRYAL_SHA256_CTX *)self->state, bufinfo.buf, bufinfo.len);
    return mp_const_none;
}

STATIC mp_obj_t uhashlib_sha256_digest(mp_obj_t self_in) {
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    vstr_t vstr;
    vstr_init_len(&vstr, SHA256_BLOCK_SIZE);
    sha256_final((CRYAL_SHA256_CTX *)self->state, (byte *)vstr.buf);
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
#endif

STATIC MP_DEFINE_CONST_FUN_OBJ_2(uhashlib_sha256_update_obj, uhashlib_sha256_update);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uhashlib_sha256_digest_obj, uhashlib_sha256_digest);

STATIC const mp_rom_map_elem_t uhashlib_sha256_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&uhashlib_sha256_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_digest), MP_ROM_PTR(&uhashlib_sha256_digest_obj) },
};

STATIC MP_DEFINE_CONST_DICT(uhashlib_sha256_locals_dict, uhashlib_sha256_locals_dict_table);

STATIC const mp_obj_type_t uhashlib_sha256_type = {
    { &mp_type_type },
    .name = MP_QSTR_sha256,
    .make_new = uhashlib_sha256_make_new,
    .locals_dict = (void *)&uhashlib_sha256_locals_dict,
};
#endif

#if MICROPY_PY_UHASHLIB_SHA1
STATIC mp_obj_t uhashlib_sha1_update(mp_obj_t self_in, mp_obj_t arg);

#if MICROPY_SSL_AXTLS
STATIC mp_obj_t uhashlib_sha1_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_obj_hash_t *o = m_new_obj_var(mp_obj_hash_t, char, sizeof(SHA1_CTX));
    o->base.type = type;
    SHA1_Init((SHA1_CTX *)o->state);
    if (n_args == 1) {
        uhashlib_sha1_update(MP_OBJ_FROM_PTR(o), args[0]);
    }
    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t uhashlib_sha1_update(mp_obj_t self_in, mp_obj_t arg) {
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(arg, &bufinfo, MP_BUFFER_READ);
    SHA1_Update((SHA1_CTX *)self->state, bufinfo.buf, bufinfo.len);
    return mp_const_none;
}

STATIC mp_obj_t uhashlib_sha1_digest(mp_obj_t self_in) {
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    vstr_t vstr;
    vstr_init_len(&vstr, SHA1_SIZE);
    SHA1_Final((byte *)vstr.buf, (SHA1_CTX *)self->state);
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
#endif

#if MICROPY_SSL_MBEDTLS

#if MBEDTLS_VERSION_NUMBER < 0x02070000
#define mbedtls_sha1_starts_ret mbedtls_sha1_starts
#define mbedtls_sha1_update_ret mbedtls_sha1_update
#define mbedtls_sha1_finish_ret mbedtls_sha1_finish
#endif

STATIC mp_obj_t uhashlib_sha1_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_obj_hash_t *o = m_new_obj_var(mp_obj_hash_t, char, sizeof(mbedtls_sha1_context));
    o->base.type = type;
    mbedtls_sha1_init((mbedtls_sha1_context *)o->state);
    mbedtls_sha1_starts_ret((mbedtls_sha1_context *)o->state);
    if (n_args == 1) {
        uhashlib_sha1_update(MP_OBJ_FROM_PTR(o), args[0]);
    }
    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t uhashlib_sha1_update(mp_obj_t self_in, mp_obj_t arg) {
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(arg, &bufinfo, MP_BUFFER_READ);
    mbedtls_sha1_update_ret((mbedtls_sha1_context *)self->state, bufinfo.buf, bufinfo.len);
    return mp_const_none;
}

STATIC mp_obj_t uhashlib_sha1_digest(mp_obj_t self_in) {
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    vstr_t vstr;
    vstr_init_len(&vstr, 20);
    mbedtls_sha1_finish_ret((mbedtls_sha1_context *)self->state, (byte *)vstr.buf);
    mbedtls_sha1_free((mbedtls_sha1_context *)self->state);
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
#endif

STATIC MP_DEFINE_CONST_FUN_OBJ_2(uhashlib_sha1_update_obj, uhashlib_sha1_update);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uhashlib_sha1_digest_obj, uhashlib_sha1_digest);

STATIC const mp_rom_map_elem_t uhashlib_sha1_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&uhashlib_sha1_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_digest), MP_ROM_PTR(&uhashlib_sha1_digest_obj) },
};
STATIC MP_DEFINE_CONST_DICT(uhashlib_sha1_locals_dict, uhashlib_sha1_locals_dict_table);

STATIC const mp_obj_type_t uhashlib_sha1_type = {
    { &mp_type_type },
    .name = MP_QSTR_sha1,
    .make_new = uhashlib_sha1_make_new,
    .locals_dict = (void *)&uhashlib_sha1_locals_dict,
};

int str2dec(const uint8_t *src, int srclen, uint8_t *dst)
{
	int i = 0, j = 0;
	int dstlen = 0;
	uint8_t temp[250] = {0};
	uint8_t dec_buf[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	uint8_t ascii_buf[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	// unsigned char temp[150] = {0};
	// unsigned char dec_buf[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	// unsigned char ascii_buf[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	for (i=0; i<srclen; i++)
	{
		for (j=0; j<16; j++)
		{
			if (src[i] == ascii_buf[j])
			{
				temp[i] = dec_buf[j];
			}
		}
	}

	if (srclen % 2 == 0)
	{
		dstlen =  srclen / 2;
		for (i=0; i<dstlen; i++)
		{
			dst[i] = temp[i*2] * 16 + temp[i*2+1];
		}
		return dstlen;
	}
	else
	{
		dstlen = srclen / 2 + 1;
		for (i=0; i<dstlen-1; i++)
		{
			dst[i] = temp[i*2] * 16 + temp[i*2+1];
		}
		dst[i] = temp[i*2];
		return dstlen;
	}
}

//int dec2str(const unsigned char *src, int srclen, unsigned char *dst)
int dec2str(const uint8_t *src, int srclen, uint8_t *dst)
{
	int i = 0;
	uint8_t h = 0;
	uint8_t l = 0;
	uint8_t ascii_buf[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	// unsigned char h = 0;
	// unsigned char l = 0;
	// unsigned char ascii_buf[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	for (i=0; i<srclen; i++)
	{
		h = (src[i] >> 4) & 0x0f;
		l = src[i] & 0x0f;
		dst[2*i] = ascii_buf[h];
		dst[2*i+1] = ascii_buf[l];
	}
	dst[2*i] = '\0';
	return (2*i);
}


STATIC mp_obj_t mp_hash_sha1(mp_obj_t arg)
{
	char out_buff[20] = {0};
	char str_buff[25] = {0};
	char return_buff[45] = {0};
	
	mp_buffer_info_t argbuff;
    mp_get_buffer_raise(arg, &argbuff, MP_BUFFER_READ);

	str2dec((uint8_t *)argbuff.buf, argbuff.len, (uint8_t *)out_buff);
	
	SHA1(str_buff, out_buff, 20);
	dec2str( (uint8_t *)str_buff, 20,  (uint8_t *)return_buff);
	
	return mp_obj_new_str(return_buff, strlen(return_buff));
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_hash_sha1_obj, mp_hash_sha1);

STATIC mp_obj_t mp_hash_aes_ecb(mp_obj_t aes_key, mp_obj_t aes_data)
{
	int dlen;
	char out[100] = {0};
	char return_buff[200] = {0};
	
	mp_aes_context ctx;
	
	mp_buffer_info_t key_buff;
    mp_get_buffer_raise(aes_key, &key_buff, MP_BUFFER_READ);
	
	mp_buffer_info_t data_buff;
    mp_get_buffer_raise(aes_data, &data_buff, MP_BUFFER_READ);
	
	aes_setkey_enc(&ctx, (unsigned char*)key_buff.buf, 128);
	
    aes_crypt_ecb(&ctx, AES_ENCRYPT, (unsigned char*)data_buff.buf, (unsigned char *)out, data_buff.len, &dlen);
	
	dec2str((uint8_t *)out, 96, (uint8_t *)return_buff);

	
	return mp_obj_new_str(return_buff, strlen(return_buff));
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_hash_aes_ecb_obj, mp_hash_aes_ecb);


STATIC mp_obj_t mp_hash_b64(mp_obj_t b64_data)
{
	int buflen;

	char out_buff[100] = {0};
	char return_buff[130] = {0};
	
	mp_buffer_info_t b64_buff;
    mp_get_buffer_raise(b64_data, &b64_buff, MP_BUFFER_READ);

	//mp_printf(&mp_plat_print, "[b64]buf =[%s]\n", (char *)b64_buff.buf);
	buflen = str2dec((uint8_t *)b64_buff.buf, b64_buff.len, (uint8_t *)out_buff);
	//mp_printf(&mp_plat_print, "[b64]buflen=%d \n", buflen);
	
	mp_base64_encode((unsigned char *)out_buff, return_buff, buflen);
	
	 //mp_printf(&mp_plat_print, "[b64]len=%d \n", len);
	 //mp_printf(&mp_plat_print, "[b64]str_len=%d \n", strlen(return_buff));
	 //mp_printf(&mp_plat_print, "[b64]return_buff=%s \n", return_buff);

	return mp_obj_new_str(return_buff, strlen(return_buff));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_hash_b64_obj, mp_hash_b64); 


#endif

#if MICROPY_PY_UHASHLIB_MD5
STATIC mp_obj_t uhashlib_md5_update(mp_obj_t self_in, mp_obj_t arg);

#if MICROPY_SSL_AXTLS
STATIC mp_obj_t uhashlib_md5_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_obj_hash_t *o = m_new_obj_var(mp_obj_hash_t, char, sizeof(MD5_CTX));
    o->base.type = type;
    MD5_Init((MD5_CTX *)o->state);
    if (n_args == 1) {
        uhashlib_md5_update(MP_OBJ_FROM_PTR(o), args[0]);
    }
    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t uhashlib_md5_update(mp_obj_t self_in, mp_obj_t arg) {
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(arg, &bufinfo, MP_BUFFER_READ);
    MD5_Update((MD5_CTX *)self->state, bufinfo.buf, bufinfo.len);
    return mp_const_none;
}

STATIC mp_obj_t uhashlib_md5_digest(mp_obj_t self_in) {
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    vstr_t vstr;
    vstr_init_len(&vstr, MD5_SIZE);
    MD5_Final((byte *)vstr.buf, (MD5_CTX *)self->state);
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
#endif // MICROPY_SSL_AXTLS

#if MICROPY_SSL_MBEDTLS

#if MBEDTLS_VERSION_NUMBER < 0x02070000
#define mbedtls_md5_starts_ret mbedtls_md5_starts
#define mbedtls_md5_update_ret mbedtls_md5_update
#define mbedtls_md5_finish_ret mbedtls_md5_finish
#endif

STATIC mp_obj_t uhashlib_md5_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_obj_hash_t *o = m_new_obj_var(mp_obj_hash_t, char, sizeof(mbedtls_md5_context));
    o->base.type = type;
    mbedtls_md5_init((mbedtls_md5_context *)o->state);
    mbedtls_md5_starts_ret((mbedtls_md5_context *)o->state);
    if (n_args == 1) {
        uhashlib_md5_update(MP_OBJ_FROM_PTR(o), args[0]);
    }
    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t uhashlib_md5_update(mp_obj_t self_in, mp_obj_t arg) {
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(arg, &bufinfo, MP_BUFFER_READ);
    mbedtls_md5_update_ret((mbedtls_md5_context *)self->state, bufinfo.buf, bufinfo.len);
    return mp_const_none;
}

STATIC mp_obj_t uhashlib_md5_digest(mp_obj_t self_in) {
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    vstr_t vstr;
    vstr_init_len(&vstr, 16);
    mbedtls_md5_finish_ret((mbedtls_md5_context *)self->state, (byte *)vstr.buf);
    mbedtls_md5_free((mbedtls_md5_context *)self->state);
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
#endif // MICROPY_SSL_MBEDTLS

STATIC MP_DEFINE_CONST_FUN_OBJ_2(uhashlib_md5_update_obj, uhashlib_md5_update);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uhashlib_md5_digest_obj, uhashlib_md5_digest);

STATIC const mp_rom_map_elem_t uhashlib_md5_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&uhashlib_md5_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_digest), MP_ROM_PTR(&uhashlib_md5_digest_obj) },
};
STATIC MP_DEFINE_CONST_DICT(uhashlib_md5_locals_dict, uhashlib_md5_locals_dict_table);

STATIC const mp_obj_type_t uhashlib_md5_type = {
    { &mp_type_type },
    .name = MP_QSTR_md5,
    .make_new = uhashlib_md5_make_new,
    .locals_dict = (void *)&uhashlib_md5_locals_dict,
};
#endif // MICROPY_PY_UHASHLIB_MD5

STATIC const mp_rom_map_elem_t mp_module_uhashlib_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_uhashlib) },
    #if MICROPY_PY_UHASHLIB_SHA256
    { MP_ROM_QSTR(MP_QSTR_sha256), MP_ROM_PTR(&uhashlib_sha256_type) },
    #endif
    #if MICROPY_PY_UHASHLIB_SHA1
    { MP_ROM_QSTR(MP_QSTR_sha1), MP_ROM_PTR(&uhashlib_sha1_type) },
    { MP_ROM_QSTR(MP_QSTR_hash_sha1), MP_ROM_PTR(&mp_hash_sha1_obj) },
	{ MP_ROM_QSTR(MP_QSTR_aes_ecb), MP_ROM_PTR(&mp_hash_aes_ecb_obj) },
	{ MP_ROM_QSTR(MP_QSTR_hash_base64), MP_ROM_PTR(&mp_hash_b64_obj) },
    #endif
    #if MICROPY_PY_UHASHLIB_MD5
    { MP_ROM_QSTR(MP_QSTR_md5), MP_ROM_PTR(&uhashlib_md5_type) },
    #endif
};

STATIC MP_DEFINE_CONST_DICT(mp_module_uhashlib_globals, mp_module_uhashlib_globals_table);

const mp_obj_module_t mp_module_uhashlib = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_uhashlib_globals,
};

#endif // MICROPY_PY_UHASHLIB
