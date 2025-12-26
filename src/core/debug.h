#pragma once

#include <stdio.h>
#include <assert.h>

#define ASSERT(exp, ...)																		\
	if (!(exp))																						\
	{																								\
		printf_s("ASSERTION FAILED: %s \n" __FILE__ "(%d)\n", #exp, (int)__LINE__);					\
		assert(exp);																				\
	}		