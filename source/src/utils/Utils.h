#pragma once
#include <UnigineComponentSystem.h>

#define FLOGERR(COND, MSG, ...)                                       \
	if (!(COND))                                                      \
	{                                                                 \
		Unigine::Log::error("%s: " MSG, __FUNCTION__, ##__VA_ARGS__); \
		removeComponent<__this_class>(node);                          \
		return;                                                       \
	}

template <class C, class T>
UNIGINE_INLINE void convertTo(Unigine::ComponentVariableArray<C> &arr, Unigine::Vector<T> &ret)
{
	int n = arr.size();

	ret.resize(ret.size() + n);

	for (int i = 0; i < n; ++i)
	{
		auto el = arr[i].get();
		ret.append(el);
	}
}

// Walks up parents of `n` looking for `target`. Used to attribute a contact /
// raycast hit on a child mesh to the player character root.
UNIGINE_INLINE bool isInHierarchy(Unigine::NodePtr n, const Unigine::NodePtr &target)
{
	while (n)
	{
		if (n == target)
			return true;
		n = n->getParent();
	}
	return false;
}

UNIGINE_INLINE Unigine::Math::vec3 horizontal(const Unigine::Math::vec3 &v)
{
	return {v.x, v.y, 0.0f};
}

void addConsoleCommand(const Unigine::String &name, const Unigine::PropertyParameterPtr &param);
void removeConsoleCommand(const Unigine::String &name);

void setMouseGrab(bool grab);
