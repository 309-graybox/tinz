#pragma once
#include <UnigineComponentSystem.h>

#define FLOGERR(COND, MSG, ...)                                                          \
	if (!(COND))                                                                         \
	{                                                                                    \
		Unigine::Log::error(String::format("%s: %s", __FUNCTION__, MSG), ##__VA_ARGS__); \
		removeComponent<__this_class>(node);                                             \
		return;                                                                          \
	}

template <class C, class T>
void convertTo(Unigine::ComponentVariableArray<C> &arr, Unigine::Vector<T> &ret)
{
	int n = arr.size();

	ret.resize(ret.size() + n);

	for (int i = 0; i < n; ++i)
	{
		auto el = arr[i].get();
		ret.append(el);
	}
}
