#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define BUF_SIZE 8096
void error_handling(char *message);

//char s_serv_ip[] ={"192.168.1.122"};
char s_serv_ip[] ={"127.0.0.1"};

int s_sin_port = 9010;
int s_sleep_time = 10;
int s_client_num = 20;
int s_recv_data_len = 12;     //单位M


uint64_t getNowTime()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}


void * thread_loop(void *arg) 
{
    uint64_t 	start_time;
    
    int recv_total = 0;
    pthread_t pid = *((pthread_t *)(arg));
    int sock;
    char message[BUF_SIZE];
	int str_len;
	struct sockaddr_in serv_adr;

    start_time = getNowTime();
    printf("thread_loop into, pid = %lu\n", pthread_self());
	sock=socket(PF_INET, SOCK_STREAM, 0);   
	if(sock==-1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr(s_serv_ip);
	serv_adr.sin_port=htons(s_sin_port);
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("connect() error!");
	else
		puts("Connected...........");
	sprintf(message, "%lu.aac", pthread_self());	
	FILE	* outAacFile		= fopen( message, "wb" );	
	
	while(1) 
	{
		str_len=read(sock, message, BUF_SIZE);
		if(str_len == 0)
			break;
		
		//printf("thread pid[%d] from server: data len = %d, total = %d\n", pid, str_len, recv_total);
		fwrite( message, 1, str_len, outAacFile ); 
		
		recv_total += str_len;
		sprintf(message, "dataTotal = %d", recv_total);			
		write(sock, message, strlen(message));
		if(recv_total % (1024*50) == 0)
			printf("pid[%lu] read server: %d bytes\n", pthread_self(), recv_total);
	}

	printf("thread_loop exit, pid = %d, consume time = %lu ms\n", pid, getNowTime() - start_time);
	close(sock);
	fclose(outAacFile);
}

int main(int argc, char *argv[])
{
	int i;
	pthread_t tidp[100];

	if(argc!=3) {
		printf("Usage : %s <client_num> <port>\n", argv[0]);
		exit(1);
	}
	
    s_client_num = atoi(argv[1]);
	s_sin_port = atoi(argv[2]);

    for(i = 0; i < s_client_num; i++)
    {
        /* 创建线程pthread */
        if ((pthread_create(&tidp[i], NULL, thread_loop, (void*)(&tidp[i]))) == -1)
        {
            printf("create error!\n");
            return 1;
        }
        printf("pthread_create thread pid = %d\n", tidp[i]);
	}


	/* 等待线程pthread释放 */
    if (pthread_join(tidp[0], NULL))                  
    {
        printf("thread is not exit...\n");
        return -2;
    }
    printf("thread is  exit... tidp[0] = %d\n", tidp[0]);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
