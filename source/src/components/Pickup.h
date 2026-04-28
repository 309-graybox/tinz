#pragma once
#include <UnigineComponentSystem.h>
#include <UnigineEvent.h>

class Pickup: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(Pickup, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	enum class Mode : int
	{
		Instant = 0,
		Magnet = 1,
		Interact = 2,
	};

	PROP_PARAM(Switch, mode, 0, "Instant,Magnet,Interact", "Pickup Mode")
	PROP_PARAM(Float, range, 3.0f)
	PROP_PARAM(Float, lifetime, 0.0f) // 0 = infinite

	// Magnet
	PROP_PARAM(Float, magnetSpeed, 6.0f)
	PROP_PARAM(Float, magnetTimeout, 3.0f)

	// Rotation (deg/sec around node-local Z). 0 = no spin.
	PROP_PARAM(Float, rotationSpeed, 60.0f)
	PROP_PARAM(Float, magnetRotationMul, 3.0f) // speed multiplier while magneting

	// Interact
	PROP_PARAM(Float, interactHoldTime, 0.0f) // 0 = tap, >0 = hold seconds
	PROP_PARAM(String, interactPromptText)    // shown in PickupPromptUI; empty = default

	// Stacking (logic in a follow-up task)
	PROP_PARAM(String, typeId)
	PROP_PARAM(Int, count, 1)
	PROP_PARAM(Toggle, stackable)

	Mode getMode() const noexcept { return static_cast<Mode>(static_cast<int>(mode)); }
	bool isReady() const noexcept { return _state == State::Idle; }
	bool isMagneting() const noexcept { return _state == State::Magnet; }
	bool isInteracting() const noexcept { return _state == State::Interact; }
	float getInteractProgress01() const noexcept;

	// Driven by PlayerInteraction
	void startMagnet(const Unigine::NodePtr &player);
	void startInteract(const Unigine::NodePtr &player);
	void cancelInteract();
	void pickUp(const Unigine::NodePtr &player); // immediate (Instant mode or end of Magnet/Interact)

	// Subclass extension (GoldPickup, AmmoPickup, ...)
	virtual bool canBePickedUp(const Unigine::NodePtr &player) const { return true; }

	// External subscribers (HUD, sound, inventory)
	Unigine::EventInvoker<Unigine::NodePtr, int> &eventPickedUp() noexcept { return _event_picked_up; }
	Unigine::EventInvoker<Unigine::NodePtr> &eventMagnetStarted() noexcept { return _event_magnet_started; }
	Unigine::EventInvoker<Unigine::NodePtr> &eventInteractStarted() noexcept { return _event_interact_started; }
	Unigine::EventInvoker<Unigine::NodePtr> &eventInteractCancelled() noexcept { return _event_interact_cancelled; }
	// Fires synchronously right before node.deleteLater(). Subscribers must drop refs.
	Unigine::EventInvoker<Pickup *> &eventDestroyed() noexcept { return _event_destroyed; }

protected:
	// Override for type-specific reward (give gold, give ammo, etc.)
	virtual void onPickedUp(const Unigine::NodePtr &player, int amount) {}

private:
	enum class State
	{
		Idle,
		Magnet,
		Interact,
	};

	void init();
	void update();

	void tickLifetime(float dt);
	void tickRotation(float dt);
	void tickMagnet(float dt);
	void tickInteract(float dt);

	void complete(const Unigine::NodePtr &player);

private:
	State _state = State::Idle;
	Unigine::NodePtr _target_player;

	float _life_timer = 0.0f;
	float _magnet_timer = 0.0f;
	float _interact_timer = 0.0f;

	Unigine::EventInvoker<Unigine::NodePtr, int> _event_picked_up;
	Unigine::EventInvoker<Unigine::NodePtr> _event_magnet_started;
	Unigine::EventInvoker<Unigine::NodePtr> _event_interact_started;
	Unigine::EventInvoker<Unigine::NodePtr> _event_interact_cancelled;
	Unigine::EventInvoker<Pickup *> _event_destroyed;
};
