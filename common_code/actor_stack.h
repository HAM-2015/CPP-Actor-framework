#ifndef __ACTOR_STACK_H
#define __ACTOR_STACK_H

#include <boost/coroutine/stack_context.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/atomic/atomic.hpp>
#include <list>

using namespace std;

struct stack_pck 
{
	boost::coroutines::stack_context _stack;
	int _tick;
};

/*!
@brief Actor’ª≥ÿ
*/
class actor_stack_pool
{
	struct stack_pool_pck 
	{
		boost::mutex _mutex;
		list<stack_pck> _pool;
	};
private:
	actor_stack_pool();
public:
	~actor_stack_pool();
	static void enable();
	static bool isEnable();
public:
	static stack_pck getStack(size_t size);
	static void recovery(stack_pck& stack);
private:
	void clearThread();
private:
	bool _exit;
	bool _clearWait;
	bool _isBack;
	stack_pck _nextPck;
	stack_pool_pck _stackPool[256];
	boost::mutex _clearMutex;
	boost::thread _clearThread;
	boost::condition_variable _clearVar;
	boost::atomic<int> _stackCount;
	boost::atomic<size_t> _stackTotalSize;
	static boost::shared_ptr<actor_stack_pool> _actorStackPool;
};

#endif