#ifndef _BUFFER_H_
#define _BUFFER_H_
#include <cstdlib>
#include <stdint.h>
#include <memory>
using namespace std;

class Buffer
{
public:
	typedef std::shared_ptr<Buffer> BufferPtr;
    static Buffer * CreateInstance(int      size = 2048, const char * filter = nullptr);
    static int DestructorCount;
    static int ConstructorCount;
    Buffer(){}
    virtual ~Buffer(){}
    virtual unsigned char * Data() { return nullptr;}
    virtual int Size() {return 0;}
    virtual bool Add(unsigned char * data, int len){return false;}
    virtual void Clear() {}
    virtual Buffer * Clone() { return nullptr;}
};

#endif //__TB_BUFFER_H__

