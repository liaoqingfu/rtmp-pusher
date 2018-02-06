#include "terminal_server_thread.h"

TerminalServerThread::TerminalServerThread(int port)
{}
~TerminalServerThread::TerminalServerThread()
{}
void TerminalServerThread::Loop()
{}
int TerminalServerThread::startLoop()
{
	return 0
}
void TerminalServerThread::Select()
{
	int i = 0;
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(s_sin_port);
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");

	FD_ZERO(&reads_);
	FD_SET(serv_sock, &reads_);
	fd_max=serv_sock;

	while(exiting_)
	{
		cpy_reads_=reads_;
		timeout.tv_sec=5;
		timeout.tv_usec=5000;

		if((fd_num=select(fd_max+1, &cpy_reads_, 0, 0, &timeout))==-1)
			break;
		
		if(fd_num==0)
			continue;

		for(i=0; i<fd_max+1; i++)
		{
			if(FD_ISSET(i, &cpy_reads_))
			{
				if(i==serv_sock)     // connection request!
				{
					adr_sz=sizeof(clnt_adr);
					clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
					FD_SET(clnt_sock, &reads_);
					if(fd_max<clnt_sock)
						fd_max=clnt_sock;
					printf("connected client: %d \n", clnt_sock);
				}
				else    // read message!
				{
					str_len=read(i, buf, BUF_SIZE);
					//printf("str_len = %d\n", str_len);
					if(str_len==0)    // close request!
					{
						FD_CLR(i, &reads_);
						close(i);
						printf("closed client: %d \n", i);
					}
					else
					{
						write(i, buf, BUF_SIZE);    // echo!
					}
				}
			}
		}
	}
	close(serv_sock);
}