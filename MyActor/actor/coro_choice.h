#ifndef __CORO_CHOICE_H
#define __CORO_CHOICE_H

#ifdef DISABLE_BOOST_CORO

#ifdef ENABLE_WIN_FIBER

#define FIBER_CORO

#else //ENABLE_WIN_FIBER

#define LIB_CORO

#endif //ENABLE_WIN_FIBER

#else //DISABLE_BOOST_CORO

#define BOOST_CORO

#endif //DISABLE_BOOST_CORO

#endif