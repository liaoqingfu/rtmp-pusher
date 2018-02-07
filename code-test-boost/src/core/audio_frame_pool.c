#include "audio_frame_pool.h"


AudioFramePool::AudioFramePool(int round)
	: round_(round)
{
	
}
AudioFramePool::~AudioFramePool()
{
	// ���ٶ���
	MutexLockGuard lock(mutex_);
	std::map<EAudioType, BoundedQueue *>::iterator
		mapItem = framePoolMap_.find(eAudioMp3);
	if(mapItem != framePoolMap_.end())
	{
		delete (mapItem->second);
		printf("~AudioFramePool delete eAudioMp3 framequeue\n");
	}
	
	mapItem = framePoolMap_.find(eAudioAac);
	if(mapItem != framePoolMap_.end())
	{
		delete (mapItem->second);
		printf("~AudioFramePool delete eAudioAac framequeue\n");
	}
}
int AudioFramePool::RegisterFramesPool(EAudioType audioType, int maxFrames)	// ע��֡��
{
	int ret = 0;
	MutexLockGuard lock(mutex_);
	// �Ȳ����Ƿ��Ѿ�ע��
	if (framePoolMap_.find(audioType) != framePoolMap_.end())  
	{
		// ˵���Ѿ�ע�����
		return 1;
	}
	else
	{
		//��ʽ1
		//FrameQueuePtr frameQueue(new BoundedQueue<Buffer::BufferPtr>(maxFrames)); 
		// ��ʽ2
		BoundedQueue *frameQueue = new BoundedQueue(maxFrames, false); 	
		if(frameQueue != nullptr)
		{
			framePoolMap_.insert(std::make_pair(audioType, frameQueue));  
			ret = 0;
		}
		else
		{
			ret = -1;
		}
	}	
	return ret;	
}
int AudioFramePool::UnregisterFramesPool(EAudioType audioType)
{
	MutexLockGuard lock(mutex_);
	if(framePoolMap_.erase(audioType) == 1)		// ɾ���ɹ�
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int AudioFramePool::PutFrame(EAudioType audioType, Buffer::BufferPtr &buf)
{
 	MutexLockGuard lock(mutex_);
	std::map<EAudioType, BoundedQueue *>::iterator
		mapItem = framePoolMap_.find(audioType);
	if(mapItem != framePoolMap_.end())
	{
		if(BoundedQueue::eOk == mapItem->second->put(round_, buf))
		{
			return 0;
		}
		else 
		{
			return 1;				
		}
	}
	else
	{
		return -1;		// ������
	}
}

int AudioFramePool::TakeFrame(EAudioType audioType, Buffer::BufferPtr &buf)
{
	MutexLockGuard lock(mutex_);
	std::map<EAudioType, BoundedQueue *>::iterator  
		mapItem = framePoolMap_.find(audioType);
	if(mapItem != framePoolMap_.end())
	{
		Buffer::BufferPtr tmp;
		//buf = mapItem->second->take();	
		if(BoundedQueue::eOk == mapItem->second->take(buf))
		{
			return 0;
		}
		else 
		{
			return 1;				
		}
	}
	else
	{
		return -1;		// ������
	}
}

