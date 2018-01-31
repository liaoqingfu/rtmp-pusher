/*
 * tutorial02.c
 * A pedagogical video player that will stream through every video frame as fast as it can.
 *
 * This tutorial was written by Stephen Dranger (dranger@gmail.com).
 *
 * Code based on FFplay, Copyright (c) 2003 Fabrice Bellard,
 * and a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)
 * Tested on Gentoo, CVS version 5/01/07 compiled with GCC 4.1.1
 *
 * Use the Makefile to build all examples.
 *
 * Run using
 * tutorial02 myvideofile.mpg
 *
 * to play the video stream on your pWindow.
 */


/* 必须记住，使用C++编译器时一定要使用extern "C"，否则找不到链接文件 */
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <sys/time.h>


#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

#include "sdl_display.h"
#include "sdl_audio_out.h"


#ifdef __cplusplus
}
#endif





uint64_t getNowTime()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}


/*
 * 编译方式
 * gcc -w -o 3-1-main 3-1-main.c packet_queue.c -lSDL2 -lavcodec -lswscale -lavutil     \
 * -lavformat -lswresample -I/usr/local/ffmpeg/include -L/usr/local/ffmpeg/lib
 */
int main( int argc, char *argv[] )
{
	AVFormatContext *pFormatCtx = NULL;
	int		i;
	int		audioStream;
	int		videoStream;

	// 视频解码 
	AVCodecContext	*pVideoCodecCtx = NULL;         // 视频解码器上下文 
	AVCodec		*pVideoCodec	= NULL;         	// 视频解码器 
	AVFrame		*pVideoFrame	= NULL;

	// 音频解码
	AVCodecContext	*pAudioCodecCtx		= NULL; 	// 视频解码器上下文
	AVCodec		*pAudioCodec		= NULL; 		// 视频解码器 
	AVFrame		*pAudioFrame		= NULL;
	int			firstGotAudioFrame	= 0;
	int64_t		audioFirstPts;
	int 		audioBufferFrames  = 0;

	// 播放的起始时间
	uint64_t 	playStartTime;

	AVPacket	packet;
	int			frameFinished; // 是否获取帧 

	AVDictionary *optionsDict	= NULL;
	

	// 控制事件
	SDL_Event event;


	if ( argc < 2 )
	{
		fprintf( stderr, "Usage: test <file>\n" );
		exit( 1 );
	}
	
	// 注册复用器,编码器等
	av_register_all();

	// 初始化SDL模块
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER ) )
	{
		fprintf( stderr, "Could not initialize SDL - %s\n", SDL_GetError() );
		exit( 1 );
	}

	// 打开多媒体文件
	if ( avformat_open_input( &pFormatCtx, argv[1], NULL, NULL ) != 0 )
	{
		fprintf( stderr, "avformat_open_input failed\n" );
		return(-1);     
	}
	// 获取多媒体流信息
	if ( avformat_find_stream_info( pFormatCtx, NULL ) < 0 )
	{
		fprintf( stderr, "avformat_find_stream_info failed\n" );
		return(-1);     
	}
	
	// 打印有关输入或输出格式的详细信息, 该函数主要用于debug
	av_dump_format( pFormatCtx, 0, argv[1], 0 );

	// 查找音频/视频流 
	audioStream	= -1;
	videoStream	= -1;
	for ( i = 0; i < pFormatCtx->nb_streams; i++ )
	{
		if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
		{
			audioStream = i;
		}
		else if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
		{
			videoStream = i;
		}
	}

	if ( audioStream == -1 )
	{
		fprintf( stderr, "Can't find the audio stream\n" );
		return(-1);
	}
	if ( videoStream == -1 )
	{
		fprintf( stderr, "Can't find the video stream\n" );
		return(-1);
	}

	// ----------------------处理音频 ----------------------------------
	// 获取音频解码器上下文
	pAudioCodecCtx = pFormatCtx->streams[audioStream]->codec;

	// 通过解码器ID创建一个解码器
	pAudioCodec = avcodec_find_decoder( pAudioCodecCtx->codec_id );
	if ( pAudioCodec == NULL )
	{
		fprintf( stderr, "Unsupported audio codec!\n" );
		return(-1);                                    
	}
	// 使用 AVCodec初始化AVCodecContext 
	if ( avcodec_open2( pAudioCodecCtx, pAudioCodec, NULL ) < 0 )
	{
		fprintf( stderr, "avcodec_open2 audio  failed!\n" );
		return(-1);                                    
	}

	// ----------------------处理视频 ----------------------------------
	// 获取视频解码器上下文
	pVideoCodecCtx = pFormatCtx->streams[videoStream]->codec;

	// 通过解码器ID创建一个解码器 
	pVideoCodec = avcodec_find_decoder( pVideoCodecCtx->codec_id );
	if ( pVideoCodec == NULL )
	{
		fprintf( stderr, "Unsupported video codec!\n" );
		return(-1);
	}

	// 使用 AVCodec初始化AVCodecContext 
	if ( avcodec_open2( pVideoCodecCtx, pVideoCodec, &optionsDict ) < 0 )
	{
		fprintf( stderr, "avcodec_open2 video  failed!\n" );
		return(-1);  
	}
	// 分配视频帧
	pVideoFrame = av_frame_alloc();
	if(!pVideoFrame)
	{
		fprintf( stderr, "av_frame_alloc pVideoFrame  failed!\n" );
		return(-1);
	}
	
	//初始化audio 输出
	SDL2AudioOutInit(pAudioCodecCtx);
	printf( "width = %d, height = %d\n", pVideoCodecCtx->width, pVideoCodecCtx->height );
	SDL2DisplayInit(pVideoCodecCtx->width, pVideoCodecCtx->height, pVideoCodecCtx->pix_fmt);

	

	playStartTime = getNowTime();
	/* 使用音频PTS去控制读取速度，音频缓存5帧再播放，视频有数据则直接播放 */
	while ( av_read_frame( pFormatCtx, &packet ) >= 0 )
	{
		if ( packet.stream_index == videoStream ) /* 是一个视频帧 */
		{
			// 解视频帧
			avcodec_decode_video2( pVideoCodecCtx, pVideoFrame, &frameFinished, &packet );
			// 确定已经获取到视频帧
			if ( frameFinished )
			{
				SDL2Display(pVideoFrame, pVideoCodecCtx->height);
			}
			// 释放packet占用的内存 
			av_free_packet( &packet );
		}
		else if ( packet.stream_index == audioStream )         /* 是一个音频帧 */
		{
			/* 如果为音频帧则先入队列 */
			SDL2AudioPushPacket( &packet);
			if ( 1 == firstGotAudioFrame )                  /* 通过pts去控制码流读取速率 */
			{
				while ( (packet.pts - audioFirstPts) > (getNowTime() - playStartTime) + 500 )
				{
					/*
					printf( " (packet.pts - audioFirstPts) = %u,  (getNowTime() - playStartTime) = %u,dif = %ld\n",
					 	(packet.pts - audioFirstPts), (getNowTime() - playStartTime),
					 	(packet.pts - audioFirstPts) - (getNowTime() - playStartTime) );
					 	*/
					SDL_Delay( 30 );
				}
			}
			//将audio packet入队列
			if ( 0 == firstGotAudioFrame )
			{
				firstGotAudioFrame	= 1;
				audioFirstPts		= packet.pts;
									
			}
			if(audioBufferFrames < 10)
			{
				if(audioBufferFrames++ >8)
				{
					audioBufferFrames = 10;				//缓存一些帧再开启音频
					SDL2AudioOutPause(0);	
				}
			}
		}
		else  
		{
			av_free_packet( &packet );
		}

		SDL_PollEvent( &event );
		switch ( event.type )
		{
		case SDL_QUIT:
			SDL_Quit();
			exit( 0 );
			break;
		default:
			break;
		}
	}

	

	/* Free the YUV frame */
	av_free( pVideoFrame );
	
	/* Close the codec */
	avcodec_close( pVideoCodecCtx );

	/* Close the video file */
	avformat_close_input( &pFormatCtx );
	SDL2DisplayDestory();
	return(0);
}


