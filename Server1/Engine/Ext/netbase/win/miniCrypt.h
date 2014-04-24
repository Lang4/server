#pragma once
#ifdef __cplusplus
#define MINI_RC5 1
extern "C" {
#endif //__cplusplus

#ifdef MINI_RC5
#define RC5_ENCRYPT	1
#define RC5_DECRYPT	0

/* 32 bit.  For Alpha,things may get weird */
typedef DWORD RC5_32_INT;

#define RC5_32_BLOCK		8
#define RC5_32_KEY_LENGTH	16 /* This is a default,max is 255 */

/* This are the only values supported.  Tweak the code if you want more
 * * The most supported modes will be
 * * RC5-32/12/16
 * * RC5-32/16/8
 * */
#define RC5_8_ROUNDS	8
#define RC5_12_ROUNDS	12
#define RC5_16_ROUNDS	16

typedef struct rc5_key_st
{
	/* Number of rounds */
	int rounds;
	RC5_32_INT data[2*(RC5_16_ROUNDS+1)];
} RC5_32_KEY;

void RC5_32_set_key(RC5_32_KEY *key,int len,const BYTE *data,int rounds);
void RC5_32_encrypt(RC5_32_INT *d,RC5_32_KEY *key);
void RC5_32_decrypt(RC5_32_INT *d,RC5_32_KEY *key);
#endif //MINI_RC5

#define ZES_ENCRYPT	1
#define ZES_DECRYPT	0

//#define ZES_LONG unsigned long
typedef DWORD ZES_LONG;
typedef BYTE ZES_cblock[8];
typedef /* const */ BYTE const_ZES_cblock[8];

/* With "const",gcc 2.8.1 on Solaris thinks that ZES_cblock *
 * * and const_ZES_cblock * are incompatible pointer types. */

typedef struct ZES_ks
{
	union
	{
		ZES_cblock cblock;
		/* make sure things are correct size on machines with
		 * 		* 8 byte longs */
		ZES_LONG deslong[2];
	} ks[16];
} ZES_key_schedule;

void ZES_random_key(ZES_cblock *ret);
void ZES_set_key(const_ZES_cblock *key,ZES_key_schedule *schedule);
void ZES_encrypt1(ZES_LONG *data,ZES_key_schedule *ks,int enc);
void ZES_encrypt3(ZES_LONG *data,ZES_key_schedule *ks1,ZES_key_schedule *ks2,ZES_key_schedule *ks3);
void ZES_decrypt3(ZES_LONG *data,ZES_key_schedule *ks1,ZES_key_schedule *ks2,ZES_key_schedule *ks3);

#ifdef __cplusplus
}
#endif //__cplusplus
