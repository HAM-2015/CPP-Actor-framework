#include "wx_strand.h"
#include "bind_wx_run.h"

#ifdef ENABLE_WX_ACTOR
wx_strand::wx_strand()
{
	_wx = NULL;
}

wx_strand::~wx_strand()
{

}

shared_wx_strand wx_strand::create(ios_proxy& iosProxy, bind_wx_run_base* wx)
{
	shared_wx_strand res(new wx_strand, [](wx_strand* p){delete p; });
	res->_iosProxy = &iosProxy;
	res->_wx = wx;
	res->_wxThreadID = wx->thread_id();
	res->_timer = new actor_timer(res);
	res->_weakThis = res;
	return res;
}

shared_wx_strand wx_strand::create(bind_wx_run_base* wx)
{
	shared_wx_strand res(new wx_strand, [](wx_strand* p){delete p; });
	res->_wx = wx;
	res->_wxThreadID = wx->thread_id();
	res->_weakThis = res;
	return res;
}

shared_strand wx_strand::clone()
{
	return create(*_iosProxy, _wx);
}

bool wx_strand::in_this_ios()
{
	return running_in_this_thread();
}

bool wx_strand::running_in_this_thread()
{
	assert(boost::thread::id() != _wxThreadID);
	return boost::this_thread::get_id() == _wxThreadID;
}

void wx_strand::_post(const std::function<void()>& h)
{
	_wx->post(h);
}

#endif