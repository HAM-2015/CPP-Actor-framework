#include "bind_wx_run.h"

DEFINE_EVENT_TYPE(wxEVT_POST)

bind_wx_run::bind_wx_run() : _isClosed(false), _postOptions(16)
{
	set_thread_id();
}

bind_wx_run::~bind_wx_run()
{
	assert(boost::this_thread::get_id() == _threadID);
}

void bind_wx_run::wx_close()
{
	assert(boost::this_thread::get_id() == _threadID);
	{
		boost::unique_lock<boost::shared_mutex> ul(_postMutex);
		_isClosed = true;
		__disconnectCloseEvent();
		__unbindPostEvent();
	}
	__cancel();
}

void bind_wx_run::__closeEventHandler(wxCloseEvent& event)
{
	OnClose();
}

void bind_wx_run::start_wx_actor()
{
	assert(boost::this_thread::get_id() == _threadID);
	__connectCloseEvent();
	__bindPostEvent();
}

void bind_wx_run::set_thread_id()
{
	_threadID = boost::this_thread::get_id();
}

boost::thread::id bind_wx_run::thread_id()
{
	assert(boost::thread::id() != _threadID);
	return _threadID;
}

void bind_wx_run::post_queue_size(size_t fixedSize)
{
	assert(boost::this_thread::get_id() == _threadID);
	_postOptions.expand_fixed(fixedSize);
}

actor_handle bind_wx_run::create_wx_actor(ios_proxy& ios, const my_actor::main_func& mainFunc, size_t stackSize /*= DEFAULT_STACKSIZE*/)
{
	assert(!_isClosed);
	return my_actor::create(wx_strand::create(ios, this), mainFunc, stackSize);
}

actor_handle bind_wx_run::create_wx_actor(const my_actor::main_func& mainFunc, size_t stackSize /*= DEFAULT_STACKSIZE*/)
{
	assert(!_isClosed);
	return my_actor::create(wx_strand::create(this), mainFunc, stackSize);
}

void bind_wx_run::__postRun(wxEvent& ue)
{
	_mutex.lock();
	assert(!_postOptions.empty());
	auto h = std::move(_postOptions.front());
	_postOptions.pop_front();
	_mutex.unlock();
	assert(h);
	h();
}

void bind_wx_run::OnClose()
{

}
