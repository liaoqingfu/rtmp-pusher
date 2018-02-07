#ifndef _AUDIO_ENCODER_H_
#define _AUDIO_ENCODER_H_
#define nullptr NULL

namespace AudioCode
{
enum AudioFileType
{
	AFT_DEFAULT,
	AFT_PCM,
	AFT_WAV,
	AFT_MP3,
	AFT_AAC,
	AFT_G7221,
	AFT_OPUS,
	//...OTHER TYPE
};

class AudioEncoder
{
public:
	AudioEncoder() {}
	virtual ~AudioEncoder() {}
	virtual int Init(const unsigned char *headData, const int dataLength) {}
	virtual int Init(const int sampleRate, const unsigned char channels, const int bitRate) {}
	virtual int Encode(const unsigned char *inputData, int inputDataLength, unsigned char *outputData, int &outputDataLength) = 0;
protected:
	int 			sampleRate_ = 44100;
	unsigned char 	channels_ = 2;
	int 			bitRate_ = 128000;
};
}
#endif
