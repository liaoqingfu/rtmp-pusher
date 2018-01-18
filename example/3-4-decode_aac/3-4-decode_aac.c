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


	int64_t src_ch_layout = AV_CH_LAYOUT_STEREO, dst_ch_layout = AV_CH_LAYOUT_STEREO;
    int src_rate = 48000, dst_rate = 96000;
    uint8_t **src_data = NULL, **dst_data = NULL;
    int src_nb_channels = 0, dst_nb_channels = 0;
    int src_linesize, dst_linesize;
    int src_nb_samples = 1024, dst_nb_samples, max_dst_nb_samples;
    enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16, dst_sample_fmt = AV_SAMPLE_FMT_S16;
    const char *dst_filename = NULL;
    FILE *dst_file;
    int dst_bufsize;
    const char *fmt;
    double t;
    int ret;

	

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
    FILE * fS16x2 = fopen("s16lex2-96000.pcm","wb+");  
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

			
			
			if ( !pPcmSwrCtx)
			{
				/* create resampler context */
			    pPcmSwrCtx = swr_alloc();
			    if (!pPcmSwrCtx) {
			        fprintf(stderr, "Could not allocate resampler context\n");
			        ret = AVERROR(ENOMEM);
			        break;
			    }
    			/* set options */
    			src_ch_layout = pFrame->channel_layout;
    			src_rate = pFrame->sample_rate;
    			src_sample_fmt = (enum AVSampleFormat)(pFrame->format);
			    av_opt_set_int(pPcmSwrCtx, "in_channel_layout",    src_ch_layout, 0);
			    av_opt_set_int(pPcmSwrCtx, "in_sample_rate",       src_rate, 0);
			    av_opt_set_sample_fmt(pPcmSwrCtx, "in_sample_fmt", src_sample_fmt, 0);

			    av_opt_set_int(pPcmSwrCtx, "out_channel_layout",    dst_ch_layout, 0);
			    av_opt_set_int(pPcmSwrCtx, "out_sample_rate",       dst_rate, 0);
			    av_opt_set_sample_fmt(pPcmSwrCtx, "out_sample_fmt", dst_sample_fmt, 0);

			    /* initialize the resampling context */
			    if ((ret = swr_init(pPcmSwrCtx)) < 0) {
			        fprintf(stderr, "Failed to initialize the resampling context\n");
			        break;
			    }
			    src_nb_samples = pFrame->nb_samples;
			    max_dst_nb_samples = dst_nb_samples =  av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);
			    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
			    printf("max_dst_nb_samples = %d, dst_nb_channels = %d\n", max_dst_nb_samples);
			    
			}

			dst_nb_samples = av_rescale_rnd(swr_get_delay(pPcmSwrCtx, pFrame->sample_rate) +  src_nb_samples, 
								dst_rate, 
								pFrame->sample_rate, 
								AV_ROUND_UP);
			printf("dst_nb_samples = %d\n", dst_nb_samples);					
	        if (dst_nb_samples > max_dst_nb_samples) {
	            printf("dst_nb_samples:%d > max_dst_nb_samples:%d\n", dst_nb_samples, max_dst_nb_samples);
	            max_dst_nb_samples = dst_nb_samples;
	        }
        
			uint8_t *pAudioBuf = audioBuf;
			/* convert to destination format */
	        ret = swr_convert(pPcmSwrCtx, (uint8_t * *) &pAudioBuf, dst_nb_samples, 
	        		(const uint8_t **)(const uint8_t * *) pFrame->data, pFrame->nb_samples);
	        if (ret < 0) {
	            fprintf(stderr, "Error while converting\n");
	            break;
	        }
	        dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
	                                                 ret, dst_sample_fmt, 1);
	       	printf("ret = %d, dst_bufsize = %d\n", ret, dst_bufsize);	                                          
	        if (dst_bufsize < 0) {
	            fprintf(stderr, "Could not get sample buffer size\n");
	            break;
	        }
	        printf("in:%d out:%d\n",  src_nb_samples, ret);
			
	
			SavePcmS16x2(pAudioBuf, dst_bufsize, fS16x2);
			/*
			44100->48000
			in:1024 out:1115
			dst_nb_samples = 1132
			ret = 1115, dst_bufsize = 4460
			in:1024 out:1115
			dst_nb_samples = 1131
			ret = 1114, dst_bufsize = 4456
			in:1024 out:1114
			dst_nb_samples = 1132
			ret = 1115, dst_bufsize = 4460
			*/
	
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
    if ( pPcmSwrCtx != NULL )
	{
		swr_free( &pPcmSwrCtx );
		pPcmSwrCtx = NULL;
	}
    
	printf("return 0\n");
	return 0;
}



