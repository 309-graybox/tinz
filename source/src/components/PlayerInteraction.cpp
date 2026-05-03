#include "PlayerInteraction.h"
#include "Interactable.h"
#include "Pickup.h"

#include <UnigineGame.h>
#include <UnigineInput.h>
#include <UnigineWorld.h>
#include <UnigineLog.h>
#include <UniginePlayers.h>
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

REGISTER_COMPONENT(PlayerInteraction)

using namespace Unigine;
using namespace Unigine::Math;

void PlayerInteraction::init()
{
	const float side = (float)scanRadius * 2.0f;
	_trigger = WorldTrigger::create(vec3(side));
	_trigger->setName("PlayerInteractionTrigger");
	_trigger->setParent(node);
	_trigger->setPosition(Vec3_zero);
	_trigger->setExcludeNodes({node});
	_trigger->setTriggerInteractionEnabled(true);

	_trigger->getEventEnter().connect(this, &PlayerInteraction::onTriggerEnter);
	_trigger->getEventLeave().connect(this, &PlayerInteraction::onTriggerLeave);

	auto ei = EISystem::get();
	if (!ei)
	{
		Log::error("Can not get EnhancedInput!\n");
		return;
	}

	auto player = ComponentSystem::get()->getComponent<EILocalPlayer>(playerNode);
	if (!player)
	{
		Log::error("Can not get EILocalPlayer\n");
		return;
	}

	auto action_reg = ei->getActionRegistry();
	auto action_interact = action_reg->create("interact");
	if (!action_interact)
	{
		Log::error("Can not load interact action\n");
		return;
	}

	_binding_interact = player->bind(action_interact, eTriggerState::Triggered, [this](EIActionValueInstance inst) {
		if (!compare(inst.getValue().value.x, 0.0f))
			_interact_requested = true;
	});
}

void PlayerInteraction::shutdown()
{
	if (_trigger)
	{
		_trigger.deleteLater();
		_trigger.clear();
	}

	auto ei = EISystem::get();
	if (!ei)
		return;
	auto player = ComponentSystem::get()->getComponent<EILocalPlayer>(playerNode);
	if (!player)
		return;
	if (_binding_interact)
		player->unbind(_binding_interact);
	_binding_interact = nullptr;
}

void PlayerInteraction::update()
{
	const Vec3 player_pos = node->getWorldPosition();

	// Snapshot — pickup->pickUp() fires eventDestroyed synchronously,
	// which mutates _in_range mid-iteration. Validate against live set each step.
	Vector<Interactable *> snapshot = _in_range;

	for (int i = 0; i < snapshot.size(); ++i)
	{
		Interactable *interactable = snapshot[i];
		if (!interactable)
			continue;
		if (_in_range.find(interactable) == _in_range.end())
			continue; // removed by a prior iteration's destroy event

		Pickup *p = dynamic_cast<Pickup *>(interactable);
		if (!p)
			continue;
		if (!p->isReady())
			continue;

		const NodePtr p_node = p->getNode();
		if (!p_node)
			continue;

		const float dist = (float)length(p_node->getWorldPosition() - player_pos);
		if (dist > p->range)
			continue;
		if (!p->canBePickedUp(node))
			continue;

		switch (p->getMode())
		{
			case Pickup::Mode::Instant:
				p->pickUp(node);
				break;
			case Pickup::Mode::Magnet:
				tryStartMagnet(p);
				break;
			case Pickup::Mode::Interact:
				break; // handled by focus resolver
		}
	}

	updateHover(resolveFocus());
	handleInteractInput();
}

void PlayerInteraction::tryStartMagnet(Pickup *pickup)
{
	if (magnetRequiresLineOfSight && !hasLineOfSight(pickup))
		return;
	pickup->startMagnet(node);
}

bool PlayerInteraction::hasLineOfSight(const Pickup *pickup) const
{
	if (!pickup || !pickup->getNode())
		return false;

	const NodePtr cam = cameraNode;
	const Vec3 from = cam ? cam->getWorldPosition() : node->getWorldPosition();
	const Vec3 to = pickup->getNode()->getWorldPosition();

	const int mask = (int)magnetLineOfSightMask;
	auto hit = World::getIntersection(from, to, mask);
	if (!hit)
		return true;

	// Hit must be the pickup itself (or its child).
	NodePtr hit_node = static_ptr_cast<Node>(hit);
	return hit_node && hit_node == pickup->getNode();
}

Interactable *PlayerInteraction::resolveFocus() const
{
	Interactable *hit = raycastFocus();
	return canFocus(hit) ? hit : nullptr;
}

Interactable *PlayerInteraction::raycastFocus() const
{
	const NodePtr cam = cameraNode;
	PlayerPtr player = dynamic_ptr_cast<Player>(cam);
	if (!player)
		player = Game::getPlayer();
	if (!player)
		return nullptr;

	const ivec2 mouse = Input::getMousePosition();
	const Vec3 from = player->getWorldPosition();
	const Vec3 dir = Vec3(player->getDirectionFromMainWindow(mouse.x, mouse.y));
	const Vec3 to = from + dir * (float)hoverRayDistance;

	ObjectPtr hit = World::getIntersection(from, to, (int)hoverIntersectionMask);
	return hit ? ComponentSystem::get()->getComponentInParent<Interactable>(hit) : nullptr;
}

bool PlayerInteraction::canFocus(Interactable *interactable) const
{
	if (!interactable)
		return false;
	if (!interactable->isInteractionReady() && !interactable->isInteracting())
		return false;
	if (!isInInteractRange(interactable))
		return false;
	if (!interactable->canInteract(node))
		return false;

	return true;
}

bool PlayerInteraction::isInInteractRange(const Interactable *interactable) const
{
	if (!interactable || !interactable->getNode())
		return false;

	const Vec3 player_pos = node->getWorldPosition();
	const float dist = (float)length(interactable->getNode()->getWorldPosition() - player_pos);
	return dist <= interactable->range;
}

void PlayerInteraction::updateHover(Interactable *next_focus)
{
	if (_current_focus == next_focus)
	{
		if (_current_focus)
			_current_focus->tickHover(node);
		return;
	}

	if (_current_focus)
	{
		if (_current_focus->isInteracting())
			_current_focus->cancelInteract();
		_current_focus->endHover(node);
	}

	_current_focus = next_focus;

	if (_current_focus)
		_current_focus->tickHover(node);
}

void PlayerInteraction::handleInteractInput()
{
	const bool pressed = _interact_requested;
	_interact_requested = false;

	// Cancel any in-flight interaction whose object is no longer the focus.
	for (int i = 0; i < _in_range.size(); ++i)
	{
		Interactable *interactable = _in_range[i];
		if (interactable && interactable->isInteracting() && interactable != _current_focus)
			interactable->cancelInteract();
	}

	if (pressed && _current_focus && _current_focus->isInteractionReady())
		_current_focus->startInteract(node);
}

void PlayerInteraction::onTriggerEnter(const Unigine::NodePtr &n)
{
	Interactable *interactable = ComponentSystem::get()->getComponentInParent<Interactable>(n);
	if (!interactable)
		return;
	if (_in_range.find(interactable) == _in_range.end())
	{
		_in_range.append(interactable);
		interactable->eventDestroyed().connect(this, &PlayerInteraction::onInteractableDestroyed);
	}
}

void PlayerInteraction::onInteractableDestroyed(Interactable *interactable)
{
	auto it = _in_range.find(interactable);
	if (it != _in_range.end())
		_in_range.remove(it);
	if (interactable == _current_focus)
		_current_focus = nullptr;
}

void PlayerInteraction::onTriggerLeave(const Unigine::NodePtr &n)
{
	Interactable *interactable = ComponentSystem::get()->getComponentInParent<Interactable>(n);
	if (!interactable)
		return;

	if (interactable->isInteracting())
		interactable->cancelInteract();
	if (interactable == _current_focus)
	{
		interactable->endHover(node);
		_current_focus = nullptr;
	}

	auto it = _in_range.find(interactable);
	if (it != _in_range.end())
		_in_range.remove(it);
}
