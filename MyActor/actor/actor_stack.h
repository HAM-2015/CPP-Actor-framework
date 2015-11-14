#ifndef __ACTOR_STACK_H
#define __ACTOR_STACK_H

#include "coro_choice.h"
#if (defined LIB_CORO) || (defined BOOST_CORO)

#include <algorithm>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <boost/thread/thread.hpp>
#include "msg_queue.h"

using namespace std;

struct StackPck_ 
{
	const void* _stackTop;
	const size_t _stackSize;
	int _tick;
};

/*!
@brief Actor’ª≥ÿ
*/
class ActorStackPool_
{
	struct stack_pool_pck 
	{
		stack_pool_pck()
		:_pool(100000){}
		std::mutex _mutex;
		msg_list<StackPck_> _pool;
	};
public:
	ActorStackPool_();
	~ActorStackPool_();
public:
	static StackPck_ getStack(size_t size);
	static void recovery(StackPck_& stack);
private:
	void clearThread();
private:
	bool _exitSign;
	bool _clearWait;
	stack_pool_pck _stackPool[256];
	std::mutex _clearMutex;
	boost::thread _clearThread;
	std::atomic<int> _stackCount;
	std::condition_variable _clearVar;
	std::atomic<size_t> _stackTotalSize;
};
#endif

#endif