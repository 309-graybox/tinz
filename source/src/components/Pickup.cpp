#include "Pickup.h"

#include "../audio/SoundManager.h"

#include <UnigineGame.h>
#include <UnigineLog.h>

REGISTER_COMPONENT(Pickup)

using namespace Unigine;
using namespace Unigine::Math;

namespace
{
constexpr float kPickupCompleteRadius = 0.3f;
constexpr float kMagnetRampTime = 0.25f; // seconds to reach full magnetSpeed (ease-in)
} // namespace

void Pickup::init()
{
	_state = State::Idle;
	_life_timer = 0.0f;
	_magnet_timer = 0.0f;
	_interact_timer = 0.0f;
}

void Pickup::update()
{
	const float dt = Game::getIFps();

	tickLifetime(dt);
	tickRotation(dt);

	switch (_state)
	{
		case State::Magnet: tickMagnet(dt); break;
		case State::Interact: tickInteract(dt); break;
		default: break;
	}
}

void Pickup::tickRotation(float dt)
{
	if (rotationSpeed <= 0.0f || !node)
		return;

	float speed = rotationSpeed;
	if (_state == State::Magnet)
		speed *= magnetRotationMul;

	const quat step(vec3(0.0f, 0.0f, 1.0f), speed * dt);
	node->setRotation(node->getRotation() * step);
}

void Pickup::tickLifetime(float dt)
{
	if (lifetime <= 0.0f) // 0 = infinite
		return;

	_life_timer += dt;
	if (_life_timer >= lifetime && _state == State::Idle)
	{
		_event_destroyed.run(this);
		node.deleteLater();
	}
}

void Pickup::startMagnet(const NodePtr &player)
{
	if (_state != State::Idle || !player)
		return;

	_state = State::Magnet;
	_target_player = player;
	_magnet_timer = 0.0f;

	// Stop participating in trigger queries — pickup is "in flight",
	// shouldn't be re-detected, shouldn't merge.
	if (node)
		node->setTriggerInteractionEnabled(false);

	_event_magnet_started.run(player);
}

void Pickup::tickMagnet(float dt)
{
	if (!_target_player || !node)
	{
		_state = State::Idle;
		return;
	}

	_magnet_timer += dt;

	const Vec3 self_pos = node->getWorldPosition();
	const Vec3 player_pos = _target_player->getWorldPosition();
	const Vec3 to_player = player_pos - self_pos;
	const float dist = (float)length(to_player);

	if (dist <= kPickupCompleteRadius || _magnet_timer >= magnetTimeout)
	{
		pickUp(_target_player);
		return;
	}

	const float ramp01 = clamp(_magnet_timer / kMagnetRampTime, 0.0f, 1.0f);
	const float speed = magnetSpeed * ramp01 * ramp01; // quadratic ease-in
	const Vec3 dir = to_player / max(dist, 0.0001f);
	const Vec3 step = dir * (speed * dt);

	if ((float)length(step) >= dist)
		node->setWorldPosition(player_pos);
	else
		node->setWorldPosition(self_pos + step);
}

void Pickup::startInteract(const NodePtr &player)
{
	if (_state != State::Idle || !player)
		return;

	_state = State::Interact;
	_target_player = player;
	_interact_timer = 0.0f;

	_event_interact_started.run(player);

	if (interactHoldTime <= 0.0f)
		pickUp(player); // tap-mode: complete immediately
}

void Pickup::tickInteract(float dt)
{
	if (!_target_player)
	{
		cancelInteract();
		return;
	}

	_interact_timer += dt;
	if (_interact_timer >= interactHoldTime)
		pickUp(_target_player);
}

void Pickup::cancelInteract()
{
	if (_state != State::Interact)
		return;

	NodePtr who = _target_player;
	_state = State::Idle;
	_target_player.clear();
	_interact_timer = 0.0f;
	_event_interact_cancelled.run(who);
}

float Pickup::getInteractProgress01() const noexcept
{
	if (_state != State::Interact || interactHoldTime <= 0.0f)
		return 0.0f;
	return clamp(_interact_timer / (float)interactHoldTime, 0.0f, 1.0f);
}

void Pickup::pickUp(const NodePtr &player)
{
	if (!canBePickedUp(player))
	{
		// Cannot pick up right now — bail back to Idle so caller can retry.
		_state = State::Idle;
		_target_player.clear();
		if (node)
			node->setTriggerInteractionEnabled(true);
		return;
	}

	const int amount = (int)count;
	onPickedUp(player, amount);
	_event_picked_up.run(player, amount);

	if (node)
		audio::SoundManager::play3DAt(soundPickedUp.get(), node->getWorldPosition());

	if (node)
	{
		_event_destroyed.run(this);
		node.deleteLater();
	}

	_state = State::Idle;
	_target_player.clear();
}
