#include "mp3_decoder.h"
#include <stdio.h>

#define OUTPUT_BUFFER_SIZE 8092

namespace AudioCode
{
Mp3Decoder::Mp3Decoder()
{}

Mp3Decoder::~Mp3Decoder() 
{}

int Mp3Decoder::Init(const int withHead)
{
	mad_stream_init(&stream_);
    mad_frame_init(&frame_);
    mad_synth_init(&synth_);
}

int Mp3Decoder::Init(const int withHead, const unsigned char *header, int headerLength)
{

}

int scale(mad_fixed_t sample)
{
    sample += (1L << (MAD_F_FRACBITS - 16));
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    return sample >> (MAD_F_FRACBITS + 1 - 16);
}


/**
* 每次只进一帧数据
*/
int Mp3Decoder::Decode(const unsigned char *inputData, int inputDataLength, unsigned char *outputData, int &outputDataLength)
{
	mad_stream_buffer(&stream_, inputData, inputDataLength);
	
	if (mad_frame_decode(&frame_, &stream_))
	{
	   if (MAD_RECOVERABLE(stream_.error))
	   {
		   if (stream_.error != MAD_ERROR_LOSTSYNC)
		   {
			   printf("mad_frame_decode wran : code %x !\n", stream_.error);
		   }
		   return -1;
	   }
	   else if (stream_.error == MAD_ERROR_BUFLEN)
	   {
	   		printf("mad_frame_decode MAD_ERROR_BUFLEN !\n");
		   return -1;;
	   }
	   else
	   {
		   printf("mad_frame_decode error = %x!\n", stream_.error);
		   return -1;;
	   }
	}

	mad_synth_frame(&synth_, &frame_);

	struct mad_pcm *pcm = &synth_.pcm;
	unsigned int nchannels, nsamples, n;
    mad_fixed_t const *left_ch, *right_ch;
    unsigned char  *outputPtr;
    int wrote, speed, err;

    nchannels = pcm->channels;
    n = nsamples = pcm->length;		// 每个Channel的sample数量
    left_ch = pcm->samples[0];
    right_ch = pcm->samples[1];

    outputPtr = outputData;
	printf("nsamples = %d\n", nsamples);
    while (nsamples--)
    {
        signed int sample;
        sample = scale(*left_ch++);
        *(outputPtr++) = sample >> 0;
        *(outputPtr++) = sample >> 8;

        if (nchannels == 2)
        {
            sample = scale(*right_ch++);
            *(outputPtr++) = sample >> 0;
            *(outputPtr++) = sample >> 8;
        }
        else if (nchannels == 1)
        {
            *(outputPtr++) = sample >> 0;
            *(outputPtr++) = sample >> 8;
        }
    }
    outputDataLength = pcm->length * pcm->channels *2;

    printf("outputDataLength = %d\n", outputDataLength);
}
	

}
