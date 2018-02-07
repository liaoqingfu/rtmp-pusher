#ifndef _TERMINAL_SERVER_THREAD_H_
#define _TERMINAL_SERVER_THREAD_H_


#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include "thread.h"
#include "terminal_stream_observer.h"

using namespace darren;
using namespace std;

// 子终端连接超级终端的server程序
const int NET_BUF_SIZE = 2048;

class TerminalServerThread
{
public:
	typedef boost::shared_ptr<TerminalServerThread> TerminalServerPtr;
	TerminalServerThread(TerminalStreamObserver::TerminalObserverPtr terminalObserver, const int port = 9005);

	~TerminalServerThread();
	void Loop();
	int startLoop();
	void Select();
private:
	Thread 	thread_;
	bool 	exiting_; 
	int port_;
	
	char buf_[NET_BUF_SIZE];


	TerminalStreamObserver::TerminalObserverPtr terminalObserver_;
};
#endif
