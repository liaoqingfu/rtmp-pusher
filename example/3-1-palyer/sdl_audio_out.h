#ifndef _SDL_AUDIO_OUT_H_
#define _SDL_AUDIO_OUT_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#include <sys/time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <libavformat/avformat.h>


void SDL2AudioOutInit(AVCodecContext	*pAudioCodecCtx);
void SDL2AudioPushPacket(AVPacket *packet);
void SDL2AudioOutPause(int pause);

#ifdef __cplusplus
}
#endif

#endif

