#pragma once
#include <UnigineComponentSystem.h>

class CharacterMovement;
class Hitbox;
class PlayerCameraManager;
struct HitInfo;

// Stomp-attack driver. Goes on the player's character node alongside
// CharacterMovement. Owns nothing of the volume itself — points to a child
// node that carries a Hitbox, and gates that Hitbox's active state by the
// player's vertical speed.
//
// Flow:
//   per-tick: hitbox.active = (vertical_speed < verticalSpeedThreshold)
//   on hit:   apply vertical bounce + camera trauma to the player
//
// The Hitbox itself does the damage application (standalone mode) — its own
// PROP_PARAMs (damage, damageType="stomp", teamId=0, applyDamageOnHit=true,
// applyPlayerKnockback=false) are designer-set in the editor.
//
// Per-enemy stomp customization (e.g. bigger bounce off a heavier enemy) is
// out of scope here — uniform bounce/shake. Add a Stompable component on the
// target later if you need it.
class PlayerStomp: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(PlayerStomp, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROP_PARAM(Node, stompHitbox, "", "", "Child node carrying the Hitbox component used for stomp. Damage / shape / team configured on the Hitbox itself.")

	PROP_PARAM(Float, verticalSpeedThreshold, -2.0f, "", "Stomp hitbox is active while the player's vertical speed is below this (m/s, negative = falling). -2 means \"falling at least 2 m/s\".")
	PROP_PARAM(Float, bouncePower, 5.0f, "", "Vertical speed (m/s) given to the player on a successful stomp")
	PROP_PARAM(Float, cameraShake, 0.5f, "", "Camera trauma added on a successful stomp. 0 = none")

private:
	void init();
	void update();

	void onStompHit(const HitInfo &info);

private:
	Hitbox *_hitbox = nullptr;
	CharacterMovement *_movement = nullptr;
	PlayerCameraManager *_camera_manager = nullptr;
	Unigine::EventConnection _hitConn;
};
