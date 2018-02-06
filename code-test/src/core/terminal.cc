#include "terminal.h"
#include <cstring>

Terminal::Terminal(int id, ETerminalType terminalType)		// 必须一开始就指定ID和终端类型
	:id_(id_), 
	terminalType_(terminalType)
{

}
Terminal::~Terminal()
{

}
void Terminal::SetSocketHandle(int socketHandle)					// 设置socket句柄
{
	socketHandle_ = socketHandle;
}
int Terminal::GetSocketHandle()
{
	return socketHandle_;
}


void Terminal::SetId(int id)
{
	id_ = id;
}
int Terminal::GetId()
{
	return id_;
}

void Terminal::Send(Buffer::BufferPtr &buf)
{
	// 通过socket将数据发送出去
	// 这里测试先直接发送，真正工程使用引入线程池来进行
	if(!buf)
	{
		if(buf->Size() < BUFFER_LENGTH)
			memcpy(&buffer_[0], buf->Data(), buf->Size());		// 这里只是为了测试性能
		
		printf("id = %d, type = %d, send buffer len = %d\n", id_, terminalType_, buf->Size());
		buf.reset();		// 释放内存
	}
}

Terminal::ETerminalType Terminal::GetTerminalType()
{
	return terminalType_;
}

void Terminal::SetTerminalType(ETerminalType terminalType)
{
	terminalType_ = terminalType;
}


