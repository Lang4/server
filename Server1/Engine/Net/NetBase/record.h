#pragma once
#include <vector>
#include <string>
#include <list>
/**
 * windows network epoll 
 */
namespace mynet{
    /**
	 * Record      
	 */
    class Record{
    public:
        Record(void *cmd,unsigned int len)
        {
            contents = new unsigned char[len];
            memcpy(contents,cmd,len);
            contentSize = len;
            offset = 0;
        }
        Record()
        {
            offset = 0;
            contentSize = 0;
            contents = NULL;
        }
        ~Record()
        {
            if (contents) delete contents;
            contents = NULL;
        }
        virtual unsigned int recv(void *buffer,unsigned int len)
        {
            if (empty()) return 0;
            len = leftsize() < len ? leftsize(): len;
            memcpy(buffer,contents + offset,len);
            offset += len;
            return len;
        }
        unsigned int leftsize()
        {
            return contentSize - offset;
        }
        template<class CONNECTION>
        bool sendOver(CONNECTION *connection)
        {
            unsigned int leftSize = leftsize();
            if ( 0 == leftSize) return true;
            int sendLen = connection->send(contents + offset,leftSize);
            offset += sendLen;
            if (sendLen < leftSize) return false;
            return true;
        }
        unsigned int offset;
        bool empty()
        {
            return offset == contentSize;
        }
        unsigned int contentSize;
        unsigned char *contents;
    };
};