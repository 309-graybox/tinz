#pragma once
#include <UnigineComponentSystem.h>
#include <UnigineEvent.h>

struct DamageInfo: Unigine::ComponentStruct
{
	PROP_PARAM(Node, source)
	PROP_PARAM(String, type, "physical")
	PROP_PARAM(Float, amount, 1.0f)
};

UNIGINE_INLINE DamageInfo makeDamageInfo(const Unigine::NodePtr &source, const Unigine::String &type, float amount)
{
	DamageInfo di;
	di.source = source;
	di.type = type;
	di.amount = amount;
	return di;
}

struct MaterialFloat4ParamInfo: Unigine::ComponentStruct
{
	PROP_PARAM(Toggle, apply_to_target, false)
	PROP_PARAM(Node, target, "", "", "", "apply_to_target=1")
	PROP_PARAM(Int, surface, 0)
	PROP_PARAM(String, param)
	PROP_PARAM(Vec4, death_value, {1.0f, 1.0f, 1.0f, 1.0f})

	Unigine::MaterialPtr material;
	Unigine::Math::vec4 init_value;
};

class Entity: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(Entity, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_PARAM(Toggle, has_hp)
	PROP_PARAM(Float, max_hp)
	PROP_PARAM(Int, teamId, 0, "", "Team identifier used by Hitbox to skip same-team targets (friendly fire prevention). 0 = player, 1 = enemy by convention.")
	PROP_PARAM(Float, invulnerabilityTime, 0.0f, "", "Damage immunity duration in seconds after taking damage. 0 = disabled")
	PROP_PARAM(Toggle, persistOnDeath, false, "", "If true, the node is NOT auto-deleted on death — owner reacts via eventDied (used by the player so respawn can revive it)")
	PROP_PARAM(Float, deleteTimer, 0.0f)
	PROP_PARAM(String, soundOnDamage, "", "Damage Sound", "SoundManager event id or direct audio path")
	PROP_PARAM(Float, damageShake, 0.7f, "", "Camera trauma added when this entity is the player and receives damage")

	PROP_ARRAY(Node, enable_on_death)
	PROP_ARRAY(Node, disable_on_death)
	PROP_ARRAY_STRUCT(MaterialFloat4ParamInfo, mat_float4_params)

	bool takeDamage(const DamageInfo &damageInfo);
	bool isDead() const noexcept { return _hp <= Unigine::Math::Consts::EPS; }
	bool isAlive() const noexcept { return !isDead(); }
	bool isInvulnerable() const noexcept;
	int getTeamId() const noexcept { return teamId; }
	float getHP() const noexcept { return _hp; }
	float getMaxHP() const noexcept { return max_hp; }
	bool heal(float amount);
	void kill() { takeDamage(makeDamageInfo(node, "selfharm", Unigine::Math::Consts::INF)); }
	void revive(); // restores _hp to max_hp; safe to call regardless of state

	Unigine::Event<Entity *> &hpChanged() noexcept { return _eventHpChanged; }
	Unigine::Event<Entity *> &eventDied() noexcept { return _event_died; }

private:
	void init();
	void update();
	void shutdown();

private:
	void updateDeathStates();
	void setEnabledArr(const Unigine::Vector<Unigine::NodePtr> &arr, bool enabled);
	void applyMatParams(const Unigine::Vector<MaterialFloat4ParamInfo> &params, bool dead);

private:
	float _hp = 0.0f;
	float _invulnerable_until = 0.0f;
	bool _init_gravity = false;
	float _init_damping = 0.0f;
	float _init_mass = 0.0f;
	float _death_time = 0.0f;

	Unigine::EventInvoker<Entity *> _eventHpChanged;
	Unigine::EventInvoker<Entity *> _event_died;

	Unigine::Vector<Unigine::NodePtr> _enableOnDeath;
	Unigine::Vector<Unigine::NodePtr> _disableOnDeath;
	Unigine::Vector<MaterialFloat4ParamInfo> _matFloatParams;
};
