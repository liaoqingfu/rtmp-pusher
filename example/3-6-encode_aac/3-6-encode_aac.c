#ifdef __cplusplus
extern "C"
{
#endif
#include <math.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>

#ifdef __cplusplus
}
#endif

void rearrangePcm(uint8_t *leftData, uint8_t *rightData, uint8_t *inputData, int sampleNum)
{
	/** 
	*	在这里写入文件时我做了一些处理，这是有原因的。 
	*	下面的意思是，LRLRLR...的方式写入文件，每次写入4096个字节 
	*/	
	for(int i = 0; i < sampleNum; i++)
	{
		(*(uint32_t *)(leftData + i *4)) = (*(uint32_t *)(inputData + i * 2*4));
		(*(uint32_t *)(rightData + i *4)) = (*(uint32_t *)(inputData + i * 2* 4 + 4));
	}

}

int main(int argc, char **argv){
    AVFrame *frame;
    AVCodec *codec = NULL;
    AVPacket packet;
    AVCodecContext *codecContext;
    int readSize=0;
    int ret=0,getPacket;
    FILE * fileIn,*fileOut;
    int frameCount=0;
    /* register all the codecs */
    av_register_all();
	char *padts = (char *)malloc(sizeof(char) * 7); 
	 int profile = 2;                                            //AAC LC  
    int freqIdx = 4;                                            //44.1KHz  
    int chanCfg = 2;            //MPEG-4 Audio Channel Configuration. 1 Channel front-center  
    padts[0] = (char)0xFF;      // 11111111     = syncword  
    padts[1] = (char)0xF1;      // 1111 1 00 1  = syncword MPEG-2 Layer CRC  
    padts[2] = (char)(((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));  
    padts[6] = (char)0xFC;  
	
    if(argc!=3){
        fprintf(stdout,"usage:./a.out xxx.pcm xxx.aac\n");
        return -1;
    }

    //1.我们需要读一帧一帧的数据，所以需要AVFrame结构
    //读出的一帧数据保存在AVFrame中。
    frame  = av_frame_alloc();
    // AV_SAMPLE_FMT_FLTP 音频数据format分很多种类型，16bit,32bit等，而2016 ffmpeg只支持最新的AAC格式，32bit，也就是AV_SAMPLE_FMT_FLTP。
	//所以，想对PCM进行编码得先确保PCM是AV_SAMPLE_FMT_FLTP类型的。
    frame->format = AV_SAMPLE_FMT_FLTP;	
    frame->nb_samples = 1024;
    frame->sample_rate = 44100;
    frame->channels = 2;
    fileIn =fopen(argv[1],"r+");
    frame->data[0] = av_malloc(1024*4);
	frame->data[1] = av_malloc(1024*4);
	uint8_t pcmBuff[1024*4*2];
    //2.读出来的数据保存在AVPacket中，因此，我们还需要AVPacket结构体
    //初始化packet
    memset(&packet, 0, sizeof(AVPacket));  
    av_init_packet(&packet);


    //3.读出来的数据，我们需要编码，因此需要编码器
    //下面的函数找到h.264类型的编码器
    /* find the mpeg1 video encoder */
    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec){
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    //有了编码器，我们还需要编码器的上下文环境，用来控制编码的过程
    codecContext = avcodec_alloc_context3(codec);//分配AVCodecContext实例
    if (!codecContext){
        fprintf(stderr, "Could not allocate video codec context\n");
        return -1;
    }
    /* put sample parameters */  
    codecContext->sample_rate = frame->sample_rate;  
    codecContext->channels = frame->channels;  
    codecContext->sample_fmt = frame->format;  
    /* select other audio parameters supported by the encoder */  
    codecContext->channel_layout = AV_CH_LAYOUT_STEREO;
    //准备好了编码器和编码器上下文环境，现在可以打开编码器了
    //根据编码器上下文打开编码器
 
    if (avcodec_open2(codecContext, codec, NULL) < 0){
        fprintf(stderr, "Could not open codec\n");
        return -1;
    }
    //4.准备输出文件
    fileOut= fopen(argv[2],"w+");
    //下面开始编码
    printf("L:%d \n", __LINE__);
    while(1)
    {
    	//printf("L:%d \n", __LINE__);
    	// 将一帧的数据读取出来，将LRLR..的格式排列为 LL .... RR...
        //读一帧数据出来
        readSize = fread(pcmBuff,1,1024*2*4,fileIn);
        
        if(readSize == 0){
            fprintf(stdout,"end of file\n");
            frameCount++;
            break;
        }
        rearrangePcm(frame->data[0], frame->data[1], pcmBuff, readSize/2/4);
        //frame->linesize[0] = 4096;
        //frame->linesize[1] = 4096;
        //printf("L:%d readSize = %d\n", __LINE__, readSize);
        //初始化packet
        av_init_packet(&packet);
        /* encode the image */
        frame->pts = frameCount;
        //printf("L:%d \n", __LINE__);
        ret = avcodec_encode_audio2(codecContext, &packet, frame, &getPacket); //将AVFrame中的像素信息编码为AVPacket中的码流
        //printf("L:%d \n", __LINE__);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            goto out;
        }

        if (getPacket) {
            frameCount++;
            //获得一个完整的编码帧
            padts[3] = (char)(((chanCfg & 3) << 6) + ((7 + packet.size) >> 11));  
            padts[4] = (char)(((7 + packet.size) & 0x7FF) >> 3);  
            padts[5] = (char)((((7 + packet.size) & 7) << 5) + 0x1F);  
            fwrite(padts, 7, 1, fileOut); 
            
            //printf("Write frame %3d (size=%5d), %x %x %x\n", frameCount, packet.size, packet.data[0], packet.data[1], packet.data[2]);
            fwrite(packet.data, 1,packet.size, fileOut);
            av_packet_unref(&packet);
        }

    }
	
    /* flush buffer */
    for ( getPacket= 1; getPacket; frameCount++){
        frame->pts = frameCount;
        ret = avcodec_encode_audio2(codecContext, &packet, NULL, &getPacket);       //输出编码器中剩余的码流
        if (ret < 0){
            fprintf(stderr, "Error encoding frame\n");
            goto out;
        }
        if (getPacket){
        //获得一个完整的编码帧
            padts[3] = (char)(((chanCfg & 3) << 6) + ((7 + packet.size) >> 11));  
            padts[4] = (char)(((7 + packet.size) & 0x7FF) >> 3);  
            padts[5] = (char)((((7 + packet.size) & 7) << 5) + 0x1F);  
            fwrite(padts, 7, 1, fileOut); 
            printf("flush buffer Write frame %3d (size=%5d)\n", frameCount, packet.size);
            fwrite(packet.data, 1, packet.size, fileOut);
            av_packet_unref(&packet);
        }
    } //for (got_output = 1; got_output; frameIdx++) 
	//printf("L:%d \n", __LINE__);
out:
	//printf("L:%d \n", __LINE__);
    fclose(fileIn);
    fclose(fileOut);
    av_frame_free(&frame);
    avcodec_close(codecContext);
    av_free(codecContext);
    av_free(padts);
    return 0;
}

