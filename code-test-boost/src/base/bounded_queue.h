#ifndef _BOUNDED_QUEUE_H_
#define _BOUNDED_QUEUE_H_

#include <vector>
#include <assert.h>
#include <boost/shared_ptr.hpp>
#include "mutex.h"
#include "condition.h"
#include "buffer.h"
#include <stdio.h>
using namespace std;

class BoundedQueue
{
public:
	typedef boost::shared_ptr<BoundedQueue> QueuePtr;
	typedef enum {eOk = 0, eFull, eEmpty}ERetCode;
	typedef enum {eAudioMp3 = 0,  eAudioAac}EAudioType;
	explicit BoundedQueue(size_t maxSize, bool block = true)
		: mutex_(), 
		capacity_(maxSize), 
		notEmpty_(mutex_), 
		notFull_(mutex_),
		block_(block)
	{
		queue_.reserve(capacity_);
	}

	~BoundedQueue()
	{
		printf("~BoundedQueue\n");
	}
	
	ERetCode put(const Buffer::BufferPtr & x)
	{
		{
			MutexLockGuard lock(mutex_);
			while(queue_.size() == capacity_)
			{
				if(!block_)
				{
					return eFull;
				}
				notFull_.wait();
			}
			assert(!(queue_.size() == capacity_));
			queue_.push_back(x);
		}
		
		notEmpty_.notify();
		return eOk;
	}

	// 如果round不等于0,如果队列满则覆盖, 慎用该函数，如果存储对象不是shared_ptr，将会导致内存泄漏，如果为shared_ptr则会
	// 自动调用对象的析构函数释放资源.
	ERetCode put(int round, const Buffer::BufferPtr & x)
	{
		{
			MutexLockGuard lock(mutex_);
			if(round == 0)
			{
				while(queue_.size() == capacity_)
				{
					if(!block_)
					{
						return eFull;
					}
					notFull_.wait();
				}
				assert(!(queue_.size() == capacity_));
			}
			else
			{
				if(queue_.size() == capacity_)
				{
					Buffer::BufferPtr front(queue_.front());
					queue_.erase(queue_.begin());		// 取出元素
				}
			}
			queue_.push_back(x);
		}
		
		notEmpty_.notify();
		return eOk;
	}
	
	ERetCode take(Buffer::BufferPtr & x)
	{
		MutexLockGuard lock(mutex_);
		while(queue_.empty())
		{
			if(!block_)
			{
				return eEmpty;
			}
			notEmpty_.wait();
		}
		assert(!queue_.empty());
		Buffer::BufferPtr front(queue_.front());
		queue_.erase(queue_.begin());
		x = front;
		notFull_.notify();
		
		return eOk;
	}
	
	
	bool empty() const
	{
		MutexLockGuard lock(mutex_);
		return queue_.empty();
	}
	
	bool full() const
	{
		MutexLockGuard lock(mutex_);
		return queue_.size() == capacity_;
	}
	
	size_t size() const
	{
		MutexLockGuard lock(mutex_);
		return queue_.size();
	}
	
	size_t capacity() const
	{
		MutexLockGuard lock(mutex_);
		return queue_.capacity();
	}
	
private:
	mutable MutexLock mutex_;
	size_t capacity_;
	bool block_;				// 是否阻塞
	Condition notEmpty_;
	Condition notFull_;
	std::vector<Buffer::BufferPtr> queue_;
};

#endif

