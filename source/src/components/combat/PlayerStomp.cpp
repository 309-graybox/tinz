#include "components/combat/PlayerStomp.h"
#include "components/combat/Hitbox.h"
#include "player/camera/PlayerCameraManager.h"
#include "player/movement/CharacterMovement.h"
#include "utils/Utils.h"

REGISTER_COMPONENT(PlayerStomp)

using namespace Unigine;

void PlayerStomp::init()
{
	NodePtr hbn = stompHitbox.get();
	FLOGERR(hbn, "PlayerStomp \"%s\": stompHitbox is not set\n", node->getName());

	_hitbox = getComponent<Hitbox>(hbn);
	FLOGERR(_hitbox, "PlayerStomp \"%s\": stompHitbox node \"%s\" has no Hitbox component\n",
		node->getName(), hbn->getName());

	_movement = getComponent<CharacterMovement>(node);
	if (!_movement)
		Log::warning("PlayerStomp \"%s\": no CharacterMovement on this node — bounce / threshold gating will be disabled\n",
			node->getName());

	_camera_manager = getComponent<PlayerCameraManager>(node);

	// Damage application stays on the Hitbox itself (standalone mode).
	// We just listen to know when a stomp landed so we can bounce the player.
	_hitbox->eventHit().connect(_hitConn, this, &PlayerStomp::onStompHit);
	_hitbox->setActive(false);
}

void PlayerStomp::update()
{
	if (!_hitbox)
		return;

	const bool falling_fast = _movement && _movement->getVerticalSpeed() < verticalSpeedThreshold.get();
	_hitbox->setActive(falling_fast);
}

void PlayerStomp::onStompHit(const HitInfo &)
{
	if (_movement && bouncePower > 0.0f)
		_movement->applyVerticalBounce(bouncePower);
	if (_camera_manager && cameraShake > 0.0f)
		_camera_manager->addTrauma(cameraShake);
}
