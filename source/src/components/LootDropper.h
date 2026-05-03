#pragma once

#include <UnigineComponentSystem.h>

class Entity;

struct LootDropEntry: Unigine::ComponentStruct
{
	PROP_PARAM(Node, dropNode, "", "Drop Node", "Template node that will be cloned on death")
	PROP_PARAM(Float, chance, 1.0f, "", "Drop chance (0..1 or 0..100)")
	PROP_PARAM(Int, minCount, 1)
	PROP_PARAM(Int, maxCount, 1)
};

class LootDropper: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(LootDropper, Unigine::ComponentBase)
	COMPONENT_INIT(init)

	PROP_ARRAY_STRUCT(LootDropEntry, drops)
	PROP_PARAM(Float, scatterRadius, 0.45f, "", "Random horizontal scatter radius")
	PROP_PARAM(Float, scatterHeight, 0.1f, "", "Random vertical scatter height")

private:
	void init();
	void onEntityDied(Entity *entity);
	void dropLoot();

	int sampleCount(int min_count, int max_count) const;
	bool rollChance(float chance_value) const;
	Unigine::Math::Vec3 sampleDropPosition(const Unigine::Math::Vec3 &origin) const;
};
