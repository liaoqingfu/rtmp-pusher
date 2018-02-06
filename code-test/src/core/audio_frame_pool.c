#include "audio_frame_pool.h"


AudioFramePool::AudioFramePool(int round)
	: round_(round)
{
	
}
AudioFramePool::~AudioFramePool()
{

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
		//FrameQueuePtr frameQueue(new BoundedBlockingQueue<Buffer::BufferPtr>(maxFrames)); 
		// ��ʽ2
		FrameQueuePtr frameQueue = std::make_shared<BoundedBlockingQueue<Buffer::BufferPtr>>(maxFrames); 	
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
	auto mapItem = framePoolMap_.find(audioType);
	if(mapItem != framePoolMap_.end())
	{
		//Buffer::BufferPtr buff(Buffer::CreateInstance(100));
		return mapItem->second->put(round_, buf);		
	}
	else
	{
		return 1;		// ������
	}
}

int AudioFramePool::TakeFrame(EAudioType audioType, Buffer::BufferPtr &buf)
{
	MutexLockGuard lock(mutex_);
	auto mapItem = framePoolMap_.find(audioType);
	if(mapItem != framePoolMap_.end())
	{
		buf = mapItem->second->take();	
		return 0;
	}
	else
	{
		return -1;		// ������
	}
}

