#include "aac_encoder.h"
#include <stdio.h>
namespace AudioCode
{
AacEncoder::AacEncoder()
{

}
AacEncoder::~AacEncoder()
{
	if(aacEncClose( &encodeHandle_ ) != AACENC_OK)
	{
		printf("aacEncClose failed\n");
	}
}
int AacEncoder::Init(const unsigned char *headData, const int dataLength)
{
 	
}
int AacEncoder::Init(const int sampleRate, const unsigned char channels, const int bitRate)
{
	if ( aacEncOpen( &encodeHandle_, 0, channels ) != AACENC_OK )
	{
		printf( "Unable to open fdkaac encoder\n" );
		return(-1);
	}

	if ( aacEncoder_SetParam( encodeHandle_, AACENC_AOT, 2 ) != AACENC_OK ) /* aac lc */
	{
		printf( "Unable to set the AOT\n" );
		return(-1);
	}

	if ( aacEncoder_SetParam( encodeHandle_, AACENC_SAMPLERATE, sampleRate ) != AACENC_OK )
	{
		printf( "Unable to set the AOT\n" );
		return(-1);
	}
	if ( aacEncoder_SetParam( encodeHandle_, AACENC_CHANNELMODE, channels ) != AACENC_OK ) /* 2 channle */
	{
		printf( "Unable to set the channel mode\n" );
		return(-1);
	}
	if ( aacEncoder_SetParam( encodeHandle_, AACENC_BITRATE, bitRate ) != AACENC_OK ) //bitrates=48000
	{
		printf( "Unable to set the bitrate\n" );
		return(-1);
	}
	if ( aacEncoder_SetParam( encodeHandle_, AACENC_TRANSMUX, 2 ) != AACENC_OK ) /* 0-raw 2-adts */
	{
		printf( "Unable to set the ADTS transmux\n" );
		return(-1);
	}

	if ( aacEncEncode( encodeHandle_, NULL, NULL, NULL, NULL ) != AACENC_OK )
	{
		printf( "Unable to initialize the encoder\n" );
		return(-1);
	}
	
	
	AACENC_InfoStruct info = { 0 };
	if ( aacEncInfo( encodeHandle_, &info ) != AACENC_OK )
	{
		printf( "Unable to get the encoder info\n" );
		return(-1);
	}
	
	printf( "AACEncoderInit   inputChannels = %d, frameLength = %d\n", info.inputChannels, info.frameLength);

	return 0;
}

int AacEncoder::Encode(const unsigned char *inputData, int inputDataLength, unsigned char *outputData, int &outputDataLength)
{

	int size = inputDataLength;
	void *inbuf =(void *)inputData;

	AACENC_BufDesc	in_buf		= { 0 };
	AACENC_BufDesc  out_buf = { 0 };
	AACENC_InArgs	in_args		= { 0 };
	AACENC_OutArgs	out_args	= { 0 };
	int		in_identifier	= IN_AUDIO_DATA;
	int		in_elem_size	= 2;

	in_args.numInSamples		= size / 2;    
	in_buf.numBufs			= 1;
	in_buf.bufs			= &inbuf;  
	in_buf.bufferIdentifiers	= &in_identifier;
	in_buf.bufSizes			= &size;
	in_buf.bufElSizes		= &in_elem_size;

	int	out_identifier	= OUT_BITSTREAM_DATA;
	void	*out_ptr	= outputData;
	int	out_size	= outputDataLength;//
	int	out_elem_size	= 1;
	out_buf.numBufs			= 1;
	out_buf.bufs			= &out_ptr;
	out_buf.bufferIdentifiers	= &out_identifier;
	out_buf.bufSizes		= &out_size;
	out_buf.bufElSizes		= &out_elem_size;

	if ( (aacEncEncode( encodeHandle_, &in_buf, &out_buf, &in_args, &out_args ) ) != AACENC_OK )
	{
		return -1;
	}

	outputDataLength = out_args.numOutBytes;
	
	return	0;
}

}
