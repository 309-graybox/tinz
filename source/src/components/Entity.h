#pragma once
#include <UnigineComponentSystem.h>

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

	void takeDamage(const DamageInfo &damageInfo);
	bool isDead() const noexcept { return Unigine::Math::compare(_hp, 0.0f); }
	bool isAlive() const noexcept { return !isDead(); }

private:
	void init();
	void update();
	void shutdown();

private:
	float _hp;
};
