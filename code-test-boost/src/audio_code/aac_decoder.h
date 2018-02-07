#ifndef _AAC_DECODER_H_
#define _AAC_DECODER_H_


/*
1. dir2/foo2.h (优先位置, 详情如下)
2. C 系统文件
3. C++ 系统文件
4. 其他库的 .h 文件
5. 本项目内 .h 文件

*/
#include "audio_decoder.h"
#include <fdk-aac/aacdecoder_lib.h>

namespace AudioCode
{
class AacDecoder : public AudioDecoder
{
public:
	AacDecoder();
	virtual ~AacDecoder();
	virtual int Init(const int withHead);
	virtual int Init(const int withHead, const unsigned char *header, int headerLength);
	virtual int Decode(const unsigned char *inputData, int inputDataLength, unsigned char *outputData, int &outputDataLength);
private:
	HANDLE_AACDECODER decoderHandle_;
	int withHead_;			// 0 解码时数据不带adts header， 1解码是数据带adts header
};
}
#endif

