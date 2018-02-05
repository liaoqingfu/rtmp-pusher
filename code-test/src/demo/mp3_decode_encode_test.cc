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
#include "mp3_decoder.h"

#include "audio_encoder.h"
#include "mp3_encoder.h"
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



/*** Note- Assumes little endian ***/
void printBits(size_t const size, void const * const ptr);
int findFrameSamplingFrequency(const unsigned char ucHeaderByte);
int findFrameBitRate (const unsigned char ucHeaderByte);
int findMpegVersionAndLayer (const unsigned char ucHeaderByte);
int findFramePadding (const unsigned char ucHeaderByte);
void printmp3details(unsigned int nFrames, unsigned int nSampleRate, double fAveBitRate);

#define OUTBUFF_MP3_PCM_FRAME 4608// (1152*2*4*2)//32768*4
#define OUTBUFF_MP3_RAW_FRAME 4608
#define OUTBUFF_AAC_RAW_FRAME 4096
#define OUTBUFF_AAC_PCM_FRAME 4096

int mp3DecodeEncode(const char *src_file, const char *dst_file)
{
    FILE * ifMp3;
    FILE	* outMp3File		= NULL;
    FILE	*outPcmFile;
    FILE	* outAacFile		= NULL;
    
    ifMp3 = fopen(src_file,"rb");
    outMp3File		= fopen( dst_file, "wb" );
    outPcmFile		= fopen( "16.pcm", "wb" );
	outAacFile		= fopen( "16.aac", "wb" );
    
 	
 	unsigned char poutbuf;
 	
 	int size;

 	AudioDecoder *mp3Decoder = new Mp3Decoder();
   	if(mp3Decoder->Init(1) != 0)
   	{
        fprintf(stderr, "mp3Decoder->Init(1) failed\n");
		return -1;
    }

    AudioEncoder *mp3Encoder = new Mp3Encoder();
    if(mp3Encoder->Init(44100, 2, 192000) != 0)
    {
        fprintf(stderr, "mp3Encoder->Init(44100, 2, 128000) failed\n");
		return -1;
    }

    AudioEncoder *aacEncoder = new AacEncoder();
    if(aacEncoder->Init(44100, 2, 192000) != 0)
    {
        fprintf(stderr, "mp3Encoder->Init(44100, 2, 192000) failed\n");
		return -1;
    }

    
    int ret;


    
    //ifMp3 = fopen("floating.mp3","rb");
    if (ifMp3 == NULL)
    {
        perror("Error");
        return -1;
    }
    printf("File opened\n");

    //get file size:
    fseek(ifMp3, 0, SEEK_END);
    long int lnNumberOfBytesInFile = ftell(ifMp3);
    rewind(ifMp3);

    unsigned char ucHeaderByte1, ucHeaderByte2, ucHeaderByte3, ucHeaderByte4;   //stores the 4 bytes of the header
    unsigned char mp3PcmBuffer[OUTBUFF_MP3_PCM_FRAME];
	unsigned char mp3RawBuffer[OUTBUFF_MP3_RAW_FRAME];
	unsigned char aacRawBuffer[OUTBUFF_AAC_RAW_FRAME];
	unsigned char aacPcmBuffer[OUTBUFF_AAC_PCM_FRAME];
	int aacPcmIndex = 0;
	int remainPcmByte = 0;
	
    int nFrames, nFileSampleRate;
    float fBitRateSum=0;
    long int lnPreviousFramePosition;
	aacPcmIndex = 0;
syncWordSearch:
    while( ftell(ifMp3) < lnNumberOfBytesInFile)
    {
    	ucHeaderByte1 = getc(ifMp3);
        if( ucHeaderByte1 == 0xFF )
        {
            ucHeaderByte2 = getc(ifMp3);
            unsigned char ucByte2LowerNibble = ucHeaderByte2 & 0xF0;
            if( ucByte2LowerNibble == 0xF0 || ucByte2LowerNibble == 0xE0 )
            {
                /*if(nFrames>1){
                    printf("Previous Frame Length: %ld\n\n",ftell(ifMp3)-2 -lnPreviousFramePosition);
                }*/
                ++nFrames;
                //printf("Found frame %d at offset = %ld B\nHeader Bits:\n", nFrames, ftell(ifMp3));
                usleep(10000);       
                //get the rest of the header:
                ucHeaderByte3=getc(ifMp3);
                ucHeaderByte4=getc(ifMp3);
                //print the header:
                //printBits(sizeof(ucHeaderByte1),&ucHeaderByte1);
                //printBits(sizeof(ucHeaderByte2),&ucHeaderByte2);
                //printBits(sizeof(ucHeaderByte3),&ucHeaderByte3);
               	// printBits(sizeof(ucHeaderByte4),&ucHeaderByte4);
                //get header info:
                int nFrameSamplingFrequency = findFrameSamplingFrequency(ucHeaderByte3);
                int nFrameBitRate = findFrameBitRate(ucHeaderByte3);
                int nMpegVersionAndLayer = findMpegVersionAndLayer(ucHeaderByte2);

                if( nFrameBitRate==0 || nFrameSamplingFrequency == 0 || nMpegVersionAndLayer==0 )
                {//if this happens then we must have found the sync word but it was not actually part of the header
                    --nFrames;
                    printf("Error: not a header\n\n");
                    goto syncWordSearch;
                }
                fBitRateSum += nFrameBitRate;
                if(nFrames==1)
                { 
                	nFileSampleRate = nFrameSamplingFrequency; 
                }
                int nFramePadded = findFramePadding(ucHeaderByte3);
                //calculate frame size:
                int nFrameLength = int((144 * (float)nFrameBitRate /
                                                  (float)nFrameSamplingFrequency ) + nFramePadded);
               // printf("Frame Length: %d Bytes \n\n", nFrameLength);

                lnPreviousFramePosition=ftell(ifMp3)-4; //the position of the first byte of this frame
				mp3RawBuffer[0] = ucHeaderByte1;
				mp3RawBuffer[1] = ucHeaderByte2;
				mp3RawBuffer[2] = ucHeaderByte3;
				mp3RawBuffer[3] = ucHeaderByte4;
              
				if(1)//nFrameBitRate == 192000)
				{
					if(fread(&mp3RawBuffer[4], 1, nFrameLength-4,ifMp3)  != (nFrameLength-4))
					{
						printf("Error: fread a frame\n\n");
						goto syncWordSearch;
					}
					size = OUTBUFF_MP3_PCM_FRAME;
					if(mp3Decoder->Decode(mp3RawBuffer, nFrameLength, mp3PcmBuffer, size) != 0)
					{
						 printf("mp3Decoder->Decode failed, size = %d\n", size);
					}
					else 
					{
		               	//printf("write a frame size = %d, pcm size = %d, BitRate = %d\n", nFrameLength, size, nFrameBitRate);
		               	//fwrite( mp3PcmBuffer, 1, size, outPcmFile);
		               	nFrameLength = OUTBUFF_MP3_RAW_FRAME;
		               	if(mp3Encoder->Encode(mp3PcmBuffer, size, mp3RawBuffer, nFrameLength) == 0)
		               	{
		               		//printf("write a mp3 frame size = %d\n", nFrameLength);
							fwrite( mp3RawBuffer, 1, nFrameLength, outMp3File ); 
		               	}

						
						remainPcmByte = OUTBUFF_AAC_PCM_FRAME - aacPcmIndex;			// 还差多少数据
						//printf("1 remainPcmByte size = %d, aacPcmIndex = %d\n", remainPcmByte, aacPcmIndex);
						memcpy(&aacPcmBuffer[aacPcmIndex], mp3PcmBuffer, remainPcmByte);
		
		               	nFrameLength = OUTBUFF_AAC_RAW_FRAME;
		               	if(aacEncoder->Encode(aacPcmBuffer, OUTBUFF_AAC_PCM_FRAME, aacRawBuffer, nFrameLength) == 0)
		               	{
							fwrite( aacRawBuffer, 1, nFrameLength, outAacFile ); 
							//printf("1 write a aac frame size = %d\n", nFrameLength);
		               	}
		               	else
		               	{
							printf("1 write a aac frame failed\n");
		               	}
		               	aacPcmIndex = OUTBUFF_MP3_PCM_FRAME - remainPcmByte;		//MP3 buffer剩余的数据

		               	//printf("2 remainPcmByte size = %d, aacPcmIndex = %d\n", remainPcmByte, aacPcmIndex);
		               	memcpy(&aacPcmBuffer[0], &mp3PcmBuffer[remainPcmByte], aacPcmIndex);
		               	
		               	if(aacPcmIndex >= OUTBUFF_AAC_PCM_FRAME)
		               	{
							aacPcmIndex -= OUTBUFF_AAC_PCM_FRAME;
							nFrameLength = OUTBUFF_AAC_RAW_FRAME;			// 一定要设置，让底层知道你的buffer长度.
							if(aacEncoder->Encode((const unsigned char *)aacPcmBuffer, OUTBUFF_AAC_PCM_FRAME, aacRawBuffer, nFrameLength) == 0)
			               	{
								fwrite( aacRawBuffer, 1, nFrameLength, outAacFile); 
								//printf("2 write a aac frame size = %d\n", nFrameLength);
			               	}
			               	else
			               	{
								printf("2 write a aac frame failed\n");
			               	}
		               	}
		               	
	               	}
               	}
               	else
               	{
					printf("nFrameBitRate = %d\n", nFrameBitRate);
               	}
            }
        }
    }
    float fFileAveBitRate= fBitRateSum/nFrames;

    //printmp3details(nFrames,nFileSampleRate,fFileAveBitRate);
	fclose( outMp3File );
	fclose(ifMp3);
	fclose(outPcmFile);
	fclose(outAacFile);

	if(mp3Decoder)
		delete mp3Decoder;
	if(aacEncoder)
		delete aacEncoder;	
	if(mp3Encoder)
		delete mp3Encoder;	
    return 0;
}

void printmp3details(unsigned int nFrames, unsigned int nSampleRate, double fAveBitRate)
{
    printf("MP3 details:\n");
    printf("Frames: %d\n", nFrames);
    printf("Sample rate: %d\n", nSampleRate);
    printf("Ave bitrate: %0.0f\n", fAveBitRate);
}

int findFramePadding (const unsigned char ucHeaderByte)
{
    //get second to last bit to of the byte
    unsigned char ucTest = ucHeaderByte & 0x02;
    //this is then a number 0 to 15 which correspond to the bit rates in the array
    int nFramePadded;
    if( (unsigned int)ucTest==2 )
    {
        nFramePadded = 1;
        //printf("padded: true\n");
    }
    else
    {
        nFramePadded = 0;
        //printf("padded: false\n");
    }
    return nFramePadded;
}

int findMpegVersionAndLayer (const unsigned char ucHeaderByte)
{
    int MpegVersionAndLayer;
    //get bits corresponding to the MPEG verison ID and the Layer
    unsigned char ucTest = ucHeaderByte & 0x1E;
    //we are working with MPEG 1 and Layer III
    if(ucTest == 0x1A)
    {
        MpegVersionAndLayer = 1;
        //printf("MPEG Version 1 Layer III \n");
    }
    else
    {
        MpegVersionAndLayer = 1;
        //printf("Not MPEG Version 1 Layer III \n");
    }
    return MpegVersionAndLayer;
}

int findFrameBitRate (const unsigned char ucHeaderByte)
{
    unsigned int bitrate[] = {0,32000,40000,48000,56000,64000,80000,96000,
                         112000,128000,160000,192000,224000,256000,320000,0};
    //get first 4 bits to of the byte
    unsigned char ucTest = ucHeaderByte & 0xF0;
    //move them to the end
    ucTest = ucTest >> 4;
    //this is then a number 0 to 15 which correspond to the bit rates in the array
     int unFrameBitRate = bitrate[(unsigned int)ucTest];
     if(unFrameBitRate < 64000)
    printf("Bit Rate: %u\n",unFrameBitRate);
    return unFrameBitRate;
}

 int findFrameSamplingFrequency(const unsigned char ucHeaderByte)
{
    unsigned int freq[] = {44100,48000,32000,00000};
    //get first 2 bits to of the byte
    unsigned char ucTest = ucHeaderByte & 0x0C;
    ucTest = ucTest >> 6;
    //then we have a number 0 to 3 corresponding to the freqs in the array
     int unFrameSamplingFrequency = freq[(unsigned int)ucTest];
    if(unFrameSamplingFrequency != 44100 && unFrameSamplingFrequency != 48000)
    printf("Sampling Frequency: %u\n",unFrameSamplingFrequency);
    return unFrameSamplingFrequency;
}

void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {
        for (j=7;j>=0;j--)
        {
            byte = b[i] & (1<<j);
            byte >>= j;
            printf("%u", byte);
        }
    }
    puts("");
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
	mp3DecodeEncode( src_file, dst_file);
	curTime = GetNowTime();
	printf( "time O1 12 = %ldms, consume time = %ldms\n", curTime, curTime - preTime );
	return(0);
}



