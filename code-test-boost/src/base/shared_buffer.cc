#include "shared_buffer.h"
#include <string.h>

using namespace std;


SharedBuffer::SharedBuffer(int size)
{
    buf_ = new unsigned char[size];
    capacity_ = size;
	size_ = 0;
	// printf("SharedBuffer size = %d\n", size);
	Buffer::ConstructorCount++;
}
SharedBuffer::~SharedBuffer()
{
	if(buf_)
	{
	 	//printf("~SharedBuffer count = %d,%d\n",  ++Buffer::DestructorCount, Buffer::ConstructorCount);
		delete [] buf_;
		buf_ = nullptr;
	}
	else
	{
		printf("~SharedBuffer again\n");
	}
}

unsigned char * SharedBuffer::Data()
{
    return buf_;
}
int SharedBuffer::Size()
{
    return size_;
}

bool SharedBuffer::Add(unsigned char *data, int len)
{
	if((!buf_) || (!data) || (len < 0))
	{
		printf("add failed, buf_ = 0x%x, data = 0x%x, len = %d\n", buf_, data, len);
		return false;
	}

    if(len + size_ > capacity_)	//ÖØÐÂÉêÇëÄÚ´æ
   	{
   		printf("new buffer again, len = %d, size_ = %d, capacity_ = %d\n", len, size_, capacity_);
		capacity_ = ((len + size_)>>11 + 1) * 2048;
		unsigned char *tempBuf  = new unsigned char[capacity_];
		if(!tempBuf)
		{
			printf("new tempBuf faile\n");
			return false;
		}
		memcpy(tempBuf, buf_, size_);
		delete [] buf_;
		buf_ = tempBuf;
	}
	memcpy(buf_ + size_, data, len);
	size_ += len;

	return true;
}

void SharedBuffer::Clear()
{
    size_ = 0;
}

Buffer *SharedBuffer::Clone()
{
	Buffer *tbBfuffer = new SharedBuffer(capacity_);
	count++;
	if(!tbBfuffer)
		return nullptr;
	
	if(tbBfuffer->Add(buf_, size_))
    {
    	return tbBfuffer;
	}
	else
	{
		delete tbBfuffer;
		return nullptr;
	}
	
	
}

