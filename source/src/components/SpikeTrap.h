#pragma once

#include <UnigineComponentSystem.h>
#include <UnigineWorlds.h>

class SpikeTrap: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(SpikeTrap, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROP_PARAM(Toggle, active, true)
	PROP_PARAM(Node, trigger, "Trigger", "WorldTrigger that detects stepping on spikes")
	PROP_PARAM(Float, damage, 10.0f)
	PROP_PARAM(Float, cooldown, 1.0f, "", "Minimum seconds between damage ticks")
	PROP_PARAM(Toggle, repeatWhileInside, true, "", "If true, damages again every cooldown while player stays inside")

private:
	void init();
	void update();

	void onEnter(const Unigine::NodePtr &n);
	void onLeave(const Unigine::NodePtr &n);
	bool isPlayerNode(const Unigine::NodePtr &n) const;
	bool tryDamagePlayer();

private:
	Unigine::WorldTriggerPtr _trigger;
	bool _playerInside = false;
	float _cooldownTimer = 0.0f;
};
