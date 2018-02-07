#ifndef _AUDIO_FRAME_POOL_
#define _AUDIO_FRAME_POOL_
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "bounded_queue.h"
#include "mutex.h"

#include "shared_buffer.h"
#include "boost/unordered_map.hpp" 


#include <map>

// 音频帧缓存池, MP3帧或者AAC帧都先缓存在AudioFramePool，然后分发给各个终端.


using namespace std;
class AudioFramePool
{
public:
	typedef  boost::shared_ptr<AudioFramePool> AudioFramePoolPtr;
	typedef  boost::shared_ptr<BoundedQueue> FrameQueuePtr;
	typedef enum {eAudioMp3 = 0,  eAudioAac}EAudioType;
	explicit AudioFramePool(int round);
	~AudioFramePool();
	int RegisterFramesPool(EAudioType audioType, int maxFrames);			// 注册帧池
	int UnregisterFramesPool(EAudioType audioType);
	int PutFrame(EAudioType audioType, Buffer::BufferPtr &buf);
	int TakeFrame(EAudioType audioType, Buffer::BufferPtr &buf);
private:
	int round_ = 1;				// round_ 是否循环覆盖
	typedef  std::map<EAudioType, BoundedQueue*> FramePoolMap;

	FramePoolMap framePoolMap_;
	mutable MutexLock mutex_;
};

#endif
