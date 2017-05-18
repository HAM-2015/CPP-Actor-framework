#include "qt_strand.h"
#include "bind_qt_run.h"

#ifdef ENABLE_QT_ACTOR
qt_strand::qt_strand()
{
	_ui = NULL;
#if (ENABLE_QT_ACTOR && ENABLE_UV_ACTOR)
	_strandChoose = boost_strand::strand_ui;
#endif
}

qt_strand::~qt_strand()
{
	assert(!_ui);
}

shared_qt_strand qt_strand::create(io_engine& ioEngine, bind_qt_run_base* ui)
{
	shared_qt_strand res = std::make_shared<qt_strand>();
	res->_ioEngine = &ioEngine;
	res->_ui = ui;
	res->_actorTimer = new ActorTimer_(res);
	res->_overTimer = new overlap_timer(res);
	res->_weakThis = res;
	return res;
}

void qt_strand::release()
{
	assert(in_this_ios());
	_ui = NULL;
}

bool qt_strand::released()
{
	assert(in_this_ios());
	return !_ui;
}

shared_strand qt_strand::clone()
{
	assert(_ui);
	return create(*_ioEngine, _ui);
}

bool qt_strand::in_this_ios()
{
	assert(_ui);
	return _ui->running_in_ui_thread();
}

bool qt_strand::running_in_this_thread()
{
	assert(_ui);
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

bool qt_strand::only_self()
{
	assert(_ui);
	return _ui->only_self();
}

bind_qt_run_base* qt_strand::self_ui()
{
	return _ui;
}
#endif