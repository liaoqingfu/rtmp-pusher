#include "mp3_encoder.h"


namespace AudioCode
{

Mp3Encoder::Mp3Encoder()
{
	
}

Mp3Encoder::~Mp3Encoder()
{
	lame_close(mp3Handle_);
}

int Mp3Encoder::Init(const unsigned char *headData, const int dataLength)
{
	mp3Handle_ = lame_init ();
	
	lame_set_in_samplerate (mp3Handle_, 44100);
	lame_set_VBR (mp3Handle_, vbr_off);
	lame_init_params (mp3Handle_);
	return 0;
}

int Mp3Encoder::Init(const int sampleRate, const unsigned char channels, const int bitRate)
{
	mp3Handle_ = lame_init ();
	
	lame_set_in_samplerate (mp3Handle_, sampleRate);
	//lame_set_VBR (mp3Handle_, vbr_default);
	lame_set_brate(mp3Handle_,bitRate);
	lame_set_num_channels(mp3Handle_,2);//默认为2双通道
 	lame_set_mode(mp3Handle_, STEREO);			// 立体声
	lame_set_quality(mp3Handle_,5); /* 2=high 5 = medium 7=low 音质*/
	lame_set_VBR(mp3Handle_, vbr_default);
	lame_init_params (mp3Handle_);

	return 0;
}

// 格式LRLRLR...，不是LLL...RRR...
int Mp3Encoder::Encode(const unsigned char *inputData, int inputDataLength, 
	unsigned char *outputData, int &outputDataLength)
{
	
	int size;
	int bufSize = outputDataLength;

	size = lame_encode_buffer_interleaved (mp3Handle_, (short int *)inputData, MP3_FRAME_SAMPLES, outputData, bufSize);
	//size = lame_encode_buffer(mp3Handle_,  pPcmData[0], pPcmData[1], MP3_FRAME_SAMPLES, outputData, bufSize);
	//printf("len = %d, samples = %d, bufSize = %d, size = %d\n", inputDataLength/4, MP3_FRAME_SAMPLES, bufSize, size);
	outputDataLength = size;
	return 0;
}


}

