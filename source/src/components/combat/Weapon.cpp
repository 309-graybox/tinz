#include "components/combat/Weapon.h"
#include "components/Entity.h"
#include "player/movement/CharacterMovement.h"
#include "utils/Utils.h"

#include <UnigineGame.h>

REGISTER_COMPONENT(Weapon)

using namespace Unigine;
using namespace Unigine::Math;

void Weapon::init()
{
	_phase = Phase::Idle;
	_phase_timer = 0.0f;
	_configured = false;
	_hitboxes.clear();
	_hit_connections.disconnectAll();
	_swing_hit_registry.clear();
}

void Weapon::configureManagedHitboxes()
{
	auto cs = ComponentSystem::get();

	const int n = hitboxes.size();
	for (int i = 0; i < n; ++i)
	{
		NodePtr hbn = hitboxes[i].get();
		if (!hbn)
			continue;

		Hitbox *hb = cs->getComponent<Hitbox>(hbn);
		if (!hb)
		{
			Log::warning("Weapon \"%s\": node \"%s\" in hitboxes list has no Hitbox component — skipped\n",
				node->getName(), hbn->getName());
			continue;
		}

		// Detector mode — Weapon owns damage application. Team is inherited
		// by the Hitbox itself from the wielder Entity, so we don't push it.
		hb->applyDamageOnHit = false;
		// We control activation through the timeline.
		hb->setActive(false);

		hb->eventHit().connect(_hit_connections, this, &Weapon::onHitboxHit);
		_hitboxes.append(hb);
	}
}

bool Weapon::startSwing()
{
	if (isBusy())
		return false;

	_swing_hit_registry.clear();
	enterPhase(Phase::Startup);
	_event_swing_started.run();
	return true;
}

void Weapon::cancelSwing()
{
	if (_phase == Phase::Idle)
		return;

	const bool was_active = (_phase == Phase::Active);
	setHitboxesActive(false);
	_phase = Phase::Idle;
	_phase_timer = 0.0f;

	if (was_active)
		_event_active_ended.run();
	_event_swing_ended.run();
}

void Weapon::update()
{
	if (!_configured)
	{
		configureManagedHitboxes();
		_configured = true;
	}

	if (_phase == Phase::Idle)
		return;

	_phase_timer -= Game::getIFps();
	if (_phase_timer > 0.0f)
		return;

	switch (_phase)
	{
		case Phase::Idle:
			break;

		case Phase::Startup:
			enterPhase(Phase::Active);
			_event_active_started.run();
			break;

		case Phase::Active:
			_event_active_ended.run();
			enterPhase(Phase::Recovery);
			break;

		case Phase::Recovery:
			_event_swing_ended.run();
			if (cooldown.get() > 0.0f)
				enterPhase(Phase::Cooldown);
			else
				enterPhase(Phase::Idle);
			break;

		case Phase::Cooldown:
			enterPhase(Phase::Idle);
			break;
	}
}

void Weapon::enterPhase(Phase phase)
{
	_phase = phase;
	switch (phase)
	{
		case Phase::Idle:
			_phase_timer = 0.0f;
			setHitboxesActive(false);
			break;

		case Phase::Startup:
			_phase_timer = max(startupTime.get(), 0.0f);
			setHitboxesActive(false);
			break;

		case Phase::Active:
			_phase_timer = max(activeTime.get(), 0.0f);
			setHitboxesActive(true);
			break;

		case Phase::Recovery:
			_phase_timer = max(recoveryTime.get(), 0.0f);
			setHitboxesActive(false);
			break;

		case Phase::Cooldown:
			_phase_timer = max(cooldown.get(), 0.0f);
			setHitboxesActive(false);
			break;
	}

	// A zero-duration phase shouldn't stall the machine for one frame —
	// progress through it on the next update tick by leaving the timer at 0.
	// (Active with 0 time is a no-op swing; Recovery with 0 time is fine.)
}

void Weapon::setHitboxesActive(bool active)
{
	for (Hitbox *hb : _hitboxes)
	{
		if (hb)
			hb->setActive(active);
	}
}

void Weapon::onHitboxHit(const HitInfo &info)
{
	Entity *entity = info.target_entity;
	if (!entity || entity->isDead() || entity->isInvulnerable())
		return;

	const int entity_id = entity->getNode()->getID();
	if (_swing_hit_registry.contains(entity_id))
		return;

	DamageInfo di;
	di.source = node;
	di.type = damageType;
	di.amount = damage;
	if (!entity->takeDamage(di))
		return;

	_swing_hit_registry.append(entity_id);

	// Knockback target = whichever Entity we hit owns CharacterMovement
	// (i.e. the player, by current convention).
	if (applyPlayerKnockback)
	{
		if (auto cm = getComponent<CharacterMovement>(entity->getNode()))
			cm->applyDamageKnockback(node->getWorldPosition());
	}

	HitInfo forwarded = info;
	forwarded.damage = damage;
	_event_hit.run(forwarded);
}
