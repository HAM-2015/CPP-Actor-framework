#ifndef __CHECK_ACTOR_STACK_H
#define __CHECK_ACTOR_STACK_H

//Ä¬ÈÏ¶ÑÕ»´óÐ¡64k
#define kB	*1024
#define DEFAULT_STACKSIZE	(64 kB)

#if (_DEBUG || DEBUG)
#define STACK_SIZE(__debug__, __release__) (__debug__)
#define STACK_SIZE_REL(__release__) DEFAULT_STACKSIZE
#else
#define STACK_SIZE(__debug__, __release__) (__release__)
#define STACK_SIZE_REL(__release__) (__release__)
#endif
//////////////////////////////////////////////////////////////////////////

#if (_DEBUG || DEBUG)
#	if (defined _WIN64) || (defined __x86_64__)
#define STACK_SIZE64(__debug32__, _debug64__, __release32, __release64__) (_debug64__)
#	else
#define STACK_SIZE64(__debug32__, _debug64__, __release32, __release64__) (__debug32__)
#	endif
#else
#	if (defined _WIN64) || (defined __x86_64__)
#define STACK_SIZE64(__debug32__, _debug64__, __release32, __release64__) (__release64__)
#define STACK_SIZE_REL64(__release32__, __release64__) (__release64__)
#	else
#define STACK_SIZE64(__debug32__, _debug64__, __release32, __release64__) (__release32)
#define STACK_SIZE_REL64(__release32__, __release64__) (__release32__)
#	endif
#endif
//////////////////////////////////////////////////////////////////////////

//¶ÑÕ»µ×Ô¤Áô¿Õ¼ä£¬¼ì²â¶ÑÕ»Òç³ö
#ifdef CHECK_ACTOR_STACK
#if (_DEBUG || DEBUG)
#		if (defined _WIN64) || (defined __x86_64__)
#define STACK_RESERVED_SPACE_SIZE		(24 kB)
#		else
#define STACK_RESERVED_SPACE_SIZE		(16 kB)
#		endif
#	else
#		if (defined _WIN64) || (defined __x86_64__)
#define STACK_RESERVED_SPACE_SIZE		(16 kB)
#		else
#define STACK_RESERVED_SPACE_SIZE		(8 kB)
#		endif
#	endif
#else //CHECK_ACTOR_STACK
#define STACK_RESERVED_SPACE_SIZE		0
#endif //CHECK_ACTOR_STACK

#endif