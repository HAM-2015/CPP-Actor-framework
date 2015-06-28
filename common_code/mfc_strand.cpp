#include "mfc_strand.h"
#include "bind_mfc_run.h"
#include "actor_timer.h"

#ifdef ENABLE_MFC_ACTOR
mfc_strand::mfc_strand()
{
	_mfc = NULL;
}

mfc_strand::~mfc_strand()
{

}

shared_strand mfc_strand::create(ios_proxy& iosProxy, bind_mfc_run* mfc)
{
	std::shared_ptr<mfc_strand> res(new mfc_strand, [](mfc_strand* p){delete p; });
	res->_iosProxy = &iosProxy;
	res->_mfc = mfc;
	res->_mfcThreadID = mfc->thread_id();
	res->_timer = new actor_timer(res);
	return res;
}

shared_strand mfc_strand::create( bind_mfc_run* mfc )
{
	std::shared_ptr<mfc_strand> res(new mfc_strand, [](mfc_strand* p){delete p; });
	res->_mfc = mfc;
	res->_mfcThreadID = mfc->thread_id();
	return res;
}

shared_strand mfc_strand::clone()
{
	return create(*_iosProxy, _mfc);
}

bool mfc_strand::in_this_ios()
{
	return running_in_this_thread();
}

bool mfc_strand::running_in_this_thread()
{
	assert(boost::thread::id() != _mfcThreadID);
	return boost::this_thread::get_id() == _mfcThreadID;
}

void mfc_strand::_post( const std::function<void ()>& h )
{
	_mfc->post(h);
}

#endif