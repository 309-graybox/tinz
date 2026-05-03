#pragma once
#include "Interactable.h"

#include <UnigineEvent.h>

class Pickup: public Interactable
{
public:
	COMPONENT_DEFINE(Pickup, Interactable)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	enum class Mode : int
	{
		Instant = 0,
		Magnet = 1,
		Interact = 2,
	};

	PROP_PARAM(Switch, mode, 0, "Instant,Magnet,Interact", "Pickup Mode")
	PROP_PARAM(Float, lifetime, 0.0f) // 0 = infinite

	// Magnet
	PROP_PARAM(Float, magnetSpeed, 6.0f)
	PROP_PARAM(Float, magnetTimeout, 3.0f)

	// Rotation (deg/sec around node-local Z). 0 = no spin.
	PROP_PARAM(Float, rotationSpeed, 60.0f)
	PROP_PARAM(Float, magnetRotationMul, 3.0f) // speed multiplier while magneting

	// Stacking (logic in a follow-up task)
	PROP_PARAM(String, typeId)
	PROP_PARAM(Int, count, 1)
	PROP_PARAM(Toggle, stackable)

	// Audio. Either a registered SoundManager event id or a direct asset path.
	// Empty = silent.
	PROP_PARAM(String, soundPickedUp)

	Mode getMode() const noexcept { return static_cast<Mode>(static_cast<int>(mode)); }
	bool isReady() const noexcept { return _state == State::Idle && isInteractionReady(); }
	bool isMagneting() const noexcept { return _state == State::Magnet; }

	// Driven by PlayerInteraction
	void startMagnet(const Unigine::NodePtr &player);
	void pickUp(const Unigine::NodePtr &player); // immediate (Instant mode or end of Magnet/Interact)

	// Subclass extension (GoldPickup, AmmoPickup, ...)
	virtual bool canBePickedUp(const Unigine::NodePtr &player) const;
	bool canInteract(const Unigine::NodePtr &player) const override;

	// External subscribers (HUD, sound, inventory)
	Unigine::EventInvoker<Unigine::NodePtr, int> &eventPickedUp() noexcept { return _event_picked_up; }
	Unigine::EventInvoker<Unigine::NodePtr> &eventMagnetStarted() noexcept { return _event_magnet_started; }

protected:
	// Override for type-specific reward (give gold, give ammo, etc.)
	virtual void onPickedUp(const Unigine::NodePtr &player, int amount) {}
	void onInteract(const Unigine::NodePtr &player) override;

private:
	enum class State
	{
		Idle,
		Magnet,
	};

	void init();
	void update();

	void tickLifetime(float dt);
	void tickRotation(float dt);
	void tickMagnet(float dt);

private:
	State _state = State::Idle;
	Unigine::NodePtr _target_player;

	float _life_timer = 0.0f;
	float _magnet_timer = 0.0f;

	Unigine::EventInvoker<Unigine::NodePtr, int> _event_picked_up;
	Unigine::EventInvoker<Unigine::NodePtr> _event_magnet_started;
};
