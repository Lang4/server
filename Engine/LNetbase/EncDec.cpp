//#include "../../engine/include/engine.h"
#include "EncDec.h"
#include <string.h>

CEncrypt::CEncrypt()
{
	/*
	m_fDES_random_key  = ::DES_random_key  ;
	m_fDES_set_key  = ::DES_set_key  ;
	m_fDES_encrypt1 = ::DES_encrypt1 ;
	m_fDES_encrypt3 = ::DES_encrypt3 ;
	m_fDES_decrypt3 = ::DES_decrypt3 ;

	m_fRC5_32_decrypt = ::RC5_32_decrypt ;
	m_fRC5_32_encrypt = ::RC5_32_encrypt ;
	m_fRC5_32_set_key = ::RC5_32_set_key ;

	m_fidea_set_encrypt_key = ::idea_set_encrypt_key  ;
	m_fidea_set_decrypt_key = ::idea_set_decrypt_key  ;
	m_fidea_encrypt = ::idea_encrypt;

	m_fCAST_set_key = ::CAST_set_key;
	m_fCAST_encrypt = ::CAST_encrypt;
	m_fCAST_decrypt = ::CAST_decrypt;
	*/

	bzero(&key_des, sizeof(key_des));
	bzero(&key_rc5, sizeof(key_rc5));
	haveKey_des = false;
	haveKey_rc5 = false;
	//key_des = new DES_key_schedule;
	//key_rc5 = new RC5_32_KEY;
	method = ENCDEC_NONE;
}

/*
CEncrypt::~CEncrypt()
{
	//if (key_des) delete key_des; 
	//if (key_rc5) delete key_rc5; 
}
*/

void CEncrypt::DES_set_key(const_DES_cblock *key, DES_key_schedule *schedule)
{
	//m_fDES_set_key(key,schedule);
	::DES_set_key(key, schedule);
}

void CEncrypt::DES_random_key(DES_cblock *ret)
{
	//m_fDES_random_key(ret);
	::DES_random_key(ret);
}

void CEncrypt::DES_encrypt1(DES_LONG *data, DES_key_schedule *ks, int enc)
{
	//m_fDES_encrypt1(data,ks,&MyDES_SPtrans,enc);
	::DES_encrypt1(data, ks, &MyDES_SPtrans, enc);
}

void CEncrypt::DES_encrypt3(DES_LONG *data, DES_key_schedule *ks1,
			DES_key_schedule *ks2, DES_key_schedule *ks3)
{
	//m_fDES_encrypt3(data,ks1,ks2,ks3,&MyDES_SPtrans);
	::DES_encrypt3(data, ks1, ks2, ks3, &MyDES_SPtrans);
}

void CEncrypt::DES_decrypt3(DES_LONG *data, DES_key_schedule *ks1,
			DES_key_schedule *ks2, DES_key_schedule *ks3)
{
	//m_fDES_decrypt3(data,ks1,ks2,ks3,&MyDES_SPtrans);
	::DES_decrypt3(data, ks1, ks2, ks3, &MyDES_SPtrans);
}

void CEncrypt::RC5_32_set_key(RC5_32_KEY *key, int len, const unsigned char *data,
	int rounds)
{
	//m_fRC5_32_set_key(key,len,data,rounds);
	::RC5_32_set_key(key,len,data,rounds);
}

void CEncrypt::RC5_32_encrypt(RC5_32_INT*d, RC5_32_KEY *key)
{
	//m_fRC5_32_encrypt(d,key);
	::RC5_32_encrypt(d,key);
}

void CEncrypt::RC5_32_decrypt(RC5_32_INT*d, RC5_32_KEY *key)
{
	//m_fRC5_32_decrypt(d,key);
	::RC5_32_decrypt(d,key);
}

void CEncrypt::idea_set_encrypt_key(const unsigned char *key, IDEA_KEY_SCHEDULE *ks)
{
	//m_fidea_set_encrypt_key(key,ks);
	::idea_set_encrypt_key(key,ks);
}

void CEncrypt::idea_set_decrypt_key(IDEA_KEY_SCHEDULE *ek, IDEA_KEY_SCHEDULE *dk)
{
	//m_fidea_set_decrypt_key(ek,dk);
	::idea_set_decrypt_key(ek,dk);
}

void CEncrypt::idea_encrypt(unsigned long *d, IDEA_KEY_SCHEDULE *key)
{
	//m_fidea_encrypt(d,key);
	::idea_encrypt(d,key);
}

void CEncrypt::CAST_set_key(CAST_KEY *key, int len, const unsigned char *data)
{
	//m_fCAST_set_key(key,len,data);
	::CAST_set_key(key,len,data);
}

void CEncrypt::CAST_encrypt(CAST_LONG *data, CAST_KEY *key)
{
	//m_fCAST_encrypt(data,key);
	::CAST_encrypt(data,key);
}

void CEncrypt::CAST_decrypt(CAST_LONG *data, CAST_KEY *key)
{
	//m_fCAST_decrypt(data,key);
	::CAST_decrypt(data,key);
}

int CEncrypt::encdec_des(unsigned char *data, unsigned int nLen, bool enc)
{
	if ((0==data)||(!haveKey_des)) return -1;

	unsigned int offset = 0;
	if(nLen >= 8)
	{
		while (offset<=nLen-8)
		{       
			//		DES_encrypt1((DES_LONG*)(data+offset), &key_des, enc);
			if(enc)
				des.encrypt((uint8_t*)(data+offset));
			else
				des.decrypt((uint8_t*)(data+offset));
			offset += 8;
		}
	}

	return nLen-offset;
}

int CEncrypt::encdec_rc5(unsigned char *data, unsigned int nLen, bool enc)
{
	if ((0==data)||(!haveKey_rc5)) return -1;

	unsigned int offset = 0;
	if(nLen >= 8)
	{
		while (offset<=nLen-8)
		{
			RC5_32_INT d[2];
			memcpy(d, data+offset, sizeof(d));
			if (enc)
				RC5_32_encrypt(d, &key_rc5);
			else
				RC5_32_decrypt(d, &key_rc5);
			/*
			   if (enc)
			   RC5_32_encrypt((RC5_32_INT *)data+offset, key_rc5);
			   else
			   RC5_32_decrypt((RC5_32_INT *)data+offset, key_rc5);
			 */
			memcpy(data+offset, d, sizeof(d));
			offset += sizeof(d);
		}
	}

	return nLen-offset;
}

void CEncrypt::random_key_des(DES_cblock *ret)
{
	::DES_random_key(ret);
}

void CEncrypt::set_key_des(const_DES_cblock *key)
{
//	::DES_set_key(key, &key_des);
	des.setDes((uint8_t*)key);
	haveKey_des = true;
}

void CEncrypt::set_key_rc5(const unsigned char *data, int nLen, int rounds = RC5_8_ROUNDS)
{
	//Zebra::logger->debug("rc5_key:%s", data);
	::RC5_32_set_key(&key_rc5, nLen, data, rounds);
	//Zebra::logger->debug("value:%d %d %d %d", key_rc5.data[0], key_rc5[1], key_rc5[2], key_rc5[3]);
	haveKey_rc5 = true;
} 

int CEncrypt::encdec(void *data, unsigned int nLen, bool enc)
{
	if (ENCDEC_NONE==method) return -1;

	if (ENCDEC_DES==method) return encdec_des((unsigned char *)data, nLen, enc);

	if (ENCDEC_RC5==method) return encdec_rc5((unsigned char *)data, nLen, enc);

	return -2;
}

void CEncrypt::setEncMethod(encMethod m)
{
	method = m;
}

CEncrypt::encMethod CEncrypt::getEncMethod() const
{
	return method;
}
