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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpconfigport.h"
#include "obj.h"
#include "helios_debug.h"

#define QPY_SHA1_LOG(msg, ...)      custom_log(Sha1, msg, ##__VA_ARGS__)


typedef unsigned char b8;
typedef unsigned b32;

static int rpr (b32 * h) 
{
  QPY_SHA1_LOG("%08x%08x%08x%08x%08x\n", *h, *(h + 1), *(h + 2), *(h + 3),
          *(h + 4));
  return 0;
}

/*
static int die (char *msg) 
{
  fputs (msg, stderr);
  exit (0);
}                               // libpk 
*/

static b8 *rd32be (b32 * n, b8 * msg) 
{
  *n = *msg << 24;
  ++msg;
  *n |= *msg << 16;
  ++msg;
  *n |= *msg << 8;
  ++msg;
  *n |= *msg;
  return ++msg;
}                               /* libpk */

    /* do this outside b32 h[5]={ 0x67452301, 0xefcdab89, 0x98badcfe, */
    /* 0x10325476, 0xc3d2e1f0}; */

typedef struct {
  unsigned char *d;
  unsigned l[2];
} bitstr;


typedef struct {
  b32 h[5];
  b32 l[2];
} hsh;

static int hsh_rst (hsh * h) 
{
  h->h[0] = 0x67452301;
  h->h[1] = 0xefcdab89;
  h->h[2] = 0x98badcfe;
  h->h[3] = 0x10325476;
  h->h[4] = 0xc3d2e1f0;
  h->l[0] = 0;
  h->l[1] = 0;

  return 0;
}

static b32 f (b32 t, b32 * a, b32 * w) 
{
  b32 temp = 0;
  b32 k[4] = { 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6 };
  temp = ((a[0] << 5) | (a[0] >> 27)) + a[4] + w[t];
  switch (t / 20) {
  case 0:
    temp += k[0];
    temp += (a[1] & a[2]) | ((~a[1]) & a[3]);
    break;
  case 1:
    temp += k[1];
    temp += a[1] ^ a[2] ^ a[3];
    break;
  case 2:
    temp += k[2];
    temp += (a[1] & a[2]) | (a[1] & a[3]) | (a[2] & a[3]);
    break;
  case 3:
    temp += k[3];
    temp += a[1] ^ a[2] ^ a[3];
    break;
  }
  return temp;
}


static int grind (b32 * w, b32 * h) 
{
  b32 t, temp;
  b32 a[5];
  for (t = 16; t < 80; t++) {
    temp = w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16];
    w[t] = (temp << 1) | (temp >> 31);
  }
  for (t = 0; t < 5; t++)
    a[t] = h[t];
  for (t = 0; t < 80; t++) {
    temp = f(t, a, w);
    a[4] = a[3];
    a[3] = a[2];
    a[2] = (a[1] << 30) | (a[1] >> 2);
    a[1] = a[0];
    a[0] = temp;
  }
  for (t = 0; t < 5; t++)
    h[t] += a[t];
  return 0;
}


static int sha1_nxt (b8 * msg, b32 bits, hsh * h) 
{
  b32 w[80];
  int i;

  if (bits != 512)
    return 1;                   /* bad length */
  h->l[0] += bits;
  if (h->l[0] < bits) {
    h->l[1]++;
    if (!(h->l[1]))
      return 2;
  }                             /* msg size overflow */
  for (i = 0; i < 16; i++)
    msg = rd32be (w + i, msg);
  grind (w, h->h);
  return 0;
}

static int sha1_end (b8 * msg, b32 bits, hsh * h) 
{
  b32 w[80];
  int i = 0, j, b;
  if (bits >= 512)
    return 1;                   /* cant end with this chunk size */
  if (bits) {                   /* do incomplete chunk */
    h->l[0] += bits;
    if (h->l[0] < bits) {
      h->l[1]++;
      if (!(h->l[1]))
        return 2;
    }                           /* msg size overflow */
    if ((j = bits / 32)) {        /* do all complete words */
      for (i = 0; i < j; i++)
        msg = rd32be (w + i, msg);
      bits %= 32;
    }
    b = bits;                   /* do final word */
    w[i] = 1 << (31 - bits);
    w[i] |= *msg << 24;
    ++msg;
    b -= 8;
    if (b > 0) {
      w[i] |= *msg << 16;
      ++msg;
      b -= 8;
      if (b > 0) {
        w[i] |= *msg << 8;
        ++msg;
        b -= 8;
        if (b > 0) {
          w[i] |= *msg;
          ++msg;
          b -= 8;
        }
      }
    }
    w[i] &= 0xffffffff << (31 - bits);
    i++;
    if (j >= 14) {
      if (i == 15)
        w[15] = 0;
      grind (w, h->h);
      i = 0;
    }

  }
  else {                        /* add final chunk having length */
    w[0] = 0x80000000;
    i = 1;
  }
  for (; i < 14; i++)
    w[i] = 0;                   /* fill with zero, leave 64 bits */
  w[i] = h->l[1];
  i++;
  w[i] = h->l[0];               /* fill last 64 bits with length */
  grind (w, h->h);
  h->l[0] = 0;
  h->l[1] = 0;
  return 0;
}


static int sha1_finish (bitstr * msg, hsh * h) 
{
  bitstr p;

  p = *msg;
  while (p.l[1] || (p.l[0] >= 512)) {
    sha1_nxt(p.d, 512, h);     /* FIXME check return value? */
    p.d += 64;
    if (p.l[0] < 512) {
      if (p.l[1]) {
        p.l[1]--;
      }
      else
        return 1;               /* length underflow; FIXME redundant? */
    }
    p.l[0] -= 512;
  }
  sha1_end (p.d, p.l[0], h);    /* FIXME check return value? */
  return 0;
}

static int sha1 (bitstr * msg, hsh * h) 
{
  // int i;
  // bitstr p;

  hsh_rst (h);
  return sha1_finish (msg, h);
}


/* rfc3174
it all boiled down to reading big endian numbers correctly ;)
and realizing the you are not writing anything in big endian form :P
Wed Jul 16 00:32:21 IST 2014
*/


static b8 *wr32be (b32 n, b8 * msg) 
{
  b32 mask = 0xff;
  *msg = n >> 24;
  ++msg;
  *msg = (n >> 16) & mask;
  ++msg;
  *msg = (n >> 8) & mask;
  ++msg;
  *msg = n & mask;
  return ++msg;
}                               /* libpk */

static int hmac_sha1 (bitstr * key, bitstr * msg, unsigned mac[5]) 
{
  unsigned k[16] = { 0 };
  unsigned o[16], i[16];
  unsigned x;
  hsh h, fin;


  if (key->l[1] || key->l[0] > 512) {
    sha1 (key, &h);
    for (x = 0; x < 5; x++)
      wr32be (h.h[x], (b8 *)k + x);   /* FIXME: if wr32be works not */
  }
  else
    memcpy (k, key->d, key->l[0] / 8 + (key->l[0] % 8 && 1));

  for (x = 0; x < 16; x++) {
    o[x] = 0x5c5c5c5c ^ k[x];
    i[x] = 0x36363636 ^ k[x];
  }


  hsh_rst (&h);
  sha1_nxt ((b8 *)i, 512, &h);
  sha1_finish (msg, &h);
  for (x = 0; x < 5; x++)
    wr32be (h.h[x], (b8 *)(h.h + x));   /* FIXME: if wr32be works not */
  hsh_rst (&fin);
  sha1_nxt ((b8 *)o, 512, &fin);
  sha1_end ((b8 *)(h.h), 160, &fin);

  for (x = 0; x < 5; x++)
    mac[x] = fin.h[x];
  return 0;

}

STATIC mp_obj_t ql_hmac_sha1 (size_t n_args, const mp_obj_t *args) 
{
	// int ret = 0;
	bitstr m;
	bitstr k;
	b32 hash[5];
	
	char *msg = (char *)mp_obj_str_get_str(args[1]);
	char *key = (char *)mp_obj_str_get_str(args[0]);
	char buff[50];

	m.d = (unsigned char *)msg;
	m.l[0] = 8*strlen(msg);
	m.l[1] = 0;

	k.d = (unsigned char *)key;
	k.l[0] = 8*strlen(key);
	k.l[1] = 0;

	hmac_sha1 (&k, &m, hash);
	rpr (hash);
	snprintf(buff, 50, "%08x%08x%08x%08x%08x", hash[0], hash[1], hash[2], hash[3], hash[4]);
	return mp_obj_new_str(buff, strlen(buff));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ql_hmac_sha1_obj, 1, 2, ql_hmac_sha1);


STATIC const mp_rom_map_elem_t mp_module_hmac_sha1_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_hmacSha1) },
	{ MP_ROM_QSTR(MP_QSTR_hmac_sha1), MP_ROM_PTR(&ql_hmac_sha1_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_hmac_sha1_globals, mp_module_hmac_sha1_globals_table);

const mp_obj_module_t mp_module_hmacSha1 = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&mp_module_hmac_sha1_globals,
};



