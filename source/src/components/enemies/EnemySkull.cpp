#include "components/enemies/EnemySkull.h"
#include "components/combat/Hitbox.h"
#include "game/GameState.h"
#include "utils/Utils.h"
#include "tuning/DebugTuning.h"

#include <UnigineGame.h>
#include <UnigineVisualizer.h>
#include <UnigineWorld.h>

REGISTER_COMPONENT(EnemySkull)

using namespace Unigine;
using namespace Unigine::Math;

void EnemySkull::initSkull()
{
	_body = node->getObjectBodyRigid();
	FLOGERR(_body, "EnemySkull \"%s\": no BodyRigid on node — disabling component\n", node->getName());

	_body->setGravity(false);
	_body->setLinearDamping(linearDamping);
	// Skulls steady-orbit / hover in place can drop velocity below the freeze
	// threshold and stop responding to setLinearVelocity for a frame. Disable
	// freezing entirely — they're cheap and shouldn't sleep on their own.
	_body->setFreezable(false);

	_spawnPos = node->getWorldPosition();

	// Stop the hitbox the moment we die, in case its update would otherwise
	// run for one more frame after death (component init order is unspecified
	// and the hitbox node may not be in disable_on_death).
	eventDied().connect(_diedConn, this, &EnemySkull::onSelfDied);
}

void EnemySkull::configureHitbox()
{
	// Deferred from init: same-node component init order is unspecified, so
	// Hitbox::init may run after our initSkull and overwrite _active. Doing
	// the lookup + activation on the first update sidesteps that.
	auto cs = ComponentSystem::get();
	if (NodePtr hbn = hitboxNode.get())
		_hitbox = cs->getComponent<Hitbox>(hbn);
	else
		_hitbox = cs->getComponentInChildren<Hitbox>(node);

	if (_hitbox)
	{
		_hitbox->eventHit().connect(_hitboxHitConn, this, &EnemySkull::onHitboxHit);
		_hitbox->setActive(true);
	} else
	{
		Log::warning("EnemySkull \"%s\": no Hitbox found (set hitboxNode or add a Hitbox to a child) — skull will not damage the player\n", node->getName());
	}
}

void EnemySkull::updateSkull()
{
	if (isDead() || !_body)
		return;

	if (!_hitboxConfigured)
	{
		configureHitbox();
		_hitboxConfigured = true;
	}

	const float ifps = Game::getIFps();

	// Cooldown after a successful non-lethal hit (dieOnHit=false). The hitbox
	// was deactivated in onHitboxHit; re-arm it here when the timer expires.
	if (_hitboxCooldownTimer > 0.0f)
	{
		_hitboxCooldownTimer = max(_hitboxCooldownTimer - ifps, 0.0f);
		if (_hitboxCooldownTimer <= 0.0f && _hitbox)
			_hitbox->setActive(true);
	}

	const NodePtr target = game::GameState::getPlayerCharacter();
	if (!target)
	{
		_alerted = false;
		_ramming = false;
		_memoryTimer = 0.0f;
		applySteering(vec3_zero, ifps);
		return;
	}

	const Vec3 myPos = node->getWorldPosition();
	const Vec3 targetPos = target->getWorldPosition();
	const float dist = (float)length(targetPos - myPos);

	// LOS rays come from / aim at bbox-derived points, not node pivots. Pivots
	// often sit at the feet, which makes the ray graze the floor and report the
	// terrain as the blocker. Two sample points on the target (center + top)
	// also handle waist-high cover.
	bool sees = false;
	if (dist <= sightRange)
	{
		const WorldBoundBox myBB = node->getWorldBoundBox();
		const WorldBoundBox tgtBB = target->getWorldBoundBox();
		const Vec3 from = myBB.getCenter();
		const Vec3 tgtCenter = tgtBB.getCenter();
		const Vec3 tgtTop = Vec3(tgtCenter.x, tgtCenter.y, tgtBB.maximum.z);
		sees = hasLineOfSight(from, tgtCenter, target)
			|| hasLineOfSight(from, tgtTop, target);
	}
	if (sees)
		_memoryTimer = memoryDuration;
	else
		_memoryTimer = max(_memoryTimer - ifps, 0.0f);

	// While the memory timer is alive we still "know" the player's current
	// position and keep chasing — losing sight doesn't immediately blank us.
	_alerted = sees || _memoryTimer > 0.0f;

	if (!_alerted)
	{
		// Just lost the player completely → teleport home and reset state so
		// the skull is ready to ambush from its spawn point again.
		if (_wasAlerted)
		{
			Mat4 t = node->getWorldTransform();
			t.setTranslate(_spawnPos);
			_body->setTransform(t);
			_body->setLinearVelocity(vec3_zero);
		}

		_ramming = false; // re-evaluate flank approach when we reacquire.
		_wasAlerted = false;
		applySteering(vec3_zero, ifps);
		return;
	}
	_wasAlerted = true;

	vec3 desired = computeDesiredVelocity(target, myPos, targetPos);
	// Damp vertical movement so the skull stays reachable for stomp-kills —
	// without this it darts up out of the player's jump arc.
	desired.z *= verticalSpeedFactor;
	applySteering(desired, ifps);

	if (DebugTuning::get()->show_skulls_direction)
	{
		vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
		switch (getBehavior())
		{
			case Behavior::Direct: color = vec4(1.0f, 0.2f, 0.2f, 1.0f); break;
			case Behavior::Orbit: color = vec4(0.2f, 0.4f, 1.0f, 1.0f); break;
			case Behavior::Flank: color = vec4(0.2f, 1.0f, 0.3f, 1.0f); break;
		}
		Visualizer::renderLine3D(myPos, targetPos, color);
	}
}

bool EnemySkull::hasLineOfSight(const Vec3 &from, const Vec3 &to, const NodePtr &target) const
{
	Vector<NodePtr> exclude = {node};
	auto hit = World::getIntersection(from, to, (int)sightMask, exclude);
	if (!hit)
		return true;

	NodePtr hitNode = static_ptr_cast<Node>(hit);
	return isInHierarchy(hitNode, target);
}

vec3 EnemySkull::computeDesiredVelocity(const NodePtr &target, const Vec3 &myPos, const Vec3 &targetPos)
{
	const vec3 toTarget = vec3(targetPos - myPos);
	const float dist = length(toTarget);
	if (dist < 1e-4f)
		return vec3_zero;

	const vec3 toTargetDir = toTarget / dist;

	switch (getBehavior())
	{
		case Behavior::Direct:
			return toTargetDir * speed;

		case Behavior::Orbit:
		{
			// Inside the close-up radius the orbit is pointless — switch to a
			// direct ram so the skull actually finishes the kill.
			if (dist < orbitDistance)
				return toTargetDir * speed;

			// Tangent perpendicular to (skull→player) in the horizontal plane.
			// Cross with world up gives a stable rotation direction.
			const vec3 up(0.0f, 0.0f, 1.0f);
			vec3 tangent = cross(up, toTargetDir);
			const float tlen = length(tangent);
			if (tlen < 1e-4f)
				return toTargetDir * speed; // singularity (skull above/below)
			tangent /= tlen;

			// Tangent at full speed + constant inward pull → spiral in.
			return tangent * speed + toTargetDir * orbitApproachSpeed;
		}

		case Behavior::Flank:
		{
			// Sticky once committed — without this the skull oscillates around
			// the flank point as it leaves it and re-evaluates as "far again".
			if (_ramming)
				return toTargetDir * speed;

			// Player's back direction in the horizontal plane. Character forward
			// is its world +Y axis (same convention as CharacterMovement.cpp:68).
			vec3 backDir = horizontal(-target->getWorldDirection(Math::AXIS_Y));
			const float blen = length(backDir);
			if (blen < 1e-4f)
			{
				_ramming = true;
				return toTargetDir * speed;
			}
			backDir /= blen;

			// Skull's horizontal radius vector from the player.
			const vec3 PS = horizontal(vec3(myPos - targetPos));
			const float PS_len = length(PS);
			if (PS_len < 1e-4f)
				return toTargetDir * speed;
			const vec3 PS_dir = PS / PS_len;

			// How aligned are we with the player's back? cos(angle).
			const float behindness = dot(PS_dir, backDir);
			const float commitCos = cos(flankCommitAngle * Consts::DEG2RAD);
			if (behindness > commitCos)
			{
				_ramming = true;
				return toTargetDir * speed;
			}

			// Tangent — perpendicular to the radius in the horizontal plane.
			// Pick the rotation direction that takes the shorter arc to backDir.
			const vec3 up(0.0f, 0.0f, 1.0f);
			vec3 tangent = cross(up, PS_dir);
			const float crossZ = PS_dir.x * backDir.y - PS_dir.y * backDir.x;
			if (crossZ < 0.0f)
				tangent = -tangent;

			// Hold the ring at flankDistance: pull in if too far, push out if too close.
			const float radialErr = PS_len - flankDistance;
			const vec3 radial = -PS_dir * sign(radialErr) * flankRadialCorrection;

			return tangent * speed + radial;
		}
	}
	return vec3_zero;
}

void EnemySkull::applySteering(const vec3 &desiredVel, float ifps)
{
	if (!_body)
		return;

	const vec3 currentVel = _body->getLinearVelocity();
	vec3 deltaVel = desiredVel - currentVel;
	const float maxDelta = accel * ifps;
	const float dlen = length(deltaVel);
	if (dlen > maxDelta && dlen > 1e-6f)
		deltaVel = deltaVel * (maxDelta / dlen);
	_body->setLinearVelocity(currentVel + deltaVel);
}

void EnemySkull::onHitboxHit(const HitInfo &)
{
	if (dieOnHit)
	{
		Log::message("%s rammed the player and self-destructed\n", node->getName());
		kill();
		return;
	}

	// Non-kamikaze variant: park the hitbox for `attackCooldown` seconds. The
	// update loop re-arms it (which also clears the per-activation hit
	// registry, so the same player can be hit again next swing).
	if (_hitbox)
		_hitbox->setActive(false);
	_hitboxCooldownTimer = max(attackCooldown.get(), 0.0f);
}

void EnemySkull::onSelfDied(Entity *)
{
	if (_hitbox)
		_hitbox->setActive(false);
}
