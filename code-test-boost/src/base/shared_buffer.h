#ifndef _SHARED_BUFFER_H_
#define _SHARED_BUFFER_H_

#include "buffer.h"
#include <vector>

class SharedBuffer:
        public Buffer
{



public:
    explicit SharedBuffer(int size = 2048);
    virtual ~SharedBuffer();

    virtual unsigned char * Data();
    virtual int Size();
    virtual bool Add(unsigned char * data, int len);
    virtual void Clear();
    virtual Buffer *Clone();
protected:
    unsigned char *buf_;
	int capacity_;
	int size_;
	int count;
};
#endif

