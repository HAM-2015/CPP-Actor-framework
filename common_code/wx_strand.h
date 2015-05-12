#ifndef __WX_STRAND_H
#define __WX_STRAND_H

#ifdef ENABLE_WX_ACTOR
#include <boost/thread/thread.hpp>
#include "shared_strand.h"

class bind_wx_run_base;

class wx_strand : public boost_strand
{
private:
	wx_strand();
	~wx_strand();
public:
	static shared_strand create(ios_proxy& iosProxy, bind_wx_run_base* wx);
	static shared_strand create(bind_wx_run_base* wx);
private:
	shared_strand clone();
	bool in_this_ios();
	bool running_in_this_thread();
	void _post(const std::function<void()>& h);
private:
	bind_wx_run_base* _wx;
	boost::thread::id _wxThreadID;
};
#endif

#endif