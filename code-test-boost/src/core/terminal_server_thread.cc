#include "terminal_server_thread.h"
#include <cstring>
TerminalServerThread::TerminalServerThread(TerminalStreamObserver::TerminalObserverPtr &terminalObserver,
													const int port)
	:terminalObserver_(terminalObserver),
	port_(port),
	thread_(std::bind(&TerminalServerThread::Loop, this))
{}
TerminalServerThread::~TerminalServerThread()
{
	exiting_ = true;
  	thread_.join();
}
void TerminalServerThread::Loop()
{
	Select();
}
int TerminalServerThread::startLoop()
{
	assert(!thread_.started());
  	thread_.start();
	return 0;
}


void TerminalServerThread::Select()
{
	int serv_sock;
	int clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	

	socklen_t adr_sz_;
	int fd_max, str_len, fd_num;
	struct timeval	timeout;
	fd_set		reads, cpy_reads;

	int i = 0;
	serv_sock = socket( PF_INET, SOCK_STREAM, 0 );
	memset( &serv_adr, 0, sizeof(serv_adr) );
	serv_adr.sin_family		= AF_INET;
	serv_adr.sin_addr.s_addr	= htonl( INADDR_ANY );
	serv_adr.sin_port		= htons( port_ );

	printf("");

	if ( bind( serv_sock, (struct sockaddr *) &serv_adr, sizeof(serv_adr) ) == -1 )
		printf( "bind() error" );
	if ( listen( serv_sock, 5 ) == -1 )
		printf( "listen() error" );

	FD_ZERO( &reads );
	FD_SET( serv_sock, &reads );
	fd_max = serv_sock;
	printf("fd_max = %d, port_ = %d\n", fd_max, port_);
	while ( !exiting_ )
	{
		cpy_reads	= reads;
		timeout.tv_sec	= 5;
		timeout.tv_usec = 5000;

		if ( (fd_num = select( fd_max + 1, &cpy_reads, 0, 0, &timeout ) ) == -1 )
			break;

		if ( fd_num == 0 )
		{
			printf("fd_num = 0\n");
			continue;
		}
		for ( i = 0; i < fd_max + 1; i++ )
		{
			if ( FD_ISSET( i, &cpy_reads ) )
			{
				if ( i == serv_sock ) /* connection request! */
				{
					socklen_t  adr_sz		= sizeof(clnt_adr);
					clnt_sock	= accept( serv_sock, (struct sockaddr *) &clnt_adr, &adr_sz );
					FD_SET( clnt_sock, &reads );
					if ( fd_max < clnt_sock )
						fd_max = clnt_sock;
					printf( "connected client: %d \n", clnt_sock );
					// 新建一个终端并加入到组织中去
					Terminal::ETerminalType terminalType;
					if(clnt_sock %2  == 0)
					{
						terminalType  = Terminal::eTerminalMp3;
					}
					else
					{
						terminalType  = Terminal::eTerminalAac;
					}
					Terminal::TerminalPtr terminal = std::make_shared<Terminal>(clnt_sock,terminalType);
					terminal->SetSocketHandle(clnt_sock);
					terminalObserver_->RegisterTerminal(clnt_sock, terminal);
				}
				else  
				{                                        		/* read message! */
					str_len = read( i, buf_, NET_BUF_SIZE );
					buf_[str_len] = '\0';
					// 终端每收到一帧数据就回复给服务器,报告终端ID以及数据累计长度
					//printf("buf_ = %s\n", buf_); 				
					if ( str_len == 0 )                     	/* close request! */
					{
						str_len = write( i, buf_, NET_BUF_SIZE );
						printf("str_len1 = %d\n", str_len);
						FD_CLR( i, &reads );
						close( i );
						printf( "closed client: %d \n", i );
						// 删除一个终端
						terminalObserver_->UnregisterTerminal(i);
						str_len = write( i, buf_, NET_BUF_SIZE );
						printf("str_len2 = %d\n", str_len);
					}
					else  
					{
						//write( i, buf_, NET_BUF_SIZE );      		/* echo! */
					}
					
				}
			}
		}
	}
	close( serv_sock );
	printf("close( serv_sock )\n");
}