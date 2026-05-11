#pragma once
#include <UnigineComponentSystem.h>

class Entity;

// Volume on a node that can BE hit. Designer places a child node at the right
// body offset, picks a shape and sets dimensions — the volume's world position
// is the node's world transform.
//
// Two shapes:
//   Sphere  — `radius`, centered on the node.
//   Capsule — `radius` + `capsuleHeight`, axis is the node's local Z (same
//             convention as Hitbox capsule). For an upright character this
//             means the Hurtbox node should be oriented so its local Z is up.
//
// Why a separate component instead of using the node's bounds: physics shapes
// keep colliders apart by their full radii. A skull's Hitbox volume centered
// on its physics body never overlaps the player's mesh origin — the bodies
// stop them at a 0.4+0.3 m gap. Hurtbox gives an explicit "what part of me
// can be hit" volume so the Hitbox shape vs Hurtbox volume test resolves
// before the physics gap.
//
// teamId and the damage-receiving Entity are auto-derived from the closest
// Entity in the parent chain — this component carries no team config of its
// own. Place multiple Hurtbox nodes on one Entity for multi-region targets
// (head, body, limbs); they all route hits to the same Entity, with optional
// per-region behaviour layered on top later if needed.
class Hurtbox: public Unigine::ComponentBase
{
public:
	enum class Shape : int
	{
		Sphere = 0,
		Capsule = 1,
	};

	COMPONENT_DEFINE(Hurtbox, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_PARAM(Switch, shapeType, 0, "Sphere,Capsule", "", "Volume shape. Capsule axis defaults to local Z but is configurable per-hurtbox.")
	PROP_PARAM(Float, radius, 0.5f, "", "Sphere/capsule radius (m).")
	PROP_PARAM(Float, capsuleHeight, 1.0f, "", "Length of the capsule's cylindrical mid-section (m). Total length is capsuleHeight + 2*radius.", "", "shapeType=1")
	PROP_PARAM(Switch, capsuleAxis, 2, "X,Y,Z", "", "Local axis the capsule extends along. Default Z matches UNIGINE's ShapeCapsule convention; set Y for character meshes whose body axis runs along local Y.", "", "shapeType=1")

	Shape getShape() const noexcept { return static_cast<Shape>(static_cast<int>(shapeType)); }
	float getRadius() const noexcept { return radius; }
	float getCapsuleHeight() const noexcept { return capsuleHeight; }

	// Radius of a sphere that fully encloses this hurtbox volume around its
	// node origin. For Sphere = radius. For Capsule = radius + height/2.
	// Used by Hitbox for a cheap distance pre-cull.
	float enclosingRadius() const noexcept;

	// All currently-live Hurtbox instances. Hitbox iterates this instead of
	// using World::getIntersection — Hurtbox NodeDummies don't carry useful
	// bounds, so a bounds-based broad phase misses them. Each Hurtbox
	// registers itself on init() and removes itself on shutdown().
	static const Unigine::Vector<Hurtbox *> &registry();

	// World-space center of the volume. Uses the node's current world
	// transform — animated/rigged characters can parent the Hurtbox to a
	// bone for the volume to follow the animation.
	Unigine::Math::Vec3 getCenter() const { return node->getWorldPosition(); }

	// Capsule's inner segment endpoints (cap centers) in world space.
	// Defined for any shape — for a Sphere both endpoints equal getCenter().
	void getCapsuleSegment(Unigine::Math::Vec3 &a, Unigine::Math::Vec3 &b) const;

	// Resolve the Entity that takes damage when this hurtbox is hit. Cached
	// on first call. Returns null if no Entity is reachable from this node.
	Entity *getEntity();

private:
	void init();
	void update();
	void shutdown();

	Entity *_entity = nullptr;
	bool _entity_resolved = false;
	bool _registered = false;
};
