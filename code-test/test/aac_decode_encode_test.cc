#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include "audio_decoder.h"
#include "aac_decoder.h"
#include "mp3_decoder.h"

#include "audio_encoder.h"
#include "aac_encoder.h"

using namespace AudioCode;



#define DMX_ANC_BUFFSIZE	128
#define DECODER_MAX_CHANNELS	8
#define MAX_LAYERS		2
#define FRAME_MAX_LEN	1024 * 5
#define BUFFER_MAX_LEN	1024 * 1024 * 1


long GetNowTime()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	/* printf("second = %d, u = %d\n", tv.tv_sec, tv.tv_usec); */
	return( (long) (tv.tv_sec * 1000 + tv.tv_usec / 1000) );
}

/**
 * fetch one ADTS frame
 */
int GetOneAdtsFrame(unsigned char* buffer, size_t bufSize, unsigned char* data ,size_t* dataSize)
{
    size_t size = 0;

    if(!buffer || !data || !dataSize )
    {
        return -1;
    }

    while(1)
    {
        if(bufSize  < 7 )
        {
            return -1;
        }

        if((buffer[0] == 0xff) && ((buffer[1] & 0xf0) == 0xf0) )
        {
            size |= ((buffer[3] & 0x03) <<11);     //high 2 bit
            size |= buffer[4]<<3;                //middle 8 bit
            size |= ((buffer[5] & 0xe0)>>5);        //low 3bit
            break;
        }
        --bufSize;
        ++buffer;
    }

    if(bufSize < size)
    {
        return -1;
    }

    memcpy(data, buffer, size);
    *dataSize = size;
    
    return 0;
}



/**
 * fetch one ADTS frame
 */
int GetOneMp3Frame(unsigned char* buffer, size_t bufSize, unsigned char* data ,size_t* dataSize)// size_t *skipBuf)
{
    size_t size = 0;
    int skipBuf_ = 0;

	*skipBuf  = 0;
    if(!buffer || !data || !dataSize )
    {
        return -1;
    }

    while(1)
    {
        if(bufSize  < 7 )
        {
            return -1;
        }

        if((buffer[0] == 0xff) && ((buffer[1] & 0xe0) == 0xe0) )
        {
        	printf("mp3 frame header 0x%02x, 0x%02x\n", buffer[0], buffer[1]);
            size |= ((buffer[3] & 0x03) <<11);     //high 2 bit
            size |= buffer[4]<<3;                //middle 8 bit
            size |= ((buffer[5] & 0xe0)>>5);        //low 3bit
            break;
        }
        --bufSize;
        ++buffer;
        skipBuf++;
    }

    if(bufSize < size)
    {
        return -1;
    }

    memcpy(data, buffer, size);
    *dataSize = size ;

    return 0;
}

/* gcc -o aac aac_decode_encode.c -l faad -l faac */
int testDocodeEncode( const char *src_file, const char *dst_file, int divison )
{
	static unsigned char	frame[FRAME_MAX_LEN];
	static unsigned char	buffer[BUFFER_MAX_LEN] = { 0 };

	static short resampleBuff[FRAME_MAX_LEN];

	int	resampleIndex	= 0;
	int	i		= 0;
	short	*pcmPtr		= NULL;
	int	resampleCount	= 0;
	FILE	* ifile		= NULL;
	FILE	* ofile		= NULL;

	FILE	* oPcmfile	= NULL;
	long	frameNum	= 0;

	unsigned long	samplerate;
	unsigned char	channels;
	size_t		dataSize	= 0;
	size_t		size		= 0;

	
	long	preTime;
	long	curTime;

    int encodeInit = 0;

	unsigned char *pAacData	= new unsigned char [8192];
	int		aacLen;
	unsigned char *pPcmData	= new unsigned char [8192];
	int		pcmLen;

	AudioDecoder  *pAacDec= new Mp3Decoder();
	pAacDec->Init(1);

	AudioEncoder *pAacEnc = new AacEncoder();

	ifile		= fopen( src_file, "rb" );
	ofile		= fopen( dst_file, "wb" );
	oPcmfile	= fopen( "16.pcm", "wb" );


	dataSize = fread( buffer, 1, BUFFER_MAX_LEN, ifile );
	
	if ( GetOneMp3Frame( buffer, dataSize, frame, &size ) < 0 )
	{
		return(-1);
	}

    long palyStartTime = GetNowTime();
    long palyCurTime = 0;
    long frameTime = 23;
    double frameCount = 0;

    
    printf("palyStartTime %ld\n",  palyStartTime);
    unsigned char *pInputData = buffer;
    int ret;
	while(GetOneAdtsFrame(pInputData, dataSize, frame, &size) == 0)
    {
       	//printf("frame size %d\n", size);

        
		/* decode ADTS frame */
		preTime = GetNowTime();
		ret = pAacDec->Decode( frame, size, pPcmData, pcmLen);
		curTime = GetNowTime();
        frameCount++;
		if((curTime - palyStartTime) < (frameCount * frameTime))
		{
            //usleep(frameTime * 1000);
		}
	 	printf("pcmLen size %d, duration = %d\n", pcmLen, (curTime - palyStartTime)); 
		if ( pcmLen > 0 )
		{
			pcmPtr = (short *) pPcmData;
			for ( i = 0; i < pcmLen / 2; )
			{
				resampleBuff[resampleIndex++]	= *(pcmPtr + i);
				resampleBuff[resampleIndex++]	= *(pcmPtr + i + 1);
				i += (divison * 2);
			}

			if ( ++resampleCount >= divison )
			{
				if(encodeInit == 0)
				{
				    encodeInit = 1;
				    printf("sampleRate = %d\n", pAacDec->GetSampleRate());
                     // 初始化编码器
                    pAacEnc->Init(pAacDec->GetSampleRate()/divison, pAacDec->GetChannels(), 128000/divison);
				}
				aacLen = 8192;
				if(pAacEnc->Encode((const unsigned char *)resampleBuff, pcmLen, pAacData, aacLen) != 0)
				{
					printf("aac Encode failed\n");
				}
			 	//printf("aacLen size %d, frame size = %d\n", aacLen,  size);
				
				fwrite( pAacData, 1, aacLen, ofile ); //2个通道
				/*
				 * fflush(ofile);
				 * fwrite(resampleBuff, 1, pcmLen, oPcmfile);
				 * fflush(oPcmfile);
				 */
				resampleCount	= 0;
				resampleIndex	= 0;
				if ( frameNum++ % 100 == 0 )
					printf( "frame2 = %ld, t = %ld, dec time = %ld\n", frameNum, GetNowTime() / 1000, curTime - preTime );
			}
		}
		dataSize	-= size;
		pInputData	+= size;
	}

	if ( pAacData )
		delete [] pAacData;
	if ( pPcmData )
		delete []   pPcmData ;
		
	if(pAacDec)
		delete pAacDec;
	if(pAacEnc)
		delete pAacEnc;
		
	fclose( ifile );
	fclose( ofile );
	fclose( oPcmfile );
}


static void show_usage(void)
{
    printf("./code-test srcFile dstFile division\n");
}

int main( int argc, char *argv[] )
{
	char	src_file[128]	= { 0 };
	char	dst_file[128]	= { 0 };
	int	divison		= 2;  		// 分频44.1k 转成 4/2 分频
	long	preTime		= 0;
	long	curTime		= 0;
	/* analyse parameter */
	if ( argc < 3 )
	{
		show_usage();
		return(-1);
	}
	sscanf( argv[1], "%s", src_file );
	sscanf( argv[2], "%s", dst_file );
	printf( "src %s, dst %s, d %s\n", argv[1], argv[2], argv[3] );
	divison = atoi( argv[3] );
	preTime = GetNowTime();
	printf( "time 11 = %ldms\n", preTime );
	testDocodeEncode( src_file, dst_file, divison );
	curTime = GetNowTime();
	printf( "time O1 12 = %ldms, consume time = %ldms\n", curTime, curTime - preTime );
	return(0);
}



