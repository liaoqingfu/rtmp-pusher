#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <mpg123.h>


/*** Note- Assumes little endian ***/
void printBits(size_t const size, void const * const ptr);
int findFrameSamplingFrequency(const unsigned char ucHeaderByte);
int findFrameBitRate (const unsigned char ucHeaderByte);
int findMpegVersionAndLayer (const unsigned char ucHeaderByte);
int findFramePadding (const unsigned char ucHeaderByte);
void printmp3details(unsigned int nFrames, unsigned int nSampleRate, double fAveBitRate);

#define OUTBUFF 4608// (1152*2*4*2)//32768*4

int main(int argc, char** argv)
{
    FILE * ifMp3;
    FILE	* ofile		= NULL;
    
    ifMp3 = fopen(argv[1],"rb");
    ofile		= fopen( "raw_s16_2.pcm", "wb" );
    
 	unsigned char outbuf[OUTBUFF];
 	unsigned char poutbuf;
 	size_t size;
    mpg123_init();
    int ret;
    mpg123_handle *m = m = mpg123_new(NULL, &ret);
    if (ret != MPG123_OK) {
        fprintf(stderr, "some error: %s", mpg123_plain_strerror(ret));

    }
    mpg123_param(m, MPG123_VERBOSE, 2, 0); /* Brabble a bit about the parsing/decoding. */
    mpg123_open_feed(m);
    
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
	unsigned char inputBuff[8196];
    int nFrames, nFileSampleRate;
    float fBitRateSum=0;
    long int lnPreviousFramePosition;
	//mpg123_format_none(m);
	//mpg123_format(m, 44100, 2, MPG123_ENC_16);
	mpg123_format_none(m);
	mpg123_format(m, 44100, 2, MPG123_ENC_SIGNED_16);
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
                printf("Found frame %d at offset = %ld B\nHeader Bits:\n",
                       nFrames, ftell(ifMp3));
                //usleep(1000);       
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
                printf("Frame Length: %d Bytes \n\n", nFrameLength);

                lnPreviousFramePosition=ftell(ifMp3)-4; //the position of the first byte of this frame
				inputBuff[0] = ucHeaderByte1;
				inputBuff[1] = ucHeaderByte2;
				inputBuff[2] = ucHeaderByte3;
				inputBuff[3] = ucHeaderByte4;
              
				if(1)//nFrameBitRate == 192000)
				{
					  //move file position by forward by frame length to bring it to next frame:
	               // fseek(ifMp3,nFrameLength-4,SEEK_CUR);
					if(fread(&inputBuff[4], 1, nFrameLength-4,ifMp3)  != (nFrameLength-4))
					{
						printf("Error: fread a frame\n\n");
						goto syncWordSearch;
					}
					
					ret = mpg123_decode(m, inputBuff, nFrameLength, outbuf, OUTBUFF, &size);
					//ret = mpg123_decode_frame(m, inputBuff, nFrameLength, &poutbuf, OUTBUFF, &size);
	        		if (ret == MPG123_NEW_FORMAT) {
			            long rate;
			            int channels, enc;
			            mpg123_getformat(m, &rate, &channels, &enc);
			            printf("New format: %li Hz, %i channels, encoding value %i\n", rate, channels, enc);
			        }
			        if(ret == MPG123_ERR ||
	               		ret == MPG123_NEED_MORE)
	               	{
						printf("some error: %s\n", mpg123_strerror(m));
	               	}
	               	printf("write a frame size = %d, pcm size = %d, BitRate = %d\n", nFrameLength, size, nFrameBitRate);
	               	fwrite( outbuf, 1, size, ofile ); 
	               	//ret = mpg123_decode(m, NULL, 0, outbuf, OUTBUFF, &size);
	               	//printf("ret: %s\n", mpg123_strerror(m));
               	}
               	else
               	{
					printf("nFrameBitRate = %d\n", nFrameBitRate);
               	}
            }
        }
    }
    float fFileAveBitRate= fBitRateSum/nFrames;

    printmp3details(nFrames,nFileSampleRate,fFileAveBitRate);
	fclose( ofile );
	fclose(ifMp3);
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
        printf("padded: true\n");
    }
    else
    {
        nFramePadded = 0;
        printf("padded: false\n");
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
        printf("MPEG Version 1 Layer III \n");
    }
    else
    {
        MpegVersionAndLayer = 1;
        printf("Not MPEG Version 1 Layer III \n");
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

