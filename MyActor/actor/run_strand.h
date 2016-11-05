#ifndef __RUN_STRAND_H
#define __RUN_STRAND_H

#include "qt_strand.h"
#include "uv_strand.h"

#ifdef ENABLE_QT_ACTOR

template <typename Handler>
void boost_strand::post_ui(Handler&& handler)
{
	static_cast<qt_strand*>(this)->_post_ui(std::forward<Handler>(handler));
}

template <typename Handler>
void boost_strand::dispatch_ui(Handler&& handler)
{
	static_cast<qt_strand*>(this)->_dispatch_ui(std::forward<Handler>(handler));
}

#endif

#ifdef ENABLE_UV_ACTOR

template <typename Handler>
void boost_strand::post_uv(Handler&& handler)
{
	static_cast<uv_strand*>(this)->_post_uv(std::forward<Handler>(handler));
}

template <typename Handler>
void boost_strand::dispatch_uv(Handler&& handler)
{
	static_cast<uv_strand*>(this)->_dispatch_uv(std::forward<Handler>(handler));
}

#endif

#endif