#pragma once
#include <stdint.h>

class Des
{
	uint32_t m_encKey[32];
	uint32_t m_decKey[32];

	public:
	Des(){}

	void setDes(uint8_t* pkey);

	void decrypt(uint8_t* pblock,int index=0);

	void encrypt(uint8_t* pblock,int index=0);

	void generateWorkingKey(bool encrypting,uint8_t* key,int off, uint32_t* newKey);

	void desFunc(uint32_t* wKey, uint8_t* inp, int inOff,uint8_t* out, int outOff);
};
