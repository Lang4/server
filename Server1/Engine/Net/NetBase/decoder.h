#pragma once
#include <vector>
namespace mynet{
	class Record;
	class Target;
	 struct stGetPackage{
        virtual void doGetCommand(Target *target,void *cmd,unsigned int len) = 0;
    };
   
	/**
     * parse my message 
	 */
    class Decoder{
    public:
        Decoder(){
            currentoffset = 0;
            nowstate = 0;
            tag = 0;
        }
    private:
        unsigned int currentoffset;  
        std::vector<unsigned char> contents;
        
        enum{
            START,
            END,
            PICK_HEAD,
            PICK_BODY,
        };
        
        unsigned char nowstate; //
        unsigned char tag; //
        static const unsigned int MAX_DATASIZE = 65536; //
        enum{
            ZIP = 1 << 0, // ZIP
            DES = 1 << 1, // DES
            MIN_HEAD = 1 << 2, //
            MAX_HEAD = 1 << 3, //
        };
        
        unsigned leftsize();
        template<typename Record>
        bool pickdata(Record *record)
        {
            unsigned int left_size = leftsize();
            if (left_size == 0) return true;
            int ret = record->recv(&contents[currentoffset],left_size);
            if (ret < left_size)
            {
                currentoffset += ret;
                return false;
            }
            currentoffset += ret;
            return true;
        }
        void refresh();
        template<typename RECORD>
        bool run(RECORD *record)
        {
            if ( currentoffset == contents.size())
            {
                refresh();
                return pickdata(record) || isFinished();
            }
            return pickdata(record);
        }
        bool isFinished();
        void setbodysize(unsigned int size);
        unsigned int getbodysize();
    public:
        template<typename RECORD>
        unsigned int decode(RECORD * target,void *buffer,unsigned int maxSize)  
        {
            while(run(target))
            {
                if (isFinished())
                {
                    undes();
                    unsigned int retSize = unzip((unsigned char*)buffer,maxSize,0);
                    return retSize;
                }
            }
            return 0;
        }
        template<typename RECORD>
        void decode(RECORD *record,Target *target,stGetPackage *callback) // 
        {
            while(run(record))
            {
                if (isFinished())
                {
                    undes();
                    if (tag & ZIP)
                    {
                        unsigned char buffer[MAX_DATASIZE]={'\0'};
                        unsigned int retSize = unzip(buffer,MAX_DATASIZE,0);
                        if (callback)
                            callback->doGetCommand(target,buffer,retSize);
                    }
                    else if (contents.size())
                    {
                        if (callback)
                            callback->doGetCommand(target,&contents[0],contents.size());
                    }
                }
            }
        }
        Record * getRecord();
        void encode(void *data,unsigned int len,bool ziptag = false,bool destag = false);
    private:
        unsigned int unzip_size(unsigned int zip_size);
        unsigned int unzip(unsigned char *buffer,unsigned int len,unsigned int offset);
        
        void undes();
        unsigned int zip(void *data,unsigned int len,unsigned int offset);
		
        void des();
    };
};