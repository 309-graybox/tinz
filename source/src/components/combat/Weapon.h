#pragma once
#include "components/combat/Hitbox.h"
#include <UnigineComponentSystem.h>
#include <UnigineEvent.h>

// A Weapon orchestrates a swing across one or more child Hitbox volumes:
//   timing  — startup → active → recovery → cooldown → idle
//   damage  — owns the damage/type/knockback profile applied on each hit
//   dedupe  — at most one hit per target Entity per swing across all hitboxes
//
// During a swing, managed hitboxes operate in "detector mode": they don't
// apply damage themselves (Hitbox::applyDamageOnHit is forced to false here);
// they emit eventHit and the Weapon does the takeDamage call with its own
// stats. This keeps damage tuning in one place — the Weapon — even when the
// weapon has multiple hitboxes (tip + body + handle).
//
// Wielder team comes from the Entity that sits above the Weapon's node in the
// hierarchy — Hitbox derives it on its own, the Weapon doesn't push it.
//
// Caller controls swings via startSwing(). The Weapon will not auto-replay;
// hooking up combos / input belongs in a higher-level controller.
class Weapon: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(Weapon, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	enum class Phase: int
	{
		Idle = 0,
		Startup = 1,
		Active = 2,
		Recovery = 3,
		Cooldown = 4,
	};

	PROP_GROUP("Hitboxes")
	PROP_ARRAY(Node, hitboxes, "", "Child nodes carrying Hitbox components managed by this weapon. Their applyDamageOnHit will be forced to false. Team is inherited from the wielder Entity automatically.")

	PROP_GROUP("Damage")
	PROP_PARAM(Float, damage, 10.0f, "", "Damage applied per target per swing")
	PROP_PARAM(String, damageType, "physical", "", "Tag forwarded to Entity::takeDamage")
	PROP_PARAM(Toggle, applyPlayerKnockback, true, "", "If the target is the player, call CharacterMovement::applyDamageKnockback (uses the player's own knockback speed/duration tuning).")

	PROP_GROUP("Timing")
	PROP_PARAM(Float, startupTime, 0.1f, "", "Wind-up before active frames (s). Hitboxes are off.")
	PROP_PARAM(Float, activeTime, 0.15f, "", "Hitboxes are active for this many seconds")
	PROP_PARAM(Float, recoveryTime, 0.3f, "", "Recovery after active frames (s). Weapon is busy, hitboxes off.")
	PROP_PARAM(Float, cooldown, 0.0f, "", "Extra delay after recovery before another swing can start (s)")

	bool isBusy() const noexcept { return _phase != Phase::Idle; }
	Phase getPhase() const noexcept { return _phase; }

	// Begin a swing if idle. Returns false if the weapon is mid-swing or in
	// cooldown — caller should treat that as "input ignored".
	bool startSwing();

	// Abort the current swing immediately: turn off managed hitboxes, drop
	// the swing-wide hit registry, return to Idle (skips cooldown).
	void cancelSwing();

	Unigine::Event<> &eventSwingStarted() noexcept { return _event_swing_started; }
	Unigine::Event<> &eventActiveStarted() noexcept { return _event_active_started; }
	Unigine::Event<> &eventActiveEnded() noexcept { return _event_active_ended; }
	Unigine::Event<> &eventSwingEnded() noexcept { return _event_swing_ended; }

	// Fires after the Weapon has applied damage to a target. The HitInfo's
	// `damage` reflects the weapon's damage value (not the hitbox's).
	Unigine::Event<const HitInfo &> &eventHit() noexcept { return _event_hit; }

private:
	void init();
	void update();

	void configureManagedHitboxes();
	void setHitboxesActive(bool active);
	void enterPhase(Phase phase);
	void onHitboxHit(const HitInfo &info);

private:
	Phase _phase = Phase::Idle;
	float _phase_timer = 0.0f;

	// Resolved on first update, not in init() — component init order across
	// nodes is unspecified, so we wait until everyone has had a chance to set
	// up before we read/write neighbours.
	bool _configured = false;
	Unigine::Vector<Hitbox *> _hitboxes;
	Unigine::EventConnections _hit_connections;

	// Swing-wide dedupe across multiple managed hitboxes. A single Hitbox's
	// own registry already prevents double-hit within its own activation; this
	// extends that guarantee to "tip-hitbox and body-hitbox both touched the
	// same enemy in one swing → still one damage application".
	Unigine::HashSet<int> _swing_hit_registry;

	Unigine::EventInvoker<> _event_swing_started;
	Unigine::EventInvoker<> _event_active_started;
	Unigine::EventInvoker<> _event_active_ended;
	Unigine::EventInvoker<> _event_swing_ended;
	Unigine::EventInvoker<const HitInfo &> _event_hit;
};
