#include "player/interaction/PlayerInteraction.h"
#include "components/interaction/Interactable.h"
#include "components/interaction/Pickup.h"
#include "tuning/DebugTuning.h"

#include <UnigineGame.h>
#include <UnigineGui.h>
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
	_gui = Gui::getCurrent();
	ensureInteractPrompt();

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
	hideInteractPrompt();
	if (_interact_prompt)
	{
		_interact_prompt.deleteLater();
		_interact_prompt.clear();
	}

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
	if (DebugTuning::get()->show_interaction_trigger)
	{
		_trigger->renderVisualizer();
	}

	const Vec3 player_pos = node->getWorldPosition();

	updateRange();

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
	updateInteractPrompt(resolveInteractCandidate());
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

Interactable *PlayerInteraction::resolveInteractCandidate() const
{
	Interactable *best = nullptr;
	float best_dist2 = Consts::INF;

	for (int i = 0; i < _in_range.size(); ++i)
	{
		Interactable *interactable = _in_range[i];
		if (!interactable || !interactable->isEnabled())
			continue;
		if (!interactable->isInteractionReady() && !interactable->isInteracting())
			continue;

		const bool was_in_range = _range_interactables.find(interactable) != _range_interactables.end();
		const bool in_range = was_in_range ? shouldKeepInteractRange(interactable) : isInInteractRange(interactable);
		if (!in_range)
			continue;
		if (!interactable->canInteract(node))
			continue;

		const NodePtr i_node = interactable->getNode();
		if (!i_node)
			continue;

		const float dist2 = (float)length2(i_node->getWorldPosition() - node->getWorldPosition());
		if (dist2 < best_dist2)
		{
			best_dist2 = dist2;
			best = interactable;
		}
	}

	return best;
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

	const bool was_in_range = _range_interactables.find(interactable) != _range_interactables.end();
	const bool is_in_range = was_in_range ? shouldKeepInteractRange(interactable) : isInInteractRange(interactable);
	if (!is_in_range)
		return false;
	if (!interactable->canInteract(node))
		return false;

	return true;
}

bool PlayerInteraction::isInInteractRange(const Interactable *interactable) const
{
	return interactable && isWithinDistance(interactable, (float)interactable->range);
}

bool PlayerInteraction::shouldKeepInteractRange(const Interactable *interactable) const
{
	return interactable && isWithinDistance(interactable, interactable->getRangeExitDistance());
}

bool PlayerInteraction::isWithinDistance(const Interactable *interactable, float distance) const
{
	if (!interactable || !interactable->getNode())
		return false;

	const Vec3 player_pos = node->getWorldPosition();
	const float dist = (float)length(interactable->getNode()->getWorldPosition() - player_pos);
	return dist <= distance;
}

void PlayerInteraction::updateRange()
{
	Vector<Interactable *> candidates;
	ComponentSystem::get()->getComponentsInWorld<Interactable>(candidates, true);

	Vector<Interactable *> next_range;
	for (int i = 0; i < candidates.size(); ++i)
	{
		Interactable *interactable = candidates[i];
		if (!interactable || !interactable->isEnabled())
			continue;

		const bool was_in_range = _range_interactables.find(interactable) != _range_interactables.end();
		const bool is_in_range = was_in_range ? shouldKeepInteractRange(interactable) : isInInteractRange(interactable);
		if (!is_in_range)
			continue;

		trackInteractable(interactable);
		next_range.append(interactable);
		interactable->tickRange(node);
	}

	Vector<Interactable *> previous = _range_interactables;
	for (int i = 0; i < previous.size(); ++i)
	{
		Interactable *interactable = previous[i];
		if (!interactable || next_range.find(interactable) != next_range.end())
			continue;

		if (interactable->isInteracting())
			interactable->cancelInteract();
		interactable->endRange(node);
	}

	_range_interactables = next_range;
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
		if (interactable && interactable->isInteracting())
			interactable->cancelInteract();
	}

	// if (pressed && _current_focus && _current_focus->isInteractionReady())
	// 	_current_focus->startInteract(node);

	if (pressed)
	{
		if (!_in_range.empty())
		{
			auto np = node->getWorldPosition();
			std::sort(_in_range.begin(), _in_range.end(), [np](const Interactable *a, const Interactable *b) {
				auto da = distance2(a->getNode()->getWorldPosition(), np);
				auto db = distance2(b->getNode()->getWorldPosition(), np);
				return da < db;
			});

			_in_range[0]->startInteract(node);
		} else
		{
			Log::message("No iteractables in range\n");
		}
	}
}

void PlayerInteraction::ensureInteractPrompt()
{
	if (_interact_prompt)
		return;
	if (!_gui)
		_gui = Gui::getCurrent();
	if (!_gui)
		return;

	_interact_prompt = WidgetLabel::create(_gui, "");
	if (!_interact_prompt)
		return;

	_interact_prompt->setFontSize(24);
	_interact_prompt->setPosition(0, -48);
	_interact_prompt->setHidden(true);
	_gui->addChild(_interact_prompt, Gui::ALIGN_CENTER);
}

void PlayerInteraction::updateInteractPrompt(Interactable *candidate)
{
	ensureInteractPrompt();
	if (!_interact_prompt)
		return;

	if (!candidate)
	{
		hideInteractPrompt();
		return;
	}

	const char *custom = candidate->interactPromptText.get();
	String text;
	if (custom && custom[0] != '\0')
	{
		text = custom;
	} else
	{
		text = ((float)candidate->interactHoldTime > Consts::EPS) ? "Hold Interact" : "Interact";
	}

	_interact_prompt->setText(text.get());
	_interact_prompt->setHidden(false);
}

void PlayerInteraction::hideInteractPrompt()
{
	if (_interact_prompt)
		_interact_prompt->setHidden(true);
}

void PlayerInteraction::onTriggerEnter(const Unigine::NodePtr &n)
{
	Interactable *interactable = ComponentSystem::get()->getComponentInParent<Interactable>(n);
	if (!interactable)
		return;
	if (_in_range.find(interactable) == _in_range.end())
	{
		_in_range.append(interactable);
		trackInteractable(interactable);
	}
}

void PlayerInteraction::onInteractableDestroyed(Interactable *interactable)
{
	auto it = _in_range.find(interactable);
	if (it != _in_range.end())
		_in_range.remove(it);
	it = _range_interactables.find(interactable);
	if (it != _range_interactables.end())
		_range_interactables.remove(it);
	it = _tracked_interactables.find(interactable);
	if (it != _tracked_interactables.end())
		_tracked_interactables.remove(it);
	if (interactable == _current_focus)
		_current_focus = nullptr;
}

void PlayerInteraction::trackInteractable(Interactable *interactable)
{
	if (!interactable || _tracked_interactables.find(interactable) != _tracked_interactables.end())
		return;

	_tracked_interactables.append(interactable);
	interactable->eventDestroyed().connect(this, &PlayerInteraction::onInteractableDestroyed);
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
