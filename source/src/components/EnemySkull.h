#pragma once
#include "Entity.h"
#include <UnigineEvent.h>
#include <UniginePhysics.h>

// Flying skull enemy. Three behaviors selectable in editor:
//   Direct — fly straight at the player.
//   Orbit  — spiral around the player while continuously closing the distance.
//   Flank  — orbit at a fixed radius until aligned with the player's back,
//            then commit to a straight ram.
//
// Vision: skull only acts while there is a free line of sight no longer than
// `sightRange`. Out of sight → idle (zero velocity). Damage triggers on the
// BodyRigid's contact-enter event with anything in the player character's
// hierarchy (subscribed once in init).
class EnemySkull: public Entity
{
public:
	COMPONENT_DEFINE(EnemySkull, Entity)
	COMPONENT_INIT(initSkull)
	COMPONENT_UPDATE(updateSkull)

	enum class Behavior : int
	{
		Direct = 0,
		Orbit = 1,
		Flank = 2,
	};

	PROP_PARAM(Switch, behavior, 0, "Direct,Orbit,Flank", "Skull behavior")

	PROP_GROUP("Sight")
	PROP_PARAM(Float, sightRange, 20.0f, "", "Maximum free line-of-sight length at which the skull notices the player")
	PROP_PARAM(Mask, sightMask, ~0, "", "Intersection mask for the line-of-sight raycast")
	PROP_PARAM(Float, memoryDuration, 3.0f, "", "After losing line of sight, the skull keeps tracking the player for this many seconds before going idle")

	PROP_GROUP("Movement")
	PROP_PARAM(Float, speed, 8.0f, "", "Target chase speed")
	PROP_PARAM(Float, accel, 60.0f, "", "Maximum velocity change per second — smooths jerks and reactions to collisions")
	PROP_PARAM(Float, linearDamping, 0.0f, "", "Linear damping applied to BodyRigid on init. 0 is fine since velocity is overwritten every frame")

	PROP_GROUP("Orbit")
	PROP_PARAM(Float, orbitDistance, 1.5f, "", "Below this distance the blue skull abandons the orbit and rams the player directly")
	PROP_PARAM(Float, orbitApproachSpeed, 2.0f, "", "Constant inward pull applied while orbiting (m/s) — controls how fast the spiral tightens")

	PROP_GROUP("Flank")
	PROP_PARAM(Float, flankDistance, 3.0f, "", "Radius the green skull holds while orbiting around the player toward their back")
	PROP_PARAM(Float, flankCommitAngle, 30.0f, "", "Once the skull is within this angle (degrees) of the player's back, it commits to a straight ram")
	PROP_PARAM(Float, flankRadialCorrection, 2.0f, "", "How aggressively (m/s) the skull corrects toward `flankDistance` while orbiting")

	PROP_GROUP("Attack")
	PROP_PARAM(Float, attackDamage, 10.0f)
	PROP_PARAM(Float, attackCooldown, 1.0f, "", "Minimum time between hits (only meaningful when dieOnHit is off)")
	PROP_PARAM(Toggle, dieOnHit, true, "", "Skull is destroyed immediately after dealing damage (kamikaze)")

	PROP_GROUP("Debug")
	PROP_PARAM(Toggle, debugDraw, false, "", "Draw a line from the skull to its current target. Color encodes behavior")

	Behavior getBehavior() const noexcept { return static_cast<Behavior>(static_cast<int>(behavior)); }
	bool isAlerted() const noexcept { return _alerted; }

private:
	void initSkull();
	void updateSkull();

	bool hasLineOfSight(const Unigine::Math::Vec3 &from, const Unigine::Math::Vec3 &to,
		const Unigine::NodePtr &target) const;

	Unigine::Math::vec3 computeDesiredVelocity(const Unigine::NodePtr &target,
		const Unigine::Math::Vec3 &myPos, const Unigine::Math::Vec3 &targetPos);

	void applySteering(const Unigine::Math::vec3 &desiredVel, float ifps);

	void onContactEnter(const Unigine::BodyPtr &body, int num);

private:
	Unigine::BodyRigidPtr _body;
	Unigine::EventConnection _contactEnterConn;

	bool _alerted = false;
	bool _wasAlerted = false; // Used to detect the alerted→idle transition for the spawn teleport.
	bool _ramming = false; // Flank: sticky once we commit to the ram dash.
	float _attackTimer = 0.0f;
	float _memoryTimer = 0.0f; // Counts down after LOS is lost; while > 0 we still know where the player is.
	Unigine::Math::Vec3 _spawnPos = Unigine::Math::Vec3_zero;
};
