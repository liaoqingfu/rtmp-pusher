#include "sdl_display.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <sys/time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#ifdef __cplusplus
}
#endif

// 只有一个实例

/* 视频输出 */
SDL_Texture		*pFrameTexture	= NULL;
SDL_Window		*pWindow = NULL;
SDL_Renderer 	*pRenderer = NULL; /* = SDL_CreateRenderer(pWindow, -1, 0); */

SDL_Rect	rect;
struct SwsContext	*pSwsCtx = NULL;
AVFrame* pFrameYUV = NULL;
enum AVPixelFormat sPixFmt = AV_PIX_FMT_YUV420P;


int SDL2DisplayInit(int width, int height, enum AVPixelFormat pix_fmt)
{
	pWindow = SDL_CreateWindow( "Video Window",
					   SDL_WINDOWPOS_UNDEFINED,
					   SDL_WINDOWPOS_UNDEFINED,
					   width, height,
					   SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL );
	if ( !pWindow )
	{
		fprintf( stderr, "SDL: could not set video mode - exiting\n" );
		return -1;
	}
	
	SDL_RendererInfo	info;
	pRenderer = SDL_CreateRenderer( pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	if ( !pRenderer )
	{
		av_log( NULL, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError() );
		pRenderer = SDL_CreateRenderer( pWindow, -1, 0 );
	}
	if ( pRenderer )
	{
		if ( !SDL_GetRendererInfo( pRenderer, &info ) )
			av_log( NULL, AV_LOG_VERBOSE, "Initialized %s renderer.\n", info.name );
	}

	pFrameTexture = SDL_CreateTexture( pRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
	if(!pFrameTexture)
	{
		fprintf( stderr, "SDL: SDL_CreateTexture failed\n" );
		return -1;
	}
	printf("pix_fmt = %d, %d\n", pix_fmt, AV_PIX_FMT_YUV420P);
	// ffmepg decode output format为YUV420
	pSwsCtx = sws_getContext
			  (
			width, height, pix_fmt,
			width, height, AV_PIX_FMT_YUV420P,
			SWS_BICUBIC,
			NULL,
			NULL,
			NULL
			  );
	if(!pSwsCtx)
	{
		fprintf( stderr, "SDL: sws_getContext failed\n" );
		return -1;
	}		  
	rect.x	= 0;
	rect.y	= 0;
	rect.w	= width;
	rect.h	= height;

	// 分配用于保存转换后的视频帧
	pFrameYUV = av_frame_alloc();
	if (!pFrameYUV)
	{
		fprintf( stderr, "av_frame_alloc pFrameYUV  failed!\n" );
		return -1;
	}

	int numBytes = avpicture_get_size( AV_PIX_FMT_YUV420P, width,  height );
	uint8_t* buffer = (uint8_t *) av_malloc( numBytes * sizeof(uint8_t) );

	avpicture_fill( (AVPicture *) pFrameYUV, buffer, AV_PIX_FMT_YUV420P, width, height );

	return 0;
}


static uint64_t getNowTime()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void SDL2Display(AVFrame *pVideoFrame, int height)
{
	static uint64_t sPreTime = 0;
	static uint64_t sPrePts = 0;
	uint64_t curTime = getNowTime();
	AVFrame *pDisplayFrame;
	//printf( "play pts = %lu\n", pVideoFrame->pts);

	if(!pVideoFrame)
	{
		printf( "pVideoFrame is null\n");
		return;
	}
	//printf( "duration pts dif = %d,  t = %d\n",  pVideoFrame->pts - sPrePts,  curTime - sPreTime);
	sPreTime = curTime;
	sPrePts = pVideoFrame->pts;
	//printf( "fix_format1 %d, %d\n", pFrameYUV->format, pVideoFrame->format);
	if(pFrameYUV)
	{
		sws_scale			// 经过scale，pts信息被清除？
			(
				pSwsCtx,
				(uint8_t const * const *) pVideoFrame->data,
				pVideoFrame->linesize,
				0,
				height,
				pFrameYUV->data,
				pFrameYUV->linesize
			);
		pDisplayFrame = pFrameYUV;
	}
	else
	{
		pDisplayFrame = pVideoFrame;
	}
	
	//printf( "fix_format2 %d, %d\n", pFrameYUV->format, pVideoFrame->format);
	
	/*
	 * 视频帧直接显示
	 * //iPitch 计算yuv一行数据占的字节数
	 */
	SDL_UpdateTexture( pFrameTexture, &rect, pDisplayFrame->data[0], pDisplayFrame->linesize[0] );
	SDL_RenderClear( pRenderer );
	SDL_RenderCopy( pRenderer, pFrameTexture, &rect, &rect );
	SDL_RenderPresent( pRenderer );
}

void SDL2DisplayDestory()
{
	if(pSwsCtx)
		sws_freeContext(pSwsCtx);  
	if(pFrameTexture)
		SDL_DestroyTexture( pFrameTexture );
	if(pRenderer)
		SDL_DestroyRenderer(pRenderer);
	if(pWindow)
		SDL_DestroyWindow(pWindow);
	if(pFrameYUV)
		av_free( pFrameYUV );	
}


