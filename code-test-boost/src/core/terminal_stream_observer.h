#ifndef _TERMINAL_STREAM_OBSERVER_H_
#define _TERMINAL_STREAM_OBSERVER_H_
#include <boost/shared_ptr.hpp>
#include "thread.h"
#include "audio_frame_pool.h"
#include "terminal.h"
#include "mutex.h"
using namespace darren;
using namespace std;

// 处理流数据
class TerminalStreamObserver// : public Thread
{
public:
	typedef boost::shared_ptr<TerminalStreamObserver> TerminalObserverPtr;;
	TerminalStreamObserver(AudioFramePool::AudioFramePoolPtr audioFramePool);
	virtual ~TerminalStreamObserver();
	int RegisterTerminal(int id, Terminal *terminal);			// 注册帧池 观察者模式
	int UnregisterTerminal(int id);
	void NotifyAll(Terminal::ETerminalType terminalType);
	void Loop();
	int startLoop();

private:
	AudioFramePool::AudioFramePoolPtr audioFramePool_; 
	
	std::map<int, Terminal *> terminalMap_;		// 通过ID去作为唯一标识
	mutable MutexLock mutex_;
	Buffer::BufferPtr frameBuf_;
	Thread m_thread;
	bool m_exiting;
};
#endif
