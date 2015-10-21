#include "bind_wx_run.h"

#ifdef ENABLE_WX_ACTOR

DEFINE_EVENT_TYPE(wxEVT_POST)

bind_wx_run_base::bind_wx_run_base()
:_isClosed(false), _postOptions(16)
{
	_threadID = boost::this_thread::get_id();
}

bind_wx_run_base::~bind_wx_run_base()
{
	assert(boost::this_thread::get_id() == _threadID);
}

actor_handle bind_wx_run_base::create_wx_actor(io_engine& ios, const my_actor::main_func& mainFunc, size_t stackSize /*= DEFAULT_STACKSIZE*/)
{
	assert(!_isClosed);
	return my_actor::create(make_wx_strand(ios), mainFunc, stackSize);
}

actor_handle bind_wx_run_base::create_wx_actor(const my_actor::main_func& mainFunc, size_t stackSize /*= DEFAULT_STACKSIZE*/)
{
	assert(!_isClosed);
	return my_actor::create(make_wx_strand(), mainFunc, stackSize);
}

shared_wx_strand bind_wx_run_base::make_wx_strand()
{
	return wx_strand::create(this);
}

shared_wx_strand bind_wx_run_base::make_wx_strand(io_engine& ios)
{
	return wx_strand::create(ios, this);
}

void bind_wx_run_base::wait_closed(my_actor* host)
{
	boost::unique_lock<boost::shared_mutex> ul(_postMutex);
	if (!_isClosed)
	{
		actor_trig_handle<> ath;
		_closedCallback.push_back(host->make_trig_notifer_to_self(ath));
		ul.unlock();
		host->wait_trig(ath);
	}
}

boost::thread::id bind_wx_run_base::thread_id()
{
	assert(boost::thread::id() != _threadID);
	return _threadID;
}

void bind_wx_run_base::post(const std::function<void()>& h)
{
	{
		boost::shared_lock<boost::shared_mutex> sl(_postMutex);
		if (!_isClosed)
		{
			_mutex.lock();
			_postOptions.push_back(h);
			_mutex.unlock();
			post_event();
			return;
		}
	}
	assert(false);
}

void bind_wx_run_base::wx_close()
{
	assert(boost::this_thread::get_id() == _threadID);
	boost::unique_lock<boost::shared_mutex> ul(_postMutex);
	_isClosed = true;
	disconnect();
	while (!_closedCallback.empty())
	{
		_closedCallback.front()();
		_closedCallback.pop_front();
	}
}

void bind_wx_run_base::OnClose()
{

}

void bind_wx_run_base::post_queue_size(size_t fixedSize)
{
	assert(boost::this_thread::get_id() == _threadID);
	_mutex.lock();
	_postOptions.expand_fixed(fixedSize);
	_mutex.unlock();
}

void bind_wx_run_base::postRun(wxEvent& ue)
{
	_mutex.lock();
	assert(!_postOptions.empty());
	auto h = std::move(_postOptions.front());
	_postOptions.pop_front();
	_mutex.unlock();
	assert(h);
	h();
}

#endif //ENABLE_WX_ACTOR