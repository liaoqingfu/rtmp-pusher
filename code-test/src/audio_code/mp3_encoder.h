#ifndef _MP3_ENCODER_H_
#define _MP3_ENCODER_H_
#include "audio_encoder.h"

#include <lame/lame.h>
namespace AudioCode
{

#define MP3_FRAME_SAMPLES		1152
#define MP3_CHANNELS			2

class Mp3Encoder:public AudioEncoder
{
public:
	Mp3Encoder();
	virtual ~Mp3Encoder();
	virtual int Init(const unsigned char *headData, const int dataLength);
	virtual int Init(const int sampleRate, const unsigned char channels, const int bitRate);
	virtual int Encode(const unsigned char *inputData, int inputDataLength, unsigned char *outputData, int &outputDataLength);
private:
	lame_t mp3Handle_ = nullptr;
	short int pcmData_[MP3_CHANNELS][MP3_FRAME_SAMPLES];			// 暂存两个Channel的pcm数据
};
}
#endif
