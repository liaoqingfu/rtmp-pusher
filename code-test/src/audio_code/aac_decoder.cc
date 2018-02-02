#include "aac_decoder.h"
#include <stdio.h>
namespace AudioCode
{

#define MAX_LAYERS		2
#define FRAME_MAX_LEN	1024 * 5
#define BUFFER_MAX_LEN	1024 * 1024 * 1


AacDecoder::AacDecoder()
{
	withHead_ = 1;
	decoderHandle_ = nullptr;
}
AacDecoder::~AacDecoder()
	
{
	if(decoderHandle_)
	{
		aacDecoder_Close( decoderHandle_ );
	}
}
int AacDecoder::Init(const int withHead)
{
	decoderHandle_ = aacDecoder_Open( TT_MP4_ADTS, 1 );
	if (aacDecoder_SetParam( decoderHandle_, AAC_CONCEAL_METHOD, 0 ) != AAC_DEC_OK )
	{
		printf( "Unable to set error concealment method\n" );
		return(-1);
	}
	return 0;
}
int AacDecoder::Init(const int withHead, const unsigned char *header, int headerLength)
{
	return 0;
}
int AacDecoder::Decode(const unsigned char *inputData, int inputDataLength, unsigned char *outputData, int &outputDataLength)
{
	AAC_DECODER_ERROR	err;
	UCHAR			* inBuffer[MAX_LAYERS];
	UINT			inBufferLength[MAX_LAYERS]	= { 0 };
	UINT			bytesValid			= (UINT) inputDataLength;
	inBuffer[0]		= (UCHAR *)inputData;
	inBufferLength[0]	= (UINT) inputDataLength;

	err = aacDecoder_Fill( decoderHandle_, inBuffer, inBufferLength, &bytesValid );
	if ( err != AAC_DEC_OK )
	{
		printf( "[aac] aacDecoder_Fill() failed: %x\n", err );
		return(-1);
	}

	err = aacDecoder_DecodeFrame( decoderHandle_, (INT_PCM *) outputData, 4096, 0 );
	if ( err == AAC_DEC_NOT_ENOUGH_BITS )
	{
		printf( "[aac] aacDecoder_DecodeFrame() net enough bits: %x\n", err );
		return(-1);
	}
	if ( err != AAC_DEC_OK )
	{
		printf( "[aac] aacDecoder_DecodeFrame() failed: %x\n", err );
		return(-1);
	}

	CStreamInfo *info = aacDecoder_GetStreamInfo( decoderHandle_ );

	/* printf("numChannels = %d, frameSize= %d, sampleRate= %d\n", info->numChannels, info->frameSize, info->sampleRate); */

    channels_ = info->numChannels;
    sampleRate_ = info->sampleRate;
    bitRate_ = info->bitRate;
    sampleBit_ = 
    outputDataLength = (int) (info->frameSize * info->numChannels * 2);
    
	return 0;
}
}
