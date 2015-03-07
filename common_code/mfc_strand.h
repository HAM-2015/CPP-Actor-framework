#ifndef __MFC_STRAND_H
#define __MFC_STRAND_H

#ifdef ENABLE_MFC_ACTOR
#include <boost/thread/thread.hpp>
#include "shared_strand.h"
#include "bind_mfc_run.h"

class mfc_strand: public boost_strand
{
private:
	mfc_strand();
public:
	~mfc_strand();
	static shared_strand create(ios_proxy& iosProxy, bind_mfc_run* mfc);
	static shared_strand create(bind_mfc_run* mfc);
private:
	shared_strand clone();
	bool in_this_ios();
	bool running_in_this_thread();
	void _post(const boost::function<void ()>& h);
private:
	bind_mfc_run* _mfc;
	boost::thread::id _mfcThreadID;
};
#endif

#endif