#pragma once
#include "UnigineFmt.h"

#define FLOGERR(COND, ...)                                  \
	if (!(COND))                                            \
	{                                                       \
		Unigine::Log::error(__FUNCTION__ ": " __VA_ARGS__); \
		return;                                             \
	}\
