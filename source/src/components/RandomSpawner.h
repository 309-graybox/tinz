#pragma once

#include <UnigineComponentSystem.h>

class Entity;

class RandomSpawner: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(RandomSpawner, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROP_PARAM(Float, cooldown, 5.0f, "", "Seconds between spawn attempts")
	PROP_PARAM(Toggle, cooldownAfterDeathOnly, true, "", "If true, next cooldown starts only after spawned entity dies")
	PROP_PARAM(Node, spawnPoint, "", "Spawn Point", "World position for spawned node")
	PROP_ARRAY(File, spawnNodes)

private:
	void init();
	void update();

	bool spawnOne();
	int pickTemplateIndex();
	bool isActiveSpawnAlive() const;
	Unigine::Math::Mat4 getSpawnTransform() const;

private:
	float _cooldown_timer = 0.0f;
	Unigine::NodePtr _active_spawn;
};
