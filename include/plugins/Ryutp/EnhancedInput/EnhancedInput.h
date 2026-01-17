#pragma once
#include "EIKey.h"
#include "Defines.h"
#include <UnigineComponentSystem.h>
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
	None = 1 << 0,
	Triggered = 1 << 1,
	Started = 1 << 2,
	Ongoing = 1 << 3,
	Canceled = 1 << 4,
	Completed = 1 << 5
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
	virtual void destroy(T *v) = 0;

	virtual bool save(int i) = 0;

	virtual bool saveDummy(const char *path) = 0;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
struct EIAction final
{
	Unigine::String name = "";
	Unigine::String desctiption = "";
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
	eTriggerState _state;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EIModifier: public Unigine::ComponentStruct
{
public:
	virtual const char *getClassName() const noexcept = 0;

	virtual EIActionValue modify(EIActionValue v) = 0;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EITrigger: public Unigine::ComponentStruct
{
public:
	PROP_PARAM(Float, treshold, 0.5f);

	virtual const char *getClassName() const noexcept = 0;

	virtual eTriggerState update(EIActionValue v) = 0;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
struct EIKeyActionMapping final
{
	const EIAction *action = nullptr;
	EIKey key;
	Unigine::Vector<SPtr<EIModifier>> modifiers;
	Unigine::Vector<SPtr<EITrigger>> triggers;
};

class EIContext
{
public:
	Unigine::String name = "";
	Unigine::String description = "";
	bool autoRegistration = false;

	virtual EIKeyActionMapping *map(const EIAction *action, EIKey key) = 0;
	virtual void unmap(const EIAction *action, EIKey key) = 0;
	virtual void unmap(const EIAction *action) = 0;
	virtual void unmap() = 0;

	virtual Unigine::Vector<EIKeyActionMapping *> &getMappings() noexcept = 0;
	virtual const Unigine::Vector<EIKeyActionMapping *> &getMappings() const noexcept = 0;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
struct EIBinding
{
	eTriggerState state;
	std::function<void(EIActionValueInstance)> callback;
};

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

	virtual void addContext(EIContext *context) = 0;
	virtual void removeContext(EIContext *context) = 0;

	virtual EICreatorRegistry<EIModifier> *getModifierRegistry() = 0;
	virtual EICreatorRegistry<EITrigger> *getTriggerRegistry() = 0;

	virtual EIFileSystemRegistry<EIAction> *getActionRegistry() = 0;
	virtual EIFileSystemRegistry<EIContext> *getContextRegistry() = 0;

	virtual EIBinding *bind(const EIAction *action, eTriggerState state, std::function<void(EIActionValueInstance)> callback) = 0;
	virtual void unbind(const EIAction *action, EIBinding *binding) = 0;
};
