#include "buffer.h"
#include "shared_buffer.h"

int Buffer::DestructorCount = 0;
int Buffer::ConstructorCount = 0;

Buffer * Buffer::CreateInstance(int size , const char * filter)
{
    Buffer * tb_buffer = nullptr;
    if(filter == nullptr )
	{
        tb_buffer = new SharedBuffer(size);
    }
    return tb_buffer;
}

