#include "OfferingBowl.h"

#include "Inventory.h"
#include "../audio/SoundManager.h"

#include <UnigineGame.h>
#include <UnigineLog.h>

REGISTER_COMPONENT(OfferingBowl)

using namespace Unigine;
using namespace Unigine::Math;

void OfferingBowl::init()
{
	_requirements_state.clear();
	for (int i = 0; i < requirements.size(); ++i)
	{
		auto &cfg = requirements[i];
		if (cfg->typeId.get()[0] == '\0')
			continue;

		RequirementState entry;
		entry.type_id = cfg->typeId.get();
		entry.required = max((int)cfg->requiredCount, 0);
		entry.deposited = 0;

		if (entry.required > 0)
			_requirements_state.append(entry);
	}

	_visual_state.clear();
	for (int i = 0; i < visuals.size(); ++i)
	{
		auto &cfg = visuals[i];
		auto n = World::loadNode(cfg->sourceNode);
		if (n)
			n->setEnabled(false);

		VisualState entry;
		entry.type_id = cfg->typeId.get();
		entry.node = n;
		entry.used = false;
		_visual_state.append(entry);
	}

	_flights.clear();
	_drain_source.clear();
	_drain_timer = 0.0f;
	_draining = false;
	_was_filled = isFilled();
}

void OfferingBowl::update()
{
	const float dt = Game::getIFps();
	updateFlights(dt);

	if (!_draining)
		return;
	if (!_drain_source)
	{
		stopDraining();
		return;
	}
	if (lockWhenFilled && isFilled())
	{
		stopDraining();
		return;
	}

	Inventory *inventory = resolveInventory(_drain_source);
	if (!inventory)
	{
		stopDraining();
		return;
	}

	const float step = max((float)transferInterval, 0.0f);
	if (step <= Consts::EPS)
	{
		if (!drainOne(inventory))
			stopDraining();
		return;
	}

	_drain_timer += dt;
	while (_drain_timer >= step)
	{
		_drain_timer -= step;
		if (!drainOne(inventory))
		{
			stopDraining();
			break;
		}
	}
}

bool OfferingBowl::canInteract(const NodePtr &interactor) const
{
	if (!Interactable::canInteract(interactor))
		return false;
	if (_draining)
		return false;
	if (lockWhenFilled && isFilled())
		return false;

	const Inventory *inventory = resolveInventory(interactor);
	return inventory && hasAnyTransferable(inventory);
}

bool OfferingBowl::isFilled() const noexcept
{
	if (_requirements_state.empty())
		return false;

	for (int i = 0; i < _requirements_state.size(); ++i)
	{
		const RequirementState &req = _requirements_state[i];
		if (req.required > 0 && req.deposited < req.required)
			return false;
	}
	return true;
}

float OfferingBowl::getFillProgress01() const noexcept
{
	const int total_required = getTotalRequired();
	if (total_required <= 0)
		return 0.0f;
	return clamp((float)getTotalDeposited() / (float)total_required, 0.0f, 1.0f);
}

void OfferingBowl::onInteract(const NodePtr &interactor)
{
	Interactable::onInteract(interactor);

	if (!canInteract(interactor))
		return;

	_drain_source = interactor;
	_drain_timer = max((float)transferInterval, 0.0f);
	_draining = true;
}

Inventory *OfferingBowl::resolveInventory(const NodePtr &interactor) const
{
	if (!interactor)
		return nullptr;

	return ComponentSystem::get()->getComponent<Inventory>(interactor);
}

bool OfferingBowl::hasAnyTransferable(const Inventory *inventory) const
{
	if (!inventory)
		return false;

	for (int i = 0; i < _requirements_state.size(); ++i)
	{
		const RequirementState &req = _requirements_state[i];
		if (req.required <= 0 || req.deposited >= req.required)
			continue;
		if (inventory->getCount(req.type_id.get()) <= 0)
			continue;
		if (requireVisualNode && findFreeVisual(req.type_id.get()) < 0)
			continue;
		return true;
	}
	return false;
}

int OfferingBowl::pickRequirementToDrain(const Inventory *inventory) const
{
	if (!inventory)
		return -1;

	for (int i = 0; i < _requirements_state.size(); ++i)
	{
		const RequirementState &req = _requirements_state[i];
		if (req.required <= 0 || req.deposited >= req.required)
			continue;
		if (inventory->getCount(req.type_id.get()) <= 0)
			continue;
		if (requireVisualNode && findFreeVisual(req.type_id.get()) < 0)
			continue;
		return i;
	}
	return -1;
}

bool OfferingBowl::drainOne(Inventory *inventory)
{
	const int req_index = pickRequirementToDrain(inventory);
	if (req_index < 0)
		return false;

	RequirementState &req = _requirements_state[req_index];
	const int visual_index = findFreeVisual(req.type_id.get());
	if (requireVisualNode && visual_index < 0)
		return false;

	const int removed = inventory->removeItem(req.type_id.get(), 1);
	if (removed <= 0)
		return false;

	req.deposited = min(req.deposited + removed, req.required);

	if (visual_index >= 0)
		launchVisual(visual_index);

	if (node)
		audio::SoundManager::play3DAt(soundDrain.get(), node->getWorldPosition());

	if (!_was_filled && isFilled())
	{
		_was_filled = true;
		if (node)
			audio::SoundManager::play3DAt(soundFilled.get(), node->getWorldPosition());
		Log::message("OfferingBowl \"%s\" filled\n", node->getName());
	}

	return true;
}

int OfferingBowl::findFreeVisual(const char *type_id) const
{
	if (!type_id || !*type_id)
		return -1;

	for (int i = 0; i < _visual_state.size(); ++i)
	{
		const VisualState &v = _visual_state[i];
		if (v.used)
			continue;
		if (v.type_id != type_id)
			continue;
		if (!v.node)
			continue;
		return i;
	}
	return -1;
}

void OfferingBowl::launchVisual(int visual_index)
{
	if (visual_index < 0 || visual_index >= _visual_state.size())
		return;

	VisualState &v = _visual_state[visual_index];
	// FIXME
	// v.used = true;
	if (!v.node)
		return;

	v.node->setEnabled(true);

	const Vec3 bowl_pos = node ? node->getWorldPosition() : v.node->getWorldPosition();
	const float r = max((float)arrivalRadius, 0.0f);
	const float angle = Game::getRandomFloat(0.0f, Consts::PI2);
	const float dist = Game::getRandomFloat(0.0f, r);

	FlightState flight;
	flight.node = v.node;
	flight.start = _drain_source->getWorldPosition();
	flight.end = bowl_pos + Vec3(cos(angle) * dist, sin(angle) * dist, 0.0f);
	flight.duration = max((float)flightDuration, 0.01f);
	flight.timer = 0.0f;
	_flights.append(flight);
}

void OfferingBowl::updateFlights(float dt)
{
	const float height = max((float)arcHeight, 0.0f);

	for (int i = _flights.size() - 1; i >= 0; --i)
	{
		FlightState &flight = _flights[i];
		if (!flight.node)
		{
			_flights.removeFast(i);
			continue;
		}

		flight.timer += dt;
		const float duration = max(flight.duration, 0.0001f);
		const float t = clamp(flight.timer / duration, 0.0f, 1.0f);

		Vec3 pos = flight.start + (flight.end - flight.start) * t;
		pos.z += 4.0f * height * t * (1.0f - t);
		flight.node->setWorldPosition(pos);

		if (t >= 1.0f - Consts::EPS)
		{
			flight.node->setWorldPosition(flight.end);
			_flights.removeFast(i);
		}
	}
}

void OfferingBowl::stopDraining()
{
	_draining = false;
	_drain_source.clear();
	_drain_timer = 0.0f;
}

int OfferingBowl::getTotalRequired() const
{
	int total = 0;
	for (int i = 0; i < _requirements_state.size(); ++i)
		total += max(_requirements_state[i].required, 0);
	return total;
}

int OfferingBowl::getTotalDeposited() const
{
	int total = 0;
	for (int i = 0; i < _requirements_state.size(); ++i)
		total += clamp(_requirements_state[i].deposited, 0, _requirements_state[i].required);
	return total;
}
