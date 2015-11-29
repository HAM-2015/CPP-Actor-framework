#ifndef __QT_STRAND_H
#define __QT_STRAND_H

#ifdef ENABLE_QT_UI
#ifdef ENABLE_QT_ACTOR
#include <thread>
#include "shared_strand.h"

class bind_qt_run_base;
class qt_strand;
typedef std::shared_ptr<qt_strand> shared_qt_strand;

class qt_strand : public boost_strand
{
private:
	qt_strand();
	~qt_strand();
public:
	static shared_qt_strand create(io_engine& ioEngine, bind_qt_run_base* qt);
	static shared_qt_strand create(bind_qt_run_base* qt);
private:
	shared_strand clone();
	bool in_this_ios();
	bool running_in_this_thread();
	void _post(const std::function<void()>& h);
	void _post(std::function<void()>&& h);
private:
	bind_qt_run_base* _qt;
	boost::thread::id _qtThreadID;
};
#endif
#endif

#endif