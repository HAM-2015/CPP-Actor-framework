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
	shared_qt_strand res(new qt_strand);
	res->_ioEngine = &ioEngine;
	res->_qt = qt;
	res->_actorTimer = new ActorTimer_(res);
	res->_timerBoost = new TimerBoost_(res);
	res->_weakThis = res;
	return res;
}

shared_strand qt_strand::clone()
{
	return create(*_ioEngine, _qt);
}

bool qt_strand::in_this_ios()
{
	return _qt->run_in_ui_thread();
}

bool qt_strand::running_in_this_thread()
{
	return _qt->running_in_this_thread();
}

bool qt_strand::empty(bool)
{
	assert(running_in_this_thread());
	return 0 == _qt->task_number();
}

bool qt_strand::sync_safe()
{
	return true;
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