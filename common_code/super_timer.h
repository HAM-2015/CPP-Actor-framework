#ifndef __SUPER_TIMER_H
#define __SUPER_TIMER_H

#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <list>
#include "shared_strand.h"

using namespace std;

class super_resolution_timer_trig;
class query_performance_counter;

class super_timer
{
	friend class query_performance_counter;
	friend class super_resolution_timer_trig;

	struct timer_pck 
	{
		timer_pck(const boost::function<void ()>& h, long long tt)
		{
			_bNofify = true;
			_h = h;
			_trigTick = tt;
		}

		bool _bNofify;
		long long _trigTick;
		boost::function<void ()> _h;
	};
private:
	super_timer(shared_strand strand);
public:
	~super_timer();
public:
	void async_wait(long long microsecond, const boost::function<void (boost::system::error_code)>& h);
	void cancel(boost::system::error_code& ec);
private:
	void notifyHandler(const boost::function<void (boost::system::error_code)>& h, int id);
private:
	bool _hasTm;
	int _idCount;
	shared_strand _strand;
	list<timer_pck>::iterator _nd;
};

class query_performance_counter
{
	friend class super_timer;
private:
	query_performance_counter();
public:
	~query_performance_counter();
	static bool enable();
	static void close();
	static long long getTick();
private:
	static list<super_timer::timer_pck>::iterator registTimer(const boost::function<void ()>& h, long long timeoutMicrosecond);
	static void cancelTimer(list<super_timer::timer_pck>::iterator nd);
public:
	void timerThread();
private:
	bool _close;
	boost::thread _timerThread;
	boost::mutex _mutex;
	list<super_timer::timer_pck> _timerList;
	static query_performance_counter* _this;
};

#endif