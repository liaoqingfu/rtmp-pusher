#ifndef _MP3_DECODER_H_
#define _MP3_DECODER_H_

#include "audio_decoder.h"
#include <mad.h>

namespace AudioCode
{
class Mp3Decoder : public AudioDecoder
{
public:
	Mp3Decoder();
	virtual ~Mp3Decoder();
	virtual int Init(const int withHead);
	virtual int Init(const int withHead, const unsigned char *header, int headerLength);
	virtual int Decode(const unsigned char *inputData, int inputDataLength, unsigned char *outputData, int &outputDataLength);
	

private:
	struct mad_synth synth_;
	struct mad_frame frame_;
	struct mad_stream stream_;

};
}
#endif

