#ifndef __QT_STRAND_H
#define __QT_STRAND_H

#ifdef ENABLE_QT_UI
#ifdef ENABLE_QT_ACTOR
#include "shared_strand.h"

class bind_qt_run_base;
class qt_strand;
typedef std::shared_ptr<qt_strand> shared_qt_strand;

class qt_strand : public boost_strand
{
	friend boost_strand;
	FRIEND_SHARED_PTR(qt_strand);
private:
	qt_strand();
	~qt_strand();
public:
	static shared_qt_strand create(io_engine& ioEngine, bind_qt_run_base* ui);
private:
	shared_strand clone();
	bool in_this_ios();
	bool running_in_this_thread();
	bool sync_safe();
	bool is_running();
	template <typename Handler>
	void dispatch_ui(Handler&& handler);
	template <typename Handler>
	void post_ui(Handler&& handler);
private:
	bind_qt_run_base* _ui;
};

template <typename Handler>
void boost_strand::post_ui(Handler&& handler)
{
	static_cast<qt_strand*>(this)->post_ui(TRY_MOVE(handler));
}

template <typename Handler>
void boost_strand::dispatch_ui(Handler&& handler)
{
	static_cast<qt_strand*>(this)->dispatch_ui(TRY_MOVE(handler));
}

#endif
#endif

#endif