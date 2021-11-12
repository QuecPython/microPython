//
// Created by ol on 2020/8/31.
//

#ifndef UNTITLED_AES_H
#define UNTITLED_AES_H
#include "string.h"


#define AES_ENCRYPT     1
#define AES_DECRYPT     0
/* 128 bit key */
#define AES_KEY_LEN     128

typedef struct
{
    int nr;                     /*!<  number of rounds  */
    unsigned int* rk;               /*!<  AES round keys    */
    unsigned int buf[68];           /*!<  unaligned data    */
}mp_aes_context;

void aes_setkey_enc( mp_aes_context* ctx, const unsigned char* key, int keysize );
void aes_setkey_dec( mp_aes_context* ctx, const unsigned char* key, int keysize );
void aes_crypt_ecb_update( mp_aes_context* ctx, int mode, const unsigned char input[16], unsigned char output[16] );
int aes_crypt_ecb( mp_aes_context* ctx, int mode, const unsigned char* input, unsigned char *output, int slen, int* dlen );

int mp_base64_encode( const unsigned char * bindata, char * base64, int binlength );
#endif //UNTITLED_AES_H
