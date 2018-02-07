#include "mp3_decoder.h"
#include <stdio.h>


#define OUTPUT_BUFFER_SIZE 4608				


namespace AudioCode
{
Mp3Decoder::Mp3Decoder()
{
	
}

Mp3Decoder::~Mp3Decoder() 
{
	if(mpg123Handle_)
		mpg123_delete(mpg123Handle_);
	mpg123_exit();
}

int Mp3Decoder::Init(const int withHead)
{
	int ret;
	mpg123_init();
	mpg123Handle_ = mpg123_new(NULL, &ret);
    if (ret != MPG123_OK) 
    {
        fprintf(stderr, "some error: %s", mpg123_plain_strerror(ret));
        return -1;
    }
    mpg123_param(mpg123Handle_, MPG123_VERBOSE, 2, 0); /* Brabble a bit about the parsing/decoding. */
    mpg123_open_feed(mpg123Handle_);
    
    mpg123_format_none(mpg123Handle_);
	mpg123_format(mpg123Handle_, 44100, 2, MPG123_ENC_SIGNED_16);	// 统一设置为44.1？

	return 0;
}

int Mp3Decoder::Init(const int withHead, const unsigned char *header, int headerLength)
{

}


/**
* 每次只进一帧数据
*/
int Mp3Decoder::Decode(const unsigned char *inputData, int inputDataLength, unsigned char *outputData, int &outputDataLength)
{
	int ret;
	size_t size = 0;

	if(!mpg123Handle_)		// 如果空则重新初始化
	{
		//Init(1);
		
	}
	ret = mpg123_decode(mpg123Handle_, inputData, inputDataLength, outputData, OUTPUT_BUFFER_SIZE, &size);
	if (ret == MPG123_NEW_FORMAT)
	{
        long rate;
        int channels, enc;
        mpg123_getformat(mpg123Handle_, &rate, &channels, &enc);
        printf("New format: %li Hz, %i channels, encoding value %i\n", rate, channels, enc);
    }
    if(ret == MPG123_ERR ||
   		ret == MPG123_NEED_MORE)
   	{
		printf("some error: %s, size = %d\n", mpg123_strerror(mpg123Handle_), size);
		if(mpg123Handle_)					// 出错则重置
		{
			//mpg123_delete(mpg123Handle_);
			//mpg123Handle_ = NULL;
		}
		ret = -1;
   	}
   	else
   	{
		ret = 0;
   	}
   	outputDataLength = size;

   	return ret;
}
	

}
