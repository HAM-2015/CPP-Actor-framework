#include "bind_wx_run.h"

DEFINE_EVENT_TYPE(wxEVT_POST)

shared_strand bind_wx_run_base::make_wx_strand()
{
	return wx_strand::create(this);
}

shared_strand bind_wx_run_base::make_wx_strand(ios_proxy& ios)
{
	return wx_strand::create(ios, this);
}
