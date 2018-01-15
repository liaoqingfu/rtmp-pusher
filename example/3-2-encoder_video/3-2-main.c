#ifdef __cplusplus
extern "C"
{
#endif

#include <math.h>
#include <stdlib.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/frame.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif


int main( int argc, char **argv )
{
	AVFrame		*frame;
	AVCodec		*codec = NULL;
	AVPacket	packet;
	AVCodecContext	*codecContext;
	int		readSize	= 0;
	int		ret		= 0, getPacket;
	FILE		* fileIn, *fileOut;
	int		frameCount = 0;
	/* register all the codecs */
	av_register_all();

	if (argc != 4)
	{
		fprintf( stdout, "usage:./encodec.bin xxx.yuv width height\n" );
		return(-1);
	}

	/*
	 * 1.我们需要读一帧一帧的数据，所以需要AVFrame结构
	 * 读出的一帧数据保存在AVFrame中。
	 */
	frame		= av_frame_alloc();
	frame->width	= atoi( argv[2] );			//设置帧宽度
	frame->height	= atoi( argv[3] );			//设置帧高度
	fprintf( stdout, "width=%d,height=%d\n", frame->width, frame->height );
	frame->format = AV_PIX_FMT_YUV420P;			//设置源文件的数据格式
	av_image_alloc( frame->data, frame->linesize, frame->width, frame->height, (enum AVPixelFormat) frame->format, 32 );
	fileIn = fopen( argv[1], "r+" );

	/*
	 * 2.读出来的数据保存在AVPacket中，因此，我们还需要AVPacket结构体
	 * 初始化packet
	 */
	av_init_packet( &packet );


	/*
	 * 3.读出来的数据，我们需要编码，因此需要编码器
	 * 面的函数找到h.264类型的编码器
	 */
	/* find the mpeg1 video encoder */
	codec = avcodec_find_encoder( AV_CODEC_ID_H264 );
	if ( !codec )
	{
		fprintf( stderr, "Codec not found\n" );
		exit( 1 );
	}

	/*有了编码器，我们还需要编码器的上下文环境，用来控制编码的过程 */
	codecContext = avcodec_alloc_context3( codec ); /* 分配AVCodecContext实例 */
	if ( !codecContext )
	{
		fprintf( stderr, "Could not allocate video codec context\n" );
		return(-1);
	}
	/* put sample parameters */
	codecContext->bit_rate = 400000;
	/* resolution must be a multiple of two */
	codecContext->width	= 300;
	codecContext->height	= 200;
	/* frames per second */
	codecContext->time_base = (AVRational) { 1, 25 };


	/* emit one intra frame every ten frames
	 * check frame pict_type before passing frame
	 * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	 * then gop_size is ignored and the output of encoder
	 * will always be I frame irrespective to gop_size
	 */
	codecContext->gop_size		= 10;
	codecContext->max_b_frames	= 1;
	codecContext->pix_fmt		= AV_PIX_FMT_YUV420P;
	av_opt_set( codecContext->priv_data, "preset", "slow", 0 );

	/* 准备好了编码器和编码器上下文环境，现在可以打开编码器了 */
	if ( avcodec_open2( codecContext, codec, NULL ) < 0 ) /* 根据编码器上下文打开编码器 */
	{
		fprintf( stderr, "Could not open codec\n" );
		return(-1);
	}

	/* 4.准备输出文件 */
	fileOut = fopen( "test.h264", "w+" );
	/*
	YUV420 planar数据， 以720×488大小图象YUV420 planar为例，

	其存储格式是： 共大小为(720×480×3>>1)字节，
	分为三个部分:Y,U和V
		Y分量：    	(720×480)	  	个字节  
		U(Cb)分量：(720×480>>2)		个字节
		V(Cr)分量：(720×480>>2)		个字节
	*/
	/*下面开始编码 */
	while ( 1 )
	{
		/* 读一帧数据出来 */
		readSize = fread( frame->data[0], 1, frame->linesize[0] * frame->height, fileIn );
		if ( readSize == 0 )
		{
			fprintf( stdout, "end of file\n" );
			frameCount++;
			break;
		}
		readSize	= fread( frame->data[1], 1, frame->linesize[1] * frame->height / 2, fileIn );
		readSize	= fread( frame->data[2], 1, frame->linesize[2] * frame->height / 2, fileIn );
		/* 初始化packet */
		av_init_packet( &packet );
		/* encode the image */
		frame->pts	= frameCount;
		ret		= avcodec_encode_video2( codecContext, &packet, frame, &getPacket ); /* 将AVFrame中的像素信息编码为AVPacket中的码流 */
		if ( ret < 0 )
		{
			fprintf( stderr, "Error encoding frame\n" );
			return(-1);
		}

		if ( getPacket )
		{
			frameCount++;
			/* 获得一个完整的编码帧 */
			printf( "Write frame %3d (size=%5d)\n", frameCount, packet.size );
			fwrite( packet.data, 1, packet.size, fileOut );
			av_packet_unref( &packet );
		}
	}

	/* flush buffer */
	for ( getPacket = 1; getPacket; frameCount++ )
	{
		fflush( stdout );
		frame->pts	= frameCount;
		ret		= avcodec_encode_video2( codecContext, &packet, NULL, &getPacket ); /* 输出编码器中剩余的码流 */
		if ( ret < 0 )
		{
			fprintf( stderr, "Error encoding frame\n" );
			return(-1);
		}

		if ( getPacket )
		{
			printf( "Write frame %3d (size=%5d)\n", frameCount, packet.size );
			fwrite( packet.data, 1, packet.size, fileOut );
			av_packet_unref( &packet );
		}
	} /* for (got_output = 1; got_output; frameIdx++) */

	fclose( fileIn );
	fclose( fileOut );
	av_frame_free( &frame );
	avcodec_close( codecContext );
	av_free( codecContext );

	return(0);
}


