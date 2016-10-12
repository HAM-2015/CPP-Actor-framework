#ifndef __CHECK_ACTOR_STACK_H
#define __CHECK_ACTOR_STACK_H

#define kB	*1024

//Ò³Ãæ´óÐ¡
#ifndef MEM_PAGE_SIZE
#define MEM_PAGE_SIZE (4 kB)
#endif

#ifndef STACK_BLOCK_SIZE
#ifdef WIN32
#if _WIN32_WINNT >= 0x0502
#define STACK_BLOCK_SIZE (64 kB)
#else
#define STACK_BLOCK_SIZE (1024 kB)
#endif
#elif __linux__
#define STACK_BLOCK_SIZE (32 kB)
#endif
#endif

//Ä¬ÈÏ¶ÑÕ»
#ifdef WIN32
#	if (_DEBUG || DEBUG) && (_WIN32_WINNT >= 0x0502)
#define DEFAULT_STACKSIZE	(256 kB - STACK_RESERVED_SPACE_SIZE)
#	else
#define DEFAULT_STACKSIZE	(STACK_BLOCK_SIZE - STACK_RESERVED_SPACE_SIZE)
#	endif
#elif __linux__
#	if (_DEBUG || DEBUG)
#define DEFAULT_STACKSIZE	(256 kB - STACK_RESERVED_SPACE_SIZE)
#	else
#define DEFAULT_STACKSIZE	(STACK_BLOCK_SIZE - STACK_RESERVED_SPACE_SIZE)
#	endif
#endif

//×î´ó¶ÑÕ»
#define MAX_STACKSIZE	(1024 kB - STACK_RESERVED_SPACE_SIZE)

#define TRY_SIZE(__s__) (0x80000000 | (__s__))
#define IS_TRY_SIZE(__s__) (0x80000000 & (__s__))
#define GET_TRY_SIZE(__s__) (0x1FFFFF & (__s__))

#if (_DEBUG || DEBUG)
#define STACK_SIZE(__debug__, __release__) (__debug__)
#define STACK_SIZE_REL(__release__) DEFAULT_STACKSIZE
#else
#define STACK_SIZE(__debug__, __release__) (__release__)
#define STACK_SIZE_REL(__release__) (__release__)
#endif
//////////////////////////////////////////////////////////////////////////

#if (_DEBUG || DEBUG)
#	if (defined _WIN64) || (defined __x86_64__) || (defined _ARM64)
#define STACK_SIZE64(__debug32__, __debug64__, __release32, __release64__) (__debug64__)
#	else
#define STACK_SIZE64(__debug32__, __debug64__, __release32, __release64__) (__debug32__)
#	endif
#else
#	if (defined _WIN64) || (defined __x86_64__) || (defined _ARM64)
#define STACK_SIZE64(__debug32__, __debug64__, __release32, __release64__) (__release64__)
#define STACK_SIZE_REL64(__release32__, __release64__) (__release64__)
#	else
#define STACK_SIZE64(__debug32__, __debug64__, __release32, __release64__) (__release32)
#define STACK_SIZE_REL64(__release32__, __release64__) (__release32__)
#	endif
#endif
//////////////////////////////////////////////////////////////////////////

//¶ÑÕ»µ×Ô¤Áô¿Õ¼ä£¬¼ì²â¶ÑÕ»Òç³ö
#if (_DEBUG || DEBUG)
#	if (defined _WIN64) || (defined __x86_64__) || (defined _ARM64)
#define STACK_RESERVED_SPACE_SIZE (24 kB)
#	else
#define STACK_RESERVED_SPACE_SIZE (16 kB)
#	endif
#else
#	if (defined _WIN64) || (defined __x86_64__) || (defined _ARM64)
#define STACK_RESERVED_SPACE_SIZE (16 kB)
#	else
#define STACK_RESERVED_SPACE_SIZE (8 kB)
#	endif
#endif

//Õ»×´Ì¬±£Áô¿Õ¼ä
#define CORO_CONTEXT_STATE_SPACE (1 * MEM_PAGE_SIZE)

#ifndef MEM_POOL_LENGTH
#define MEM_POOL_LENGTH 100000
#endif

#define ACTOR_TLS_INDEX 0
#define ACTOR_SAFE_STACK_INDEX 1
#define QT_UI_TLS_INDEX 2
#define UV_TLS_INDEX 3

static_assert(0 < MEM_PAGE_SIZE && MEM_PAGE_SIZE % (4 kB) == 0, "");
static_assert(0 < MEM_POOL_LENGTH && MEM_POOL_LENGTH < 10000000, "");
static_assert(STACK_BLOCK_SIZE >= (32 kB) && STACK_BLOCK_SIZE % MEM_PAGE_SIZE == 0, "");

#ifdef WIN32

#ifdef _WIN64
static_assert(8 == sizeof(void*), "");
#else
static_assert(4 == sizeof(void*), "");

#endif
#elif __linux__

#if (__x86_64__ || _ARM64)
static_assert(8 == sizeof(void*), "");
#elif (__i386__ || _ARM32)
static_assert(4 == sizeof(void*), "");
#endif

#if (__arm__ || __aarch64__)
#if !(defined _ARM32) && !(defined _ARM64)
#error "please define _ARM32 or _ARM64"
#endif
#endif

#endif

#endif