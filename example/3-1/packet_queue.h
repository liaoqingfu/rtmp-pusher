#ifndef PACKET_QUEUE_H_
#define PACKET_QUEUE_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>

typedef struct PacketQueue
{
    AVPacketList    *first_pkt;     //队头的一个packet, 注意类型不是AVPacket
    AVPacketList    *last_pkt;      //队尾packet
    int             nb_packets;     // paket个数
    int             size;           //
    SDL_mutex       *mutex;         //
    SDL_cond        *cond;          // 条件变量
}PacketQueue;

void packet_queue_init(PacketQueue *queue);
int packet_queue_put(PacketQueue *queue, AVPacket *packet);
int packet_queue_get(PacketQueue *queue, AVPacket *pakcet, int block);

#ifdef __cplusplus
}
#endif
#endif

