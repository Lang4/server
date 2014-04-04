#ifndef _ENCDEC_H
#define _ENCDEC_H

#ifndef _MY_RC5_H
#define _MY_RC5_H
#include <stdlib.h>
#include "Des.h"

#define RC5_ENCRYPT	1
#define RC5_DECRYPT	0

/* 32 bit.  For Alpha, things may get weird */
#define RC5_32_INT unsigned int

#define RC5_32_BLOCK		8
#define RC5_32_KEY_LENGTH	16 /* This is a default, max is 255 */

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

#define c2ln(c,l1,l2,n)	{ \
	c+=n; \
		l1=l2=0; \
		switch (n) { \
			case 8: l2 =((RC5_32_INT )(*(--(c))))<<24L; \
			case 7: l2|=((RC5_32_INT )(*(--(c))))<<16L; \
			case 6: l2|=((RC5_32_INT )(*(--(c))))<< 8L; \
			case 5: l2|=((RC5_32_INT )(*(--(c))));     \
			case 4: l1 =((RC5_32_INT )(*(--(c))))<<24L; \
			case 3: l1|=((RC5_32_INT )(*(--(c))))<<16L; \
			case 2: l1|=((RC5_32_INT )(*(--(c))))<< 8L; \
			case 1: l1|=((RC5_32_INT )(*(--(c))));     \
		} \
}
#undef c2l
#define c2l(c,l)	(l =((RC5_32_INT )(*((c)++)))    , \
		l|=((RC5_32_INT )(*((c)++)))<< 8L, \
		l|=((RC5_32_INT )(*((c)++)))<<16L, \
		l|=((RC5_32_INT )(*((c)++)))<<24L)

#define RC5_32_MASK	0xffffffffL

#define RC5_16_P	0xB7E1
#define RC5_16_Q	0x9E37
#define RC5_32_P	0xB7E15163L
#define RC5_32_Q	0x9E3779B9L
#define RC5_64_P	0xB7E151628AED2A6BLL
#define RC5_64_Q	0x9E3779B97F4A7C15LL

#define ROTATE_l32(a,n)     (((a)<<(int)(n))|((a)>>(32-(int)(n))))
#define ROTATE_r32(a,n)     (((a)>>(int)(n))|((a)<<(32-(int)(n))))
	/*
	 * #define ROTATE_l32(a,n)     _lrotl(a,n)
	 * #define ROTATE_r32(a,n)     _lrotr(a,n)
	 * */

#define E_RC5_32(a,b,s,n) \
	a^=b; \
	a=ROTATE_l32(a,b); \
	a+=s[n]; \
	a&=RC5_32_MASK; \
	b^=a; \
	b=ROTATE_l32(b,a); \
	b+=s[n+1]; \
	b&=RC5_32_MASK;

#define D_RC5_32(a,b,s,n) \
b-=s[n+1]; \
	b&=RC5_32_MASK; \
	b=ROTATE_r32(b,a); \
	b^=a; \
	a-=s[n]; \
	a&=RC5_32_MASK; \
	a=ROTATE_r32(a,b); \
	a^=b;

typedef void (* f_RC5_32_decrypt)(RC5_32_INT *d, RC5_32_KEY *key);
typedef void (* f_RC5_32_encrypt)(RC5_32_INT *d, RC5_32_KEY *key);
typedef void (* f_RC5_32_set_key)(RC5_32_KEY *key, int len, const unsigned char *data,
		int rounds);
#endif

#ifndef _MY_DES_H
#define  _MY_DES_H

#include <stdlib.h>
#define DES_ENCRYPT	1
#define DES_DECRYPT	0
//#define DES_LONG unsigned long
#define DES_LONG unsigned int
typedef unsigned char DES_cblock[8];
typedef /* const */ unsigned char const_DES_cblock[8];
typedef DES_LONG t_DES_SPtrans[8][64];
/* With "const", gcc 2.8.1 on Solaris thinks that DES_cblock *
 * * and const_DES_cblock * are incompatible pointer types. */

extern t_DES_SPtrans MyDES_SPtrans;

typedef struct DES_ks
{
	union
	{
		DES_cblock cblock;
		/* make sure things are correct size on machines with
		 * 		* 8 byte longs */
		DES_LONG deslong[2];
	} ks[16];
} DES_key_schedule;

extern const DES_LONG (*sp)[8][64];

#define	ROTATE(a,n)	(((a)>>(int)(n))|((a)<<(32-(int)(n))))

#define LOAD_DATA(R,S,u,t,E0,E1,tmp) \
u=R^s[S  ]; \
	t=R^s[S+1]
#define LOAD_DATA_tmp(a,b,c,d,e,f) LOAD_DATA(a,b,c,d,e,f,g)

#define D_ENCRYPT(LL,R,S) {\
	LOAD_DATA_tmp(R,S,u,t,E0,E1); \
		t=ROTATE(t,4); \
		LL^=\
		(*sp)[0][(u>> 2L)&0x3f]^ \
		(*sp)[2][(u>>10L)&0x3f]^ \
		(*sp)[4][(u>>18L)&0x3f]^ \
		(*sp)[6][(u>>26L)&0x3f]^ \
		(*sp)[1][(t>> 2L)&0x3f]^ \
		(*sp)[3][(t>>10L)&0x3f]^ \
		(*sp)[5][(t>>18L)&0x3f]^ \
		(*sp)[7][(t>>26L)&0x3f]; }

#define PERM_OP(a,b,t,n,m) ((t)=((((a)>>(n))^(b))&(m)),\
		(b)^=(t),\
		(a)^=((t)<<(n)))

#define IP(l,r) \
{ \
	register DES_LONG tt; \
		PERM_OP(r,l,tt, 4,0x0f0f0f0fL); \
		PERM_OP(l,r,tt,16,0x0000ffffL); \
		PERM_OP(r,l,tt, 2,0x33333333L); \
		PERM_OP(l,r,tt, 8,0x00ff00ffL); \
		PERM_OP(r,l,tt, 1,0x55555555L); \
}

#define FP(l,r) \
{ \
	register DES_LONG tt; \
		PERM_OP(l,r,tt, 1,0x55555555L); \
		PERM_OP(r,l,tt, 8,0x00ff00ffL); \
		PERM_OP(l,r,tt, 2,0x33333333L); \
		PERM_OP(r,l,tt,16,0x0000ffffL); \
		PERM_OP(l,r,tt, 4,0x0f0f0f0fL); \
}



#define HPERM_OP(a,t,n,m) ((t)=((((a)<<(16-(n)))^(a))&(m)),\
		(a)=(a)^(t)^(t>>(16-(n))))
#define ITERATIONS 16
#define HALF_ITERATIONS 8

	/* used in des_read and des_write */
#define MAXWRITE	(1024*16)
#define BSIZE		(MAXWRITE+4)

#undef c2l
#define c2l(c,l)	(l =((DES_LONG)(*((c)++)))    , \
		l|=((DES_LONG)(*((c)++)))<< 8L, \
		l|=((DES_LONG)(*((c)++)))<<16L, \
		l|=((DES_LONG)(*((c)++)))<<24L)

	extern const DES_LONG des_skb[8][64];

	typedef void (* f_DES_random_key) (DES_cblock *key);

	typedef void (* f_DES_set_key) (const_DES_cblock *key, DES_key_schedule *schedule);

	typedef void (* f_DES_encrypt1) (DES_LONG *data, DES_key_schedule *ks,t_DES_SPtrans * sp, int enc);

	typedef void (* f_DES_encrypt3) (DES_LONG *data, DES_key_schedule *ks1,
			DES_key_schedule *ks2, DES_key_schedule *ks3,t_DES_SPtrans * sp);

typedef void (* f_DES_decrypt3) (DES_LONG *data, DES_key_schedule *ks1,
		DES_key_schedule *ks2, DES_key_schedule *ks3,t_DES_SPtrans * sp);

#endif

#ifndef _MY_CAST_H
#define _MY_CAST_H

#define CAST_ENCRYPT	1
#define CAST_DECRYPT	0

#define CAST_LONG unsigned long

#define CAST_BLOCK	8
#define CAST_KEY_LENGTH	16
 
typedef struct cast_key_st
{
		CAST_LONG data[32];
			int short_key;	/* Use reduced rounds for short key */
} CAST_KEY;

#if defined(OPENSSL_SYS_WIN32) && defined(_MSC_VER)
#define ROTL(a,n)     (_lrotl(a,n))
#else
#define ROTL(a,n)     ((((a)<<(n))&0xffffffffL)|((a)>>(32-(n))))
#endif

extern const CAST_LONG MyCAST_S_table0[256];
extern const CAST_LONG MyCAST_S_table1[256];
extern const CAST_LONG MyCAST_S_table2[256];
extern const CAST_LONG MyCAST_S_table3[256];
extern const CAST_LONG MyCAST_S_table4[256];
extern const CAST_LONG MyCAST_S_table5[256];
extern const CAST_LONG MyCAST_S_table6[256];
extern const CAST_LONG MyCAST_S_table7[256];

#define E_CAST(n,key,L,R,OP1,OP2,OP3) \
{ \
	CAST_LONG a,b,c,d; \
		t=(key[n*2] OP1 R)&0xffffffff; \
		t=ROTL(t,(key[n*2+1])); \
		a=MyCAST_S_table0[(t>> 8)&0xff]; \
		b=MyCAST_S_table1[(t    )&0xff]; \
		c=MyCAST_S_table2[(t>>24)&0xff]; \
		d=MyCAST_S_table3[(t>>16)&0xff]; \
		L^=(((((a OP2 b)&0xffffffffL) OP3 c)&0xffffffffL) OP1 d)&0xffffffffL; \
}



#define CAST_exp(l,A,a,n) \
A[n/4]=l; \
	a[n+3]=(l    )&0xff; \
	a[n+2]=(l>> 8)&0xff; \
	a[n+1]=(l>>16)&0xff; \
	a[n+0]=(l>>24)&0xff;

#define S4 MyCAST_S_table4
#define S5 MyCAST_S_table5
#define S6 MyCAST_S_table6
#define S7 MyCAST_S_table7


typedef void (* f_CAST_set_key)(CAST_KEY *key, int len, const unsigned char *data);
typedef void (* f_CAST_encrypt)(CAST_LONG *data, CAST_KEY *key);
typedef void (* f_CAST_decrypt)(CAST_LONG *data, CAST_KEY *key);

#endif

#ifndef _MY_IDEA_H
#define _MY_IDEA_H

#define IDEA_INT unsigned int

typedef struct idea_key_st
{
	IDEA_INT data[9][6];
} IDEA_KEY_SCHEDULE;

#define idea_mul(r,a,b,ul) \
ul=(unsigned long)a*b; \
	if (ul != 0) \
{ \
	r=(ul&0xffff)-(ul>>16); \
		r-=((r)>>16); \
} \
else \
	r=(-(int)a-b+1); /* assuming a or b is 0 and in range */ 

#define E_IDEA(num) \
x1&=0xffff; \
	idea_mul(x1,x1,*p,ul); p++; \
	x2+= *(p++); \
	x3+= *(p++); \
	x4&=0xffff; \
	idea_mul(x4,x4,*p,ul); p++; \
	t0=(x1^x3)&0xffff; \
	idea_mul(t0,t0,*p,ul); p++; \
	t1=(t0+(x2^x4))&0xffff; \
	idea_mul(t1,t1,*p,ul); p++; \
	t0+=t1; \
	x1^=t1; \
	x4^=t0; \
	ul=x2^t0; /* do the swap to x3 */ \
x2=x3^t1; \
	x3=ul;
#define n2s(c,l)	(l =((IDEA_INT)(*((c)++)))<< 8L, \
		l|=((IDEA_INT)(*((c)++)))      )

	/* taken directly from the 'paper' I'll have a look at it later */
inline IDEA_INT inverse(unsigned int xin)
{
	long n1,n2,q,r,b1,b2,t;

	if (xin == 0)
		b2=0;
	else
	{
		n1=0x10001;
		n2=xin;
		b2=1;
		b1=0;

		do	{
			r=(n1%n2);
			q=(n1-r)/n2;
			if (r == 0)
			{ if (b2 < 0) b2=0x10001+b2; }
			else
			{
				n1=n2;
				n2=r;
				t=b2;
				b2=b1-q*b2;
				b1=t;
			}
		} while (r != 0);
	}
	return((IDEA_INT)b2);
}

typedef void (* f_idea_set_encrypt_key)(const unsigned char *key, IDEA_KEY_SCHEDULE *ks);
typedef void (* f_idea_set_decrypt_key)(IDEA_KEY_SCHEDULE *ek, IDEA_KEY_SCHEDULE *dk);
typedef void (* f_idea_encrypt)(unsigned long *d, IDEA_KEY_SCHEDULE *key);

#endif

#ifndef _MD5EX_H
#define _MD5EX_H

class Stream;
bool MD5Data(const void* pData,int size,unsigned char *pMD5 /* 16 Byte*/);
bool MD5Stream(Stream* pStream,unsigned char *pMD5 /* 16 Byte*/);
bool MD5File(const char* pszFile,unsigned char *pMD5 /* 16 Byte*/);
bool MD5String(const char* string,unsigned char* pMD5 /* 16 Byte*/);

#endif

extern void DES_random_key(DES_cblock *ret);
extern void DES_set_key(const_DES_cblock *key, DES_key_schedule *schedule);
extern void DES_encrypt1(DES_LONG *data, DES_key_schedule *ks,t_DES_SPtrans * sp, int enc);
extern void DES_encrypt3(DES_LONG *data, DES_key_schedule *ks1,
			DES_key_schedule *ks2, DES_key_schedule *ks3,t_DES_SPtrans * sp);
extern void DES_decrypt3(DES_LONG *data, DES_key_schedule *ks1,
			DES_key_schedule *ks2, DES_key_schedule *ks3,t_DES_SPtrans * sp);

extern void RC5_32_set_key(RC5_32_KEY *key, int len, const unsigned char *data,
	int rounds);
extern void RC5_32_encrypt(RC5_32_INT *d, RC5_32_KEY *key);
extern void RC5_32_decrypt(RC5_32_INT *d, RC5_32_KEY *key);

extern void idea_set_encrypt_key(const unsigned char *key, IDEA_KEY_SCHEDULE *ks);
extern void idea_set_decrypt_key(IDEA_KEY_SCHEDULE *ek, IDEA_KEY_SCHEDULE *dk);
extern void idea_encrypt(unsigned long *d, IDEA_KEY_SCHEDULE *key);

extern void CAST_set_key(CAST_KEY *key, int len, const unsigned char *data);
extern void CAST_encrypt(CAST_LONG *data, CAST_KEY *key);
extern void CAST_decrypt(CAST_LONG *data, CAST_KEY *key);

/*
class IEncrypt{
public:
	virtual void DES_random_key(DES_cblock *key) = 0;
	virtual void DES_set_key(const_DES_cblock *key, DES_key_schedule *schedule) = 0;
	virtual void DES_encrypt1(DES_LONG *data, DES_key_schedule *ks, int enc) = 0;
	virtual void DES_encrypt3(DES_LONG *data, DES_key_schedule *ks1,
				  DES_key_schedule *ks2, DES_key_schedule *ks3) = 0;
	virtual void DES_decrypt3(DES_LONG *data, DES_key_schedule *ks1,
				  DES_key_schedule *ks2, DES_key_schedule *ks3) = 0;

	virtual void RC5_32_set_key(RC5_32_KEY *key, int len, const unsigned char *data,
		    int rounds) = 0;
	virtual void RC5_32_encrypt(RC5_32_INT *d, RC5_32_KEY *key) = 0;
	virtual void RC5_32_decrypt(RC5_32_INT *d, RC5_32_KEY *key) = 0;

	virtual void idea_set_encrypt_key(const unsigned char *key, IDEA_KEY_SCHEDULE *ks) = 0;
	virtual void idea_set_decrypt_key(IDEA_KEY_SCHEDULE *ek, IDEA_KEY_SCHEDULE *dk) = 0;
	virtual void idea_encrypt(unsigned long *d, IDEA_KEY_SCHEDULE *key) = 0;

	virtual void CAST_set_key(CAST_KEY *key, int len, const unsigned char *data) = 0;
	virtual void CAST_encrypt(CAST_LONG *data, CAST_KEY *key)=0;
	virtual void CAST_decrypt(CAST_LONG *data, CAST_KEY *key)= 0;
};
*/

/*-------------------------------------------------------*/
/*-------------------------------------------------------*/
/*-------------------------------------------------------*/
class CEncrypt// : public IEncrypt
{
public:
	CEncrypt();
	enum encMethod
	{
		ENCDEC_NONE,
		ENCDEC_DES,
		ENCDEC_RC5
	};
	void random_key_des(DES_cblock *ret);
	void set_key_des(const_DES_cblock *key);
	void set_key_rc5(const unsigned char *data, int nLen, int rounds);
	int encdec(void *data, unsigned int nLen, bool enc);

	void setEncMethod(encMethod method);
	encMethod getEncMethod() const;

private:
	/*
	f_DES_random_key m_fDES_random_key;
	f_DES_set_key m_fDES_set_key;
	f_DES_encrypt1 m_fDES_encrypt1;
	f_DES_encrypt3 m_fDES_encrypt3;
	f_DES_decrypt3 m_fDES_decrypt3;

	f_RC5_32_decrypt m_fRC5_32_decrypt;
	f_RC5_32_encrypt m_fRC5_32_encrypt;
	f_RC5_32_set_key m_fRC5_32_set_key;

	f_idea_set_encrypt_key m_fidea_set_encrypt_key;
	f_idea_set_decrypt_key m_fidea_set_decrypt_key;
	f_idea_encrypt			m_fidea_encrypt;

	f_CAST_set_key			m_fCAST_set_key;
	f_CAST_encrypt			m_fCAST_encrypt;
	f_CAST_decrypt			m_fCAST_decrypt;
	*/

	void DES_random_key(DES_cblock *ret);
	void DES_set_key(const_DES_cblock *key, DES_key_schedule *schedule);
	void DES_encrypt1(DES_LONG *data, DES_key_schedule *ks, int enc);
	void DES_encrypt3(DES_LONG *data, DES_key_schedule *ks1,
				DES_key_schedule *ks2, DES_key_schedule *ks3);
	void DES_decrypt3(DES_LONG *data, DES_key_schedule *ks1,
				DES_key_schedule *ks2, DES_key_schedule *ks3);

	void RC5_32_set_key(RC5_32_KEY *key, int len, const unsigned char *data,
		int rounds);
	void RC5_32_encrypt(RC5_32_INT *d, RC5_32_KEY *key);
	void RC5_32_decrypt(RC5_32_INT *d, RC5_32_KEY *key);

	void idea_set_encrypt_key(const unsigned char *key, IDEA_KEY_SCHEDULE *ks);
	void idea_set_decrypt_key(IDEA_KEY_SCHEDULE *ek, IDEA_KEY_SCHEDULE *dk);
	void idea_encrypt(unsigned long *d, IDEA_KEY_SCHEDULE *key);

	void CAST_set_key(CAST_KEY *key, int len, const unsigned char *data);
	void CAST_encrypt(CAST_LONG *data, CAST_KEY *key);
	void CAST_decrypt(CAST_LONG *data, CAST_KEY *key);

	int encdec_des(unsigned char *data, unsigned int nLen, bool enc);
	int encdec_rc5(unsigned char *data, unsigned int nLen, bool enc);

	DES_key_schedule key_des;
	RC5_32_KEY key_rc5;
	bool haveKey_des;
	bool haveKey_rc5;

	encMethod method;
	Des des;

};

#endif

