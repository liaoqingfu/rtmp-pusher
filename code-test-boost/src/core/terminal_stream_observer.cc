#include "terminal_stream_observer.h"

TerminalStreamObserver::TerminalStreamObserver(AudioFramePool::AudioFramePoolPtr &audioFramePool)
	:m_thread(std::bind(&TerminalStreamObserver::Loop, this)),
	audioFramePool_(audioFramePool),
	m_exiting(false)
{

}
TerminalStreamObserver::~TerminalStreamObserver()
{
	m_exiting = true;
  	m_thread.join();
}
int TerminalStreamObserver::RegisterTerminal(int id, std::shared_ptr<Terminal> &terminal)		// 注册帧池 观察者模式
{
	int ret = 0;
	
	MutexLockGuard lock(mutex_);
	// 先查找是否已经注册
	if (terminalMap_.find(id) != terminalMap_.end())  
	{
		// 说明已经注册过了
		return 1;
	}
	else
	{
		if(terminal != nullptr)
		{
			terminalMap_.insert(std::make_pair(id, terminal));  
			ret = 0;
		}
		else
		{
			ret = -1;
		}
	}	
	return ret;	

}
int TerminalStreamObserver::UnregisterTerminal(int id)
{
	MutexLockGuard lock(mutex_);
	if(terminalMap_.erase(id) == 1)		// 删除成功
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

void TerminalStreamObserver::NotifyAll(Terminal::ETerminalType terminalType)
{
	MutexLockGuard lock(mutex_);

	// 终端调用自己的函数发送数据
	//printf("NotifyAll1 \n");
	// 每个终端保存自己的数据，并使用线程池来发送数据
	for (auto mapItem = terminalMap_.begin(); mapItem != terminalMap_.end(); ++mapItem) 
	{  
		Terminal::ETerminalType type = mapItem->second->GetTerminalType();
		//printf("NotifyAll2 terminalType = %d \n", type);
		if(terminalType == type)
        {
        	//printf("NotifyAll2 \n");
        	mapItem->second->Send(frameBuf_);
        }	
    }  
}

void TerminalStreamObserver::Loop()
{
	int ret;
	while(!m_exiting)
	{
		ret = audioFramePool_->TakeFrame(AudioFramePool::eAudioMp3, frameBuf_);
		if(ret == 0)
		{
			NotifyAll(Terminal::eTerminalMp3);
			frameBuf_.reset();
		}
		else
		{
			//printf("ret1 = %d \n", ret);
		}
		ret = audioFramePool_->TakeFrame(AudioFramePool::eAudioAac, frameBuf_);
		if(ret == 0)
		{
			NotifyAll(Terminal::eTerminalAac);
			frameBuf_.reset();
		}
		else
		{
			//printf("ret2 = %d \n", ret);
		}
		usleep(22000);
	}
}

int TerminalStreamObserver::startLoop()
{
  	assert(!m_thread.started());
  	m_thread.start();
}



