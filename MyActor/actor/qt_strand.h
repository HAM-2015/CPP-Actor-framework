#ifndef __QT_STRAND_H
#define __QT_STRAND_H

#include "shared_strand.h"

#ifdef ENABLE_QT_ACTOR
class bind_qt_run_base;
class qt_strand;
typedef std::shared_ptr<qt_strand> shared_qt_strand;

class qt_strand : public boost_strand
{
	friend boost_strand;
	friend bind_qt_run_base;
	FRIEND_SHARED_PTR(qt_strand);
private:
	qt_strand();
	~qt_strand();
public:
	static shared_qt_strand create(io_engine& ioEngine, bind_qt_run_base* ui);
private:
	void release();
	bool released();
	shared_strand clone();
	bool in_this_ios();
	bool running_in_this_thread();
	bool sync_safe();
	bool is_running();
	bool only_self();
	bind_qt_run_base* self_ui();
	template <typename Handler>
	void _dispatch_ui(Handler&& handler);
	template <typename Handler>
	void _post_ui(Handler&& handler);
private:
	bind_qt_run_base* _ui;
};
#endif

#endif