#pragma once
#include <UnigineComponentSystem.h>
#include <UnigineWorlds.h>
#include <UnigineCallback.h>
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

class Pickup;

// Player-side resolver for Pickup interactions.
// Owns one large WorldTrigger sized to the maximum scan radius,
// tracks pickups currently in range, picks the best Interact-mode
// focus per-frame, and drives Magnet/Instant pickups.
class PlayerInteraction: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(PlayerInteraction, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_PARAM(Node, playerNode)

	// Trigger size (max distance any Pickup in this game can be detected from).
	// Per-pickup `range` is then checked in code against this set.
	PROP_PARAM(Float, scanRadius, 15.0f)

	// Optional camera node — used only as ray origin for Magnet line-of-sight.
	// If unset, ray starts from the player node.
	PROP_PARAM(Node, cameraNode)

	// Line-of-sight check on Magnet start.
	PROP_PARAM(Toggle, magnetRequiresLineOfSight)
	PROP_PARAM(Mask, magnetLineOfSightMask) // intersection mask for LOS raycast

	Pickup *getCurrentFocus() const noexcept { return _current_focus; }

private:
	void init();
	void update();
	void shutdown();

	void onTriggerEnter(const Unigine::NodePtr &node);
	void onTriggerLeave(const Unigine::NodePtr &node);
	void onPickupDestroyed(Pickup *pickup);

	void tryStartMagnet(Pickup *pickup);
	bool hasLineOfSight(const Pickup *pickup) const;

	Pickup *resolveFocus() const;

	void handleInteractInput();

private:
	Unigine::WorldTriggerPtr _trigger;
	Unigine::EventConnection _conn_enter;
	Unigine::EventConnection _conn_leave;

	Unigine::Vector<Pickup *> _in_range;
	Pickup *_current_focus = nullptr;

	EIBinding *_binding_interact = nullptr;
	bool _interact_requested = false;
};
