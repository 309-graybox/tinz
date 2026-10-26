#pragma once
#include "EIKey.h"
#include "Defines.h"
#include "EISerializer.h"
#include <UnigineComponentSystem.h>
#include <UnigineGUID.h>
#include <memory>

#define _REGISTER_SOME(Prefix, Name, Alias)                                                      \
	class Prefix##Name                                                                           \
	{                                                                                            \
	public:                                                                                      \
		Prefix##Name()                                                                           \
		{                                                                                        \
			EICreatorRegistryImpl<Name>::get()->registerCreator(Alias, [] { return new Name; }); \
		}                                                                                        \
		~Prefix##Name()                                                                          \
		{                                                                                        \
			EICreatorRegistryImpl<Name>::get()->unregisterCreator(Alias);                        \
		}                                                                                        \
	};                                                                                           \
	Prefix##Name _##Prefix##Name##Instance

#define REGISTER_MODIFIER(Name, Alias) _REGISTER_SOME(ModifierRegistrator, Name, Alias)
#define REGISTER_TRIGGER(Name, Alias) _REGISTER_SOME(TriggerRegistrator, Name, Alias)

class EIModifier;
class EITrigger;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
ENUM(EIActionValueType, Boolean, Axis1D, Axis2D, Axis3D);
ENUM(EIActionAccumulationBehavior, Highest, Accumulative);

enum class eTriggerState
{
	None = 1,
	Triggered = 2,
	Started = 4,
	Ongoing = 8,
	Canceled = 16,
	Completed = 32
};
ENUM_FLAG_IMPL(eTriggerState);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
template <class T>
using SPtr = std::shared_ptr<T>;

template <class T, class... Args>
SPtr<T> makeSPtr(Args &&...args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
template <class T>
class EICreatorRegistry
{
public:
	virtual ~EICreatorRegistry() = default;

	virtual const char *getRegistryName() const noexcept = 0;

	virtual void registerCreator(const char *name, std::function<T *()> creator) = 0;
	virtual void unregisterCreator(const char *name) = 0;

	virtual int getIndex(const char *name) const = 0;
	virtual int getCount() const = 0;
	virtual const char *getName(int i) const = 0;
	virtual T *create(int i) = 0;
	virtual T *create(const char *name) = 0;

	virtual const Unigine::Vector<Unigine::String> &getNames() const noexcept = 0;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EIFileSystemRegistryBase
{
public:
	virtual ~EIFileSystemRegistryBase() = default;

	virtual const char *getExtension() const = 0;
	virtual int getIndexByName(const char *name) const = 0;
	virtual int getIndexByPath(const char *name) const = 0;
	virtual int getCount() const = 0;
	virtual const char *getName(int i) const = 0;
	virtual const char *getPath(int i) const = 0;

	virtual void refresh() = 0;
};

template <class T>
class EIFileSystemRegistry: public EIFileSystemRegistryBase
{
public:
	virtual int getIndex(T *v) = 0;

	virtual T *create(int i) = 0;
	virtual T *create(const char *name) = 0;
	virtual T *create(const Unigine::UGUID &guid) = 0;
	virtual void destroy(T *v) = 0;

	// Reload the cached instance from disk in place. The pointer stays valid
	// (so external holders like EIContextImpl keeping `const EIAction *`
	// references survive). Returns false if v isn't registered.
	virtual bool reload(T *v) = 0;

	virtual bool save(int i) = 0;
	virtual bool save(T *v) = 0;

	virtual bool saveDummy(const char *path) = 0;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
struct EIAction final
{
	Unigine::UGUID guid;
	Unigine::String name = "";
	Unigine::String description = "";
	EIActionValueType valueType = EIActionValueType::Boolean;
	EIActionAccumulationBehavior accumulationBehavior = EIActionAccumulationBehavior::Highest;
	Unigine::Vector<SPtr<EIModifier>> modifiers;
	Unigine::Vector<SPtr<EITrigger>> triggers;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
struct EIActionValue final
{
	EIActionValueType valueType;
	Unigine::Math::vec3 value;

	float getMagnitude2() const { return value.length2(); }
	float getMagnitude() const { return value.length(); }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EIActionValueInstance
{
public:
	EIActionValueInstance() = default;

	EIActionValueInstance(const EIAction *action, EIActionValue v, eTriggerState state)
		: _action(action)
		, _v(std::move(v))
		, _state(state)
	{
	}

	const EIAction *getAction() const { return _action; }
	EIActionValue getValue() const { return _v; }
	eTriggerState getState() const { return _state; }

	float x() const noexcept { return _v.value.x; }
	float y() const noexcept { return _v.value.y; }
	float z() const noexcept { return _v.value.z; }

private:
	const EIAction *_action = nullptr;
	EIActionValue _v;
	eTriggerState _state = eTriggerState::None;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EIModifier
{
public:
	virtual ~EIModifier() = default;

	virtual const char *getClassName() const noexcept = 0;

	virtual EIActionValue modify(EIActionValue v) = 0;

	virtual void serialize(EISerializer &) {}
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EITrigger
{
public:
	virtual ~EITrigger() = default;

	float threshold = 0.5f;

	virtual const char *getClassName() const noexcept = 0;

	// `events` summarizes physical press / release events for the underlying
	// key during this frame. Bindings-level triggers receive populated
	// events; action-level triggers receive an empty struct (the action
	// value has no single key) and fall back to threshold-crossing on v.
	virtual eTriggerState update(EIActionValue v, const EIKeyFrameEvents &events) = 0;

	// Convenience overload for callers that don't have per-frame events
	// (self-tests, ad-hoc usage). Equivalent to update(v, {}), so triggers
	// fall back to threshold-crossing detection.
	eTriggerState update(EIActionValue v) { return update(v, EIKeyFrameEvents{}); }

	virtual void serialize(EISerializer &s) { s.io("threshold", threshold); }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
struct EIKeyBinding final
{
	EIKey key;
	Unigine::Vector<SPtr<EITrigger>> triggers;
	Unigine::Vector<SPtr<EIModifier>> modifiers;
};

struct EIMapping final
{
	// bindings[0] is the value source ("primary"); bindings[1..] act as AND gates
	// (their value is ignored — only their trigger-state activity is checked).
	Unigine::Vector<EIKeyBinding> bindings;
	// Human-readable label for this specific alternative, for rebinding UI.
	// EIActionMappings::description names the action as a whole ("Move"); this
	// names one row of it ("Move Forward"), which is what a settings screen
	// lists. Empty means "no dedicated label" — fall back to the action
	// description plus the key names.
	Unigine::String description = "";
	bool consumeInput = false;
};

struct EIActionMappings final
{
	const EIAction *action = nullptr;
	Unigine::String description = "";
	Unigine::Vector<EIMapping> mappings;     // OR alternatives
};

class EIContext
{
public:
	virtual ~EIContext() = default;

	Unigine::UGUID guid;
	Unigine::String name = "";
	Unigine::String description = "";
	bool enabled = true;

	virtual EIMapping *map(const EIAction *action, EIKey key) = 0;
	virtual void unmap(const EIAction *action) = 0;
	virtual void unmap() = 0;

	virtual Unigine::Vector<EIActionMappings> &getActionMappings() noexcept = 0;
	virtual const Unigine::Vector<EIActionMappings> &getActionMappings() const noexcept = 0;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
struct EIBinding
{
	const EIAction *action;
	eTriggerState state;
	std::function<void(EIActionValueInstance)> callback;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EILocalPlayer: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(EILocalPlayer, ComponentBase);

	PROP_PARAM(Toggle, useKeyboardMouse, true);
	PROP_PARAM(Toggle, useGamepad, false);

	virtual void addContext(EIContext *context, int priority = 0) = 0;
	virtual void removeContext(EIContext *context) = 0;
	virtual EIContext *findContext(const char *name) = 0;

	virtual EIBinding *bind(const EIAction *action, eTriggerState state, std::function<void(EIActionValueInstance)> callback) = 0;
	virtual void unbind(EIBinding *binding) = 0;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EISystem
{
public:
	static EISystem *get()
	{
		static EISystem *instance = nullptr;
		static Unigine::EventConnection connection;
		static Unigine::Mutex m;

		if (instance == nullptr)
		{
			Unigine::ScopedLock l(m);
			if (instance == nullptr)
			{
				if (!connection.isValid())
				{
					Unigine::Engine::get()->getEventPluginRemoved().connect(
						connection,
						[](const char *name) {
						if (Unigine::String::equal(name, "RyutpEnhancedInput"))
						{
							instance = nullptr;
							connection.disconnect();
						}
					});
				}
				instance = Unigine::Engine::get()->getPlugin<EISystem>("RyutpEnhancedInput");
			}
		}
		return instance;
	}

	virtual EICreatorRegistry<EIModifier> *getModifierRegistry() = 0;
	virtual EICreatorRegistry<EITrigger> *getTriggerRegistry() = 0;

	virtual EIFileSystemRegistry<EIAction> *getActionRegistry() = 0;
	virtual EIFileSystemRegistry<EIContext> *getContextRegistry() = 0;

	// Runs every shipped modifier and trigger through known inputs and asserts
	// the outputs. Returns the number of failed assertions; 0 means clean run.
	// Logs PASS/FAIL details to Unigine's console + log.
	virtual int runSelfTest() = 0;
};
