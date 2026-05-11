#include "components/interaction/Interactable.h"
#include "tuning/DebugTuning.h"

#include <UnigineGame.h>
#include <UnigineLog.h>

REGISTER_COMPONENT(Interactable)

using namespace Unigine;
using namespace Unigine::Math;

void Interactable::init()
{
	_state = State::Idle;
	_in_range = false;
	_hovered = false;
	_range_interactor.clear();
	_hovered_by.clear();
	_target_interactor.clear();
	_interact_timer = 0.0f;
}

void Interactable::update()
{
	if (_state == State::Interact)
		tickInteract(Game::getIFps());
}

void Interactable::beginRange(const NodePtr &interactor)
{
	if (!interactor)
		return;

	if (_in_range && _range_interactor == interactor)
		return;

	if (_in_range)
		endRange(_range_interactor);

	_in_range = true;
	_range_interactor = interactor;
	_event_range_entered.run(interactor);
	onRangeEntered(interactor);
}

void Interactable::tickRange(const NodePtr &interactor)
{
	if (!interactor)
		return;

	beginRange(interactor);
	onInRange(interactor);
}

void Interactable::endRange(const NodePtr &interactor)
{
	if (!_in_range)
		return;

	NodePtr who = _range_interactor ? _range_interactor : interactor;
	_in_range = false;
	_range_interactor.clear();

	_event_range_left.run(who);
	onRangeLeft(who);
}

void Interactable::beginHover(const NodePtr &interactor)
{
	if (!interactor)
		return;

	if (_hovered && _hovered_by == interactor)
		return;

	if (_hovered)
		endHover(_hovered_by);

	_hovered = true;
	_hovered_by = interactor;
	_event_hover_started.run(interactor);
	onHoverStarted(interactor);
}

void Interactable::tickHover(const NodePtr &interactor)
{
	if (!interactor)
		return;

	beginHover(interactor);
	onHover(interactor);
}

void Interactable::endHover(const NodePtr &interactor)
{
	if (!_hovered)
		return;

	NodePtr who = _hovered_by ? _hovered_by : interactor;
	_hovered = false;
	_hovered_by.clear();

	_event_hover_ended.run(who);
	onHoverEnded(who);
}

void Interactable::startInteract(const NodePtr &interactor)
{
	bool can = canInteract(interactor);

	if (DebugTuning::get()->log_interaction)
	{
		Log::error("[%s] start interract with %s (state: %i; can: %i)\n", interactor->getName(), node->getName(), (int)_state, (int)can);
	}

	if (_state != State::Idle || !interactor || !can)
		return;

	_state = State::Interact;
	_target_interactor = interactor;
	_interact_timer = 0.0f;

	_event_interact_started.run(interactor);
	onInteractStarted(interactor);

	if (interactHoldTime <= 0.0f)
		completeInteract(interactor);
}

void Interactable::tickInteract(float dt)
{
	if (!_target_interactor)
	{
		cancelInteract();
		return;
	}

	_interact_timer += dt;
	if (_interact_timer >= interactHoldTime)
		completeInteract(_target_interactor);
}

void Interactable::cancelInteract()
{
	if (_state != State::Interact)
		return;

	NodePtr who = _target_interactor;
	_state = State::Idle;
	_target_interactor.clear();
	_interact_timer = 0.0f;

	_event_interact_cancelled.run(who);
	onInteractCancelled(who);
}

float Interactable::getInteractProgress01() const noexcept
{
	if (_state != State::Interact || interactHoldTime <= 0.0f)
		return 0.0f;
	return clamp(_interact_timer / (float)interactHoldTime, 0.0f, 1.0f);
}

bool Interactable::canInteract(const NodePtr &interactor) const
{
	return interactor != nullptr;
}

float Interactable::getRangeExitDistance() const
{
	return range;
}

void Interactable::onRangeEntered(const NodePtr &interactor)
{
}

void Interactable::onInRange(const NodePtr &interactor)
{
}

void Interactable::onRangeLeft(const NodePtr &interactor)
{
}

void Interactable::onHoverStarted(const NodePtr &interactor)
{
}

void Interactable::onHover(const NodePtr &interactor)
{
}

void Interactable::onHoverEnded(const NodePtr &interactor)
{
}

void Interactable::onInteractStarted(const NodePtr &interactor)
{
}

void Interactable::onInteractCancelled(const NodePtr &interactor)
{
}

void Interactable::onInteract(const NodePtr &interactor)
{
}

void Interactable::completeInteract(const NodePtr &interactor)
{
	NodePtr who = interactor;
	_state = State::Idle;
	_target_interactor.clear();
	_interact_timer = 0.0f;

	_event_interacted.run(who);
	onInteract(who);
}

void Interactable::notifyDestroyed()
{
	if (_hovered)
		endHover(_hovered_by);
	if (_in_range)
		endRange(_range_interactor);
	if (_state == State::Interact)
		cancelInteract();

	_event_destroyed.run(this);
}
