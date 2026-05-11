#pragma once
#include "utils/Utils.h"
#include <UnigineComponentSystem.h>

template <class Class>
class TuningBase: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(TuningBase, Unigine::ComponentBase)
	COMPONENT_INIT(init, INT_MIN)
	COMPONENT_SHUTDOWN(shutdown)

	static Class *get()
	{
		UNIGINE_ASSERT(_instance && "No instance found");
		return _instance;
	}

protected:
	void init()
	{
		FLOGERR(!_instance || _instance != this, "Already defined\n");

		_instance = static_cast<Class *>(this);

		Unigine::String suffix = "Tuning";
		Unigine::String prefix = _instance->getClassName();
		if (prefix.endsWith(suffix))
			prefix = prefix.substr(0, suffix.size() - 1);

		prefix = prefix.lower() + '_';

		for (const auto &v : _instance->variables)
		{
			auto param = v->getParameter();
			addConsoleCommand(prefix + param->getName(), param);
		}
	}

	void shutdown()
	{
		if (_instance != this)
			return;

		_instance = nullptr;

		Unigine::String prefix = "debug_";
		for (const auto &v : variables)
		{
			auto param = v->getParameter();
			removeConsoleCommand(prefix + param->getName());
		}
	}

private:
	static inline Class *_instance;
};
