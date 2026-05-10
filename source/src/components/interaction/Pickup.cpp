#include "components/interaction/Pickup.h"

#include "components/Entity.h"
#include "components/interaction/Inventory.h"
#include "audio/SoundManager.h"

#include <UnigineGame.h>
#include <UnigineLog.h>

#include <cstring>

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
}

void Pickup::update()
{
	const float dt = Game::getIFps();

	tickLifetime(dt);
	tickRotation(dt);

	switch (_state)
	{
		case State::Magnet: tickMagnet(dt); break;
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
	if (_life_timer >= lifetime && isReady())
	{
		notifyDestroyed();
		node.deleteLater();
	}
}

void Pickup::startMagnet(const NodePtr &player)
{
	if (!isReady() || !player)
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

bool Pickup::canBePickedUp(const NodePtr &player) const
{
	if (strcmp(typeId.get(), "health") != 0)
		return true;

	auto entity = ComponentSystem::get()->getComponent<Entity>(player);
	return entity && entity->isAlive() && entity->getHP() < entity->getMaxHP() - Consts::EPS;
}

bool Pickup::canInteract(const NodePtr &player) const
{
	return getMode() == Mode::Interact && canBePickedUp(player);
}

void Pickup::onInteract(const NodePtr &player)
{
	Interactable::onInteract(player);
	pickUp(player);
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
	if (strcmp(typeId.get(), "health") == 0)
	{
		if (auto entity = ComponentSystem::get()->getComponent<Entity>(player))
			entity->heal((float)amount);
	} else if (auto inventory = ComponentSystem::get()->getComponent<Inventory>(player))
	{
		inventory->addItem(typeId.get(), amount);
	}

	onPickedUp(player, amount);
	_event_picked_up.run(player, amount);

	if (node)
		audio::SoundManager::play3DAt(soundPickedUp.get(), node->getWorldPosition());

	if (node)
	{
		notifyDestroyed();
		node.deleteLater();
	}

	_state = State::Idle;
	_target_player.clear();
}
