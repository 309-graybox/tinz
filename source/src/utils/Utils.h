#pragma once
#include "UnigineFmt.h"

#define FLOGERR(COND, ...)                                  \
	if (!(COND))                                            \
	{                                                       \
		Unigine::Log::error(__FUNCTION__ ": " __VA_ARGS__); \
		removeComponent<__this_class>(node);                \
		return;                                             \
	}

template <class T, class C>
void convertTo(Unigine::ComponentVariableArray<C> &arr, Unigine::Vector<T> &ret)
{
	int n = arr.size();
	for (int i = 0; i < n; ++i)
	{
		auto el = Unigine::checked_ptr_cast<T>(arr[i].get());
		if (el)
			ret.append(el);
	}
}
