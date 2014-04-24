#pragma once
#define _EPESDK_IMP_

#include "EncDec.h"

CEncrypt::CEncrypt()
{
  bzero(&key_des,sizeof(key_des));
  bzero(&key_rc5,sizeof(key_rc5));
  haveKey_des = false;
  haveKey_rc5 = false;
  method = ENCDEC_NONE;
}

void CEncrypt::ZES_set_key(const_ZES_cblock *key,ZES_key_schedule *schedule)
{
  ::ZES_set_key(key,schedule);
}

void CEncrypt::ZES_random_key(ZES_cblock *ret)
{
  ::ZES_random_key(ret);
}

void CEncrypt::ZES_encrypt1(ZES_LONG *data,ZES_key_schedule *ks,int enc)
{
  ::ZES_encrypt1(data,ks,enc);
}

void CEncrypt::RC5_32_set_key(RC5_32_KEY *key,int len,const BYTE *data,int rounds)
{
  ::RC5_32_set_key(key,len,data,rounds);
}

void CEncrypt::RC5_32_encrypt(RC5_32_INT*d,RC5_32_KEY *key)
{
  ::RC5_32_encrypt(d,key);
}

void CEncrypt::RC5_32_decrypt(RC5_32_INT*d,RC5_32_KEY *key)
{
  ::RC5_32_decrypt(d,key);
}

int CEncrypt::encdec_des(BYTE *data,DWORD nLen,bool enc)
{
  if ((0==data)||(!haveKey_des)) return -1;

  DWORD offset = 0;
  while (offset<=nLen-8)
  {       
    ZES_encrypt1((ZES_LONG*)(data+offset),&key_des,enc);
    offset += 8;
  }

  return nLen-offset;
}

int CEncrypt::encdec_rc5(BYTE *data,DWORD nLen,bool enc)
{
  if ((0==data)||(!haveKey_rc5)) return -1;

  DWORD offset = 0;
  while (offset<=nLen-8)
  {
    RC5_32_INT d[2];
    memcpy(d,data+offset,sizeof(d));//,sizeof(d));
    if (enc)
      ::RC5_32_encrypt(d,&key_rc5);
    else
      ::RC5_32_decrypt(d,&key_rc5);
    memcpy(data+offset,d,sizeof(d));//,sizeof(d));
    offset += sizeof(d);
  }

  return nLen-offset;
}

void CEncrypt::random_key_des(ZES_cblock *ret)
{
  ::ZES_random_key(ret);
}

void CEncrypt::set_key_des(const_ZES_cblock *key)
{
  ::ZES_set_key(key,&key_des);
  haveKey_des = true;
}

void CEncrypt::set_key_rc5(const BYTE *data,int nLen,int rounds)
{
  ::RC5_32_set_key(&key_rc5,nLen,data,rounds);
  haveKey_rc5 = true;
} 

int CEncrypt::encdec(void *data,DWORD nLen,bool enc)
{
  switch(method)
  {
    case ENCDEC_NONE:
         return -1;
    case ENCDEC_DES:
         return encdec_des((BYTE*)data,nLen,enc);
    case ENCDEC_RC5:
         return encdec_rc5((BYTE*)data,nLen,enc);
  }
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
