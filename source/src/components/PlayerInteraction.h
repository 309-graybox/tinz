#pragma once
#include <UnigineComponentSystem.h>
#include <UnigineWorlds.h>
#include <UnigineCallback.h>
#include <UnigineGui.h>
#include <UnigineWidgets.h>
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

class Interactable;
class Pickup;

// Player-side resolver for Interactable interactions.
// Owns one large WorldTrigger sized to the maximum scan radius,
// tracks interactables currently in range, resolves mouse hover focus,
// and still drives Pickup-specific Magnet/Instant modes.
class PlayerInteraction: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(PlayerInteraction, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_PARAM(Node, playerNode)

	// Trigger size (max distance any Interactable in this game can be detected from).
	// Per-object `range` is then checked in code against this set.
	PROP_PARAM(Float, scanRadius, 7.0f)

	// Optional camera node — used only as ray origin for Magnet line-of-sight.
	// If unset, ray starts from the player node.
	PROP_PARAM(Node, cameraNode)

	// Line-of-sight check on Magnet start.
	PROP_PARAM(Toggle, magnetRequiresLineOfSight)
	PROP_PARAM(Mask, magnetLineOfSightMask) // intersection mask for LOS raycast

	// Mouse ray for hover/focus.
	PROP_PARAM(Float, hoverRayDistance, 100.0f)
	PROP_PARAM(Mask, hoverIntersectionMask, ~0)

	Interactable *getCurrentFocus() const noexcept { return _current_focus; }

private:
	void init();
	void update();
	void shutdown();

	void onTriggerEnter(const Unigine::NodePtr &node);
	void onTriggerLeave(const Unigine::NodePtr &node);
	void onInteractableDestroyed(Interactable *interactable);
	void trackInteractable(Interactable *interactable);

	void tryStartMagnet(Pickup *pickup);
	bool hasLineOfSight(const Pickup *pickup) const;

	Interactable *resolveFocus() const;
	Interactable *resolveInteractCandidate() const;
	Interactable *raycastFocus() const;
	bool canFocus(Interactable *interactable) const;
	bool isInInteractRange(const Interactable *interactable) const;
	bool shouldKeepInteractRange(const Interactable *interactable) const;
	bool isWithinDistance(const Interactable *interactable, float distance) const;
	void updateRange();
	void updateHover(Interactable *next_focus);

	void handleInteractInput();
	void ensureInteractPrompt();
	void updateInteractPrompt(Interactable *candidate);
	void hideInteractPrompt();

private:
	Unigine::WorldTriggerPtr _trigger;
	Unigine::EventConnection _conn_enter;
	Unigine::EventConnection _conn_leave;

	Unigine::Vector<Interactable *> _in_range;
	Unigine::Vector<Interactable *> _range_interactables;
	Unigine::Vector<Interactable *> _tracked_interactables;
	Interactable *_current_focus = nullptr;

	EIBinding *_binding_interact = nullptr;
	bool _interact_requested = false;

	Unigine::GuiPtr _gui;
	Unigine::WidgetLabelPtr _interact_prompt;
};
