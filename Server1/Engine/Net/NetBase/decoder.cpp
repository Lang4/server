#include "decoder.h"
#include "record.h"
namespace mynet{
	unsigned Decoder::leftsize()
	{
		return contents.size() -currentoffset;
	}
	void Decoder::refresh()
	{
		//
		switch(nowstate)
		{
			case START:
			{
				contents.resize(1);
				nowstate = PICK_HEAD;
				currentoffset = 0;
				tag = 0;
			}break;
			case PICK_HEAD:
			{
				tag = contents[0];
				if (tag & MIN_HEAD)
					contents.resize(1);
				else if (tag & MAX_HEAD)
					contents.resize(2);
				else
					printf("Decorder::refresh error %d\n",tag);
				nowstate = PICK_BODY;
				currentoffset = 0;
			}break;
			case PICK_BODY:
			{
				contents.resize(getbodysize());
				nowstate = END;
				currentoffset = 0;
				//printf("Will take body.size():%u\n",contents.size());
			}break;
			case END:
			{
				nowstate = START;
				contents.resize(0);
				currentoffset = 0;
				tag = 0;
			}break;
		}
	}
	bool Decoder::isFinished()
	{
		//  
		return ((nowstate==END) && leftsize() == 0);
	}
	void Decoder::setbodysize(unsigned int size)
	{
		if (tag & MIN_HEAD)
		{
			*(unsigned char*)(&contents[1]) = size;
		}
		if (tag & MAX_HEAD)
		{
			*(unsigned short*)(&contents[1]) = size;
		}
	}
	unsigned int Decoder::getbodysize()
	{
		if (tag & MIN_HEAD)
		{
			return *(unsigned char*)&contents[0];
		}
		if (tag & MAX_HEAD)
		{
			return *(unsigned short*)&contents[0];
		}
		return 0;
	}
	Record * Decoder::getRecord()
	{
		Record *record = new Record(&contents[0],contents.size());
		return record;
	}
	void Decoder::encode(void *data,unsigned int len,bool ziptag,bool destag) // 
	{
		//  
		tag = 0;
		unsigned int headcontent = 0;
		if (len <= 255)
		{
			tag |= MIN_HEAD;
			ziptag = false; //  
			headcontent = 1; //  
		}
		else
		{
			tag |= MAX_HEAD;
			headcontent = 2;
		}
		if (ziptag) tag |= ZIP;
		if (destag) tag |= DES;
		
		if (ziptag)
		{
			len = zip(data,len,headcontent + 1);
		}
		else
		{
			contents.resize(len + headcontent + 1);
			memcpy(&contents[headcontent+1],data,len);
		}
		if (destag)
		{
			des();
		}
		
		contents[0] = tag;
		setbodysize(len);
	}
	unsigned int Decoder::unzip_size(unsigned int zip_size)
	{
		return zip_size * 120 / 100 + 12;
	}
	unsigned int Decoder::unzip(unsigned char *buffer,unsigned int len,unsigned int offset)
	{
		return len;
		//  
		if (tag & ZIP)
		{
			unsigned int unZipLen = len;
			//int retcode = uncompress(buffer,(uLongf*)&unZipLen,&contents[offset],contents.size() - offset);
			return unZipLen;
		}
		return 0;
	}
	
	void Decoder::undes()
	{
		//  
		if (tag & DES)
		{
			
		}
	}
	unsigned int Decoder::zip(void *data,unsigned int len,unsigned int offset)
	{
		return len;
		if (tag & ZIP)
		{
			contents.resize(unzip_size(len));
			unsigned int outlen = 0;
			//int retcode = compress(&contents[offset],(uLongf*)&outlen,(const Bytef *)data,len);
			contents.resize(outlen);
			len = outlen;
		}
		return len;
	}
	
	void Decoder::des()
	{
		if (tag & DES)
		{
			
		}
	}
};