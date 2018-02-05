//https://github.com/pokey909/mp3dec/blob/e93df8a7a776b34831ffa1f6b73c15b519e5e093/main.cpp
#include <iostream>
#include <mpg123.h>
#include <fstream>
#include <sstream>

using namespace std;
#define INBUFF 16384*4
#define OUTBUFF (1152*2*4)//32768*4

int main() {
    unsigned char outbuf[OUTBUFF];
    unsigned char inbuf[INBUFF];
    mpg123_init();
    int ret;
    mpg123_handle *m = m = mpg123_new(NULL, &ret);
    if (ret != MPG123_OK) {
        fprintf(stderr, "some error: %s", mpg123_plain_strerror(ret));

    }
    mpg123_param(m, MPG123_VERBOSE, 2, 0); /* Brabble a bit about the parsing/decoding. */
    mpg123_open_feed(m);

    ifstream input;
    ofstream foutput("out.raw", std::ifstream::out | std::ifstream::binary);
    ostringstream output;
    size_t outBytes = 0;

    input.open("oh-Yuki.mp3", std::ifstream::in | std::ifstream::binary);
    while (input.good()) {
        input.read(reinterpret_cast<char *>(inbuf), INBUFF);
        streamsize len = input.gcount();
        size_t size;
        mpg123_format_none(m);
        mpg123_format(m, 44100, 2, MPG123_ENC_FLOAT_32);

        ret = mpg123_decode(m, inbuf, len, outbuf, OUTBUFF, &size);
        if (ret == MPG123_NEW_FORMAT) {
            long rate;
            int channels, enc;
            mpg123_getformat(m, &rate, &channels, &enc);
            fprintf(stderr, "New format: %li Hz, %i channels, encoding value %i\n", rate, channels, enc);
        }
        output.write(reinterpret_cast<char *>(outbuf), size);
        outBytes += size;

        while (ret != MPG123_ERR &&
               ret != MPG123_NEED_MORE) { /* Get all decoded audio that is available now before feeding more input. */
            ret = mpg123_decode(m, NULL, 0, outbuf, OUTBUFF, &size);
            output.write(reinterpret_cast<char *>(outbuf), size);
            //fprintf(stderr, "size = %d\n", size);
            outBytes += size;
        }
        if (ret == MPG123_ERR) {
            fprintf(stderr, "some error: %s", mpg123_strerror(m));
            break;
        }
        
    }
    foutput.write(output.str().data(), outBytes);
    /* Done decoding, now just clean up and leave. */
    mpg123_delete(m);
    mpg123_exit();
    return 0;
}

