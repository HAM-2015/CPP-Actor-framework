#ifndef __ACTOR_STACK_H
#define __ACTOR_STACK_H

#include <boost/coroutine/stack_context.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/atomic/atomic.hpp>
#include "msg_queue.h"

using namespace std;

struct StackPck_ 
{
	boost::coroutines::stack_context _stack;
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
		boost::mutex _mutex;
		msg_list<StackPck_> _pool;
	};
private:
	ActorStackPool_();
public:
	~ActorStackPool_();
	static void enable();
	static bool isEnable();
public:
	static StackPck_ getStack(size_t size);
	static void recovery(StackPck_& stack);
private:
	void clearThread();
private:
	bool _exit;
	bool _clearWait;
	stack_pool_pck _stackPool[256];
	boost::mutex _clearMutex;
	boost::thread _clearThread;
	boost::condition_variable _clearVar;
	boost::atomic<int> _stackCount;
	boost::atomic<size_t> _stackTotalSize;
	static std::shared_ptr<ActorStackPool_> _actorStackPool;
};

#endif