#pragma once
#include <UnigineComponentSystem.h>
#include <UnigineEvent.h>
#include <UnigineHashSet.h>

class Entity;
class Hitbox;
class Hurtbox;

struct HitInfo
{
	Hitbox *hitbox = nullptr;
	Hurtbox *hurtbox = nullptr;
	Entity *target_entity = nullptr;
	float damage = 0.0f;

	// World-space direction from the hitbox center toward the target. Useful
	// for knockback / VFX orientation.
	Unigine::Math::vec3 direction = Unigine::Math::vec3_zero;

	// World-space approximation of the contact point — the hurtbox's center.
	Unigine::Math::Vec3 position = Unigine::Math::Vec3_zero;
};

// Active damage volume that follows its node's transform. While active, every
// tick performs an overlap query around the node and damages any Entity it
// finds in the bounding-volume's intersection (walking up the candidate's
// parent chain to locate Entity). A per-activation hit registry deduplicates
// so one swing damages each Entity only once.
//
// Three shapes (selected by `shapeType`):
//   Sphere  — `radius`. Centered on the node's world position.
//   Capsule — `radius` + `capsuleHeight`. Axis is the node's local Z; total
//             length is capsuleHeight + 2*radius. Rotate the node to aim it.
//   Box     — `boxHalfExtents`. OBB oriented by the node's transform.
//
// Activation is manual: call setActive(true) when the swing's active frames
// start, setActive(false) when they end. Toggling off→on resets the registry,
// re-arming the hitbox for the next swing.
//
// Knockback handling is currently player-only (forwarded to
// CharacterMovement::applyDamageKnockback). Other entity types should listen
// to eventHit() and apply pushback themselves — the HitInfo carries the
// world-space hit direction.
class Hitbox: public Unigine::ComponentBase
{
public:
	enum class Shape: int
	{
		Sphere = 0,
		Capsule = 1,
		Box = 2,
	};

	COMPONENT_DEFINE(Hitbox, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROP_GROUP("Volume")
	PROP_PARAM(Switch, shapeType, 0, "Sphere,Capsule,Box", "", "Volume shape. Capsule axis is the node's local Z; Box is OBB oriented by the node transform.")
	PROP_PARAM(Float, radius, 0.5f, "", "Sphere/capsule radius in meters. Ignored when shapeType=Box.")
	PROP_PARAM(Float, capsuleHeight, 1.0f, "", "Length of the capsule's cylindrical mid-section (m). Total capsule length is capsuleHeight + 2*radius.", "", "shapeType=1")
	PROP_PARAM(Vec3, boxHalfExtents, Unigine::Math::vec3(0.5f, 0.5f, 0.5f), "", "Half-extents (m) along node local X/Y/Z. Box is OBB — orientation follows the node's transform.", "", "shapeType=2")

	PROP_GROUP("Damage")
	PROP_PARAM(Float, damage, 10.0f, "", "Damage applied to the target's Entity per hit")
	PROP_PARAM(String, damageType, "physical", "", "Tag forwarded to Entity::takeDamage (used for resistances / death cause)")

	PROP_GROUP("Knockback")
	PROP_PARAM(Toggle, applyPlayerKnockback, true, "", "If the target is the player, call CharacterMovement::applyDamageKnockback (uses the player's own knockback speed/duration tuning).")

	PROP_GROUP("Activation")
	PROP_PARAM(Toggle, activeOnInit, false, "", "Start active. Otherwise the hitbox sleeps until setActive(true) is called.")
	PROP_PARAM(Toggle, deactivateOnHit, false, "", "Disable after the first valid hit. Useful for projectiles / single-target attacks.")
	PROP_PARAM(Toggle, applyDamageOnHit, true, "", "If true, the hitbox calls Entity::takeDamage itself (standalone mode — traps, projectiles). Set false when an external orchestrator (e.g. Weapon) listens to eventHit and applies damage with its own profile. The overlap check, dedupe and event still run.")

	PROP_GROUP("Debug")
	PROP_PARAM(Toggle, debugDraw, false, "", "Render the sphere volume each frame (yellow when idle, red when active)")

	Shape getShape() const noexcept { return static_cast<Shape>(static_cast<int>(shapeType)); }

	bool isActive() const noexcept { return _active; }

	// Toggling state. off→on resets the per-activation hit registry so the
	// next swing can re-hit the same targets.
	void setActive(bool v);

	// Drop the hit registry without changing active state. Use when the same
	// activation should be allowed to re-hit (e.g. multi-tick projectiles
	// with intentional pierce-and-rehit semantics).
	void resetHitRegistry() { _hit_registry.clear(); }

	Unigine::Event<const HitInfo &> &eventHit() noexcept { return _event_hit; }

private:
	void init();
	void update();

	void doSweep();
	bool tryHit(Hurtbox *hurtbox, Entity *entity, const Unigine::Math::Vec3 &center);
	void drawDebug() const;

	// World-space radius of a sphere that fully encloses the configured shape.
	// Used as the broad-phase bound for `World::getIntersection`.
	float enclosingRadius() const;

	// Narrow-phase: does the given Hurtbox volume overlap our shape?
	// Dispatches to sphereOverlaps for sphere hurtboxes; for capsule
	// hurtboxes, samples the inner segment with stride = hurtbox.radius and
	// reuses sphereOverlaps for each sample (sufficient for game-quality
	// collision since the sphere samples tile the capsule volume).
	bool overlapsHurtbox(const Hurtbox *hurtbox) const;

	// Lower-level helper used by overlapsHurtbox and external callers.
	// "Sphere of radius `r` centered at `c` overlaps our shape volume."
	bool sphereOverlaps(const Unigine::Math::Vec3 &c, float r) const;

private:
	bool _active = false;
	// Already-hit Entity node IDs. Keyed by entity (not the candidate node)
	// so multiple sub-meshes of one target body don't each register a hit.
	Unigine::HashSet<int> _hit_registry;
	Unigine::EventInvoker<const HitInfo &> _event_hit;
};
