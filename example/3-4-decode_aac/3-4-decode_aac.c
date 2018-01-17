#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>

#ifdef __cplusplus
}
#endif




int  AACDecode(AVCodecContext *pCodecCtx, AVFrame *pcmFrame,  int *got_pcm_ptr,   const AVPacket *avpkt)
{
	return  avcodec_decode_audio4( pCodecCtx,
						    pcmFrame, got_pcm_ptr, avpkt );
}

void SavePcmS16x2(uint8_t *pAudioBuf, int pcmDataLen, FILE *file)
{
	//printf("pcmDataLen = %d\n", pcmDataLen);
	fwrite(pAudioBuf, 1, pcmDataLen, file);
}

void SavePcmS32px2(AVFrame* pFrame, int bytesPerSample, FILE *file)
{
	for(int i = 0; i < pFrame->nb_samples; i++)
	{
		fwrite(pFrame->data[0] + i*bytesPerSample, 1, bytesPerSample, file);
		fwrite(pFrame->data[1] + i*bytesPerSample, 1, bytesPerSample, file);
	}
}

// 临时测试函数，主要对比下自己修改的在做重采样时是否音质好些。
int AudioResampling(AVCodecContext * audio_dec_ctx,AVFrame * pAudioDecodeFrame,  
                    int out_sample_fmt,int out_channels ,int out_sample_rate , uint8_t * out_buf)  
{  
    //////////////////////////////////////////////////////////////////////////  
    SwrContext * swr_ctx = NULL;  
    int data_size = 0;  
    int ret = 0;  
    int64_t src_ch_layout = AV_CH_LAYOUT_STEREO; //初始化这样根据不同文件做调整  
    int64_t dst_ch_layout = AV_CH_LAYOUT_STEREO; //这里设定ok  
    int dst_nb_channels = 0;  
    int dst_linesize = 0;  
    int src_nb_samples = 0;  
    int dst_nb_samples = 0;  
    int max_dst_nb_samples = 0;  
    uint8_t **dst_data = NULL;  
    int resampled_data_size = 0;  
      
    //重新采样  
    if (swr_ctx)  
    {  
        swr_free(&swr_ctx);  
    }  
    swr_ctx = swr_alloc();  
    if (!swr_ctx)  
    {  
        printf("swr_alloc error \n");  
        return -1;  
    }  
  
    src_ch_layout = (audio_dec_ctx->channel_layout &&   
        audio_dec_ctx->channels ==   
        av_get_channel_layout_nb_channels(audio_dec_ctx->channel_layout)) ?   
        audio_dec_ctx->channel_layout :   
    av_get_default_channel_layout(audio_dec_ctx->channels);  
  
    if (out_channels == 1)  
    {  
        dst_ch_layout = AV_CH_LAYOUT_MONO;  
    }  
    else if(out_channels == 2)  
    {  
        dst_ch_layout = AV_CH_LAYOUT_STEREO;  
    }  
    else  
    {  
        //可扩展  
    }  
  
    if (src_ch_layout <= 0)  
    {  
        printf("src_ch_layout error \n");  
        return -1;  
    }  
  
    src_nb_samples = pAudioDecodeFrame->nb_samples;  
    if (src_nb_samples <= 0)  
    {  
        printf("src_nb_samples error \n");  
        return -1;  
    }  
  
    /* set options */  
    av_opt_set_int(swr_ctx, "in_channel_layout",    src_ch_layout, 0);  
    av_opt_set_int(swr_ctx, "in_sample_rate",       audio_dec_ctx->sample_rate, 0);  
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", audio_dec_ctx->sample_fmt, 0);  
  
    av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);  
    av_opt_set_int(swr_ctx, "out_sample_rate",       out_sample_rate, 0);  
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", (enum AVSampleFormat)out_sample_fmt, 0);  
    swr_init(swr_ctx);  
  
    max_dst_nb_samples = dst_nb_samples =  
        av_rescale_rnd(src_nb_samples, out_sample_rate, audio_dec_ctx->sample_rate, AV_ROUND_UP);  
    if (max_dst_nb_samples <= 0)  
    {  
        printf("av_rescale_rnd error \n");  
        return -1;  
    }  
  
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);  
    ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,  
        dst_nb_samples, (enum AVSampleFormat)out_sample_fmt, 0);  
    if (ret < 0)  
    {  
        printf("av_samples_alloc_array_and_samples error \n");  
        return -1;  
    }  
  
  
    dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, audio_dec_ctx->sample_rate) +  
        src_nb_samples, out_sample_rate, audio_dec_ctx->sample_rate,AV_ROUND_UP);  
    if (dst_nb_samples <= 0)  
    {  
        printf("av_rescale_rnd error \n");  
        return -1;  
    }  
    if (dst_nb_samples > max_dst_nb_samples)  
    {  
        av_free(dst_data[0]);  
        ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,  
            dst_nb_samples, (enum AVSampleFormat)out_sample_fmt, 1);  
        max_dst_nb_samples = dst_nb_samples;  
    }  
  
    data_size = av_samples_get_buffer_size(NULL, audio_dec_ctx->channels,  
        pAudioDecodeFrame->nb_samples,  
        audio_dec_ctx->sample_fmt, 1);  
    if (data_size <= 0)  
    {  
        printf("av_samples_get_buffer_size error \n");  
        return -1;  
    }  
    resampled_data_size = data_size;  
      
    if (swr_ctx)  
    {  
        ret = swr_convert(swr_ctx, dst_data, dst_nb_samples,   
            (const uint8_t **)pAudioDecodeFrame->data, pAudioDecodeFrame->nb_samples);  
        if (ret <= 0)  
        {  
            printf("swr_convert error \n");  
            return -1;  
        }  
  
        resampled_data_size = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,  
            ret, (enum AVSampleFormat)out_sample_fmt, 1);  
        if (resampled_data_size <= 0)  
        {  
            printf("av_samples_get_buffer_size error \n");  
            return -1;  
        }  
    }  
    else   
    {  
        printf("swr_ctx null error \n");  
        return -1;  
    }  
  
    //将值返回去  
    memcpy(out_buf,dst_data[0],resampled_data_size);  
  
    if (dst_data)  
    {  
        av_freep(&dst_data[0]);  
    }  
    av_freep(&dst_data);  
    dst_data = NULL;  
  
    if (swr_ctx)  
    {  
        swr_free(&swr_ctx);  
    }  
    return resampled_data_size;  
}  


#define AVCODE_MAX_AUDIO_FRAME_SIZE	192000  /* 1 second of 48khz 32bit audio */
#define SDL_AUDIO_BUFFER_SIZE		1024    /*  */


int main( int argc, char *argv[] )
{
	AVFormatContext *pFormatCtx;  
	const char *constFileName = "source.200kbps.768x320.aac";  
	char *filename = NULL;
	AVPacket packet; 
	int gotFrame; 
	AVCodec         *pCodec = NULL;  
	AVCodecContext  *pCodecCtx = NULL;  
	AVFrame         *pFrame = NULL;  

	uint8_t	audioBuf[(AVCODE_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	AVFrame         wantedFrame;
	SwrContext	*pPcmSwrCtx	= NULL;

	if ( argc < 2 )
	{
		filename = constFileName;
	}
	else
	{
		filename = argv[1];
	}

    /* register all the codecs */  
    av_register_all();  
  	
    /* find the aac audio decoder */  
    pCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);  
    if (!pCodec) 
    {  
        fprintf(stderr, "codec not found\n");  
    }  
    pCodecCtx = avcodec_alloc_context3(pCodec);  
  
    /* open the coderc */  
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {  
        fprintf(stderr, "could not open codec\n");  
    }  
    // Allocate video frame  
    pFrame = av_frame_alloc();  
    if(pFrame == NULL)  
        return -1;     
        
	// 读取文件
	// 分配AVFormatContext  
    pFormatCtx = avformat_alloc_context();  
  
    //打开视频文件  
    if( avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0 ) 
    {  
        printf ("av open input file failed!\n");  
        exit (1);  
    }  

    FILE * fS32Px2 = fopen("f32lex2.pcm","wb+");  
    FILE * fS16x2 = fopen("s16lex2.pcm","wb+");  
    FILE * fS16x2_2 = fopen("s16lex2_2.pcm","wb+");  
    while( av_read_frame(pFormatCtx, &packet) >= 0 ) 
    {  
  		int ret = AACDecode(pCodecCtx, pFrame, &gotFrame, &packet);
  		if ( ret < 0 )
		{
			fprintf( stderr, "Error decoding audio frame (%s)\n", "error" );
			break;
		}
        if( gotFrame ) 
        {  
			

			// 这里出来只是单声道的数据，格式为AV_SAMPLE_FMT_S32P		每个panel一个通道
			//左声道？
			//fwrite( frame->extended_data[0], 1, unpadded_linesize, audio_dst_file );
			//右声道？
			//fwrite( frame->extended_data[1], 1, unpadded_linesize, audio_dst_file );
			// data存放的数据呢？
			//printf("frame->linesize[0] = %d, %d, %d\n", frame->linesize[0], frame->linesize[1], frame->linesize[2]);
			// frame->linesize[0] = 8192, 0, 0	声音变调了
			// 存放的32
			// 写入双声道
			size_t unpadded_linesize = pFrame->nb_samples * av_get_bytes_per_sample((enum AVSampleFormat)pFrame->format );
			int bytesPerSample = av_get_bytes_per_sample((enum AVSampleFormat)pFrame->format);
			SavePcmS32px2(pFrame, bytesPerSample, fS32Px2);

			if ( pPcmSwrCtx != NULL )
			{
				swr_free( &pPcmSwrCtx );
				pPcmSwrCtx = NULL;
			}
			
			if ( !pPcmSwrCtx)
			{
					// 设置参数，供解码时候用, swr_alloc_set_opts的in部分参数
				wantedFrame.format 	= AV_SAMPLE_FMT_S16;				//只有sample交替
				wantedFrame.sample_rate	= 44100;						// 48000 这样直接重采样有噪音
				wantedFrame.channel_layout = av_get_default_channel_layout(2);
				wantedFrame.channels		= 2;

				pPcmSwrCtx = swr_alloc_set_opts( NULL,
								  wantedFrame.channel_layout, (enum AVSampleFormat) (wantedFrame.format), wantedFrame.sample_rate,
								  pFrame->channel_layout, (enum AVSampleFormat) (pFrame->format), pFrame->sample_rate,
								  0, NULL );
				// 初始化
				if ( pPcmSwrCtx == NULL || swr_init( pPcmSwrCtx ) < 0 )
				{
					fprintf( stderr, "swr_init error\n" );
					break;
				}
			}

			uint8_t *pAudioBuf = audioBuf;
			int convert_len = swr_convert(pPcmSwrCtx,	(uint8_t * *) &pAudioBuf,  AVCODE_MAX_AUDIO_FRAME_SIZE,
						   (const uint8_t * *) pFrame->data, pFrame->nb_samples);
							   
			int pcmDataLen = wantedFrame.channels * convert_len * av_get_bytes_per_sample( (enum AVSampleFormat) (wantedFrame.format));
			SavePcmS16x2(pAudioBuf, pcmDataLen, fS16x2);

			pcmDataLen = AudioResampling(pCodecCtx, pFrame, AV_SAMPLE_FMT_S16, 2, 48000, pAudioBuf);
			SavePcmS16x2(pAudioBuf, pcmDataLen, fS16x2_2);
        }  
         
  
        av_free_packet(&packet);  
    }  

    fclose(fS32Px2);  
    fclose(fS16x2);  
    fclose(fS16x2_2);
    printf("avcodec_close\n");
	avcodec_close(pCodecCtx);  
	
	printf("av_free(pCodecCtx)\n");
    av_free(pCodecCtx);  
    
    printf("av_free(pFrame)\n");
    av_free(pFrame);  
    
	printf("avformat_close_input(&pFormatCtx)\n");
    avformat_close_input(&pFormatCtx);  
    
    printf("sws_freeContext(pPcmSwrCtx)\n");
    if(pPcmSwrCtx)
    swr_free(&pPcmSwrCtx);
    
	printf("return 0\n");
	return 0;
}



