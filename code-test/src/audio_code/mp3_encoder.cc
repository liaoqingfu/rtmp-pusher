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
/*
设置比特率控制模式，默认是CBR，但是通常我们都会设置VBR。
参数是枚举，vbr_off代表CBR，vbr_abr代表ABR（因为ABR不常见，所以本文不对ABR做讲解）vbr_mtrh代表VBR。

*/
	mp3Handle_ = lame_init ();
	
	lame_set_in_samplerate (mp3Handle_, sampleRate);		// 设置最终mp3编码输出的声音的采样率，如果不设置则和输入采样率一样。
	
	lame_set_num_channels(mp3Handle_,channels);//默认为2双通道
 	lame_set_mode(mp3Handle_, STEREO);			// 立体声
	lame_set_quality(mp3Handle_,5); /* 2=high 5 = medium 7=low 音质*/
	lame_set_VBR(mp3Handle_, vbr_off);
	lame_set_brate(mp3Handle_,bitRate/1000);// 以khz为单位, 设置CBR的比特率，只有在CBR模式下才生效。
											//lame_set_VBR_mean_bitrate_kbps：设置VBR的比特率，只有在VBR模式下才生效。
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
	outputDataLength = size;

	if(size > 0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

}// end of namespace AudioCodee
