#ifndef _AAC_ENCODER_H_
#define _AAC_ENCODER_H_

#include "audio_encoder.h"
#include <fdk-aac/aacenc_lib.h>


namespace AudioCode
{
class AacEncoder:public AudioEncoder
{
public:
	AacEncoder();
	virtual ~AacEncoder();
	virtual int Init(const unsigned char *headData, const int dataLength);
	virtual int Init(const int sampleRate, const unsigned char channels, const int bitRate);
	virtual int Encode(const unsigned char *inputData, int inputDataLength, unsigned char *outputData, int &outputDataLength);
private:
	HANDLE_AACENCODER	encodeHandle_;
};
}
#endif

