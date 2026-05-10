#include "components/combat/Hitbox.h"
#include "components/combat/Hurtbox.h"
#include "components/Entity.h"
#include "player/movement/CharacterMovement.h"
#include "utils/Utils.h"

#include <UnigineGame.h>
#include <UnigineVisualizer.h>
#include <UnigineWorld.h>

REGISTER_COMPONENT(Hitbox)

using namespace Unigine;
using namespace Unigine::Math;

void Hitbox::init()
{
	_active = activeOnInit;
	_hit_registry.clear();
}

void Hitbox::setActive(bool v)
{
	if (_active == v)
		return;

	_active = v;
	if (v)
		_hit_registry.clear();
}

void Hitbox::update()
{
	if (debugDraw)
		drawDebug();

	if (!_active)
		return;

	doSweep();
}

void Hitbox::doSweep()
{
	const Vec3 center = node->getWorldPosition();
	const float er = enclosingRadius();
	if (er <= 0.0f)
		return;

	// Our team comes from the Entity that owns us (the wielder). No team
	// found = "unaligned, hits everything" — useful for stray hitboxes that
	// aren't attached to a unit (e.g. a trap that has no Entity).
	Entity *self_entity = getComponentInParent<Entity>(node);
	const int my_team = self_entity ? self_entity->getTeamId() : -1;

	// Iterate the global Hurtbox registry rather than World::getIntersection
	// — Hurtbox NodeDummies don't carry useful bounds, so a bounds-based
	// broad phase would miss them. The registry is small in practice (one
	// entry per damageable region across all live units) and the per-entry
	// cost is just a distance check before the narrow phase.
	for (Hurtbox *hurtbox : Hurtbox::registry())
	{
		if (!hurtbox)
			continue;

		Entity *entity = hurtbox->getEntity();
		if (!entity)
			continue;

		if (self_entity && entity == self_entity)
			continue; // never hit our own wielder
		if (entity->getTeamId() == my_team)
			continue;

		// Cheap distance pre-cull: if the centers are farther apart than
		// the sum of enclosing radii, no narrow-phase test can succeed.
		const float her = hurtbox->enclosingRadius();
		const float r_sum = er + her;
		if (length2(vec3(hurtbox->getCenter() - center)) > r_sum * r_sum)
			continue;

		// Narrow phase: hitbox shape vs hurtbox volume. This is the key
		// step that lets damage register across the gap that physics shapes
		// leave between bodies.
		if (!overlapsHurtbox(hurtbox))
			continue;

		if (tryHit(hurtbox, entity, center) && deactivateOnHit)
		{
			setActive(false);
			return;
		}
	}
}

float Hitbox::enclosingRadius() const
{
	const float r = max(radius.get(), 0.0f);
	switch (getShape())
	{
		case Shape::Sphere:  return r;
		case Shape::Capsule: return r + max(capsuleHeight.get(), 0.0f) * 0.5f;
		case Shape::Box:     return length(boxHalfExtents.get());
	}
	return 0.0f;
}

bool Hitbox::overlapsHurtbox(const Hurtbox *hurtbox) const
{
	if (!hurtbox)
		return false;

	const float hr = max(hurtbox->getRadius(), 0.0f);
	if (hr <= 0.0f)
		return false;

	if (hurtbox->getShape() == Hurtbox::Shape::Sphere)
		return sphereOverlaps(hurtbox->getCenter(), hr);

	// Capsule hurtbox: sample its inner segment with stride ~= radius. The
	// sphere samples then tile the capsule volume (consecutive spheres
	// always touch), so any sample-overlap implies real overlap.
	Vec3 a, b;
	hurtbox->getCapsuleSegment(a, b);
	const vec3 ab = vec3(b - a);
	const float seg_len = length(ab);

	// Number of samples: at least 2 (endpoints), spaced no further than `hr`
	// so the spheres tile. ceil(len/hr) gives the number of intervals; +1 = points.
	int samples = (seg_len > 1e-4f && hr > 0.0f)
		? (int)ceil(seg_len / hr) + 1
		: 1;
	if (samples < 2)
		samples = 1;

	if (samples == 1)
		return sphereOverlaps(a, hr);

	for (int i = 0; i < samples; ++i)
	{
		const float t = float(i) / float(samples - 1);
		const Vec3 p = a + Vec3(ab) * Scalar(t);
		if (sphereOverlaps(p, hr))
			return true;
	}
	return false;
}

bool Hitbox::sphereOverlaps(const Vec3 &c, float r) const
{
	const Mat4 t = node->getWorldTransform();
	const Vec3 my_center = t.getTranslate();

	switch (getShape())
	{
		case Shape::Sphere:
		{
			const float my_r = max(radius.get(), 0.0f);
			const float r_sum = my_r + r;
			return length2(vec3(c - my_center)) <= r_sum * r_sum;
		}

		case Shape::Capsule:
		{
			const float my_r = max(radius.get(), 0.0f);
			const float h = max(capsuleHeight.get(), 0.0f);

			// Capsule axis = the selected local axis (X/Y/Z) in world space.
			Vec3 axis;
			switch ((int)capsuleAxis)
			{
				case 0:  axis = t.getAxisX(); break;
				case 1:  axis = t.getAxisY(); break;
				default: axis = t.getAxisZ(); break;
			}
			const float al = (float)length(axis);
			if (al < 1e-6f)
				return false;
			axis /= Scalar(al);

			// Endpoints of the cylindrical inner segment.
			const Vec3 a = my_center - axis * Scalar(h * 0.5f);
			const Vec3 b = my_center + axis * Scalar(h * 0.5f);

			// Closest point on segment ab to c. Capsule-vs-sphere overlap is
			// equivalent to "segment-to-point distance ≤ capsule.r + sphere.r".
			const vec3 ab = vec3(b - a);
			const float ab_len2 = length2(ab);
			const float u = (ab_len2 > 1e-6f)
				? saturate(dot(vec3(c - a), ab) / ab_len2)
				: 0.0f;
			const Vec3 closest = a + Vec3(ab) * Scalar(u);
			const float r_sum = my_r + r;
			return length2(vec3(c - closest)) <= r_sum * r_sum;
		}

		case Shape::Box:
		{
			// OBB-vs-sphere: bring c into the node's local frame, clamp to
			// halfExtents, distance from c to clamped point ≤ r.
			// Note: assumes ~uniform scale (otherwise local distances diverge
			// from world).
			const Mat4 inv = inverse(t);
			const vec3 local = vec3(inv * c);
			const vec3 he = boxHalfExtents.get();
			const vec3 clamped(
				clamp(local.x, -he.x, he.x),
				clamp(local.y, -he.y, he.y),
				clamp(local.z, -he.z, he.z));
			const vec3 diff = local - clamped;
			return length2(diff) <= r * r;
		}
	}
	return false;
}

bool Hitbox::tryHit(Hurtbox *hurtbox, Entity *entity, const Vec3 &center)
{
	if (!entity || entity->isDead() || entity->isInvulnerable())
		return false;

	const int entity_id = entity->getNode()->getID();
	if (_hit_registry.contains(entity_id))
		return false;

	if (applyDamageOnHit)
	{
		// Standalone mode: apply damage ourselves so the hitbox is useful
		// without an external orchestrator (traps, projectiles).
		DamageInfo di;
		di.source = node;
		di.type = damageType;
		di.amount = damage;
		if (!entity->takeDamage(di))
			return false;

		// Knockback: if the Entity we hit owns the player, ask the player
		// CharacterMovement to apply its own knockback profile.
		if (applyPlayerKnockback)
		{
			if (auto cm = getComponent<CharacterMovement>(entity->getNode()))
				cm->applyDamageKnockback(center);
		}
	}

	// Managed mode skips the takeDamage / knockback path above and just emits
	// the event — the listener (Weapon) is expected to apply damage with its
	// own profile. Dedupe still happens here so the listener never sees the
	// same target twice within one activation.
	_hit_registry.append(entity_id);

	const Vec3 target_pos = hurtbox->getCenter();
	vec3 dir = vec3(target_pos - center);
	const float dlen = length(dir);
	const vec3 dir_norm = (dlen > 1e-4f) ? dir / dlen : vec3_zero;

	HitInfo info;
	info.hitbox = this;
	info.hurtbox = hurtbox;
	info.target_entity = entity;
	info.damage = damage;
	info.direction = dir_norm;
	info.position = target_pos;
	_event_hit.run(info);

	return true;
}

void Hitbox::drawDebug() const
{
	const Mat4 t = node->getWorldTransform();
	const vec4 color = _active
		? vec4(1.0f, 0.2f, 0.2f, 1.0f)
		: vec4(1.0f, 0.9f, 0.2f, 1.0f);

	switch (getShape())
	{
		case Shape::Sphere:
			Visualizer::renderSphere(max(radius.get(), 0.0f), translate(t.getTranslate()), color);
			break;

		case Shape::Capsule:
		{
			// Visualizer::renderCapsule draws along the transform's local Z;
			// rotate the viz frame so its Z column points along the chosen
			// capsule axis of `t`.
			Mat4 viz = t;
			switch ((int)capsuleAxis)
			{
				case 0: viz = t * rotateY(Scalar(90.0)); break;   // local X
				case 1: viz = t * rotateX(Scalar(-90.0)); break;  // local Y
				default: break;                                    // local Z
			}
			Visualizer::renderCapsule(max(radius.get(), 0.0f), max(capsuleHeight.get(), 0.0f), viz, color);
			break;
		}

		case Shape::Box:
			Visualizer::renderBox(boxHalfExtents.get() * 2.0f, t, color);
			break;
	}
}
