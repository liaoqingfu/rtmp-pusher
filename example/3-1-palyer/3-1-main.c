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
 * to play the video stream on your screen.
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

#ifdef __cplusplus
}
#endif


#include "packet_queue.h"

#define AVCODE_MAX_AUDIO_FRAME_SIZE	192000  /* 1 second of 48khz 32bit audio */
#define SDL_AUDIO_BUFFER_SIZE		1024    /*  */


#define ERR_STREAM	stderr
#define OUT_SAMPLE_RATE 44100

AVFrame wanted_frame;


PacketQueue audio_queue;      /* 音频队列 */
void audio_callback( void *userdata, Uint8 *stream, int len );


int audio_decode_frame( AVCodecContext *pAudioCodecCtx, uint8_t *audio_buf, int buf_size );


/*
 * 注意userdata是前面的AVCodecContext.
 * len的值常为2048,表示一次发送多少。
 * audio_buf_size：一直为样本缓冲区的大小，wanted_spec.samples.（一般每次解码这么多，文件不同，这个值不同)
 * audio_buf_index： 标记发送到哪里了。
 * 这个函数的工作模式是:
 * 1. 解码数据放到audio_buf, 大小放audio_buf_size。(audio_buf_size = audio_size;这句设置）
 * 2. 调用一次callback只能发送len个字节,而每次取回的解码数据可能比len大，一次发不完。
 * 3. 发不完的时候，会len == 0，不继续循环，退出函数，继续调用callback，进行下一次发送。
 * 4. 由于上次没发完，这次不取数据，发上次的剩余的，audio_buf_size标记发送到哪里了。
 * 5. 注意，callback每次一定要发且仅发len个数据，否则不会退出。
 * 如果没发够，缓冲区又没有了，就再取。发够了，就退出，留给下一个发，以此循环。
 * 个变量设置为static就是为了保存上次数据，也可以用全局变量，但是感觉这样更好。
 */
void audio_callback( void *userdata, Uint8 *stream, int len )
{
	AVCodecContext	*pAudioCodecCtx = (AVCodecContext *) userdata;
	int		len1		= 0;
	int		audio_size	= 0;

	/*
	 * 注意是static
	 * 为什么要分那么大？
	 */
	static uint8_t		audio_buf[(AVCODE_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	static unsigned int	audio_buf_size	= 0;
	static unsigned int	audio_buf_index = 0;

	/* 初始化stream，每次都要。 */
	SDL_memset( stream, 0, len );

	while ( len > 0 )
	{
		if ( audio_buf_index >= audio_buf_size )
		{
			/*
			 * 数据全部发送，再去获取
			 * 自定义的一个函数
			 */
			audio_size = audio_decode_frame( pAudioCodecCtx, audio_buf, sizeof(audio_buf) );
			if ( audio_size < 0 )
			{
				/* 错误则静音 */
				audio_buf_size = 1024;
				memset( audio_buf, 0, audio_buf_size );
			}else  {
				audio_buf_size = audio_size;
			}
			audio_buf_index = 0;    /* 回到缓冲区开头 */
		}

		len1 = audio_buf_size - audio_buf_index;
/*          printf("len1 = %d\n", len1); */
		if ( len1 > len )               /* len1常比len大，但是一个callback只能发len个 */
		{
			len1 = len;
		}

		/*
		 * 新程序用 SDL_MixAudioFormat()代替
		 * 混合音频， 第一个参数dst， 第二个是src，audio_buf_size每次都在变化
		 */
		SDL_MixAudio( stream, (uint8_t *) audio_buf + audio_buf_index, len, SDL_MIX_MAXVOLUME );
		/*
		 *
		 * memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
		 */
		len		-= len1;
		stream		+= len1;
		audio_buf_index += len1;
	}
}


/* 对于音频来说，一个packet里面，可能含有多帧(frame)数据。 */

int audio_decode_frame( AVCodecContext *pAudioCodecCtx,
			uint8_t *audio_buf, int buf_size )
{
	AVPacket	packet;
	AVFrame		*frame;
	int		got_frame;
	int		pkt_size = 0;
/*     uint8_t    *pkt_data = NULL; */
	int		decode_len;
	int		try_again	= 0;
	long long	audio_buf_index = 0;
	long long	data_size	= 0;
	SwrContext	*swr_ctx	= NULL;
	int		convert_len	= 0;
	int		convert_all	= 0;

	if ( packet_queue_get( &audio_queue, &packet, 1 ) < 0 )
	{
		fprintf( ERR_STREAM, "Get queue packet error\n" );
		return(-1);
	}

/*     pkt_data = packet.data; */
	pkt_size = packet.size;
	/* fprintf(ERR_STREAM, "pkt_size = %d\n", pkt_size); */

	frame = av_frame_alloc();
	while ( pkt_size > 0 )
	{
/*          memset(frame, 0, sizeof(AVFrame)); */
		/*
		 * pAudioCodecCtx:解码器信息
		 * frame:输出，存数据到frame
		 * got_frame:输出。0代表有frame取了，不意味发生了错误。
		 * packet:输入，取数据解码。
		 */
		decode_len = avcodec_decode_audio4( pAudioCodecCtx,
						    frame, &got_frame, &packet );
		if ( decode_len < 0 ) /* 解码出错 */
		{
			/* 重解, 这里如果一直<0呢？ */
			fprintf( ERR_STREAM, "Couldn't decode frame\n" );
			if ( try_again == 0 )
			{
				try_again++;
				continue;
			}
			try_again = 0;
		}


		if ( got_frame )
		{
			/*              //用定的音频参数获取样本缓冲区大小
			 *            data_size = av_samples_get_buffer_size(NULL,
			 *                    pAudioCodecCtx->channels, frame->nb_samples,
			 *                    pAudioCodecCtx->sample_fmt, 1);
			 *
			 *            assert(data_size <= buf_size);
			 * //               memcpy(audio_buf + audio_buf_index, frame->data[0], data_size);
			 */
			/*
			 * chnanels: 通道数量, 仅用于音频
			 * channel_layout: 通道布局。
			 * 多音频通道的流，一个通道布局可以具体描述其配置情况.通道布局这个概念不懂。
			 * 大概指的是单声道(mono)，立体声道（stereo), 四声道之类的吧？
			 * 详见源码及：https://xdsnet.gitbooks.io/other-doc-cn-ffmpeg/content/ffmpeg-doc-cn-07.html#%E9%80%9A%E9%81%93%E5%B8%83%E5%B1%80
			 */


			if ( frame->channels > 0 && frame->channel_layout == 0 )
			{
				/* 获取默认布局，默认应该了stereo吧？ */
				frame->channel_layout = av_get_default_channel_layout( frame->channels );
			}else if ( frame->channels == 0 && frame->channel_layout > 0 )
			{
				frame->channels = av_get_channel_layout_nb_channels( frame->channel_layout );
			}


			if ( swr_ctx != NULL )
			{
				swr_free( &swr_ctx );
				swr_ctx = NULL;
			}

			/*
			 * 设置common parameters
			 * 2,3,4是output参数，4,5,6是input参数。
			 * 重采样
			 */
			swr_ctx = swr_alloc_set_opts( NULL,
						      wanted_frame.channel_layout, (enum AVSampleFormat) (wanted_frame.format), wanted_frame.sample_rate,
						      frame->channel_layout, (enum AVSampleFormat) (frame->format), frame->sample_rate,
						      0, NULL );
			/* 初始化 */
			if ( swr_ctx == NULL || swr_init( swr_ctx ) < 0 )
			{
				fprintf( ERR_STREAM, "swr_init error\n" );
				break;
			}
			/*
			 * av_rescale_rnd(): 用指定的方式队64bit整数进行舍入(rnd:rounding),
			 * 使如a*b/c之类的操作不会溢出。
			 * swr_get_delay(): 返回 第二个参数分之一（下面是：1/frame->sample_rate)
			 * AVRouding是一个enum，1的意思是round away from zero.
			 */


			/*
			 *        int dst_nb_samples = av_rescale_rnd(
			 *                swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
			 *                wanted_frame.sample_rate, wanted_frame.format,
			 *                AVRounding(1));
			 */

			/*
			 * 转换音频。把frame中的音频转换后放到audio_buf中。
			 * 第2，3参数为output， 第4，5为input。
			 * 可以使用#define AVCODE_MAX_AUDIO_FRAME_SIZE 192000
			 * 把dst_nb_samples替换掉, 最大的采样频率是192kHz.
			 */
			convert_len = swr_convert( swr_ctx,
						   &audio_buf + audio_buf_index,
						   AVCODE_MAX_AUDIO_FRAME_SIZE,
						   (const uint8_t * *) frame->data,
						   frame->nb_samples );

			/*
			 * printf("decode len = %d, convert_len = %d\n", decode_len, convert_len);
			 * 解码了多少，解码到了哪里
			 *          pkt_data += decode_len;
			 */
			pkt_size -= decode_len;
			/* 转换后的有效数据存到了哪里，又audio_buf_index标记 */
			audio_buf_index += convert_len; /* data_size; */
			/* 返回所有转换后的有效数据的长度 */
			convert_all += convert_len;
		}
	}
	return(wanted_frame.channels * convert_all * av_get_bytes_per_sample( (enum AVSampleFormat) (wanted_frame.format) ) );
/*     return audio_buf_index; */
}


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

	/* 视频解码 */
	AVCodecContext	*pVideoCodecCtx = NULL;         /* 视频解码器上下文 */
	AVCodec		*pVideoCodec	= NULL;         /* 视频解码器 */
	AVFrame		*pVideoFrame	= NULL;

	/* 音频解码 */
	AVCodecContext	*pAudioCodecCtx		= NULL; /* 视频解码器上下文 */
	AVCodec		*pAudioCodec		= NULL; /* 视频解码器 */
	AVFrame		*pAudioFrame		= NULL;
	int		firstGotAudioFrame	= 0;
	int64_t		audioFirstPts;

	/* 播放的起始时间 */
	uint64_t playStartTime;

	AVPacket	packet;
	int		frameFinished; /* 是否获取帧 */

	AVDictionary		*optionsDict	= NULL;
	struct SwsContext	*sws_ctx	= NULL;


	/* 视频输出 */
	SDL_Texture	*bmp	= NULL;
	SDL_Window	*screen = NULL;
	SDL_Rect	rect;


	/*
	 * 音频输出
	 * SDL
	 */
	SDL_AudioSpec	wanted_spec;
	SDL_AudioSpec	spec;

	/* 控制事件 */
	SDL_Event event;


	if ( argc < 2 )
	{
		fprintf( stderr, "Usage: test <file>\n" );
		exit( 1 );
	}
	/* Register all formats and codecs */
	av_register_all();

	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER ) )
	{
		fprintf( stderr, "Could not initialize SDL - %s\n", SDL_GetError() );
		exit( 1 );
	}

	/* Open video file */
	if ( avformat_open_input( &pFormatCtx, argv[1], NULL, NULL ) != 0 )
		return(-1);     /* Couldn't open file */

	/* Retrieve stream information */
	if ( avformat_find_stream_info( pFormatCtx, NULL ) < 0 )
		return(-1);     /* Couldn't find stream information */

	/* Dump information about file onto standard error */
	av_dump_format( pFormatCtx, 0, argv[1], 0 );

	/* 查找音频/视频流 */
	audioStream	= -1;
	videoStream	= -1;
	for ( i = 0; i < pFormatCtx->nb_streams; i++ )
	{
		if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
		{
			audioStream = i;
		}else if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
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

	/*
	 * 处理音频
	 * 获取音频解码器上下文
	 */
	pAudioCodecCtx = pFormatCtx->streams[audioStream]->codec;

	/* 通过解码器ID创建一个解码器 */
	pAudioCodec = avcodec_find_decoder( pAudioCodecCtx->codec_id );
	if ( pAudioCodec == NULL )
	{
		fprintf( stderr, "Unsupported audio codec!\n" );
		return(-1);                                     /* Codec not found */
	}

	if ( avcodec_open2( pAudioCodecCtx, pAudioCodec, NULL ) < 0 )
		return(-1);                                     /* Could not open codec */

	/* 设置音频信息, 用来打开音频设备。 */
	wanted_spec.freq	= pAudioCodecCtx->sample_rate;
	wanted_spec.format	= AUDIO_S16SYS;
	wanted_spec.channels	= pAudioCodecCtx->channels;     /* 通道数 */
	wanted_spec.silence	= 0;                            /* 设置静音值 */
	wanted_spec.samples	= SDL_AUDIO_BUFFER_SIZE;        /* 读取第一帧后调整? */
	wanted_spec.callback	= audio_callback;
	wanted_spec.userdata	= pAudioCodecCtx;

	/*
	 * wanted_spec:想要打开的
	 * spec: 实际打开的,可以不用这个，函数中直接用NULL，下面用到spec的用wanted_spec代替。
	 * 这里会开一个线程，调用callback。
	 * SDL_OpenAudio->open_audio_device(开线程)->SDL_RunAudio->fill(指向callback函数）
	 * 可以用SDL_OpenAudioDevice()代替
	 */
	if ( SDL_OpenAudio( &wanted_spec, &spec ) < 0 )
	{
		fprintf( ERR_STREAM, "Couldn't open Audio:%s\n", SDL_GetError() );
		exit( -1 );
	}

	/* 设置参数，供解码时候用, swr_alloc_set_opts的in部分参数 */
	wanted_frame.format		= AV_SAMPLE_FMT_S16;
	wanted_frame.sample_rate	= spec.freq;
	wanted_frame.channel_layout	= av_get_default_channel_layout( spec.channels );
	wanted_frame.channels		= spec.channels;


	/*
	 * 处理视频
	 * 获取视频解码器上下文
	 */
	pVideoCodecCtx = pFormatCtx->streams[videoStream]->codec;

	/* 通过解码器ID创建一个解码器 */
	pVideoCodec = avcodec_find_decoder( pVideoCodecCtx->codec_id );
	if ( pVideoCodec == NULL )
	{
		fprintf( stderr, "Unsupported video codec!\n" );
		return(-1); /* Codec not found */
	}


	/* 初始化音频队列 */
	packet_queue_init( &audio_queue );

	/*
	 * 可以用SDL_PauseAudioDevice()代替,目前用的这个应该是旧的。
	 * 0是不暂停，非零是暂停
	 * 如果没有这个就放不出声音
	 */
	SDL_PauseAudio( 0 );


	/* 打开解码器 */
	if ( avcodec_open2( pVideoCodecCtx, pVideoCodec, &optionsDict ) < 0 )
		return(-1);  /* Could not open codec */

	/* Allocate video frame */
	pVideoFrame = av_frame_alloc();

	AVFrame* pFrameYUV = av_frame_alloc();
	if ( pFrameYUV == NULL )
		return(-1);


	printf( "width = %d, height = %d\n", pVideoCodecCtx->width, pVideoCodecCtx->height );
	screen = SDL_CreateWindow( "My Game Window",
				   SDL_WINDOWPOS_UNDEFINED,
				   SDL_WINDOWPOS_UNDEFINED,
				   800, 480,
				   SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL );


	if ( !screen )
	{
		fprintf( stderr, "SDL: could not set video mode - exiting\n" );
		exit( 1 );
	}
	SDL_Renderer		*renderer; /* = SDL_CreateRenderer(screen, -1, 0); */
	SDL_RendererInfo	info;
	renderer = SDL_CreateRenderer( screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	if ( !renderer )
	{
		av_log( NULL, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError() );
		renderer = SDL_CreateRenderer( screen, -1, 0 );
	}
	if ( renderer )
	{
		if ( !SDL_GetRendererInfo( renderer, &info ) )
			av_log( NULL, AV_LOG_VERBOSE, "Initialized %s renderer.\n", info.name );
	}
	/* Allocate a place to put our YUV image on that screen */
	bmp = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, pVideoCodecCtx->width, pVideoCodecCtx->height );
	/* SDL_SetTextureBlendMode(bmp,SDL_BLENDMODE_BLEND ); */

	sws_ctx = sws_getContext
		  (
		pVideoCodecCtx->width,
		pVideoCodecCtx->height,
		pVideoCodecCtx->pix_fmt,
		pVideoCodecCtx->width,
		pVideoCodecCtx->height,
		AV_PIX_FMT_YUV420P,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
		  );

	int numBytes = avpicture_get_size( AV_PIX_FMT_YUV420P, pVideoCodecCtx->width,
					   pVideoCodecCtx->height );
	uint8_t* buffer = (uint8_t *) av_malloc( numBytes * sizeof(uint8_t) );

	avpicture_fill( (AVPicture *) pFrameYUV, buffer, AV_PIX_FMT_YUV420P,
			pVideoCodecCtx->width, pVideoCodecCtx->height );

	/* Read frames and save first five frames to disk */
	i = 0;

	rect.x	= 0;
	rect.y	= 0;
	rect.w	= pVideoCodecCtx->width;
	rect.h	= pVideoCodecCtx->height;

	playStartTime = getNowTime();
	/* 使用音频PTS去控制读取速度，音频缓存5帧再播放，视频有数据则直接播放 */
	while ( av_read_frame( pFormatCtx, &packet ) >= 0 )
	{
		if ( packet.stream_index == videoStream ) /* 是一个视频帧 */
		{
			/* Decode video frame */
			avcodec_decode_video2( pVideoCodecCtx, pVideoFrame, &frameFinished,
					       &packet );

			/* Did we get a video frame? */
			if ( frameFinished )
			{
				sws_scale
				(
					sws_ctx,
					(uint8_t const * const *) pVideoFrame->data,
					pVideoFrame->linesize,
					0,
					pVideoCodecCtx->height,
					pFrameYUV->data,
					pFrameYUV->linesize
				);
				/*
				 * 视频帧直接显示
				 * //iPitch 计算yuv一行数据占的字节数
				 */
				SDL_UpdateTexture( bmp, &rect, pFrameYUV->data[0], pFrameYUV->linesize[0] );
				SDL_RenderClear( renderer );
				SDL_RenderCopy( renderer, bmp, &rect, &rect );
				SDL_RenderPresent( renderer );
			}
			/* Free the packet that was allocated by av_read_frame */
			av_free_packet( &packet );
			/* SDL_Delay(15); */
		}else if ( packet.stream_index == audioStream )         /* 是一个音频帧 */
		{
			packet_queue_put( &audio_queue, &packet );      /* 如果为音频帧则先如队列 */
			if ( 0 == firstGotAudioFrame )
			{
				firstGotAudioFrame	= 1;
				audioFirstPts		= packet.pts;
			}
			if ( 1 == firstGotAudioFrame )                  /* 通过pts去控制码流读取速率 */
			{
				while ( (packet.pts - audioFirstPts) > (getNowTime() - playStartTime) + 100 )
				{
					printf( " (packet.pts - audioFirstPts) = %u,  (getNowTime() - playStartTime) = %u\n",
						(packet.pts - audioFirstPts), (getNowTime() - playStartTime) );
					SDL_Delay( 30 );
				}
			}
		}else  {
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

	SDL_DestroyTexture( bmp );

	/* Free the YUV frame */
	av_free( pVideoFrame );
	av_free( pFrameYUV );
	/* Close the codec */
	avcodec_close( pVideoCodecCtx );

	/* Close the video file */
	avformat_close_input( &pFormatCtx );

	return(0);
}


