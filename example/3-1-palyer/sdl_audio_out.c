
#include "packet_queue.h"
#include "sdl_audio_out.h"


#define AVCODE_MAX_AUDIO_FRAME_SIZE	192000  /* 1 second of 48khz 32bit audio */
#define SDL_AUDIO_BUFFER_SIZE		1024    /*  */


#define ERR_STREAM	stderr
#define OUT_SAMPLE_RATE 44100

AVFrame wantFrame;			// 指定输出

/*
 * 音频输出
 * SDL
 */
SDL_AudioSpec	wantedSpec;
SDL_AudioSpec	spec;
PacketQueue audioQueue;      /* 音频队列 */





static void audio_callback( void *userdata, Uint8 *stream, int len );
static int audio_decode_frame( AVCodecContext *pAudioCodecCtx, uint8_t *audio_buf, int buf_size );


/*
 * 注意userdata是前面的AVCodecContext.
 * len的值常为2048,表示一次发送多少。
 * audio_buf_size：一直为样本缓冲区的大小，wantedSpec.samples.（一般每次解码这么多，文件不同，这个值不同)
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
	printf("audio_callback len = %d\n", len);
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
			}
			else  
			{
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
	int		decode_len;
	int		try_again	= 0;
	long long	audio_buf_index = 0;
	long long	data_size	= 0;
	SwrContext	*swr_ctx	= NULL;
	int		convert_len	= 0;
	int		convert_all	= 0;

	if ( packet_queue_get( &audioQueue, &packet, 1 ) < 0 )
	{
		fprintf( ERR_STREAM, "Get queue packet error\n" );
		return(-1);
	}

	pkt_size = packet.size;

	frame = av_frame_alloc();
	while ( pkt_size > 0 )
	{
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
			}
			else if ( frame->channels == 0 && frame->channel_layout > 0 )
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
						      wantFrame.channel_layout, (enum AVSampleFormat) (wantFrame.format), wantFrame.sample_rate,
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
			 *                wantFrame.sample_rate, wantFrame.format,
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
			printf("decode len = %d, convert_len = %d\n", decode_len, convert_len);						   
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
	return(wantFrame.channels * convert_all * av_get_bytes_per_sample( (enum AVSampleFormat) (wantFrame.format) ) );
}


int SDL2AudioOutInit(AVCodecContext	*pAudioCodecCtx)
{
	/* 设置音频信息, 用来打开音频设备。 */
	wantedSpec.freq	= pAudioCodecCtx->sample_rate;
	wantedSpec.format	= AUDIO_S16SYS;
	wantedSpec.channels	= pAudioCodecCtx->channels;     /* 通道数 */
	wantedSpec.silence	= 0;                            /* 设置静音值 */
	wantedSpec.samples	= SDL_AUDIO_BUFFER_SIZE;        /* 读取第一帧后调整? */
	wantedSpec.callback	= audio_callback;
	wantedSpec.userdata	= pAudioCodecCtx;

	/*
	 * wantedSpec:想要打开的
	 * spec: 实际打开的,可以不用这个，函数中直接用NULL，下面用到spec的用wantedSpec代替。
	 * 这里会开一个线程，调用callback。
	 * SDL_OpenAudio->open_audio_device(开线程)->SDL_RunAudio->fill(指向callback函数）
	 * 可以用SDL_OpenAudioDevice()代替
	 */
	if ( SDL_OpenAudio( &wantedSpec, &spec ) < 0 )
	{
		fprintf( ERR_STREAM, "Couldn't open Audio:%s\n", SDL_GetError() );
		return -1;
	}

	/* 设置参数，供解码时候用, swr_alloc_set_opts的in部分参数 */
	wantFrame.format		= AV_SAMPLE_FMT_S16;
	wantFrame.sample_rate	= spec.freq;
	wantFrame.channel_layout	= av_get_default_channel_layout( spec.channels );
	wantFrame.channels		= spec.channels;

	printf("wantFrame.sample_rate = %d\n", wantFrame.sample_rate);
	/*
	 * 可以用SDL_PauseAudioDevice()代替,目前用的这个应该是旧的。
	 * 0是不暂停，非零是暂停
	 * 如果没有这个就放不出声音
	 */
	SDL_PauseAudio( 1 );

	/* 初始化音频队列 */
	packet_queue_init( &audioQueue );

	return 0;
}

void SDL2AudioPushPacket(AVPacket *packet)
{
	packet_queue_put( &audioQueue, packet );
}

void SDL2AudioOutPause(int pause)
{
	SDL_PauseAudio(pause);
}



