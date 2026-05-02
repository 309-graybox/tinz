#include "EnemySkull.h"
#include "game/GameState.h"
#include "player/camera/PlayerCameraManager.h"
#include "player/movement/CharacterMovement.h"

#include <UnigineGame.h>
#include <UnigineVisualizer.h>
#include <UnigineWorld.h>

REGISTER_COMPONENT(EnemySkull)

using namespace Unigine;
using namespace Unigine::Math;

namespace
{

// Walks up parents of `n` looking for `target`. Used to attribute a contact /
// raycast hit on a child mesh to the player character root.
bool isInHierarchy(NodePtr n, const NodePtr &target)
{
	while (n)
	{
		if (n == target)
			return true;
		n = n->getParent();
	}
	return false;
}

vec3 horizontal(const vec3 &v)
{
	return vec3(v.x, v.y, 0.0f);
}

} // namespace

void EnemySkull::initSkull()
{
	_body = node->getObjectBodyRigid();
	if (!_body)
	{
		Log::error("EnemySkull \"%s\": no BodyRigid on node — disabling component\n", node->getName());
		setEnabled(false);
		return;
	}

	_body->setGravity(false);
	_body->setLinearDamping(linearDamping);
	// Skulls steady-orbit / hover in place can drop velocity below the freeze
	// threshold and stop responding to setLinearVelocity for a frame. Disable
	// freezing entirely — they're cheap and shouldn't sleep on their own.
	_body->setFreezable(false);
	// Make sure short, glancing contacts (e.g. ram-through) are still reported.
	_body->setHighPriorityContacts(true);

	_body->getEventContactEnter().connect(_contactEnterConn, this, &EnemySkull::onContactEnter);

	_spawnPos = node->getWorldPosition();
}

void EnemySkull::updateSkull()
{
	if (isDead() || !_body)
		return;

	const float ifps = Game::getIFps();
	_attackTimer = max(_attackTimer - ifps, 0.0f);

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

	const bool sees = dist <= sightRange && hasLineOfSight(myPos, targetPos, target);
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

	if (debugDraw)
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

void EnemySkull::onContactEnter(const BodyPtr &body, int num)
{
	if (_attackTimer > 0.0f)
		return;

	const NodePtr target = game::GameState::getPlayerCharacter();
	if (!target)
		return;

	// Identify the OTHER body in the contact (body0/body1 are the pair, one of
	// them is ours).
	BodyPtr other = body->getContactBody0(num);
	if (other == body)
		other = body->getContactBody1(num);
	if (!other)
		return;

	const ObjectPtr otherObj = other->getObject();
	if (!otherObj)
		return;

	if (!isInHierarchy(static_ptr_cast<Node>(otherObj), target))
		return;

	// Stomp from above: the skull's outward surface normal at the contact
	// faces upward → player landed on top. Engine returns the normal pointing
	// from the contact INTO `body`, so we flip the sign to get "skull-out".
	if (dot(-body->getContactNormal(num), vec3_up) >= cos(stompMaxAngle * Consts::DEG2RAD))
	{
		// Both bounce and shake live on components attached to the PlayerDummy
		// (camera) node — same lookup chain as the existing applyVerticalBounce
		// pattern. CameraShake reads state.trauma; we only request the kick.
		if (auto player = Game::getPlayer())
		{
			auto playerNode = static_ptr_cast<Node>(player);
			auto cs = ComponentSystem::get();
			if (stompBouncePower > 0.0f)
			{
				if (auto cm = cs->getComponent<CharacterMovement>(playerNode))
					cm->applyVerticalBounce(stompBouncePower);
			}
			if (auto pcm = cs->getComponent<PlayerCameraManager>(playerNode))
				pcm->addTrauma(stompShake);
		}
		Log::message("%s was killed by player\n", node->getName());
		node.deleteLater();
		return;
	}

	auto entity = ComponentSystem::get()->getComponent<Entity>(target);
	if (entity)
	{
		DamageInfo info;
		info.source = node;
		info.amount = attackDamage;
		const bool damage_applied = entity->takeDamage(info);
		if (auto player = Game::getPlayer(); damage_applied && player)
		{
			auto player_node = static_ptr_cast<Node>(player);
			if (auto cm = ComponentSystem::get()->getComponent<CharacterMovement>(player_node))
				cm->applyDamageKnockback(node->getWorldPosition());
			else
				Log::message("%s damage applied, but CharacterMovement was not found on player node\n", node->getName());
			auto pcm = ComponentSystem::get()->getComponent<PlayerCameraManager>(player_node);
			if (pcm)
				pcm->addTrauma(damageShake);
		}
	}
	_attackTimer = attackCooldown;

	if (dieOnHit)
		node.deleteLater();
}
