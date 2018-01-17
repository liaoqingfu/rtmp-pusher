#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>

#ifdef __cplusplus
}
#endif




void H264Decode(AVCodecContext *pCodecCtx, AVFrame *picture,  int *got_picture_ptr,   const AVPacket *avpkt)
{
	avcodec_decode_video2(pCodecCtx, picture, got_picture_ptr, avpkt);  	//打开的是H264裸流，所以只有视频帧
}

void SaveYUV(AVFrame* pFrameYUV, int width, int height, FILE *file)
{
	fwrite(pFrameYUV->data[0], width*height,1,		file);  
	fwrite(pFrameYUV->data[1], width*height/4,1,	file);  
	fwrite(pFrameYUV->data[2], width*height/4,1,	file);  
}

void SaveRGB(AVFrame* pFrameYUV, int width, int height, FILE *file)
{
	fwrite(pFrameYUV->data[0], width*height*3,1,	file);   
}

void SaveBMP(AVFrame* pFrameYUV, int width, int height)
{

}

int main( int argc, char *argv[] )
{
	AVFormatContext *pFormatCtx;  
	const char *filename = "source.200kbps.768x320.h264";  
	AVPacket packet; 
	int gotFrame; 

	AVCodec         *pCodec = NULL;  
	AVCodecContext  *pCodecCtx = NULL;  
	struct SwsContext      *pYuvConvertCtx = NULL;  
	struct SwsContext      *pRgbConvertCtx = NULL;  
	AVFrame         *pFrame = NULL;  
	AVFrame         *pFrameRGB = NULL;  
	AVFrame  		*pFrameYUV;//一帧画面  
	uint8_t *pYuvOutBuffer = NULL; 
	uint8_t *pRgbOutBuffer = NULL; 
	
	int yuvWidth = 320;
	int yuvHeight = 180;

	int rgbWidth = 240;
	int rgbHeight = 135;
	 /* must be called before using avcodec lib*/  
    //avcodec_init();  
    /* register all the codecs */  
    av_register_all();  
  	
    /* find the h264 video decoder */  
    pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);  
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

	pFrameYUV = av_frame_alloc();  
    if(pFrameYUV == NULL)  
        return -1;      

    pFrameRGB = av_frame_alloc();  
    if(pFrameRGB == NULL)  
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

    FILE * f_hyuv = fopen("320x180.yuv","wb+");  
    FILE * f_rgb = fopen("240x135.rgb","wb+");  
    while( av_read_frame(pFormatCtx, &packet) >= 0 ) 
    {  
  		H264Decode(pCodecCtx, pFrame, &gotFrame, &packet);
        if( gotFrame ) 
        {  
			if(!pYuvOutBuffer)
			{
        	  	//根据图像大小和格式，分配内存  
        	  	printf("yuv pic size, from [%d, %d] scale to [%d,%d]\n",  
        	  		pCodecCtx->width, pCodecCtx->height, 
        	  		yuvWidth, yuvHeight);
			    pYuvOutBuffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, yuvWidth, yuvHeight));  
			    //建立picture数据  
			    avpicture_fill((AVPicture *)pFrameYUV, pYuvOutBuffer, AV_PIX_FMT_YUV420P, yuvWidth, yuvHeight); 
			    pYuvConvertCtx = sws_getContext(
			    		pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,   
        				yuvWidth, yuvHeight, AV_PIX_FMT_YUV420P, 
        				SWS_BICUBIC, NULL, NULL, NULL);   
			}
           	// 保存为yuv
           	sws_scale(pYuvConvertCtx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,   
                    pFrameYUV->data, pFrameYUV->linesize);  
            // 经过scale后 frame的width和height已经被清空        
            //printf("pFrameYUV->width = %d, pFrameYUV->height = %d, %d, %d\n", pFrameYUV->width, pFrameYUV->height,
            //	pFrame->linesize[0], pFrameYUV->linesize[0]);
			SaveYUV(pFrameYUV, yuvWidth, yuvHeight, f_hyuv);
			
           	// 保存为RGB
			if(!pRgbOutBuffer)
			{
        	  	//根据图像大小和格式，分配内存  
        	  	printf("rgb pic size, from [%d, %d] scale to [%d,%d]\n",  
        	  		pCodecCtx->width, pCodecCtx->height, 
        	  		rgbWidth, rgbHeight);
			    pRgbOutBuffer= (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_RGB24, rgbWidth, rgbHeight));  
			    //建立picture数据  
			    avpicture_fill((AVPicture *)pFrameRGB, pRgbOutBuffer, AV_PIX_FMT_RGB24, rgbWidth, rgbHeight); 
			    pRgbConvertCtx = sws_getContext(
			    		pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,   
        				rgbWidth, rgbHeight, AV_PIX_FMT_RGB24, 
        				SWS_BICUBIC, NULL, NULL, NULL);   
			}
           	// 保存为yuv
           	sws_scale(pRgbConvertCtx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,   
                    pFrameRGB->data, pFrameRGB->linesize);  
            //经过scale后 frame的width和height已经被清空        
            //printf("pFrameRGB->width = %d, pFrameRGB->height = %d, %d, %d\n", pFrameRGB->width, pFrameRGB->height,
            //	pFrame->linesize[0], pFrameRGB->linesize[0]);
			SaveRGB(pFrameRGB, rgbWidth, rgbHeight, f_rgb);
           	

           	// 保存为BMP
            
        }  
         
  
        av_free_packet(&packet);  
    }  

    fclose(f_hyuv);  
    fclose(f_rgb);  
	avcodec_close(pCodecCtx);  
    av_free(pCodecCtx);  
    av_free(pFrame);  
    av_free(pFrameRGB);  
    av_free(pFrameYUV);
    av_free(pYuvOutBuffer);
    av_free(pRgbOutBuffer);
    avformat_close_input(&pFormatCtx);  
    sws_freeContext(pYuvConvertCtx);
    sws_freeContext(pRgbConvertCtx);
	return 0;
}



