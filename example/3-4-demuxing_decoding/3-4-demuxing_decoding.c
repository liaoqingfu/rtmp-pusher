/*
 * Copyright (c) 2012 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


/**
 * @file
 * Demuxing and decoding example.
 *
 * Show how to use the libavformat and libavcodec API to demux and
 * decode audio and video data.
 * @example demuxing_decoding.c
 */



#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <libswresample/swresample.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
//#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>


#ifdef __cplusplus
}
#endif

//#define __STDC_CONSTANT_MACROS


#define AVCODE_MAX_AUDIO_FRAME_SIZE    	192000  //1 second of 48khz 32bit audio

#define ERR_STREAM              stderr

static AVFormatContext		*fmt_ctx	= NULL;
static AVCodecContext		*video_dec_ctx	= NULL, *audio_dec_ctx;
static int			width, height;
static enum AVPixelFormat	pix_fmt;
static AVStream			*video_stream		= NULL, *audio_stream = NULL;
static const char		*src_filename		= NULL;
static const char		*video_dst_filename	= NULL;
static const char		*audio_dst_filename	= NULL;
static FILE			*video_dst_file		= NULL;
static FILE			*audio_dst_file		= NULL;

static uint8_t	*video_dst_data[4] = { NULL };
static int	video_dst_linesize[4];
static int	video_dst_bufsize;

static int	video_stream_idx	= -1;
static int  audio_stream_idx 	= -1;
static AVFrame	*frame			= NULL;
static AVPacket pkt;
static int	video_frame_count	= 0;
static int	audio_frame_count	= 0;

static AVFrame         wanted_frame;

/* Enable or disable frame reference counting. You are not supposed to support
 * both paths in your application but pick the one most appropriate to your
 * needs. Look for the use of refcount in this example to see what are the
 * differences of API usage between them. */
static int refcount = 0;

static int decode_packet( int *got_frame, int cached )
{
	int	ret	= 0;
	int	decoded = pkt.size;
	SwrContext	*swr_ctx	= NULL;
	*got_frame = 0;
	static uint8_t	audio_buf[(AVCODE_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	int convert_len;
	
	if ( pkt.stream_index == video_stream_idx )
	{
		/* decode video frame */
		ret = avcodec_decode_video2( video_dec_ctx, frame, got_frame, &pkt );
		if ( ret < 0 )
		{
			fprintf( stderr, "Error decoding video frame (%s)\n", "error" );
			//av_err2str(ret);
			return(ret);
		}

		if ( *got_frame )
		{
			if ( frame->width != width || frame->height != height ||
			     frame->format != pix_fmt )
			{
				/* To handle this change, one could call av_image_alloc again and
				 * decode the following frames into another rawvideo file. */
				fprintf( stderr, "Error: Width, height and pixel format have to be "
					 "constant in a rawvideo file, but the width, height or "
					 "pixel format of the input video changed:\n"
					 "old: width = %d, height = %d, format = %s\n"
					 "new: width = %d, height = %d, format = %s\n",
					 width, height, av_get_pix_fmt_name( pix_fmt ),
					 frame->width, frame->height,
					 av_get_pix_fmt_name( (enum AVPixelFormat)frame->format ) );
				return(-1);
			}

			printf( "video_frame%s n:%d coded_n:%d, width:%d, height:%d\n",
				cached ? "(cached)" : "",
				video_frame_count++, frame->coded_picture_number,frame->width, frame->height);


			/* copy decoded frame to destination buffer:
			 * this is required since rawvideo expects non aligned data */
			av_image_copy( video_dst_data, video_dst_linesize,
				       (const uint8_t * *) (frame->data), frame->linesize,
				       pix_fmt, width, height );

			/* write to rawvideo file */
			fwrite( video_dst_data[0], 1, video_dst_bufsize, video_dst_file );
		}
	} else if ( pkt.stream_index == audio_stream_idx )
	{
		/* decode audio frame */
		ret = avcodec_decode_audio4( audio_dec_ctx, frame, got_frame, &pkt );
		if ( ret < 0 )
		{
			fprintf( stderr, "Error decoding audio frame (%s)\n", "error" );
			//av_err2str(ret);
			return(ret);
		}


		/* Some audio decoders decode only part of the packet, and have to be
		 * called again with the remainder of the packet data.
		 * Sample: fate-suite/lossless-audio/luckynight-partial.shn
		 * Also, some decoders might over-read the packet. */
		decoded = FFMIN( ret, pkt.size );

		if ( *got_frame )
		{
			size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample((enum AVSampleFormat)frame->format );
			/*
			printf( "audio_format:%d channels:%d nb_samples:%d, sample_rate:%d, unpadded_linesize:%d, pts:%d\n",
				frame->format,
				frame->channels, frame->nb_samples, frame->sample_rate, unpadded_linesize,
				frame->pts );
				*/
			//av_ts2timestr( frame->pts, &audio_dec_ctx->time_base )

			/* Write the raw audio data samples of the first plane. This works
			 * fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
			 * most audio decoders output planar audio, which uses a separate
			 * plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
			 * In other words, this code will write only the first audio channel
			 * in these cases.
			 * You should use libswresample or libavfilter to convert the frame
			 * to packed data. */
			// 这里出来只是单声道的数据，格式为AV_SAMPLE_FMT_S32P		每个panel一个通道
			//左声道？
			//fwrite( frame->extended_data[0], 1, unpadded_linesize, audio_dst_file );
			//右声道？
			//fwrite( frame->extended_data[1], 1, unpadded_linesize, audio_dst_file );
			// data存放的数据呢？
			//printf("frame->linesize[0] = %d, %d, %d\n", frame->linesize[0], frame->linesize[1], frame->linesize[2]);
			// frame->linesize[0] = 8192, 0, 0  声音变调了
			// 存放的32
			// 写入双声道
			int bytesPerSample = av_get_bytes_per_sample((enum AVSampleFormat)frame->format);
			for(int i = 0; i < frame->nb_samples; i++)
			{
				fwrite(frame->data[0] + i*bytesPerSample, 1, bytesPerSample, audio_dst_file);
				fwrite(frame->data[1] + i*bytesPerSample, 1, bytesPerSample, audio_dst_file);
			}
			
			if ( swr_ctx != NULL )
			{
				swr_free( &swr_ctx );
				swr_ctx = NULL;
			}

			// 设置参数，供解码时候用, swr_alloc_set_opts的in部分参数
			wanted_frame.format		= AV_SAMPLE_FMT_S16;				//只有sample交替
			wanted_frame.sample_rate	= 44100;
			wanted_frame.channel_layout	= av_get_default_channel_layout(2);
			wanted_frame.channels		= 2;

			swr_ctx = swr_alloc_set_opts( NULL,
						      wanted_frame.channel_layout, (enum AVSampleFormat) (wanted_frame.format), wanted_frame.sample_rate,
						      frame->channel_layout, (enum AVSampleFormat) (frame->format), frame->sample_rate,
						      0, NULL );
			// 初始化
			if ( swr_ctx == NULL || swr_init( swr_ctx ) < 0 )
			{
				fprintf( ERR_STREAM, "swr_init error\n" );
				return -1;
			}
		  
            uint8_t *pAudioBuf = audio_buf;
            //printf("p = 0x%x, 0x%x, 0x%x, 0x%x,\n", &pAudioBuf, &audio_buf, audio_buf, pAudioBuf);
			convert_len = swr_convert(swr_ctx,	(uint8_t * *) &pAudioBuf,  AVCODE_MAX_AUDIO_FRAME_SIZE,
						   (const uint8_t * *) frame->data,	frame->nb_samples);
					   
			
			int convert_len2 = wanted_frame.channels * convert_len * av_get_bytes_per_sample( (enum AVSampleFormat) (wanted_frame.format));
			//printf("convert_len = %d, %d\n",convert_len, convert_len2);
			//fwrite(audio_buf, 1, convert_len2, audio_dst_file);
			 //fwrite(frame->data[0], 1, unpadded_linesize, audio_dst_file);

			
		}
	}


	/* If we use frame reference counting, we own the data and need
	 * to de-reference it when we don't use it anymore */
	if ( *got_frame && refcount )
		av_frame_unref( frame );

	return(decoded);
}


static int open_codec_context( int *stream_idx,
			       AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type )
{
	int		ret, stream_index;
	AVStream	*st;
	AVCodec		*dec	= NULL;
	AVDictionary	*opts	= NULL;

	ret = av_find_best_stream( fmt_ctx, type, -1, -1, NULL, 0 );
	if ( ret < 0 )
	{
		fprintf( stderr, "Could not find %s stream in input file '%s'\n",
			 av_get_media_type_string( type ), src_filename );
		return(ret);
	} else {
		stream_index	= ret;
		st		= fmt_ctx->streams[stream_index];

		/* find decoder for the stream */
		dec = avcodec_find_decoder( st->codecpar->codec_id );
		if ( !dec )
		{
			fprintf( stderr, "Failed to find %s codec\n",
				 av_get_media_type_string( type ) );
			return(AVERROR( EINVAL ) );
		}

		/* Allocate a codec context for the decoder */
		*dec_ctx = avcodec_alloc_context3( dec );
		if ( !*dec_ctx )
		{
			fprintf( stderr, "Failed to allocate the %s codec context\n",
				 av_get_media_type_string( type ) );
			return(AVERROR( ENOMEM ) );
		}

		/* Copy codec parameters from input stream to output codec context */
		if ( (ret = avcodec_parameters_to_context( *dec_ctx, st->codecpar ) ) < 0 )
		{
			fprintf( stderr, "Failed to copy %s codec parameters to decoder context\n",
				 av_get_media_type_string( type ) );
			return(ret);
		}

		/* Init the decoders, with or without reference counting */
		av_dict_set( &opts, "refcounted_frames", refcount ? "1" : "0", 0 );
		if ( (ret = avcodec_open2( *dec_ctx, dec, &opts ) ) < 0 )
		{
			fprintf( stderr, "Failed to open %s codec\n",
				 av_get_media_type_string( type ) );
			return(ret);
		}
		*stream_idx = stream_index;
	}

	return(0);
}


static int get_format_from_sample_fmt( const char **fmt,
				       enum AVSampleFormat sample_fmt )
{
	int i;
	struct sample_fmt_entry {
		enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
	} sample_fmt_entries[] = {
		{ AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
		{ AV_SAMPLE_FMT_S16, "s16be", "s16le" },
		{ AV_SAMPLE_FMT_S32, "s32be", "s32le" },
		{ AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
		{ AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
	};
	*fmt = NULL;

	for ( i = 0; i < FF_ARRAY_ELEMS( sample_fmt_entries ); i++ )
	{
		struct sample_fmt_entry *entry = &sample_fmt_entries[i];
		if ( sample_fmt == entry->sample_fmt )
		{
			*fmt = AV_NE( entry->fmt_be, entry->fmt_le );
			return(0);
		}
	}

	fprintf( stderr,
		 "sample format %s is not supported as output format\n",
		 av_get_sample_fmt_name( sample_fmt ) );
	return(-1);
}


int main( int argc, char **argv )
{
	int ret = 0, got_frame;

	if ( argc != 4 && argc != 5 )
	{
		fprintf( stderr, "usage: %s [-refcount] input_file video_output_file audio_output_file\n"
			 "API example program to show how to read frames from an input file.\n"
			 "This program reads frames from a file, decodes them, and writes decoded\n"
			 "video frames to a rawvideo file named video_output_file, and decoded\n"
			 "audio frames to a rawaudio file named audio_output_file.\n\n"
			 "If the -refcount option is specified, the program use the\n"
			 "reference counting frame system which allows keeping a copy of\n"
			 "the data for longer than one decode call.\n"
			 "\n", argv[0] );
		exit( 1 );
	}
	if ( argc == 5 && !strcmp( argv[1], "-refcount" ) )
	{
		refcount = 1;
		argv++;
	}
	src_filename		= argv[1];
	video_dst_filename	= argv[2];
	audio_dst_filename	= argv[3];

	/* register all formats and codecs */
	av_register_all();

	/* open input file, and allocate format context */
	if ( avformat_open_input( &fmt_ctx, src_filename, NULL, NULL ) < 0 )
	{
		fprintf( stderr, "Could not open source file %s\n", src_filename );
		exit( 1 );
	}

	/* retrieve stream information */
	if ( avformat_find_stream_info( fmt_ctx, NULL ) < 0 )
	{
		fprintf( stderr, "Could not find stream information\n" );
		exit( 1 );
	}

	if ( open_codec_context( &video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO ) >= 0 )
	{
		video_stream = fmt_ctx->streams[video_stream_idx];

		video_dst_file = fopen( video_dst_filename, "wb" );
		if ( !video_dst_file )
		{
			fprintf( stderr, "Could not open destination file %s\n", video_dst_filename );
			ret = 1;
			goto end;
		}

		/* allocate image where the decoded image will be put */
		width	= video_dec_ctx->width;
		height	= video_dec_ctx->height;
		pix_fmt = video_dec_ctx->pix_fmt;
		ret	= av_image_alloc( video_dst_data, video_dst_linesize,
					  width, height, pix_fmt, 1 );
		if ( ret < 0 )
		{
			fprintf( stderr, "Could not allocate raw video buffer\n" );
			goto end;
		}
		video_dst_bufsize = ret;
	}

	if ( open_codec_context( &audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO ) >= 0 )
	{
		audio_stream	= fmt_ctx->streams[audio_stream_idx];
		audio_dst_file	= fopen( audio_dst_filename, "wb" );
		if ( !audio_dst_file )
		{
			fprintf( stderr, "Could not open destination file %s\n", audio_dst_filename );
			ret = 1;
			goto end;
		}
	}

	/* dump input information to stderr */
	av_dump_format( fmt_ctx, 0, src_filename, 0 );

	if ( !audio_stream && !video_stream )
	{
		fprintf( stderr, "Could not find audio or video stream in the input, aborting\n" );
		ret = 1;
		goto end;
	}

	frame = av_frame_alloc();
	if ( !frame )
	{
		fprintf( stderr, "Could not allocate frame\n" );
		ret = AVERROR( ENOMEM );
		goto end;
	}

	/* initialize packet, set data to NULL, let the demuxer fill it */
	av_init_packet( &pkt );
	pkt.data	= NULL;
	pkt.size	= 0;

	if ( video_stream )
		printf( "Demuxing video from file '%s' into '%s'\n", src_filename, video_dst_filename );
	if ( audio_stream )
		printf( "Demuxing audio from file '%s' into '%s'\n", src_filename, audio_dst_filename );

	if ( video_stream )
		printf( "Video decoder '%s'\n", video_dec_ctx->codec->name );
	if ( audio_stream )
		printf( "Audio decoder '%s'\n", audio_dec_ctx->codec->name );
	
	/* read frames from the file */
	while ( av_read_frame( fmt_ctx, &pkt ) >= 0 )
	{
		AVPacket orig_pkt = pkt;
		do
		{
			ret = decode_packet( &got_frame, 0 );
			if ( ret < 0 )
				break;
			pkt.data	+= ret;
			pkt.size	-= ret;
		}
		while ( pkt.size > 0 );
		av_packet_unref( &orig_pkt );
	}

	/* flush cached frames */
	pkt.data	= NULL;
	pkt.size	= 0;
	do
	{
		decode_packet( &got_frame, 1 );
	}
	while ( got_frame );

	printf( "Demuxing succeeded.\n" );

	if ( video_stream )
	{
		printf( "Play the output video file with the command:\n"
			"ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n",
			av_get_pix_fmt_name( pix_fmt ), width, height,
			video_dst_filename );
	}

	if ( audio_stream )
	{
		enum AVSampleFormat	sfmt		= audio_dec_ctx->sample_fmt;
		int			n_channels	= audio_dec_ctx->channels;
		const char		*fmt;

		if ( av_sample_fmt_is_planar( sfmt ) )
		{
			const char *packed = av_get_sample_fmt_name( sfmt );
			printf( "Warning: the sample format the decoder produced is planar "
				"(%s). This example will output the first channel only.\n",
				packed ? packed : "?" );
			sfmt		= av_get_packed_sample_fmt( sfmt );
			n_channels	= 1;
		}

		if ( (ret = get_format_from_sample_fmt( &fmt, sfmt ) ) < 0 )
			goto end;

		printf( "Play the output audio file with the command:\n"
			"ffplay -f %s -ac %d -ar %d %s\n",
			fmt, n_channels, audio_dec_ctx->sample_rate,
			audio_dst_filename );
	}

end:
	avcodec_free_context( &video_dec_ctx );
	avcodec_free_context( &audio_dec_ctx );
	avformat_close_input( &fmt_ctx );
	if ( video_dst_file )
		fclose( video_dst_file );
	if ( audio_dst_file )
		fclose( audio_dst_file );
	av_frame_free( &frame );
	av_free( video_dst_data[0] );

	return(ret < 0);
}


