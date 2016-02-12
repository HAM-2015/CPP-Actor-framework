#include "qt_strand.h"
#include "bind_qt_run.h"

#ifdef ENABLE_QT_UI
#ifdef ENABLE_QT_ACTOR
qt_strand::qt_strand()
{
	_ui = NULL;
#ifdef ENABLE_NEXT_TICK
	_beginNextRound = true;
#endif
}

qt_strand::~qt_strand()
{

}

shared_qt_strand qt_strand::create(io_engine& ioEngine, bind_qt_run_base* ui)
{
	shared_qt_strand res(new qt_strand);
	res->_ioEngine = &ioEngine;
	res->_ui = ui;
	res->_actorTimer = new ActorTimer_(res);
	res->_timerBoost = new TimerBoost_(res);
	res->_weakThis = res;
	return res;
}

shared_strand qt_strand::clone()
{
	return create(*_ioEngine, _ui);
}

bool qt_strand::in_this_ios()
{
	return _ui->run_in_ui_thread();
}

bool qt_strand::running_in_this_thread()
{
	return _ui->running_in_this_thread();
}

bool qt_strand::sync_safe()
{
	return true;
}

bool qt_strand::is_running()
{
	return true;
}

#endif
#endif