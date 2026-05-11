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

		_ok = true;
		_instance = static_cast<Class *>(this);

		auto prefix = makePrefix();
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

		for (const auto &name : _commands)
			::removeConsoleCommand(name);
		_commands.clear();

		_instance = nullptr;
	}

	// Adds a console command and tracks it so shutdown() removes it.
	// Use from derived configure() to register custom runtime parameters
	// (e.g. parameters discovered on other components in the scene).
	void addConsoleCommand(const Unigine::String &name, const Unigine::PropertyParameterPtr &param)
	{
		::addConsoleCommand(name, param);
		_commands.append(name);
	}

	Unigine::String makePrefix() const
	{
		Unigine::String suffix = "Tuning";
		Unigine::String prefix = _instance->getClassName();
		if (prefix.endsWith(suffix))
			prefix = prefix.substr(0, suffix.size() - 1);

		return prefix.lower() + '.';
	}

protected:
	bool _ok = false;
	Unigine::Vector<Unigine::String> _commands;

private:
	static inline Class *_instance;
};
