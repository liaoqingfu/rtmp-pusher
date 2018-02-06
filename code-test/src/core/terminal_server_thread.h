#ifndef _TERMINAL_SERVER_THREAD_H_
#define _TERMINAL_SERVER_THREAD_H_

#include "thread.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

// 子终端连接超级终端的server程序
const int NET_BUF_SIZE = 2048

class TerminalServerThread
{
public:
	TerminalServerThread(int port);
	~TerminalServerThread();
	void Loop();
	int startLoop();
	void Select();
private:
	Thread 	thread_;
	bool 	exiting_; 
	int port_;
	int serv_sock_;
	int clnt_sock_;
	struct sockaddr_in serv_adr_, clnt_adr_;
	struct timeval timeout_;
	fd_set reads_, cpy_reads_;

	socklen_t adr_sz_;
	int fd_max_, str_len_, fd_num_;
	char buf_[NET_BUF_SIZE];
};
#endif
