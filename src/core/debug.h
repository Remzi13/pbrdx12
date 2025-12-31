#pragma once

#include <stdio.h>
#include <assert.h>

#define ASSERT(exp, ...)																		\
	if (!(exp))																						\
	{																								\
		printf_s("ASSERTION FAILED: %s \n" __FILE__ "(%d)\n", #exp, (int)__LINE__);					\
		assert(exp);																				\
	}																								


#define CHECK(expression, validation, ...)													\
	do																							\
	{																							\
		if(!((expression) validation))															\
		{																						\
			LOG(LogType::Warning, __VA_ARGS__);												\
			__debugbreak();																		\
		}																						\
	} while(0)

#define LOG(level, message, ...)																\
	printf_s(message, __VA_ARGS__ );																\

