#include "PlayerInteraction.h"
#include "Pickup.h"

#include <UnigineGame.h>
#include <UnigineWorld.h>
#include <UnigineLog.h>
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
	auto player = ComponentSystem::get()->getComponent<EILocalPlayer>(node);
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
	Vector<Pickup *> snapshot = _in_range;

	for (int i = 0; i < snapshot.size(); ++i)
	{
		Pickup *p = snapshot[i];
		if (!p)
			continue;
		if (_in_range.find(p) == _in_range.end())
			continue; // removed by a prior iteration's destroy event
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

	_current_focus = resolveFocus();
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

Pickup *PlayerInteraction::resolveFocus() const
{
	const Vec3 player_pos = node->getWorldPosition();

	Pickup *best = nullptr;
	float best_dist = FLT_MAX;

	for (int i = 0; i < _in_range.size(); ++i)
	{
		Pickup *p = _in_range[i];
		if (!p || !p->isReady())
			continue;
		if (p->getMode() != Pickup::Mode::Interact)
			continue;
		if (!p->canBePickedUp(node))
			continue;

		const NodePtr p_node = p->getNode();
		if (!p_node)
			continue;

		const float dist = (float)length(p_node->getWorldPosition() - player_pos);
		if (dist > p->range)
			continue;

		if (dist < best_dist)
		{
			best_dist = dist;
			best = p;
		}
	}

	return best;
}

void PlayerInteraction::handleInteractInput()
{
	const bool pressed = _interact_requested;
	_interact_requested = false;

	// Cancel any in-flight interact whose pickup is no longer the focus.
	for (int i = 0; i < _in_range.size(); ++i)
	{
		Pickup *p = _in_range[i];
		if (p && p->isInteracting() && p != _current_focus)
			p->cancelInteract();
	}

	if (pressed && _current_focus && _current_focus->isReady())
		_current_focus->startInteract(node);
}

void PlayerInteraction::onTriggerEnter(const Unigine::NodePtr &n)
{
	Pickup *p = ComponentSystem::get()->getComponent<Pickup>(n);
	if (!p)
		return;
	if (_in_range.find(p) == _in_range.end())
	{
		_in_range.append(p);
		p->eventDestroyed().connect(this, &PlayerInteraction::onPickupDestroyed);
	}
}

void PlayerInteraction::onPickupDestroyed(Pickup *p)
{
	auto it = _in_range.find(p);
	if (it != _in_range.end())
		_in_range.remove(it);
	if (p == _current_focus)
		_current_focus = nullptr;
}

void PlayerInteraction::onTriggerLeave(const Unigine::NodePtr &n)
{
	Pickup *p = ComponentSystem::get()->getComponent<Pickup>(n);
	if (!p)
		return;

	if (p->isInteracting())
		p->cancelInteract();
	if (p == _current_focus)
		_current_focus = nullptr;

	auto it = _in_range.find(p);
	if (it != _in_range.end())
		_in_range.remove(it);
}
