#ifndef _SDL_DISPLAY_H_
#define _SDL_DISPLAY_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#include <sys/time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <libavformat/avformat.h>


void SDL2DisplayInit(int width, int height, enum AVPixelFormat pix_fmt);
void SDL2Display(AVFrame *pVideoFrame, AVFrame* pFrameYUV, int height);
void SDL2DisplayDestory();
#ifdef __cplusplus
}
#endif

#endif

