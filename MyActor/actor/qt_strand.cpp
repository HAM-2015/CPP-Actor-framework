#include "qt_strand.h"
#include "bind_qt_run.h"

#ifdef ENABLE_QT_UI
#ifdef ENABLE_QT_ACTOR
qt_strand::qt_strand()
{
	_qt = NULL;
#ifdef ENABLE_NEXT_TICK
	_beginNextRound = true;
#endif
}

qt_strand::~qt_strand()
{

}

shared_qt_strand qt_strand::create(io_engine& ioEngine, bind_qt_run_base* qt)
{
	shared_qt_strand res(new qt_strand, [](qt_strand* p){delete p; });
	res->_ioEngine = &ioEngine;
	res->_qt = qt;
	res->_qtThreadID = qt->thread_id();
	res->_actorTimer = new ActorTimer_(res);
	res->_timerBoost = new TimerBoost_(res);
	res->_weakThis = res;
	return res;
}

shared_qt_strand qt_strand::create(bind_qt_run_base* qt)
{
	shared_qt_strand res(new qt_strand, [](qt_strand* p){delete p; });
	res->_qt = qt;
	res->_qtThreadID = qt->thread_id();
	res->_weakThis = res;
	return res;
}

shared_strand qt_strand::clone()
{
	return create(*_ioEngine, _qt);
}

bool qt_strand::in_this_ios()
{
	return running_in_this_thread();
}

bool qt_strand::running_in_this_thread()
{
	assert(boost::thread::id() != _qtThreadID);
	return boost::this_thread::get_id() == _qtThreadID;
}

void qt_strand::_post(const std::function<void()>& h)
{
	_qt->post(h);
}

void qt_strand::_post(std::function<void()>&& h)
{
	_qt->post(std::move(h));
}

#endif
#endif