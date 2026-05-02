#pragma once
#include <UnigineComponentSystem.h>
#include <UnigineEvent.h>

struct DamageInfo: public Unigine::ComponentStruct
{
	PROP_PARAM(Node, source)
	PROP_PARAM(String, type, "physical")
	PROP_PARAM(Float, amount, 1.0f)
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
	PROP_PARAM(Toggle, persistOnDeath, false, "", "If true, the node is NOT auto-deleted on death — owner reacts via eventDied (used by the player so respawn can revive it)")

	void takeDamage(const DamageInfo &damageInfo);
	bool isDead() const noexcept { return Unigine::Math::compare(_hp, 0.0f); }
	bool isAlive() const noexcept { return !isDead(); }
	float getHP() const noexcept { return _hp; }
	float getMaxHP() const noexcept { return max_hp; }
	void revive(); // restores _hp to max_hp; safe to call regardless of state

	Unigine::EventInvoker<Entity *> &eventDied() noexcept { return _event_died; }

private:
	void init();
	void update();
	void shutdown();

private:
	float _hp;
	Unigine::EventInvoker<Entity *> _event_died;
};
